#!/usr/bin/python

"""
linuxrouter.py: Example network with Linux IP router

This example converts a Node into a router using IP forwarding
already built into Linux.


##############################################################################
# Topology with two switches and two hosts with static routes
#
#       172.16.101.0/24         172.16.102.0/24   
#  h1 ------------------- r1 ------------------ h2
#    .1                .2   .3               .1   
#
##############################################################################
"""


from mininet.topo import Topo
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.node import Node, CPULimitedHost
from mininet.log import setLogLevel, info
from mininet.util import custom, waitListening
from mininet.cli import CLI
import sys
import time

#following question shows how to reserve CPU cores to an user space process
#https://unix.stackexchange.com/questions/282256/isolcpus-kernel-parameter-has-no-effect-on-ubuntu-16-04-desktop
#Add  "nohz_full=1-3 isolcpus=1-3 rcu_nocbs=1-3 intel_idle.max_cstate=0 irqaffinity=0,1 selinux=0 audit=0 tsc=reliable" to grub config instead to make it tickless
#Also compile kernel with these kernel configs:
#CONFIG_NO_HZ_FULL=y
#CONFIG_NO_HZ_FULL_ALL=y

class LinuxRouter( Node ):
    "A Node with IP forwarding enabled."

    def config( self, **params ):
        super( LinuxRouter, self).config( **params )
        # Enable forwarding on the router
        self.cmd( 'sysctl net.ipv4.ip_forward=1' )

    def terminate( self ):
        self.cmd( 'sysctl net.ipv4.ip_forward=0' )
        super( LinuxRouter, self ).terminate()

class NetworkTopo( Topo ):
    def build( self, **_opts ):	
        h1 = self.addHost( 'h1', ip='172.16.101.1/24', defaultRoute='via 172.16.101.2' )
        h2 = self.addHost( 'h2', ip='172.16.102.1/24', defaultRoute='via 172.16.102.3' )
	r1 = self.addNode( 'r1', cls=LinuxRouter, ip='172.16.101.2/24' )

        self.addLink( h1, r1, intfName2='r1-eth2', params2={ 'ip' : '172.16.101.2/24' })
        self.addLink( h2, r1, intfName2='r1-eth3', params2={ 'ip' : '172.16.102.3/24' })

def main(cli=0):
    "Test linux router"
    topo = NetworkTopo()

    net = Mininet( topo=topo, controller = None )
    net.start()

#Currently sch_mf module is disabled
#    net['r1'].cmd( 'tc qdisc add dev r1-eth3 root handle 1: mf')

#Setup BFP. sudo cat /sys/kernel/debug/tracing/trace_pipe to see the BPF debug output
    net['r1'].cmd( 'tc qdisc add dev r1-eth3 clsact')
    net['r1'].cmd( 'tc filter add dev r1-eth3 ingress bpf da obj ../net-next/samples/bpf/xdp_fwd_kern.o sec xdp_fwd_direct')

#Setup RPS CPUs
#    net['r1'].cmd( 'sudo chmod a+rwx -R /sys/class/net/r1-eth2/queues/rx-0/')
#    net['r1'].cmd( 'sudo echo 3 > /sys/class/net/r1-eth2/queues/rx-0/rps_cpus')
#    net['r1'].cmd( 'cat /sys/class/net/r1-eth2/queues/rx-0/rps_cpus')


#Note that if you use mf module, the following test wouldn't work
#    hosts = [net['h1'], net['h2']]        
#    net.iperf(hosts, seconds=30, l4Type='UDP', udpBw='850M')

#This test however would work with mf module. But it gives very small throughput due to small window size
#    hosts = [net['h1'], net['h2']]        
#    net.iperf(hosts, seconds=30, l4Type='TCP')


# Make sure to run cubic.sh if you didn't already. Otherwise it wouldn;t work

#   testing using port 45678, TCP window size 20MB and 10 connection. 
    res = net['h2'].cmd('iperf -s -p 45678 -w 20MB &')
#Anyhing that blocks shouldn't be used in cmd(). Use popen() instead. It will create a new process. Now monitor the output of the process
    proc = net['h1'].popen('iperf -c 172.16.102.1 -p 45678 -t 30  -w 20MB -P 10')


#    print res #Don't uncomment this. Strange things happen :-(

#Parse the res to find out the PID of iperf server. The program can crash if res isn't formatted properly. Try again
    pid = res.split(" ")
    iperf_s_pid = pid[1]
    iperf_c_pid = int(pid[1]) + 1

    print pid[1], int(pid[1]) + 1

#Pin iperf server and client to core 0 and 1 respectively. Note that throughput you get depends on other applications running on a CPU. So if you get bad throughput, try restatring. Even though you close some applications, the process can sit in the backgroud
    net['h2'].cmd('sudo taskset -pc 0 {0}'.format(iperf_s_pid))
    net['h1'].cmd('sudo taskset -pc 1 {0}'.format(iperf_c_pid))
#Currently it's getting >50Gbps without isolcpus  


#Netperf try
#    net['h2'].cmd('sudo netserver -p 16604')
# -f g stands for outputing in Gb/s
#    proc = net['h1'].popen('netperf -f g -H 172.16.102.1 -p 16604 -l 30')

    for line in iter(proc.stdout.readline, b''):
	print line

    net['h2'].cmd('sudo kill -9 {0}'.format(iperf_s_pid))
    net['h1'].cmd('sudo kill -9 {0}'.format(iperf_c_pid))

    CLI( net )
    net.stop()

if __name__ == '__main__':
    args = sys.argv
    setLogLevel( 'info' )
#    cli = 0
#    if "--cli" in args:
#        cli = 1
    main()

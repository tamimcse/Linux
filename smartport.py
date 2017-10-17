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

#    net['r1'].cmd( 'tc qdisc add dev r1-eth3 root handle 1: mf')




#Note that if you use mf module, the following test wouldn't work
#    hosts = [net['h1'], net['h2']]        
#    net.iperf(hosts, seconds=30, l4Type='UDP', udpBw='850M')

#This test however would work with mf module. But it gives very small throughput due to small window size
#    hosts = [net['h1'], net['h2']]        
#    net.iperf(hosts, seconds=30, l4Type='TCP')


# Make sure to run cubic.sh if you didn't already. Otherwise it wouldn;t work

#   testing using port 45678, TCP window size 20MB and 10 connection. 
    net['h2'].cmd('iperf -s -p 45678 -w 20MB &')
#Anyhing that blocks shouldn't be used in cmd(). Use popen() instead. It will create a new process. Now monitor the output of the process
    proc = net['h1'].popen('iperf -c 172.16.102.1 -p 45678 -t 30  -w 20MB -P 10')

#    net['h2'].cmd('sudo netserver -p 16604')
#    proc = net['h1'].popen('netperf -f g -H 172.16.102.1 -p 16604 -l 30 -- -m 1500')

    for line in iter(proc.stdout.readline, b''):
	print line

    CLI( net )
    net.stop()

if __name__ == '__main__':
    args = sys.argv
    setLogLevel( 'info' )
#    cli = 0
#    if "--cli" in args:
#        cli = 1
    main()

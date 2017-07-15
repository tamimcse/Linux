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
#    .1                .1   .2               .1   
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


class NetworkTopo( Topo ):
    def build( self, **_opts ):	
        h1 = self.addHost( 'h1', ip='172.16.101.1/24', defaultRoute='via 172.16.101.2' )
        h2 = self.addHost( 'h2', ip='172.16.101.2/24', defaultRoute='via 172.16.101.1' )
	self.addLink(h1, h2)

def main(cli=0):
    "Test linux router"
    topo = NetworkTopo()

    host = custom( CPULimitedHost, cpu=0.5)
    net = Mininet( topo=topo, host=host, controller = None )
    net.start()
    net['h2'].cmd( 'tc qdisc add dev h2-eth0 root handle 1: mf')
 #   net['h2'].cmd('iperf -s -p 45678 &')
#    net['h1'].cmd('iperf -c 172.16.101.2 -p 45678 -t 30')

#    hosts = [net['h1'], net['h2']]        
#    net.iperf(hosts, seconds=30, l4Type='TCP', udpBw='1000M')

    CLI( net )
    net.stop()

if __name__ == '__main__':
    args = sys.argv
    setLogLevel( 'info' )
    cli = 0
    if "--cli" in args:
        cli = 1
    main(cli)

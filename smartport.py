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
from mininet.node import Node
from mininet.link import TCLink
from mininet.node import CPULimitedHost
from mininet.log import setLogLevel, info
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
    "A LinuxRouter connecting three IP subnets"

    def build( self, **_opts ):	
        h1 = self.addHost( 'h1', ip='172.16.101.1/24', defaultRoute='via 172.16.101.2' )
        h2 = self.addHost( 'h2', ip='172.16.102.1/24', defaultRoute='via 172.16.102.2' )


        r1 = self.addNode( 'r1', cls=LinuxRouter, ip='172.16.101.2/24' )

        self.addLink( h1, r1, intfName2='r1-eth1', params2={ 'ip' : '172.16.101.2/24' })
        self.addLink( h2, r1, intfName2='r1-eth2', params2={ 'ip' : '172.16.102.2/24' })
#       Don't move the line. It doesn't work for some reason
#        self.addLink( r1, r2, intfName1='r1-eth1', params1={ 'ip' : '172.16.10.2/24' }, intfName2='r2-eth1', params2={ 'ip' : '172.16.10.3/24' })


def main(cli=0):
    "Test linux router"
    topo = NetworkTopo()
    net = Mininet( topo=topo, controller = None )
    net.start()

    info( '*** Configuring routers:\n' )
#    net[ 'r1' ].cmd( 'ip neigh add 172.16.10.3 lladdr 2e:a9:cf:14:b4:6a dev r1-eth1' )
    net[ 'r1' ].cmd( 'ip route add 172.16.102/24 nexthop via 172.16.102.2' )

    info( '*** Routing Table on Router:\n' )
    info( net[ 'r1' ].cmd( 'route' ) )



    #net.pingAll()

    hosts = [net['h1'], net['h2']]
        

    net.iperf(hosts, seconds=30, l4Type='TCP', udpBw='10M')
    CLI( net )
    net.stop()

if __name__ == '__main__':
    args = sys.argv
    setLogLevel( 'info' )
    cli = 0
    if "--cli" in args:
        cli = 1
    main(cli)

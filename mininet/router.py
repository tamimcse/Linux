#!/usr/bin/python

"""
linuxrouter.py: Example network with Linux IP router

This example converts a Node into a router using IP forwarding
already built into Linux.


##############################################################################
# Topology with two switches and two hosts with static routes
#
#       172.16.101.0/24         172.16.10.0/24         172.16.102.0./24
#  h1 ------------------- r1 ------------------ r2------- -------------h2
#    .1                .2   .2               .3   .2                 .1
##############################################################################
"""


from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import Node
from mininet.log import setLogLevel, info
from mininet.cli import CLI


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
        r2 = self.addNode( 'r2', cls=LinuxRouter, ip='172.16.102.2/24' )

        self.addLink( h1, r1, intfName2='r1-eth0', params2={ 'ip' : '172.16.101.2/24' } )
        self.addLink( h2, r2, intfName2='r2-eth0', params2={ 'ip' : '172.16.102.2/24' } )
        self.addLink( r1, r2, intfName1='r1-eth1', params1={ 'ip' : '172.16.10.2/24' }, intfName2='r2-eth1', params2={ 'ip' : '172.16.10.3/24' } )

def run():
    "Test linux router"
    topo = NetworkTopo()
    net = Mininet( topo=topo, controller = None )
    net.start()
    info( '*** Configuring routers:\n' )
#    net[ 'r1' ].cmd( 'ip neigh add 172.16.10.3 lladdr 2e:a9:cf:14:b4:6a dev r1-eth1' )
    net[ 'r1' ].cmd( 'ip route add 172.16.102/24 nexthop via 172.16.10.3' )
    net[ 'r2' ].cmd( 'ip route add 172.16.101/24 nexthop via 172.16.10.2' )

    info( '*** Routing Table on Router:\n' )
    info( net[ 'r1' ].cmd( 'route' ) )
    CLI( net )
    net.stop()

if __name__ == '__main__':
    setLogLevel( 'info' )
    run()

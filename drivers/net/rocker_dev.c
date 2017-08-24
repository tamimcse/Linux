/*
 *  drivers/net/veth.c
 *
 *  Copyright (C) 2007 OpenVZ http://openvz.org, SWsoft Inc
 *
 * Author: Pavel Emelianov <xemul@openvz.org>
 * Ethtool interface from: Eric W. Biederman <ebiederm@xmission.com>
 *
 */

#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/ethtool.h>
#include <linux/etherdevice.h>
#include <linux/u64_stats_sync.h>
#include <linux/pci_ids.h>

#include <net/rtnetlink.h>
#include <net/dst.h>
#include <net/xfrm.h>
#include <linux/veth.h>
#include <linux/module.h>

#include "rocker_hw.h"


#define DRV_NAME	"rocker"
#define DRV_VERSION	"1.0"
#define ROCKER_FP_PORTS_MAX 62

struct pcpu_vstats {
	u64			packets;
	u64			bytes;
	struct u64_stats_sync	syncp;
};

struct rocker {
//	struct net_device __rcu	*peer;

        /* front-panel ports */
        struct net_device *fp_port[ROCKER_FP_PORTS_MAX];
        uint32_t n_ports;
        
        /* register backings */
        uint32_t test_reg;
        uint64_t test_reg64;
        dma_addr_t test_dma_addr;
        uint32_t test_dma_size;        
        
        uint16_t vendor_id;
        uint16_t device_id;
        uint8_t revision;
        uint16_t class_id;
        uint16_t subsystem_vendor_id;       /* only for header type = 0 */
        uint16_t subsystem_id;              /* only for header type = 0 */

        /*
         * pci-to-pci bridge or normal device.
         * This doesn't mean pci host switch.
         * When card bus bridge is supported, this would be enhanced.
         */
        int is_bridge;

        /* pcie stuff */
        int is_express;   /* is this device pci express? */

        /* rom bar */
        const char *romfile;        

	atomic64_t		dropped;
	unsigned		requested_headroom;
} *rocker;

/*
 * ethtool interface
 */

static struct {
	const char string[ETH_GSTRING_LEN];
} ethtool_stats_keys[] = {
	{ "peer_ifindex" },
};

static int rocker_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	cmd->supported		= 0;
	cmd->advertising	= 0;
	ethtool_cmd_speed_set(cmd, SPEED_10000);
	cmd->duplex		= DUPLEX_FULL;
	cmd->port		= PORT_TP;
	cmd->phy_address	= 0;
	cmd->transceiver	= XCVR_INTERNAL;
	cmd->autoneg		= AUTONEG_DISABLE;
	cmd->maxtxpkt		= 0;
	cmd->maxrxpkt		= 0;
	return 0;
}

static void rocker_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *info)
{
	strlcpy(info->driver, DRV_NAME, sizeof(info->driver));
	strlcpy(info->version, DRV_VERSION, sizeof(info->version));
}

static void rocker_get_strings(struct net_device *dev, u32 stringset, u8 *buf)
{
	switch(stringset) {
	case ETH_SS_STATS:
		memcpy(buf, &ethtool_stats_keys, sizeof(ethtool_stats_keys));
		break;
	}
}

static int rocker_get_sset_count(struct net_device *dev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return ARRAY_SIZE(ethtool_stats_keys);
	default:
		return -EOPNOTSUPP;
	}
}



static const struct ethtool_ops rocker_ethtool_ops = {
	.get_settings		= rocker_get_settings,
	.get_drvinfo		= rocker_get_drvinfo,
	.get_link		= ethtool_op_get_link,
	.get_strings		= rocker_get_strings,
	.get_sset_count		= rocker_get_sset_count,
};


/*
 * general routines
 */

static u64 rocker_stats_one(struct pcpu_vstats *result, struct net_device *dev)
{
	struct rocker *priv = netdev_priv(dev);
	int cpu;

	result->packets = 0;
	result->bytes = 0;
	for_each_possible_cpu(cpu) {
		struct pcpu_vstats *stats = per_cpu_ptr(dev->vstats, cpu);
		u64 packets, bytes;
		unsigned int start;

		do {
			start = u64_stats_fetch_begin_irq(&stats->syncp);
			packets = stats->packets;
			bytes = stats->bytes;
		} while (u64_stats_fetch_retry_irq(&stats->syncp, start));
		result->packets += packets;
		result->bytes += bytes;
	}
	return atomic64_read(&priv->dropped);
}


static int is_valid_rocker_mtu(int mtu)
{
	return mtu >= ETH_MIN_MTU && mtu <= ETH_MAX_MTU;
}


static void veth_dev_free(struct net_device *dev)
{
	free_percpu(dev->vstats);
	free_netdev(dev);
}


#define VETH_FEATURES (NETIF_F_SG | NETIF_F_FRAGLIST | NETIF_F_HW_CSUM | \
		       NETIF_F_RXCSUM | NETIF_F_SCTP_CRC | NETIF_F_HIGHDMA | \
		       NETIF_F_GSO_SOFTWARE | NETIF_F_GSO_ENCAP_ALL | \
		       NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_HW_VLAN_CTAG_RX | \
		       NETIF_F_HW_VLAN_STAG_TX | NETIF_F_HW_VLAN_STAG_RX )

/*
 * netlink interface
 */

static int rocker_validate(struct nlattr *tb[], struct nlattr *data[])
{
    pr_info("In rocker_validate !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	if (tb[IFLA_ADDRESS]) {
		if (nla_len(tb[IFLA_ADDRESS]) != ETH_ALEN)
			return -EINVAL;
		if (!is_valid_ether_addr(nla_data(tb[IFLA_ADDRESS])))
			return -EADDRNOTAVAIL;
	}
	if (tb[IFLA_MTU]) {
		if (!is_valid_rocker_mtu(nla_get_u32(tb[IFLA_MTU])))
			return -EINVAL;
	}
	return 0;
}

static struct rtnl_link_ops rocker_link_ops;

static int rocker_newlink(struct net *src_net, struct net_device *dev1,
			 struct nlattr *tb[], struct nlattr *data[])
{
    pr_info("In rocker_newlink !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");   
    rocker = kzalloc(sizeof(*rocker), GFP_KERNEL);
    
    struct net_device *dev;

    //Get all the netdev from namespace
    read_lock(&dev_base_lock);
    dev = first_net_device(src_net);
    while (dev) {
        rocker->fp_port[rocker->n_ports++]=dev;
        dev = next_net_device(dev);
    }      
    read_unlock(&dev_base_lock);
    
    int i;
    for(i = 0; i < rocker->n_ports; i++)
    {
        printk(KERN_INFO "found [%s]\n", rocker->fp_port[i]->name);    
    }    
    //Get namespace
    
    
    for(i = 0; i < ROCKER_FP_PORTS_MAX; i++)
    {
        
    }
//	int err;
//	struct net_device *peer, *peer2;
//	struct rocker *priv;
//	char ifname[IFNAMSIZ];
//	struct nlattr *peer_tb[IFLA_MAX + 1], **tbp;
//	unsigned char name_assign_type;
//	struct ifinfomsg *ifmp;
//	struct net *net;
//
//	/*
//	 * create and register peer first
//	 */
//	if (data != NULL && data[VETH_INFO_PEER] != NULL) {
//		struct nlattr *nla_peer;
//
//		nla_peer = data[VETH_INFO_PEER];
//		ifmp = nla_data(nla_peer);
//		err = rtnl_nla_parse_ifla(peer_tb,
//					  nla_data(nla_peer) + sizeof(struct ifinfomsg),
//					  nla_len(nla_peer) - sizeof(struct ifinfomsg));
//		if (err < 0)
//			return err;
//
//		err = rocker_validate(peer_tb, NULL);
//		if (err < 0)
//			return err;
//
//		tbp = peer_tb;
//	} else {
//		ifmp = NULL;
//		tbp = tb;
//	}
//
//	if (tbp[IFLA_IFNAME]) {
//		nla_strlcpy(ifname, tbp[IFLA_IFNAME], IFNAMSIZ);
//		name_assign_type = NET_NAME_USER;
//	} else {
//		snprintf(ifname, IFNAMSIZ, DRV_NAME "%%d");
//		name_assign_type = NET_NAME_ENUM;
//	}
//
//	net = rtnl_link_get_net(src_net, tbp);
//	if (IS_ERR(net))
//		return PTR_ERR(net);
//
//	peer = rtnl_create_link(net, ifname, name_assign_type,
//				&rocker_link_ops, tbp);
//	if (IS_ERR(peer)) {
//		put_net(net);
//		return PTR_ERR(peer);
//	}
//
//	if (tbp[IFLA_ADDRESS] == NULL)
//		eth_hw_addr_random(peer);
//
//	if (ifmp && (dev->ifindex != 0))
//		peer->ifindex = ifmp->ifi_index;
//
//	err = register_netdevice(peer);
//	put_net(net);
//	net = NULL;
//	if (err < 0)
//		goto err_register_peer;
//
//	netif_carrier_off(peer);
//
//	err = rtnl_configure_link(peer, ifmp);
//	if (err < 0)
//		goto err_configure_peer;
//
//	/*
//	 * register dev last
//	 *
//	 * note, that since we've registered new device the dev's name
//	 * should be re-allocated
//	 */
//
//	if (tb[IFLA_ADDRESS] == NULL)
//		eth_hw_addr_random(dev);
//
//	if (tb[IFLA_IFNAME])
//		nla_strlcpy(dev->name, tb[IFLA_IFNAME], IFNAMSIZ);
//	else
//		snprintf(dev->name, IFNAMSIZ, DRV_NAME "%%d");
//
//	err = register_netdevice(dev);
//	if (err < 0)
//		goto err_register_dev;
//
//	netif_carrier_off(dev);
//
//	/*
//	 * tie the deviced together
//	 */
//
//	priv = netdev_priv(dev);
//	rcu_assign_pointer(priv->peer, peer);
//
//	priv = netdev_priv(peer);
//	rcu_assign_pointer(priv->peer, dev);
//	return 0;
//
//err_register_dev:
//	/* nothing to do */
//err_configure_peer:
//	unregister_netdevice(peer);
//	return err;
//
//err_register_peer:
//	free_netdev(peer);
//	return err;
}


static const struct nla_policy rocker_policy[VETH_INFO_MAX + 1] = {
	[VETH_INFO_PEER]	= { .len = sizeof(struct ifinfomsg) },
};

static void rocker_setup(struct net_device *dev)
{

}

static struct rtnl_link_ops rocker_link_ops = {
	.kind		= DRV_NAME,
	.priv_size	= sizeof(struct rocker),
        .setup		= rocker_setup,
	.validate	= rocker_validate,
	.newlink	= rocker_newlink,
	.policy		= rocker_policy,
	.maxtype	= VETH_INFO_MAX,
};

/*
 * init/fini
 */

static __init int rocker_init(void)
{
	return rtnl_link_register(&rocker_link_ops);
}

static __exit void rocker_exit(void)
{
	rtnl_link_unregister(&rocker_link_ops);
}

module_init(rocker_init);
module_exit(rocker_exit);

MODULE_DESCRIPTION("Virtual Ethernet Tunnel");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS_RTNL_LINK(DRV_NAME);

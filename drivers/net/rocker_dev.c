/*
 *  
#include "drivers/pci/pci.h"drivers/net/veth.c
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

#include <net/rtnetlink.h>
#include <net/dst.h>
#include <net/xfrm.h>
#include <linux/veth.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/acpi.h>
#include <acpi/acpi_bus.h>

#define DRV_NAME	"rocker"
#define DRV_VERSION	"1.0"

struct pcpu_vstats {
	u64			packets;
	u64			bytes;
	struct u64_stats_sync	syncp;
};

struct veth_priv {
	struct net_device __rcu	*peer;
	atomic64_t		dropped;
	unsigned		requested_headroom;
        struct pci_dev *pci_device;
};

/*
 * ethtool interface
 */

static struct {
	const char string[ETH_GSTRING_LEN];
} ethtool_stats_keys[] = {
	{ "peer_ifindex" },
};

static int veth_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
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

static void veth_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *info)
{
	strlcpy(info->driver, DRV_NAME, sizeof(info->driver));
	strlcpy(info->version, DRV_VERSION, sizeof(info->version));
}

static void veth_get_strings(struct net_device *dev, u32 stringset, u8 *buf)
{
	switch(stringset) {
	case ETH_SS_STATS:
		memcpy(buf, &ethtool_stats_keys, sizeof(ethtool_stats_keys));
		break;
	}
}

static int veth_get_sset_count(struct net_device *dev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return ARRAY_SIZE(ethtool_stats_keys);
	default:
		return -EOPNOTSUPP;
	}
}

static void veth_get_ethtool_stats(struct net_device *dev,
		struct ethtool_stats *stats, u64 *data)
{
	struct veth_priv *priv = netdev_priv(dev);
	struct net_device *peer = rtnl_dereference(priv->peer);

	data[0] = peer ? peer->ifindex : 0;
}

static const struct ethtool_ops veth_ethtool_ops = {
	.get_settings		= veth_get_settings,
	.get_drvinfo		= veth_get_drvinfo,
	.get_link		= ethtool_op_get_link,
	.get_strings		= veth_get_strings,
	.get_sset_count		= veth_get_sset_count,
	.get_ethtool_stats	= veth_get_ethtool_stats,
};

static netdev_tx_t veth_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct veth_priv *priv = netdev_priv(dev);
	struct net_device *rcv;
	int length = skb->len;

	rcu_read_lock();
	rcv = rcu_dereference(priv->peer);
	if (unlikely(!rcv)) {
		kfree_skb(skb);
		goto drop;
	}

	if (likely(dev_forward_skb(rcv, skb) == NET_RX_SUCCESS)) {
		struct pcpu_vstats *stats = this_cpu_ptr(dev->vstats);

		u64_stats_update_begin(&stats->syncp);
		stats->bytes += length;
		stats->packets++;
		u64_stats_update_end(&stats->syncp);
	} else {
drop:
		atomic64_inc(&priv->dropped);
	}
	rcu_read_unlock();
	return NETDEV_TX_OK;
}

/*
 * general routines
 */

static u64 veth_stats_one(struct pcpu_vstats *result, struct net_device *dev)
{
	struct veth_priv *priv = netdev_priv(dev);
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

static void veth_get_stats64(struct net_device *dev,
			     struct rtnl_link_stats64 *tot)
{
	struct veth_priv *priv = netdev_priv(dev);
	struct net_device *peer;
	struct pcpu_vstats one;

	tot->tx_dropped = veth_stats_one(&one, dev);
	tot->tx_bytes = one.bytes;
	tot->tx_packets = one.packets;

	rcu_read_lock();
	peer = rcu_dereference(priv->peer);
	if (peer) {
		tot->rx_dropped = veth_stats_one(&one, peer);
		tot->rx_bytes = one.bytes;
		tot->rx_packets = one.packets;
	}
	rcu_read_unlock();
}

/* fake multicast ability */
static void veth_set_multicast_list(struct net_device *dev)
{
}

static int veth_open(struct net_device *dev)
{
	struct veth_priv *priv = netdev_priv(dev);
	struct net_device *peer = rtnl_dereference(priv->peer);

	if (!peer)
		return -ENOTCONN;

	if (peer->flags & IFF_UP) {
		netif_carrier_on(dev);
		netif_carrier_on(peer);
	}
	return 0;
}

static int veth_close(struct net_device *dev)
{
	struct veth_priv *priv = netdev_priv(dev);
	struct net_device *peer = rtnl_dereference(priv->peer);

	netif_carrier_off(dev);
	if (peer)
		netif_carrier_off(peer);

	return 0;
}

static int is_valid_veth_mtu(int mtu)
{
	return mtu >= ETH_MIN_MTU && mtu <= ETH_MAX_MTU;
}

static int veth_dev_init(struct net_device *dev)
{
	dev->vstats = netdev_alloc_pcpu_stats(struct pcpu_vstats);
	if (!dev->vstats)
		return -ENOMEM;
	return 0;
}

static void veth_dev_free(struct net_device *dev)
{
	free_percpu(dev->vstats);
	free_netdev(dev);
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void veth_poll_controller(struct net_device *dev)
{
	/* veth only receives frames when its peer sends one
	 * Since it's a synchronous operation, we are guaranteed
	 * never to have pending data when we poll for it so
	 * there is nothing to do here.
	 *
	 * We need this though so netpoll recognizes us as an interface that
	 * supports polling, which enables bridge devices in virt setups to
	 * still use netconsole
	 */
}
#endif	/* CONFIG_NET_POLL_CONTROLLER */

static int veth_get_iflink(const struct net_device *dev)
{
	struct veth_priv *priv = netdev_priv(dev);
	struct net_device *peer;
	int iflink;

	rcu_read_lock();
	peer = rcu_dereference(priv->peer);
	iflink = peer ? peer->ifindex : 0;
	rcu_read_unlock();

	return iflink;
}

static void veth_set_rx_headroom(struct net_device *dev, int new_hr)
{
	struct veth_priv *peer_priv, *priv = netdev_priv(dev);
	struct net_device *peer;

	if (new_hr < 0)
		new_hr = 0;

	rcu_read_lock();
	peer = rcu_dereference(priv->peer);
	if (unlikely(!peer))
		goto out;

	peer_priv = netdev_priv(peer);
	priv->requested_headroom = new_hr;
	new_hr = max(priv->requested_headroom, peer_priv->requested_headroom);
	dev->needed_headroom = new_hr;
	peer->needed_headroom = new_hr;

out:
	rcu_read_unlock();
}

static const struct net_device_ops veth_netdev_ops = {
	.ndo_init            = veth_dev_init,
	.ndo_open            = veth_open,
	.ndo_stop            = veth_close,
	.ndo_start_xmit      = veth_xmit,
	.ndo_get_stats64     = veth_get_stats64,
	.ndo_set_rx_mode     = veth_set_multicast_list,
	.ndo_set_mac_address = eth_mac_addr,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= veth_poll_controller,
#endif
	.ndo_get_iflink		= veth_get_iflink,
	.ndo_features_check	= passthru_features_check,
	.ndo_set_rx_headroom	= veth_set_rx_headroom,
};

#define VETH_FEATURES (NETIF_F_SG | NETIF_F_FRAGLIST | NETIF_F_HW_CSUM | \
		       NETIF_F_RXCSUM | NETIF_F_SCTP_CRC | NETIF_F_HIGHDMA | \
		       NETIF_F_GSO_SOFTWARE | NETIF_F_GSO_ENCAP_ALL | \
		       NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_HW_VLAN_CTAG_RX | \
		       NETIF_F_HW_VLAN_STAG_TX | NETIF_F_HW_VLAN_STAG_RX )

static void veth_setup(struct net_device *dev)
{
	ether_setup(dev);

	dev->priv_flags &= ~IFF_TX_SKB_SHARING;
	dev->priv_flags |= IFF_LIVE_ADDR_CHANGE;
	dev->priv_flags |= IFF_NO_QUEUE;
	dev->priv_flags |= IFF_PHONY_HEADROOM;

	dev->netdev_ops = &veth_netdev_ops;
	dev->ethtool_ops = &veth_ethtool_ops;
	dev->features |= NETIF_F_LLTX;
	dev->features |= VETH_FEATURES;
	dev->vlan_features = dev->features &
			     ~(NETIF_F_HW_VLAN_CTAG_TX |
			       NETIF_F_HW_VLAN_STAG_TX |
			       NETIF_F_HW_VLAN_CTAG_RX |
			       NETIF_F_HW_VLAN_STAG_RX);
	dev->destructor = veth_dev_free;
	dev->max_mtu = ETH_MAX_MTU;

	dev->hw_features = VETH_FEATURES;
	dev->hw_enc_features = VETH_FEATURES;
	dev->mpls_features = NETIF_F_HW_CSUM | NETIF_F_GSO_SOFTWARE;
}

/*
 * netlink interface
 */

static int veth_validate(struct nlattr *tb[], struct nlattr *data[])
{
	if (tb[IFLA_ADDRESS]) {
		if (nla_len(tb[IFLA_ADDRESS]) != ETH_ALEN)
			return -EINVAL;
		if (!is_valid_ether_addr(nla_data(tb[IFLA_ADDRESS])))
			return -EADDRNOTAVAIL;
	}
	if (tb[IFLA_MTU]) {
		if (!is_valid_veth_mtu(nla_get_u32(tb[IFLA_MTU])))
			return -EINVAL;
	}
	return 0;
}

static struct rtnl_link_ops veth_link_ops;

static void release_pcibus_dev(struct device *dev)
{
//	struct pci_bus *pci_bus = to_pci_bus(dev);
//
//	put_device(pci_bus->bridge);
//	pci_bus_remove_resources(pci_bus);
//	pci_release_bus_of_node(pci_bus);
//	kfree(pci_bus);
}

static ssize_t pci_dev_show_local_cpu(struct device *dev, bool list,
				      struct device_attribute *attr, char *buf)
{
	const struct cpumask *mask;

#ifdef CONFIG_NUMA
	mask = (dev_to_node(dev) == -1) ? cpu_online_mask :
					  cpumask_of_node(dev_to_node(dev));
#else
	mask = cpumask_of_pcibus(to_pci_dev(dev)->bus);
#endif
	return cpumap_print_to_pagebuf(list, buf, mask);
}

static ssize_t local_cpus_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return pci_dev_show_local_cpu(dev, false, attr, buf);
}
static DEVICE_ATTR_RO(local_cpus);

static ssize_t local_cpulist_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return pci_dev_show_local_cpu(dev, true, attr, buf);
}
static DEVICE_ATTR_RO(local_cpulist);

/*
 * PCI Bus Class Devices
 */
static ssize_t cpuaffinity_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const struct cpumask *cpumask = cpumask_of_pcibus(to_pci_bus(dev));

	return cpumap_print_to_pagebuf(false, buf, cpumask);
}
static DEVICE_ATTR_RO(cpuaffinity);

static ssize_t cpulistaffinity_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	const struct cpumask *cpumask = cpumask_of_pcibus(to_pci_bus(dev));

	return cpumap_print_to_pagebuf(true, buf, cpumask);
}
static DEVICE_ATTR_RO(cpulistaffinity);

/* show resources */
static ssize_t resource_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct pci_dev *pci_dev = to_pci_dev(dev);
	char *str = buf;
	int i;
	int max;
	resource_size_t start, end;

	if (pci_dev->subordinate)
		max = DEVICE_COUNT_RESOURCE;
	else
		max = PCI_BRIDGE_RESOURCES;

	for (i = 0; i < max; i++) {
		struct resource *res =  &pci_dev->resource[i];
		pci_resource_to_user(pci_dev, i, res, &start, &end);
		str += sprintf(str, "0x%016llx 0x%016llx 0x%016llx\n",
			       (unsigned long long)start,
			       (unsigned long long)end,
			       (unsigned long long)res->flags);
	}
	return (str - buf);
}
static DEVICE_ATTR_RO(resource);


static struct attribute *pcibus_attrs[] = {
	&dev_attr_cpuaffinity.attr,
	&dev_attr_cpulistaffinity.attr,
	NULL,
};


static const struct attribute_group pcibus_group = {
	.attrs = pcibus_attrs,
};


const struct attribute_group *pcibus_groups[] = {
	&pcibus_group,
	NULL,
};

static struct class pcibus_class = {
	.name		= "virtual_pci_bus",
	.dev_release	= &release_pcibus_dev,
	.dev_groups	= pcibus_groups,
};

static struct pci_bus *pci_alloc_bus(struct pci_bus *parent)
{
	struct pci_bus *b;

	b = kzalloc(sizeof(*b), GFP_KERNEL);
	if (!b)
		return NULL;

	INIT_LIST_HEAD(&b->node);
	INIT_LIST_HEAD(&b->children);
	INIT_LIST_HEAD(&b->devices);
	INIT_LIST_HEAD(&b->slots);
	INIT_LIST_HEAD(&b->resources);
	b->max_bus_speed = PCI_SPEED_UNKNOWN;
	b->cur_bus_speed = PCI_SPEED_UNKNOWN;
#ifdef CONFIG_PCI_DOMAINS_GENERIC
	if (parent)
		b->domain_nr = parent->domain_nr;
#endif
	return b;
}

struct pci_sysdata sd = {
        .domain = 0,
        .node = 2,
//        .companion = root->device
};

static void pci_dma_configure(struct pci_dev *dev)
{
    enum dev_dma_attr attr = DEV_DMA_COHERENT;
    acpi_dma_configure(&dev->dev, attr);
//	struct device *bridge = pci_get_host_bridge_device(dev);
//
//	if (IS_ENABLED(CONFIG_OF) &&
//		bridge->parent && bridge->parent->of_node) {
//			of_dma_configure(&dev->dev, bridge->parent->of_node);
//	} else if (has_acpi_companion(bridge)) {
//		struct acpi_device *adev = to_acpi_device_node(bridge->fwnode);
//		enum dev_dma_attr attr = acpi_get_dma_attr(adev);
//
//		if (attr == DEV_DMA_NOT_SUPPORTED)
//			dev_warn(&dev->dev, "DMA not supported.\n");
//		else
//			acpi_dma_configure(&dev->dev, attr);
//	}
//
//	pci_put_host_bridge_device(bridge);
}

static void pci_set_msi_domain(struct pci_dev *dev)
{
//	struct irq_domain *d;
//
//	/*
//	 * If the platform or firmware interfaces cannot supply a
//	 * device-specific MSI domain, then inherit the default domain
//	 * from the host bridge itself.
//	 */
//	d = pci_dev_msi_domain(dev);
//	if (!d)
//		d = dev_get_msi_domain(&dev->bus->dev);
//
//	dev_set_msi_domain(&dev->dev, d);
}

int pcibios_add_device(struct pci_dev *dev)
{
//	struct setup_data *data;
//	struct pci_setup_rom *rom;
//	u64 pa_data;
//
//	pa_data = boot_params.hdr.setup_data;
//	while (pa_data) {
//		data = ioremap(pa_data, sizeof(*rom));
//		if (!data)
//			return -ENOMEM;
//
//		if (data->type == SETUP_PCI) {
//			rom = (struct pci_setup_rom *)data;
//
//			if ((pci_domain_nr(dev->bus) == rom->segment) &&
//			    (dev->bus->number == rom->bus) &&
//			    (PCI_SLOT(dev->devfn) == rom->device) &&
//			    (PCI_FUNC(dev->devfn) == rom->function) &&
//			    (dev->vendor == rom->vendor) &&
//			    (dev->device == rom->devid)) {
//				dev->rom = pa_data +
//				      offsetof(struct pci_setup_rom, romdata);
//				dev->romlen = rom->pcilen;
//			}
//		}
//		pa_data = data->next;
//		iounmap(data);
//	}
//	set_dma_domain_ops(dev);
//	set_dev_domain_options(dev);
	return 0;
}

void pci_device_add(struct pci_dev *dev, struct pci_bus *bus)
{
    int ret;
    dev->pcie_mpss = 16;       
    set_dev_node(&dev->dev, pcibus_to_node(bus));    
    dev->dev.dma_mask = &dev->dma_mask;
    dev->dev.dma_parms = &dev->dma_parms;
    dev->dev.coherent_dma_mask = 0xffffffffull;    
    pci_dma_configure(dev);
    pci_set_dma_max_seg_size(dev, 65536);
    pci_set_dma_seg_boundary(dev, 0xffffffff);
    /* Clear the state_saved flag. */
    dev->state_saved = false; 
    
    list_add_tail(&dev->bus_list, &bus->devices);
    
    ret = pcibios_add_device(dev);
    WARN_ON(ret < 0);

    /* Setup MSI irq domain */
    pci_set_msi_domain(dev);

    /* Notifier could use PCI capabilities */
    dev->match_driver = false;
//    ret = device_add(&dev->dev);    
}

void set_pcie_port_type(struct pci_dev *pdev)
{
	int pos;
	u16 reg16;
	int type;
	struct pci_dev *parent;

//	pos = pci_find_capability(pdev, PCI_CAP_ID_EXP);
//	if (!pos)
//		return;
        pdev->pcie_cap = PCI_CAP_ID_EXP;
//	pci_read_config_word(pdev, pos + PCI_EXP_FLAGS, &reg16);
//	pdev->pcie_flags_reg = reg16;
        pdev->pcie_flags_reg = PCI_EXP_FLAGS_TYPE;
//	pci_read_config_word(pdev, pos + PCI_EXP_DEVCAP, &reg16);
//	pdev->pcie_mpss = reg16 & PCI_EXP_DEVCAP_PAYLOAD;
        pdev->pcie_mpss = 4096 & PCI_EXP_DEVCAP_PAYLOAD;

//	/*
//	 * A Root Port or a PCI-to-PCIe bridge is always the upstream end
//	 * of a Link.  No PCIe component has two Links.  Two Links are
//	 * connected by a Switch that has a Port on each Link and internal
//	 * logic to connect the two Ports.
//	 */
//	type = pci_pcie_type(pdev);
//	if (type == PCI_EXP_TYPE_ROOT_PORT ||
//	    type == PCI_EXP_TYPE_PCIE_BRIDGE)
//		pdev->has_secondary_link = 1;
//	else if (type == PCI_EXP_TYPE_UPSTREAM ||
//		 type == PCI_EXP_TYPE_DOWNSTREAM) {
//		parent = pci_upstream_bridge(pdev);
//
//		/*
//		 * Usually there's an upstream device (Root Port or Switch
//		 * Downstream Port), but we can't assume one exists.
//		 */
//		if (parent && !parent->has_secondary_link)
//			pdev->has_secondary_link = 1;
//	}
        pdev->has_secondary_link = 1;
}

static ssize_t rocker_dev_read_config(struct file *filp, struct kobject *kobj,
			       struct bin_attribute *bin_attr, char *buf,
			       loff_t off, size_t count)
{
    
    pr_info("Inside rocker_dev_read_config !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    return 3;
//	struct pci_dev *dev = to_pci_dev(kobj_to_dev(kobj));
//	unsigned int size = 64;
//	loff_t init_off = off;
//	u8 *data = (u8 *) buf;
//
//	/* Several chips lock up trying to read undefined config space */
//	if (file_ns_capable(filp, &init_user_ns, CAP_SYS_ADMIN))
//		size = dev->cfg_size;
//	else if (dev->hdr_type == PCI_HEADER_TYPE_CARDBUS)
//		size = 128;
//
//	if (off > size)
//		return 0;
//	if (off + count > size) {
//		size -= off;
//		count = size;
//	} else {
//		size = count;
//	}
//
//	pci_config_pm_runtime_get(dev);
//
//	if ((off & 1) && size) {
//		u8 val;
//		pci_user_read_config_byte(dev, off, &val);
//		data[off - init_off] = val;
//		off++;
//		size--;
//	}
//
//	if ((off & 3) && size > 2) {
//		u16 val;
//		pci_user_read_config_word(dev, off, &val);
//		data[off - init_off] = val & 0xff;
//		data[off - init_off + 1] = (val >> 8) & 0xff;
//		off += 2;
//		size -= 2;
//	}
//
//	while (size > 3) {
//		u32 val;
//		pci_user_read_config_dword(dev, off, &val);
//		data[off - init_off] = val & 0xff;
//		data[off - init_off + 1] = (val >> 8) & 0xff;
//		data[off - init_off + 2] = (val >> 16) & 0xff;
//		data[off - init_off + 3] = (val >> 24) & 0xff;
//		off += 4;
//		size -= 4;
//	}
//
//	if (size >= 2) {
//		u16 val;
//		pci_user_read_config_word(dev, off, &val);
//		data[off - init_off] = val & 0xff;
//		data[off - init_off + 1] = (val >> 8) & 0xff;
//		off += 2;
//		size -= 2;
//	}
//
//	if (size > 0) {
//		u8 val;
//		pci_user_read_config_byte(dev, off, &val);
//		data[off - init_off] = val;
//		off++;
//		--size;
//	}
//
//	pci_config_pm_runtime_put(dev);
//
//	return count;
}

static ssize_t rocker_dev_write_config(struct file *filp, struct kobject *kobj,
				struct bin_attribute *bin_attr, char *buf,
				loff_t off, size_t count)
{
    pr_info("Inside rocker_dev_write_config !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    return 3;
//	struct pci_dev *dev = to_pci_dev(kobj_to_dev(kobj));
//	unsigned int size = count;
//	loff_t init_off = off;
//	u8 *data = (u8 *) buf;
//
//	if (off > dev->cfg_size)
//		return 0;
//	if (off + count > dev->cfg_size) {
//		size = dev->cfg_size - off;
//		count = size;
//	}
//
//	pci_config_pm_runtime_get(dev);
//
//	if ((off & 1) && size) {
//		pci_user_write_config_byte(dev, off, data[off - init_off]);
//		off++;
//		size--;
//	}
//
//	if ((off & 3) && size > 2) {
//		u16 val = data[off - init_off];
//		val |= (u16) data[off - init_off + 1] << 8;
//		pci_user_write_config_word(dev, off, val);
//		off += 2;
//		size -= 2;
//	}
//
//	while (size > 3) {
//		u32 val = data[off - init_off];
//		val |= (u32) data[off - init_off + 1] << 8;
//		val |= (u32) data[off - init_off + 2] << 16;
//		val |= (u32) data[off - init_off + 3] << 24;
//		pci_user_write_config_dword(dev, off, val);
//		off += 4;
//		size -= 4;
//	}
//
//	if (size >= 2) {
//		u16 val = data[off - init_off];
//		val |= (u16) data[off - init_off + 1] << 8;
//		pci_user_write_config_word(dev, off, val);
//		off += 2;
//		size -= 2;
//	}
//
//	if (size) {
//		pci_user_write_config_byte(dev, off, data[off - init_off]);
//		off++;
//		--size;
//	}
//
//	pci_config_pm_runtime_put(dev);
//
//	return count;
}

static struct bin_attribute rocker_dev_config_attr = {
	.attr =	{
		.name = "config",
		.mode = S_IRUGO | S_IWUSR,
	},
	.size = PCI_CFG_SPACE_EXP_SIZE,
	.read = rocker_dev_read_config,
	.write = rocker_dev_write_config,
};


static int pci_create_attr(struct pci_dev *pdev, int num, int write_combine)
{

	int name_len = write_combine ? 13 : 10;
	struct bin_attribute *res_attr;
	char *res_attr_name;
	int retval;

	res_attr = kzalloc(sizeof(*res_attr) + name_len, GFP_ATOMIC);
	if (!res_attr)
		return -ENOMEM;

	res_attr_name = (char *)(res_attr + 1);

	sysfs_bin_attr_init(res_attr);
//	if (write_combine) {
//		pdev->res_attr_wc[num] = res_attr;
//		sprintf(res_attr_name, "resource%d_wc", num);
////		res_attr->mmap = pci_mmap_resource_wc;
//	} else {
//		pdev->res_attr[num] = res_attr;
//		sprintf(res_attr_name, "resource%d", num);
////		res_attr->mmap = pci_mmap_resource_uc;
//	}
        pdev->res_attr[num] = res_attr;
        sprintf(res_attr_name, "resource%d", num);        
        res_attr->read = rocker_dev_read_config;
        res_attr->write = rocker_dev_write_config;        
        res_attr->mmap = NULL;        
//	if (pci_resource_flags(pdev, num) & IORESOURCE_IO) {
//		res_attr->read = pci_read_config;
//		res_attr->write = pci_write_config;
//	}
	res_attr->attr.name = res_attr_name;
	res_attr->attr.mode = S_IRUSR | S_IWUSR;
	res_attr->size = pci_resource_len(pdev, num);
	res_attr->private = &pdev->resource[num];
                
	retval = sysfs_create_bin_file(&pdev->dev.kobj, res_attr);
	if (retval)
		kfree(res_attr);

	return retval;
}


int pci_setup_device(struct pci_dev *dev)
{
	u16 cmd;
	u8 hdr_type;
	int pos = 0;
	struct pci_bus_region region;
	struct resource *res;
        int i, retval;

        //slot=0, function=0    
        dev->devfn = PCI_DEVFN(0, 0);

        
        dev->vendor = 0x1b36;
        dev->device = 0x0006;
        dev->subsystem_vendor = 0x0000; 
        dev->subsystem_device = 0x0000; 

        dev_set_name(&dev->dev, "rockerdev");
        
        hdr_type = PCI_HEADER_TYPE_NORMAL;
	dev->sysdata = dev->bus->sysdata;
	dev->dev.parent = dev->bus->bridge;
	dev->dev.bus = &pci_bus_type;
	dev->hdr_type = hdr_type & 0x7f;
	dev->multifunction = !!(hdr_type & 0x80);
	dev->error_state = pci_channel_io_normal;
	set_pcie_port_type(dev);
	dev->dma_mask = 0xffffffff;
        dev->irq = 47; //Free in /proc/interrupts
        dev->pin = 0x0;        
	dev->revision = 0x01;
	dev->class = 0x2800;
        dev->block_cfg_access = 0;
        dev->is_physfn = 0;//Not a physical PCI
        dev->is_virtfn = 1; //The device is a virtual function, not physcal function
        
        
	dev_set_name(&dev->dev, "%04x:%02x:%02x.%d", pci_domain_nr(dev->bus),
		     dev->bus->number, PCI_SLOT(dev->devfn),
		     PCI_FUNC(dev->devfn));

	dev_printk(KERN_DEBUG, &dev->dev, "[%04x:%04x] type %02x class %#08x\n",
		   dev->vendor, dev->device, dev->hdr_type, dev->class);

	/* need to have dev->class ready */
	dev->cfg_size = PCI_CFG_SPACE_EXP_SIZE;

	/* "Unknown power state" */
	dev->current_state = PCI_UNKNOWN;       

	/* Expose the PCI resources from this device as files */
//	for (i = 0; i < PCI_ROM_RESOURCE; i++) 
//        {
//            retval = pci_create_attr(dev, i, 0);
//	}     
        
//        dev->res_attr[0] = &pcie_config_attr;

//	switch (dev->hdr_type) {		    /* header type */
//	case PCI_HEADER_TYPE_NORMAL:		    /* standard header */
//		if (class == PCI_CLASS_BRIDGE_PCI)
//			goto bad;
//		pci_read_irq(dev);
//		pci_read_bases(dev, 6, PCI_ROM_ADDRESS);

//
//		/*
//		 * Do the ugly legacy mode stuff here rather than broken chip
//		 * quirk code. Legacy mode ATA controllers have fixed
//		 * addresses. These are not always echoed in BAR0-3, and
//		 * BAR0-3 in a few cases contain junk!
//		 */
//		if (class == PCI_CLASS_STORAGE_IDE) {
//			u8 progif;
//			pci_read_config_byte(dev, PCI_CLASS_PROG, &progif);
//			if ((progif & 1) == 0) {
//				region.start = 0x1F0;
//				region.end = 0x1F7;
//				res = &dev->resource[0];
//				res->flags = LEGACY_IO_RESOURCE;
//				pcibios_bus_to_resource(dev->bus, res, &region);
//				dev_info(&dev->dev, "legacy IDE quirk: reg 0x10: %pR\n",
//					 res);
//				region.start = 0x3F6;
//				region.end = 0x3F6;
//				res = &dev->resource[1];
//				res->flags = LEGACY_IO_RESOURCE;
//				pcibios_bus_to_resource(dev->bus, res, &region);
//				dev_info(&dev->dev, "legacy IDE quirk: reg 0x14: %pR\n",
//					 res);
//			}
//			if ((progif & 4) == 0) {
//				region.start = 0x170;
//				region.end = 0x177;
//				res = &dev->resource[2];
//				res->flags = LEGACY_IO_RESOURCE;
//				pcibios_bus_to_resource(dev->bus, res, &region);
//				dev_info(&dev->dev, "legacy IDE quirk: reg 0x18: %pR\n",
//					 res);
//				region.start = 0x376;
//				region.end = 0x376;
//				res = &dev->resource[3];
//				res->flags = LEGACY_IO_RESOURCE;
//				pcibios_bus_to_resource(dev->bus, res, &region);
//				dev_info(&dev->dev, "legacy IDE quirk: reg 0x1c: %pR\n",
//					 res);
//			}
//		}
//		break;
//
//	case PCI_HEADER_TYPE_BRIDGE:		    /* bridge header */
//		if (class != PCI_CLASS_BRIDGE_PCI)
//			goto bad;
//		/* The PCI-to-PCI bridge spec requires that subtractive
//		   decoding (i.e. transparent) bridge must have programming
//		   interface code of 0x01. */
//		pci_read_irq(dev);
//		dev->transparent = ((dev->class & 0xff) == 1);
//		pci_read_bases(dev, 2, PCI_ROM_ADDRESS1);
//		set_pcie_hotplug_bridge(dev);
//		pos = pci_find_capability(dev, PCI_CAP_ID_SSVID);
//		if (pos) {
//			pci_read_config_word(dev, pos + PCI_SSVID_VENDOR_ID, &dev->subsystem_vendor);
//			pci_read_config_word(dev, pos + PCI_SSVID_DEVICE_ID, &dev->subsystem_device);
//		}
//		break;
//
//	case PCI_HEADER_TYPE_CARDBUS:		    /* CardBus bridge header */
//		if (class != PCI_CLASS_BRIDGE_CARDBUS)
//			goto bad;
//		pci_read_irq(dev);
//		pci_read_bases(dev, 1, 0);
//		pci_read_config_word(dev, PCI_CB_SUBSYSTEM_VENDOR_ID, &dev->subsystem_vendor);
//		pci_read_config_word(dev, PCI_CB_SUBSYSTEM_ID, &dev->subsystem_device);
//		break;
//
//	default:				    /* unknown header */
//		dev_err(&dev->dev, "unknown header type %02x, ignoring device\n",
//			dev->hdr_type);
//		return -EIO;
//
//	bad:
//		dev_err(&dev->dev, "ignoring class %#08x (doesn't match header type %02x)\n",
//			dev->class, dev->hdr_type);
//		dev->class = PCI_CLASS_NOT_DEFINED << 8;
//	}

	/* We found a fine healthy device, go go go... */
	return 0;
}

static int rocker_bus_read(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 *value)
{
    pr_info("rocker_bus_read !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    *value = 8;
    return 0;
//	return raw_pci_read(pci_domain_nr(bus), bus->number,
//				 devfn, where, size, value);
}

static int rocker_bus_write(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 value)
{
    pr_info("rocker_bus_write !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    return 0;
//	return raw_pci_write(pci_domain_nr(bus), bus->number,
//				  devfn, where, size, value);
}

struct pci_ops rocker_bus = {
	.read = rocker_bus_read,
	.write = rocker_bus_write,
};

static struct pci_dev* create_virtual_pci_dev(void)
{
    int retval;
    int err;
    int i;
    struct pci_dev *dev;
    char *name;
    struct pci_bus *bus = pci_alloc_bus(NULL);
    if(!bus)
        return NULL;

    
    //Use default domain 0000
    //Bus # 0,8,9,10 has been occupied in my PC 
    bus->number = 11;
    bus->parent = NULL; //For virtual bus
    dev_set_name(&bus->dev, "rocker_bus");
    bus->sysdata = kzalloc(sizeof(sd), GFP_KERNEL);
    memcpy(bus->sysdata, &sd, sizeof(sd));  
    bus->ops = &rocker_bus;
    
    name = dev_name(&bus->dev);
    
//    bus->dev.class = &pcibus_class;
//    class_register(&pcibus_class);
    err = device_register(&bus->dev);
    if (err)
        return NULL;
    
    dev = pci_alloc_dev(bus);
    if (!dev)
            return NULL;
    
    pci_set_of_node(dev);
    pci_device_add(dev, bus);

    if (pci_setup_device(dev)) {
            pci_bus_put(dev->bus);
            kfree(dev);
            return NULL;
    }

    err = device_register(&dev->dev);
    if(err)
        return NULL;
    pr_info("Virtual PCI Device has been registered !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

    //Expose config file from this device as file
    retval = sysfs_create_bin_file(&dev->dev.kobj, &rocker_dev_config_attr);
    if(retval)
        return retval;
    pr_info("Config file has been created !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");    
    
    
    /* Expose the PCI resources from this device as files */
    for (i = 0; i < PCI_ROM_RESOURCE; i++) 
    {
        retval = pci_create_attr(dev, i, 0);
        if(retval)
            pr_err("Couldn't create resource attribute !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");            
    }     
    
    
    return dev;
}

static int veth_newlink(struct net *src_net, struct net_device *dev,
			 struct nlattr *tb[], struct nlattr *data[])
{
	int err;
	struct net_device *peer;
	struct veth_priv *priv;
	char ifname[IFNAMSIZ];
	struct nlattr *peer_tb[IFLA_MAX + 1], **tbp;
	unsigned char name_assign_type;
	struct ifinfomsg *ifmp;
	struct net *net;
        struct pci_dev *pcidev = create_virtual_pci_dev();
        pr_info("Virtual PCI Device has been created !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        priv = netdev_priv(dev);
        if(pcidev != 0)
            priv->pci_device = pcidev;
        
	/*
	 * create and register peer first
	 */
	if (data != NULL && data[VETH_INFO_PEER] != NULL) {
		struct nlattr *nla_peer;

		nla_peer = data[VETH_INFO_PEER];
		ifmp = nla_data(nla_peer);
		err = rtnl_nla_parse_ifla(peer_tb,
					  nla_data(nla_peer) + sizeof(struct ifinfomsg),
					  nla_len(nla_peer) - sizeof(struct ifinfomsg));
		if (err < 0)
			return err;

		err = veth_validate(peer_tb, NULL);
		if (err < 0)
			return err;

		tbp = peer_tb;
	} else {
		ifmp = NULL;
		tbp = tb;
	}

	if (tbp[IFLA_IFNAME]) {
		nla_strlcpy(ifname, tbp[IFLA_IFNAME], IFNAMSIZ);
		name_assign_type = NET_NAME_USER;
	} else {
		snprintf(ifname, IFNAMSIZ, DRV_NAME "%%d");
		name_assign_type = NET_NAME_ENUM;
	}

	net = rtnl_link_get_net(src_net, tbp);
	if (IS_ERR(net))
		return PTR_ERR(net);

	peer = rtnl_create_link(net, ifname, name_assign_type,
				&veth_link_ops, tbp);
	if (IS_ERR(peer)) {
		put_net(net);
		return PTR_ERR(peer);
	}

	if (tbp[IFLA_ADDRESS] == NULL)
		eth_hw_addr_random(peer);

	if (ifmp && (dev->ifindex != 0))
		peer->ifindex = ifmp->ifi_index;

	err = register_netdevice(peer);
	put_net(net);
	net = NULL;
	if (err < 0)
		goto err_register_peer;

	netif_carrier_off(peer);

	err = rtnl_configure_link(peer, ifmp);
	if (err < 0)
		goto err_configure_peer;

	/*
	 * register dev last
	 *
	 * note, that since we've registered new device the dev's name
	 * should be re-allocated
	 */

	if (tb[IFLA_ADDRESS] == NULL)
		eth_hw_addr_random(dev);

	if (tb[IFLA_IFNAME])
		nla_strlcpy(dev->name, tb[IFLA_IFNAME], IFNAMSIZ);
	else
		snprintf(dev->name, IFNAMSIZ, DRV_NAME "%%d");

	err = register_netdevice(dev);
	if (err < 0)
		goto err_register_dev;

	netif_carrier_off(dev);

	/*
	 * tie the deviced together
	 */

	priv = netdev_priv(dev);
	rcu_assign_pointer(priv->peer, peer);

	priv = netdev_priv(peer);
	rcu_assign_pointer(priv->peer, dev);
	return 0;

err_register_dev:
	/* nothing to do */
err_configure_peer:
	unregister_netdevice(peer);
	return err;

err_register_peer:
	free_netdev(peer);
	return err;
}

static void veth_dellink(struct net_device *dev, struct list_head *head)
{
	struct veth_priv *priv;
	struct net_device *peer;

	priv = netdev_priv(dev);
	peer = rtnl_dereference(priv->peer);

	/* Note : dellink() is called from default_device_exit_batch(),
	 * before a rcu_synchronize() point. The devices are guaranteed
	 * not being freed before one RCU grace period.
	 */
	RCU_INIT_POINTER(priv->peer, NULL);
	unregister_netdevice_queue(dev, head);

	if (peer) {
		priv = netdev_priv(peer);
		RCU_INIT_POINTER(priv->peer, NULL);
		unregister_netdevice_queue(peer, head);
	}
}

static const struct nla_policy veth_policy[VETH_INFO_MAX + 1] = {
	[VETH_INFO_PEER]	= { .len = sizeof(struct ifinfomsg) },
};

static struct net *veth_get_link_net(const struct net_device *dev)
{
	struct veth_priv *priv = netdev_priv(dev);
	struct net_device *peer = rtnl_dereference(priv->peer);

	return peer ? dev_net(peer) : dev_net(dev);
}

static struct rtnl_link_ops veth_link_ops = {
	.kind		= DRV_NAME,
	.priv_size	= sizeof(struct veth_priv),
	.setup		= veth_setup,
	.validate	= veth_validate,
	.newlink	= veth_newlink,
	.dellink	= veth_dellink,
	.policy		= veth_policy,
	.maxtype	= VETH_INFO_MAX,
	.get_link_net	= veth_get_link_net,
};

/*
 * init/fini
 */

static __init int veth_init(void)
{
	return rtnl_link_register(&veth_link_ops);
}

static __exit void veth_exit(void)
{
	rtnl_link_unregister(&veth_link_ops);
}

module_init(veth_init);
module_exit(veth_exit);

MODULE_DESCRIPTION("Virtual Ethernet Tunnel");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS_RTNL_LINK(DRV_NAME);

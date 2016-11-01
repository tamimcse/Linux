#include <linux/kernel.h>
#include <linux/module.h>

#include <net/sock.h>
#include <net/netlink.h>

#define MSG_TCP_MF (0x10 + 2)  // + 2 is arbitrary. same value for kern/usr

static struct sock *tcp_mf_nl_sock;

DEFINE_MUTEX(tcp_mf_mutex);

static int
rcv_msg(struct sk_buff *skb, struct nlmsghdr *nlh)
{
    int type;
    char *data;

    type = nlh->nlmsg_type;
    if (type != MSG_TCP_MF) {
        printk("%s: expect %#x got %#x\n", __func__, MSG_TCP_MF, type);
        return -EINVAL;
    }

    data = NLMSG_DATA(nlh);
    printk("%s: %02x %02x %02x %02x %02x %02x %02x %02x\n", __func__,
            data[0], data[1], data[2], data[3],
            data[4], data[5], data[6], data[7]);
    return 0;
}

static void
nl_rcv_msg(struct sk_buff *skb)
{
    mutex_lock(&tcp_mf_mutex);
    netlink_rcv_skb(skb, &rcv_msg);
    mutex_unlock(&tcp_mf_mutex);
}

static int
tcp_mf_init(void)
{
    struct netlink_kernel_cfg cfg = {
        .input = nl_rcv_msg,
    };    
    tcp_mf_nl_sock = netlink_kernel_create(&init_net, NETLINK_USERSOCK, &cfg);    
    if (!tcp_mf_nl_sock) {
        printk(KERN_ERR "%s: receive handler registration failed\n", __func__);
        return -ENOMEM;
    }

    return 0;
}

static void
tcp_mf_exit(void)
{
    if (tcp_mf_nl_sock) {
        netlink_kernel_release(tcp_mf_nl_sock);
    }
}

module_init(tcp_mf_init);
module_exit(tcp_mf_exit);
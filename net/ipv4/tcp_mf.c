#include <linux/module.h>
#include <net/sock.h> 
#include <linux/netlink.h>
#include <linux/skbuff.h> 
#define NETLINK_USER 31

struct sock *nl_sk = NULL;
static u8 current_rate = 0, 
        req_rate = 0;
int pid = -1;


void update_rate(u8 rate)
{
    struct nlmsghdr *nlh;
    struct sk_buff *skb_out;
    int msg_size;
    int res;
    
    //update current rate
    current_rate = rate;
    
    //send the rate to the userspace
    msg_size = sizeof(current_rate);
    skb_out = nlmsg_new(msg_size, 0);
    if (!skb_out) {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    memcpy(nlmsg_data(nlh), &current_rate, msg_size);
    printk(KERN_INFO "Updating current_rate\n");

    res = nlmsg_unicast(nl_sk, skb_out, pid);
    if (res < 0)
        printk(KERN_INFO "Error while sending bak to user\n");
}

static void recv_msg(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    struct sk_buff *skb_out;
    int msg_size;
    int res;
    char *msg_recv;
    char *msg;
    
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    nlh = (struct nlmsghdr *)skb->data;
    msg_recv = (char *)nlmsg_data(nlh);
    printk(KERN_INFO "Netlink received msg payload:%s\n", msg_recv);
    if(kstrtou8(msg_recv, 10, &req_rate))
    {
        printk(KERN_ERR "Couldn't convert the req_rate from string to u8 \n");
        return;
        
    }
    pid = nlh->nlmsg_pid; /*pid of sending process */
    printk(KERN_INFO "Message received from userspace. req_rate= %d", req_rate);
    
    msg = "Message received in kernel";

    msg_size = strlen(msg);

    skb_out = nlmsg_new(msg_size, 0);
    if (!skb_out) {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    strncpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_unicast(nl_sk, skb_out, pid);
    if (res < 0)
        printk(KERN_INFO "Error while sending bak to user\n");
}

static int __init tcp_nl_init(void)
{

    printk("Entering: %s\n", __FUNCTION__);
    //nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, 0, hello_nl_recv_msg, NULL, THIS_MODULE);
    struct netlink_kernel_cfg cfg = {
        .input = recv_msg,
    };

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sk) {
        printk(KERN_ALERT "Error creating socket.\n");
        return -10;
    }
    printk(KERN_INFO "Created NETLINK_USER socket.\n");

    return 0;
}

static void __exit tcp_nl_exit(void)
{

    printk(KERN_INFO "exiting hello module\n");
    netlink_kernel_release(nl_sk);
}

module_init(tcp_nl_init); 
module_exit(tcp_nl_exit);


MODULE_AUTHOR("MD Iftakharul Islam");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("TCP CWND");
MODULE_VERSION("0.1");

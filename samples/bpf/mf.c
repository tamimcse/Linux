/* Copyright (c) 2016 Facebook
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 */
#define KBUILD_MODNAME "foo"
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/in.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <uapi/linux/bpf.h>
#include <net/ip.h>
#include "bpf_helpers.h"

#define DEFAULT_PKTGEN_UDP_PORT 9

/* copy of 'struct ethhdr' without __packed */
struct eth_hdr {
	unsigned char   h_dest[ETH_ALEN];
	unsigned char   h_source[ETH_ALEN];
	unsigned short  h_proto;
};

SEC("mf")
int handle_ingress(struct __sk_buff *skb)
{
    unsigned char *data = (unsigned char *)(long)skb->data;
    unsigned char *data_end = (unsigned char *)(long)skb->data_end;
    unsigned char *ptr, *feedback;
    
    if (data + sizeof(struct eth_hdr) + sizeof(struct iphdr) + sizeof(struct tcphdr) + 16 + 5 > data_end)
            return 0;
    
    ptr = data + sizeof(struct eth_hdr) + sizeof(struct iphdr) + sizeof(struct tcphdr) + 16;
    feedback = ptr + 2;
    *feedback = 42;
    
    char fmt[] = "%u\n";
    bpf_trace_printk(fmt, sizeof(fmt), *feedback);    
        
    return 0;
}
char _license[] SEC("license") = "GPL";

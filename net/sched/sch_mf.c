/*
 * net/sched/sch_fifo.c	The simplest MF Scheduler.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	MD Iftakharul Islam , <tamim@csebuet.org>
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/tcp.h>
#include <net/tcp.h>
#include <net/pkt_sched.h>


struct mf_sched_data {
    struct tcp_mf_cookie mfc;
};


static void parse_opt_mf(struct sk_buff *skb,
		       struct tcp_mf_cookie *mfc)
{
	unsigned char *ptr;
	const struct tcphdr *th = tcp_hdr(skb);
	int length = (th->doff * 4) - sizeof(struct tcphdr);
        u8 capacity;
        u8 perFlow;
        char *feedback;

	ptr = (const unsigned char *)(th + 1);
        
	while (length > 0) {
		int opcode = *ptr++;
		int opsize;

		switch (opcode) {
		case TCPOPT_EOL:
			return;
		case TCPOPT_NOP:	/* Ref: RFC 793 section 3.1 */
			length--;
			continue;
		default:
			opsize = *ptr++;
			if (opsize < 2) /* "silly options" */
				return;
			if (opsize > length)
				return;	/* don't parse partial options */
			switch (opcode) {
			case TCPOPT_MF:
				if (opsize == TCPOLEN_MF) {    
                                        //We wrote 3 bytes in Option Write
                                        ptr++;     
                                        capacity = 20;
                                        perFlow = capacity/3;
                                        feedback = ptr + 2;
                                        if(*feedback > perFlow)
                                            *feedback = perFlow;
					mfc->req_thput = *ptr;
                                        mfc->cur_thput = *(ptr + 1);
                                        mfc->feedback_thput = *(ptr + 2);                                        
				}
				return;                                        
			}
			ptr += opsize-2;
			length -= opsize;
		}
	}
}



static int mf_enqueue(struct sk_buff *skb, struct Qdisc *sch,
			 struct sk_buff **to_free)
{
	if (likely(sch->q.qlen < sch->limit))
		return qdisc_enqueue_tail(skb, sch);
        
        pr_err("This shouldn't happen!!!! You shouldn't drop packet here !!!!!!");
	return qdisc_drop(skb, sch, to_free);
}


static inline struct sk_buff *mf_dequeue(struct Qdisc *sch)
{
        
        struct tcphdr *tcph;
        struct mf_sched_data *q = qdisc_priv(sch);
	struct tcp_mf_cookie mfc;
        struct sk_buff *skb = qdisc_dequeue_head(sch);
        
        tcph = tcp_hdr(skb);
        if(tcph)
        {
            memset(&mfc, 0, sizeof(struct tcp_mf_cookie));
            parse_opt_mf(skb, &mfc);
            if(mfc.feedback_thput > 0 || mfc.req_thput > 0 )
                pr_err("IN SCH MF: req_thput:%d feedback_thput:%d", 
                    (int)mfc.req_thput, (int)mfc.feedback_thput);                        
        }
        
	return skb;
}

static int fifo_init(struct Qdisc *sch, struct nlattr *opt)
{
	bool bypass;
	if (opt == NULL) {
		u32 limit = qdisc_dev(sch)->tx_queue_len;
		sch->limit = limit;
	} 

	return 0;
}

static int fifo_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct tc_fifo_qopt opt = { .limit = sch->limit };

	if (nla_put(skb, TCA_OPTIONS, sizeof(opt), &opt))
		goto nla_put_failure;
	return skb->len;

nla_put_failure:
	return -1;
}

struct Qdisc_ops mf_qdisc_ops __read_mostly = {
        .next		=	NULL,  
	.id		=	"mf",
	.priv_size	=	sizeof(struct mf_sched_data),
	.enqueue	=	mf_enqueue,
	.dequeue	=	mf_dequeue,
	.peek		=	qdisc_peek_head,
	.init		=	fifo_init,
	.reset		=	qdisc_reset_queue,
        .destroy	=	qdisc_destroy,        
	.change		=	fifo_init,
	.dump		=	fifo_dump,
	.owner		=	THIS_MODULE,
};

static int __init mf_module_init(void)
{        
	return register_qdisc(&mf_qdisc_ops);
}

static void __exit mf_module_exit(void)
{
	unregister_qdisc(&mf_qdisc_ops);
}

module_init(mf_module_init)
module_exit(mf_module_exit)

MODULE_LICENSE("GPL");


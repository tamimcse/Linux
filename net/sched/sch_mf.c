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
    u32 numFlow;
    u32 capacity;
    struct tcp_mf_cookie mfc;
    struct Qdisc	*qdisc;
};


static void mf_apply(struct Qdisc *sch, struct sk_buff *skb,
		       struct tcp_mf_cookie *mfc)
{
        unsigned char *ptr;
        u8 *feedback;
	int opsize;
        int opcode;
        const struct tcphdr *th;
        struct mf_sched_data *q = qdisc_priv(sch);
        //everything in kernel is interms of bytes. So convert Mininet kbits to bytes
        u32 capacity = (q->capacity * 1024)/8; 
        s64 rate = capacity > sch->qstats.backlog? (capacity - sch->qstats.backlog)/q->numFlow : 0;
        //convert into bytes back to to KB (as MF TCP option)
        rate = rate/1024;        
//        pr_info("backlog= %u kbit, rate= %lld KB", (sch->qstats.backlog * 8/1024), rate);        
        
	th = tcp_hdr(skb);
	int length = (th->doff * 4) - sizeof(struct tcphdr);        

	ptr = (unsigned char *)(th + 1);
        
	while (length > 0) {
		opcode = *ptr++;

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
                                        feedback = ptr + 2;
//                                        pr_info("feedback= %d, rate= %lld", *feedback, rate);
                                        if(*feedback > rate)
                                            *feedback = rate;
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
        struct mf_sched_data *q = qdisc_priv(sch);
	if (likely(sch->q.qlen < sch->limit))
        {
            qdisc_qstats_backlog_inc(sch, skb);
            sch->q.qlen++;
            return qdisc_enqueue(skb, q->qdisc, to_free);
        }
	qdisc_qstats_drop(sch);	
	return qdisc_drop(skb, q->qdisc, to_free);
}


static inline struct sk_buff *mf_dequeue(struct Qdisc *sch)
{
        struct tcphdr *tcph;
        struct mf_sched_data *q = qdisc_priv(sch);
	struct tcp_mf_cookie mfc;
        struct sk_buff *skb = qdisc_dequeue_peeked(q->qdisc);
        
        if(skb)
        {
            qdisc_bstats_update(sch, skb);
            qdisc_qstats_backlog_dec(sch, skb);
            sch->q.qlen--;
            
            tcph = tcp_hdr(skb);
            if(tcph)
            {
                memset(&mfc, 0, sizeof(struct tcp_mf_cookie));
                mf_apply(sch, skb, &mfc);
                if(mfc.feedback_thput > 0 || mfc.req_thput > 0 )
                    pr_err("IN SCH MF: req_thput:%d feedback_thput:%d", 
                        (int)mfc.req_thput, (int)mfc.feedback_thput);                        
            }
            return skb;            
        }
        return NULL;
}


static int mf_init(struct Qdisc *sch, struct nlattr *opt)
{
        struct mf_sched_data *q = qdisc_priv(sch);
	if (opt == NULL) {
		u32 limit = qdisc_dev(sch)->tx_queue_len;
                sch->limit = limit;
                q->qdisc = fifo_create_dflt(sch, &bfifo_qdisc_ops, limit);
		q->qdisc->limit = limit;
                q->capacity = 1024;
                q->numFlow = 3;
	}
        
        return 0;
}


static void mf_destroy(struct Qdisc *sch)
{
	struct mf_sched_data *q = qdisc_priv(sch);
	qdisc_destroy(q->qdisc);
}

static int mf_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct tc_fifo_qopt opt = { .limit = sch->limit };

	if (nla_put(skb, TCA_OPTIONS, sizeof(opt), &opt))
		goto nla_put_failure;
	return skb->len;

nla_put_failure:
	return -1;
}

static int mf_dump_class(struct Qdisc *sch, unsigned long cl,
			  struct sk_buff *skb, struct tcmsg *tcm)
{
	struct mf_sched_data *q = qdisc_priv(sch);

	tcm->tcm_handle |= TC_H_MIN(1);
	tcm->tcm_info = q->qdisc->handle;

	return 0;
}

static int mf_graft(struct Qdisc *sch, unsigned long arg, struct Qdisc *new,
		     struct Qdisc **old)
{
	struct mf_sched_data *q = qdisc_priv(sch);

	if (new == NULL)
		new = &noop_qdisc;

	*old = qdisc_replace(sch, new, &q->qdisc);
	return 0;
}

static struct Qdisc *mf_leaf(struct Qdisc *sch, unsigned long arg)
{
	struct mf_sched_data *q = qdisc_priv(sch);
	return q->qdisc;
}

static unsigned long mf_get(struct Qdisc *sch, u32 classid)
{
	return 1;
}

static void mf_put(struct Qdisc *sch, unsigned long arg)
{
}

static void mf_walk(struct Qdisc *sch, struct qdisc_walker *walker)
{
	if (!walker->stop) {
		if (walker->count >= walker->skip)
			if (walker->fn(sch, 1, walker) < 0) {
				walker->stop = 1;
				return;
			}
		walker->count++;
	}
}

static const struct Qdisc_class_ops mf_class_ops = {
	.graft		=	mf_graft,
	.leaf		=	mf_leaf,
	.get		=	mf_get,
	.put		=	mf_put,
	.walk		=	mf_walk,
	.dump		=	mf_dump_class,
};

struct Qdisc_ops mf_qdisc_ops __read_mostly = {
        .next		=	NULL,  
        .cl_ops		=	&mf_class_ops,
	.id		=	"mf",
	.priv_size	=	sizeof(struct mf_sched_data),
	.enqueue	=	mf_enqueue,
	.dequeue	=	mf_dequeue,
	.peek		=	qdisc_peek_head,
	.init		=	mf_init,
	.reset		=	qdisc_reset_queue,
        .destroy	=	mf_destroy,        
	.change		=	mf_init,
	.dump		=	mf_dump,
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


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
#include <linux/proc_fs.h>

static unsigned long bufsize __read_mostly = 64 * 4096;
static const char procname[] = "mf_probe";

struct flow {
    __be32  saddr;
    __be32  daddr;
    //Not the actual hash table but this pointer would be used to be added to hash table
     struct hlist_node hash_item_ptr;
};

//Actual hash table of size 8
static DEFINE_HASHTABLE(flows, 3);

struct mf_sched_data {
    u32 nFlow;
    u32 capacity;
    u32 queue_sample;
    ktime_t sample_time;
    int queue_gradiant;
    
    struct tcp_mf_cookie mfc;
    struct Qdisc	*qdisc;
};

struct mf_log {
	ktime_t tstamp;
	__u32	backlog;
};

static struct {
	ktime_t		start;
        unsigned long	head, tail;
        struct mf_log *log;
} mf_probe;

static void mf_apply(struct Qdisc *sch, struct sk_buff *skb,
		       struct tcp_mf_cookie *mfc)
{
        unsigned char *ptr;
        u32 tsval;
        u8 *feedback;
	int opsize;
        int opcode;
        const struct tcphdr *th;
        struct mf_sched_data *q = qdisc_priv(sch);
        if(q->nFlow == 0)
        {
            pr_err("Some thing is terribly wrong !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            return;
        }
        //everything in kernel is interms of bytes. So convert Mininet kbits to bytes
        u32 capacity = (q->capacity * 1024)/8;
        s64 rate = capacity > sch->qstats.backlog? (capacity - sch->qstats.backlog)/q->nFlow : 0;
//        int elapsed_time = (int)ktime_to_ms (ktime_sub(ktime_get(), q->sample_time));
//        if(elapsed_time > 20)
//        {
//            //Don't do it for the very first sample
//            if(q->sample_time.tv64 > 0)
//            {
//                //change of queue per millisecond
//                q->queue_gradiant = (int)(sch->qstats.backlog - q->queue_sample) /elapsed_time;
//                pr_info("Queue diff: %d Time diff: %d queue_gradiant=%d", 
//                        (sch->qstats.backlog - q->queue_sample), 
//                        elapsed_time, q->queue_gradiant);
//            }
//            q->queue_sample = sch->qstats.backlog;
//            q->sample_time = ktime_get();
//        }
//        everything in kernel is interms of bytes. So convert Mininet kbits to bytes
//        u32 capacity = (q->capacity * 1024)/8; 
//        s64 rate;
//        if(q->queue_gradiant > 0)
//        {
//            rate = capacity > sch->qstats.backlog? (capacity - sch->qstats.backlog - 2*elapsed_time*q->queue_gradiant)/q->numFlow : 0;
//        }
//        else
//        {
//            rate = capacity > sch->qstats.backlog? (capacity - sch->qstats.backlog)/q->nFlow : 0;        
//        }
            

        //convert into bytes back to to KB (as MF TCP option)
        rate = rate/1024;        
        pr_info("backlog=%u KB, queue_gradiant=%d KB rate=%lld KB", sch->qstats.backlog/1000, q->queue_gradiant/1000, rate);        
        
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
                        
                        case TCPOPT_TIMESTAMP:
                            if (opsize == TCPOLEN_TIMESTAMP) {    
                                tsval = get_unaligned_be32(ptr);
//                                pr_info("Time stamp: %u !!!!!!!!!!!!!!!!!!!!", tsval);
                            }
                            break;
			case TCPOPT_MF:
				if (opsize == TCPOLEN_MF) {    
                                        feedback = ptr + 2;
//                                        pr_info("feedback= %d, rate= %lld", *feedback, rate);
//                                        if(*feedback > rate)
                                            *feedback = rate;
					mfc->req_thput = *ptr;
                                        mfc->cur_thput = *(ptr + 1);
                                        mfc->feedback_thput = *(ptr + 2);
                                        mfc->prop_delay_est = *(ptr + 3); 
				}
				return;                                        
			}
			ptr += opsize-2;
			length -= opsize;
		}
	}
}

static void record_mf(struct Qdisc *sch)
{
    if(mf_probe.start.tv64 == 0)
    {
        mf_probe.start = ktime_get();
    }
    struct mf_log *p = mf_probe.log + mf_probe.head;
    p->tstamp = ktime_get();
    p->backlog = sch->qstats.backlog;
    mf_probe.head = (mf_probe.head + 1) & (bufsize - 1);
}

static void write_mf(void)
{
     while(mf_probe.head > 0)
        {
            char tbuf[256];
            const struct mf_log *p
                   = mf_probe.log + mf_probe.tail;           
            struct timespec64 ts
                    = ktime_to_timespec64(ktime_sub(p->tstamp, mf_probe.start));
            int len = scnprintf(tbuf, sizeof(tbuf),
                            "%lu.%09lu %u\n",
                            (unsigned long)ts.tv_sec,
                            (unsigned long)ts.tv_nsec,
                            p->backlog);                
            mf_probe.tail = (mf_probe.tail + 1) & (bufsize - 1);
            mf_probe.head = (mf_probe.head - 1) & (bufsize - 1);
            pr_info("%s",tbuf);
        }
}

static int mf_enqueue(struct sk_buff *skb, struct Qdisc *sch,
			 struct sk_buff **to_free)
{
        struct mf_sched_data *q = qdisc_priv(sch);
        struct iphdr *iph = ip_hdr(skb);
        __be32  val;
        
        //check if it belongs to a new flow
        int found = 0;
        //Used by the hash_for_each()
        int b;
        struct flow *temp;
        
        //check the IP address belong to one of the Mininet
        if((q->nFlow < 3) && ((iph->daddr & 255) == 172))
        {   
            //the last parameter is the pointer of the item to be added (it's not the variable itself but the variable w/o ref)
            hash_for_each(flows, b, temp, hash_item_ptr)
            {
                if(temp->daddr == iph->daddr)
                    found = 1;
            };

            if(found == 0)
            {
                //Declare the flow to be added
                struct flow *f = kmalloc(sizeof(*f), GFP_KERNEL);
                f->daddr = iph->daddr;
                f->saddr = iph->saddr;
    //            f->hash_item_ptr = 0; /* Will be initilaized when added to the hashtable */
                hash_add(flows, &f->hash_item_ptr, f->saddr);
                q->nFlow++;
            }            
        }
        
//        pr_info("Number of flows: %d !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!", q->nFlow);
        
        record_mf(sch);        
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
                    pr_err("IN SCH MF: req_thput:%d feedback_thput:%d curr_thput: %d prop_delay:%d", 
                        (int)mfc.req_thput, (int)mfc.feedback_thput, (int)mfc.cur_thput, (int)mfc.prop_delay_est);                        
            }
            return skb;            
        }
        return NULL;
}

static ssize_t mfprobe_read(struct file *file, char __user *buf,
			     size_t len, loff_t *ppos)
{
        int error = 0;
        size_t cnt = 0;    
         while(mf_probe.head > 0 && cnt < len)
        {
            char tbuf[256];
            const struct mf_log *p
                   = mf_probe.log + mf_probe.tail;           
            struct timespec64 ts
                    = ktime_to_timespec64(ktime_sub(p->tstamp, mf_probe.start));
            int width = scnprintf(tbuf, sizeof(tbuf),
                            "%lu.%09lu %u\n",
                            (unsigned long)ts.tv_sec,
                            (unsigned long)ts.tv_nsec,
                            p->backlog);                
            mf_probe.tail = (mf_probe.tail + 1) & (bufsize - 1);
            mf_probe.head = (mf_probe.head - 1) & (bufsize - 1);
            if (copy_to_user(buf + cnt, tbuf, width))
                    return -EFAULT; 
            cnt += width;
        }
        return cnt == 0 ? error : cnt;
}

static const struct file_operations mfpprobe_fops = {
	.owner	 = THIS_MODULE,
	.read    = mfprobe_read,
	.llseek  = noop_llseek,
};

static void mf_probe_init(void)
{
        bufsize = roundup_pow_of_two(bufsize);
	mf_probe.start.tv64 = 0;
        mf_probe.head = mf_probe.tail = 0;
        mf_probe.log = kcalloc(bufsize, sizeof(struct mf_log), GFP_KERNEL);            
        if(!mf_probe.log)
        {
            kfree(mf_probe.log);
            return ENOMEM;
        }        
	if (!proc_create(procname, S_IRUSR, init_net.proc_net, &mfpprobe_fops))
        {
            pr_err("Cannot create mf_probe proc file");
        }
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
                q->nFlow = 0;
                q->queue_sample = 0;
                q->sample_time.tv64 = 0;
                q->queue_gradiant = 0;
	}
        mf_probe_init();
        
        return 0;
}

static void mf_destroy(struct Qdisc *sch)
{
	struct mf_sched_data *q = qdisc_priv(sch);
	qdisc_destroy(q->qdisc);
        
        //Used by the hash_for_each()
        int b;
        struct flow *temp;        
        //the last parameter is the pointer of the item to be added (it's not the variable itself but the variable w/o ref)
        hash_for_each(flows, b, temp, hash_item_ptr)
        {
            hash_del(&temp->hash_item_ptr);
        };
        q->nFlow = 0;
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
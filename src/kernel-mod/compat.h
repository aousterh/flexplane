/*
 * compat.h
 *
 *  Created on: Jan 7, 2015
 *      Author: yonch
 */

#ifndef COMPAT_H_
#define COMPAT_H_

#include <linux/version.h>

inline int ip_queue_xmit_compat(struct sock *sk, struct sk_buff *skb,
		struct flowi *fl)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,15,0)
	return ip_queue_xmit(skb, fl);
#else
	return ip_queue_xmit(sk, skb, fl);
#endif
}




#endif /* COMPAT_H_ */

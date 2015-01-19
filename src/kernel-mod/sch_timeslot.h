#ifndef SCH_TIMESLOT_H_
#define SCH_TIMESLOT_H_

#include <linux/types.h>
#include <net/sch_generic.h>

#define	TSLOT_ACTION_ADMIT_HEAD		0x0
#define TSLOT_ACTION_ADMIT_BY_ID	0x1
#define	TSLOT_ACTION_DROP_BY_ID		0x2
#define TSLOT_ACTION_MARK_BY_ID		0x3

#define MAX_REQ_DATA_PER_DST	256

struct tsq_ops {
	char			id[IFNAMSIZ];
	int			priv_size;
	int			(* new_qdisc)(void *priv, struct net *qdisc_net, u32 tslot_mul,
								u32 tslot_shift);
	void		(* stop_qdisc)(void *priv);
	void		(* add_timeslot)(void *priv, u64 src_dst_key,
			u8 *request_data);
};

struct tsq_qdisc_entry {
	struct tsq_ops *ops;
	struct Qdisc_ops qdisc_ops;
};

/**
 * Initializes global state
 */
int tsq_init(void);

/**
 * Cleans up global state
 */
void tsq_exit(void);

/**
 * Registers the timeslot queuing discipline
 */
struct tsq_qdisc_entry *tsq_register_qdisc(struct tsq_ops *ops);

/**
 * Unregisters the timeslot queuing discipline
 */
void tsq_unregister_qdisc(struct tsq_qdisc_entry *reg);

/**
 * Schedules a timeslot for flow
 */
void tsq_schedule(void *priv, u64 src_dst_key, u64 timeslot);

/**
 * Handles a timeslot from a flow (specified by src_dst_key) right now. This
 * involves admitting, marking, or dropping, according to the action. Returns
 * the number of timeslots handled (0 or 1).
 */
int tsq_handle_now(void *priv, u64 src_dst_key, u8 action, u16 id);

/**
 * Reset the ids of a flow (identified by @src_dst_key), to begin with 0 and
 * increase to (n_tslots - 1).
 */
void tsq_reset_ids(void *priv, u64 src_dst_key);

/**
 * Garbage-collects information for empty queues.
 */
void tsq_garbage_collect(void *priv);


#endif /* SCH_TIMESLOT_H_ */

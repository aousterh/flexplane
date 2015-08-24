#include "comm_core.h"

#include <rte_log.h>
#include <rte_common.h>
#include <rte_cycles.h>
#include <rte_byteorder.h>
#include <rte_ip.h>
#include <rte_memcpy.h>
#include <rte_string_fns.h>
#include <rte_errno.h>
#include <rte_mempool.h>
#include <ccan/list/list.h>
#include "control.h"
#include "comm_log.h"
#include "main.h"
#include "arp.h"
#include "dpdk-platform.h"
#include "bigmap.h"
#include "admission_core.h"
#include "admission_core_common.h"
#include "fp_timer.h"
#include "igmp.h"
#include "watchdog.h"
#include "../graph-algo/admissible.h"
#include "../graph-algo/path_selection.h"
#include "../protocol/flags.h"
#include "../protocol/fpproto.h"
#include "../protocol/pacer.h"
#include "../protocol/stat_print.h"
#include "../protocol/topology.h"
#include "../protocol/platform.h"

/* number of elements to keep in the pktdesc local core cache */
#define PKTDESC_MEMPOOL_CACHE_SIZE		256
#define PKTDESC_MEMPOOL_SIZE			(FASTPASS_WND_LEN * MAX_NODES + (N_COMM_CORES - 1) * PKTDESC_MEMPOOL_CACHE_SIZE)
/* should have as many pktdesc objs as number of in-flight packets */

#define ALLOC_REPORT_QUEUE_SIZE		(1UL << (FP_NODES_SHIFT + 1))
#define ALLOC_REPORT_QUEUE_MASK		(ALLOC_REPORT_QUEUE_SIZE - 1)

#define IGMP_SEND_INTERVAL_SEC		10

/* kill packet to shut off the arbiter remotely */
#define	IPPROTO_FASTPASS_KILL		225

/**
 * Information about a pending allocation.
 * @dst: the destination of this alloc
 * @timeslot: 16 least significant bits of the timeslot allocated
 * @flags: additional information about this alloc (path, drop, mark, etc.),
 * 			only the 4 least significant bits are used
 * @id: sequential id of the packet the allocation is for (used only in emu)
 * @data: additional data sent with this alloc, usage varies by emulated scheme
 */
struct pending_alloc {
	uint16_t	dst;
	uint16_t	timeslot;
	uint8_t		flags;
#if defined(EMULATION_ALGO)
	uint16_t	id;
	uint8_t		data[MAX_ALLOC_DATA_BYTES];
#endif
};

/**
 * A queue to record individual allocations that need to be sent to this
 * endpoint.
 * @allocs: a queue of allocs that have not yet been sent
 * @head: head index for allocs
 * @tail: tail index for allocs
 */
struct pending_alloc_queue {
	struct pending_alloc	allocs[(1 << FASTPASS_WND_LOG)];
	uint32_t				head;
	uint32_t				tail;
};

/**
 * A queue to record which destination flows have reports that need to be sent
 * to this endpoint.
 * @is_pending: 1 or 0 indicating whether each dst flow is pending or not
 * @q_pending: a queue of dsts that are pending
 * @head: head index for q_pending
 * @tail: tail index for q_pending
 */
struct alloc_report_queue {
	uint8_t is_pending[MAX_FLOWS];
	uint16_t q_pending[ALLOC_REPORT_QUEUE_SIZE];
	uint32_t head;
	uint32_t tail;
};

/**
 * Information about an end node
 * @conn: connection state (ACKs, RESET, retransmission, etc)
 * @dst_port: the port where outgoing packets should go to
 * @dst_ether: the destination ethernet address for outgoing packets
 * @dst_ip: the destination IP for outgoing packets
 * @controller_ip: the controller IP outgoing packets should use
 * @pending_allocs: a queue of pending allocations not yet sent out
 * @demands: the total demand to each destination flow
 * @alloc_to_dst: the total allocation to each destination flow
 * @acked_allocs: the total acked allocation to each destination flow
 * @report_queue: a queue of destination flows that have pending reports
 */
struct end_node_state {
	struct fpproto_conn conn;
	uint8_t dst_port;
	struct ether_addr dst_ether;
	uint32_t dst_ip;
	uint32_t controller_ip;

	/* pending allocations */
	struct pending_alloc_queue pending_allocs;

	/* demands */
	uint32_t demands[MAX_FLOWS];

	/* allocated timeslots */
	uint32_t alloc_to_dst[MAX_FLOWS];
	uint32_t acked_allocs[MAX_FLOWS];
	struct alloc_report_queue report_queue;

	/* timeout timer */
	struct fp_timer timeout_timer;

	/* egress packet timer and pacer */
	struct fp_timer tx_timer;
	struct fp_pacer tx_pacer;
};

/* whether we should output verbose debugging */
bool fastpass_debug;

/* logs */
struct comm_log comm_core_logs[RTE_MAX_LCORE];

/* per-end-node information */
static struct end_node_state end_nodes[MAX_NODES];

/* per-core information */
struct comm_core_state ccore_state[RTE_MAX_LCORE];

/* fpproto_pktdesc pool */
struct rte_mempool* pktdesc_pool[NB_SOCKETS];

static void handle_reset(void *param);
static void trigger_request(struct end_node_state *en);
static void trigger_request_voidp(void *param);
static void handle_areq(void *param, u16 *dst_and_count, int n);
static void set_retrans_timer(void *param, u64 when);
static int cancel_retrans_timer(void *param);
static void handle_skipped_ack(void *param, struct fpproto_pktdesc *pd);
static void handle_neg_ack(void *param, struct fpproto_pktdesc *pd);
static void handle_ack(void *param, struct fpproto_pktdesc *pd);

struct fpproto_ops proto_ops = {
	.handle_reset		= &handle_reset,
	.handle_areq		= &handle_areq,
	.handle_ack			= &handle_ack,
	.handle_neg_ack		= &handle_neg_ack,
	.handle_skipped_ack	= &handle_skipped_ack,
	.trigger_request	= &trigger_request_voidp,
	.set_timer			= &set_retrans_timer,
	.cancel_timer		= &cancel_retrans_timer,
};

void comm_init_global_structs(uint64_t first_time_slot)
{
	u32 i;

	fastpass_debug = true;
	uint64_t hz = rte_get_timer_hz();
	uint64_t send_timeout = (uint64_t)((double)hz * CONTROLLER_SEND_TIMEOUT_SECS);
	uint32_t send_cost = hz / NODE_MAX_PKTS_PER_SEC;
	uint32_t max_burst = (uint32_t)(NODE_MAX_BURST * send_cost);
	uint32_t min_trigger_gap = (uint32_t)((double)hz * NODE_MIN_TRIGGER_GAP_SEC);
	uint64_t now = rte_get_timer_cycles();

	COMM_DEBUG("Configuring send timeout to %f seconds: %lu TSC cycles\n",
			CONTROLLER_SEND_TIMEOUT_SECS, send_timeout);

	for (i = 0; i < MAX_NODES; i++) {
		struct end_node_state *en = &end_nodes[i];

		fpproto_init_conn(&en->conn, &proto_ops, en,
						FASTPASS_RESET_WINDOW_NS, send_timeout);
		en->pending_allocs.tail = en->pending_allocs.head = 0;
		fp_init_timer(&en->timeout_timer);
		fp_init_timer(&en->tx_timer);
		pacer_init_full(&en->tx_pacer, now, send_cost, max_burst,
				min_trigger_gap);
	}
}

/* based on init_mem in main.c */
void comm_init_core(uint16_t lcore_id, uint64_t first_time_slot)
{
	int socketid;
	char s[64];
	struct comm_core_state *core = &ccore_state[lcore_id];
        uint16_t i;

	socketid = rte_lcore_to_socket_id(lcore_id);

	/* initialize the space for encoding ALLOCs */
	memset(&core->alloc_enc_space, 0, sizeof(core->alloc_enc_space));

        for (i = 0; i < N_PARTITIONS; i++)
                core->latest_timeslot[i] = first_time_slot - 1;

	/* initialize mempool for pktdescs */
	if (pktdesc_pool[socketid] == NULL) {
		snprintf(s, sizeof(s), "pktdesc_pool_%d", socketid);
		pktdesc_pool[socketid] =
			rte_mempool_create(s,
				PKTDESC_MEMPOOL_SIZE, /* num elements */
				sizeof(struct fpproto_pktdesc), /* element size */
				PKTDESC_MEMPOOL_CACHE_SIZE, /* cache size */
				0, NULL, NULL, NULL, NULL, /* custom initialization, disabled */
				socketid, 0);
		if (pktdesc_pool[socketid] == NULL)
			rte_exit(EXIT_FAILURE,
					"Cannot init pktdesc pool on socket %d: %s\n", socketid,
					rte_strerror(rte_errno));
		else
			printf("Allocated pktdesc pool on socket %d - %llu bufs\n",
					socketid, (u64)PKTDESC_MEMPOOL_SIZE);
	}
}

/**
 * Trigger a transmission to an endpoint @en to report unACKed info about @dst.
 */
static inline void trigger_report(struct end_node_state *en,
		struct alloc_report_queue *q, uint16_t dst) {
	if (q->is_pending[dst])
		return;
	q->is_pending[dst] = 1;
	q->q_pending[q->tail & ALLOC_REPORT_QUEUE_MASK] = dst;
	q->tail++;
	comm_log_triggered_report(en - end_nodes, dst);
	trigger_request(en);
}

static inline bool report_empty(struct alloc_report_queue *q) {
	return q->head == q->tail;
}

static inline uint16_t report_pop(struct alloc_report_queue *q) {
	assert(!report_empty(q));
	uint16_t node = q->q_pending[q->head & ALLOC_REPORT_QUEUE_MASK];
	assert(!!q->is_pending[node]);
	q->is_pending[node] = 0;
	q->head++;
	return node;
}

void benchmark_cost_of_get_time(void)
{
	uint32_t i;
	uint64_t a,b,c,d;

	/** Timer tests */
	a = fp_get_time_ns();
	c = rte_rdtsc();
	for(i = 0; i < 1000; i++) {
		b = fp_get_time_ns();
		rte_pause();
	}
	d = rte_rdtsc();
	RTE_LOG(INFO, BENCHAPP, "1000 fp_get_time_ns caused %"PRIu64" difference"
			" which is %"PRIu64" TSC cycles\n",
			b - a, d - c);

	a = rte_rdtsc();
	for(i = 0; i < 1000; i++) {
		b = rte_rdtsc();
		rte_pause();
	}
	RTE_LOG(INFO, BENCHAPP, "1000 rte_rdtsc caused %"PRIu64" difference\n",
			b - a);
}

static int cancel_retrans_timer(void *param)
{
	struct end_node_state *en = (struct end_node_state *)param;
	uint16_t node_id = en - end_nodes;

	comm_log_cancel_timer(node_id);
	fp_timer_stop(&en->timeout_timer);
	return 0;
}

static void set_retrans_timer(void *param, u64 when)
{
	struct end_node_state *en = (struct end_node_state *)param;
	uint16_t node_id = en - end_nodes;
	uint64_t now = rte_get_timer_cycles();
	const unsigned lcore_id = rte_lcore_id();
	struct comm_core_state *core = &ccore_state[lcore_id];

	COMM_DEBUG("setting timer now %lu when %llu (diff=%lld)\n", now, when, (when-now));
	fp_timer_reset(&core->timeout_timers, &en->timeout_timer, when);

	comm_log_set_timer(node_id, when, when - now);
}

static void handle_areq(void *param, u16 *dst_and_count, int n)
{
	int i;
	struct end_node_state *en = (struct end_node_state *)param;
	struct comm_core_state *core = &ccore_state[rte_lcore_id()];
	u16 dst, dst_node, flow_within_dst, count;
	u32 demand;
	u32 orig_demand;
	u32 node_id = en - end_nodes;
	s32 demand_diff;
	u32 num_increases = 0;
	u8 *areq_data_counts, *areq_data;
	(void) areq_data_counts; (void) areq_data;

	COMM_DEBUG("handling A-REQ with %d destinations\n", n);

#if defined(EMULATION_ALGO)
	areq_data_counts = (u8 *) (dst_and_count + 2*n);
	areq_data = areq_data_counts + n;
	if (n & 1)
		areq_data++; /* padded to even number */
#endif

	for (i = 0; i < n; i++) {
		dst = rte_be_to_cpu_16(dst_and_count[2*i]);
		dst_node = dst >> FLOW_SHIFT;
		count = rte_be_to_cpu_16(dst_and_count[2*i + 1]);
		if (unlikely(!(node_id < NUM_NODES))) {
			comm_log_areq_invalid_src(node_id);
			return;
		}
		if (unlikely(!(dst_node < NUM_NODES))) {
			comm_log_areq_invalid_dst(node_id, dst);
			return;
		}

		orig_demand = en->demands[dst];
		demand = orig_demand - (1UL << 15);
		demand += (count - demand) & 0xFFFF;
		demand_diff = (s32)demand - (s32)orig_demand;
		if (demand_diff > 0) {
			comm_log_demand_increased(node_id, dst, orig_demand, demand, demand_diff);

#if defined(EMULATION_ALGO)
			if (emu_req_data_bytes() > 0 &&
					*areq_data_counts != demand_diff) /* todo: handle lost areqs */
				comm_log_areq_data_count_disagrees(node_id, dst,
						*areq_data_counts, demand_diff);

			/* get the sequential ID from the demand, also pass in areq data */
			add_backlog(g_admissible_status(), node_id, dst, demand_diff,
					orig_demand & 0xFFFF, areq_data);
			areq_data += emu_req_data_bytes() * demand_diff;
			areq_data_counts++;
#else
			/* no need for sequential id or additional areq data */
			add_backlog(g_admissible_status(), node_id, dst, demand_diff, 0,
					NULL);
#endif
			en->demands[dst] = demand;
			num_increases++;
		} else {
			comm_log_demand_remained(node_id, dst, orig_demand, demand);
		}
	}

	trigger_request(en);
}

static void handle_reset(void *param)
{
	struct end_node_state *en = (struct end_node_state *)param;
	uint16_t node_id = en - end_nodes;

	comm_log_handle_reset(node_id, en->conn.in_sync);

	reset_sender(g_admissible_status(), node_id);
	memset(en->demands, 0, sizeof(en->alloc_to_dst));
	memset(en->alloc_to_dst, 0, sizeof(en->alloc_to_dst));
	memset(en->acked_allocs, 0, sizeof(en->acked_allocs));

	/* report queue */
	en->report_queue.head = 0;
	en->report_queue.tail = 0;
	memset(en->report_queue.is_pending, 0,
			sizeof(en->report_queue.is_pending));
}

static void handle_skipped_ack(void *param, struct fpproto_pktdesc *pd)
{
	struct end_node_state *en = (struct end_node_state *)param;
	uint16_t node_id = en - end_nodes;

#if defined(RETRANSMIT_UNACKED_ALLOCS)
	struct pending_alloc_queue *pending_q = &en->pending_allocs;
	struct pending_alloc *alloc;
	u8 descriptor;
	int i;

	/* if any un-ACK-ed allocs, add them to the queue of pending allocs so they
	 * will be retransmitted */
	for (i = 0; i < pd->used_alloc_tslot; i++) {
		descriptor = pd->tslot_desc[i];

		/* fill the next alloc in the queue with this alloc */
		alloc = &pending_q->allocs[wnd_pos(pending_q->tail++)];

		alloc->dst = pd->dsts[(descriptor >> 4) - 1];
		alloc->flags = descriptor & FLAGS_MASK;
		alloc->timeslot = pd->base_tslot;
		alloc->id = pd->emu_tslot_desc[i].id;
	}

	/* trigger reports for affected dsts */
	for (i = 0; i < pd->n_dsts; i++)
		trigger_report(en, &en->report_queue, pd->dsts[i]);
#endif

	comm_log_skipped_ack(node_id, pd->used_alloc_tslot, pd->seqno, pd->n_dsts);
}

static void handle_neg_ack(void *param, struct fpproto_pktdesc *pd)
{
	struct end_node_state *en = (struct end_node_state *)param;
	struct comm_core_state *core = &ccore_state[rte_lcore_id()];
	uint16_t node_id = en - end_nodes;
	int i;
	uint32_t num_triggered = 0;

	/* if the alloc report was not fully acked, trigger another report */
	for (i = 0; i < pd->n_areq; i++) {
		uint16_t dst = (uint16_t)pd->areq[i].src_dst_key;
		if ((int32_t)pd->areq[i].tslots - (int32_t)en->acked_allocs[dst] > 0) {

			/* still not acked, trigger a report to end node*/
			trigger_report(en, &en->report_queue, dst);
			num_triggered++;
		}
	}

	comm_log_neg_ack(node_id, pd->n_areq, pd->used_alloc_tslot, pd->seqno,
			num_triggered);
}

static void handle_ack(void *param, struct fpproto_pktdesc *pd)
{
	struct end_node_state *en = (struct end_node_state *)param;
	struct comm_core_state *core = &ccore_state[rte_lcore_id()];
	uint16_t node_id = en - end_nodes;
	uint32_t total_acked = 0;
	uint16_t dst_count;
	int i;

#if defined(RETRANSMIT_UNACKED_ALLOCS)
	/* ack the ALLOCs in the packet rather than the cumulative counts */
	for (i = 0; i < pd->used_alloc_tslot; i++) {
		uint16_t dst = pd->dsts[(pd->tslot_desc[i] >> 4) - 1];
		en->acked_allocs[dst]++;
		total_acked++;
	}
#else
	for (i = 0; i < pd->n_areq; i++) {
		uint16_t dst = (uint16_t)pd->areq[i].src_dst_key;
		int32_t new_acked =
				(int32_t)pd->areq[i].tslots - (int32_t)en->acked_allocs[dst];

		if (new_acked > 0) {
			/* newly acked timeslots, update */
			en->acked_allocs[dst] += new_acked;
			total_acked += new_acked;
		}
	}
#endif

	comm_log_ack(node_id, pd->n_areq, total_acked, pd->seqno);
}

static void trigger_request_voidp(void *param)
{
	struct end_node_state *en = (struct end_node_state *)param;
	trigger_request(en);
}

/**
 * Set a timer to send a request to the endpoint @en at the time specified by
 * the pacer (does nothing if the pacer was already triggered).
 */
static void trigger_request(struct end_node_state *en)
{
	uint64_t now = rte_get_timer_cycles();
	u32 node_id = en - end_nodes;
	const unsigned lcore_id = rte_lcore_id();
	struct comm_core_state *core = &ccore_state[lcore_id];

	/* trigger the pacer and set a timer for this endpoint if the pacer
	 * wasn't already triggered */
	if (pacer_trigger(&en->tx_pacer, now)) {
	  COMM_DEBUG("setting trigger timer now %lu when %llu (diff=%lld)\n", now, 
pacer_next_event(&en->tx_pacer), (pacer_next_event(&en->tx_pacer)-now));

		fp_timer_reset(&core->tx_timers, &en->tx_timer,
				pacer_next_event(&en->tx_pacer));

		comm_log_triggered_send(node_id);
	}
}

/**
 * Allocates a packet to endpoint @en with payload @pd, populates the ethernet
 * and IPv4 headers, and returns the packet.
 */
static inline struct rte_mbuf *
make_packet(struct end_node_state *en, struct fpproto_pktdesc *pd,
		struct ether_addr *src_mac_addr, struct rte_mempool* pktmbuf_pool)
{
	const unsigned int socket_id = rte_socket_id();
	struct rte_mbuf *m;
	struct ether_hdr *eth_hdr;
	struct ipv4_hdr *ipv4_hdr;
	unsigned char *payload_ptr;
	uint32_t ipv4_length;
	int32_t data_len;

	// Allocate packet on the current socket
	m = rte_pktmbuf_alloc(pktmbuf_pool);
	if(m == NULL) {
		comm_log_tx_cannot_allocate_mbuf(en->dst_ip);
		return NULL;
	}

	eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);

	ipv4_hdr = (struct ipv4_hdr *)(rte_pktmbuf_mtod(m, unsigned char *)
			     + sizeof(struct ether_hdr));

	payload_ptr = (rte_pktmbuf_mtod(m, unsigned char *)
			     + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr));

	/* dst addr according to destination */
	ether_addr_copy(&en->dst_ether, &eth_hdr->d_addr);
	/* src addr according to output port*/
	ether_addr_copy(src_mac_addr, &eth_hdr->s_addr);
	/* ethernet payload is IPv4 */
	eth_hdr->ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv4);

	/* ipv4 header */
	ipv4_hdr->version_ihl = 0x45; // Version=4, IHL=5
	ipv4_hdr->type_of_service = 46 << 2; /* 46 is DSCP Expedited Forwarding */
	ipv4_hdr->packet_id = 0;
	ipv4_hdr->fragment_offset = 0;
	ipv4_hdr->time_to_live = 77;
	ipv4_hdr->next_proto_id = IPPROTO_FASTPASS;
	// ipv4_hdr->hdr_checksum will be calculated in HW
	ipv4_hdr->src_addr = en->controller_ip;
	//ipv4_hdr->src_addr = rte_cpu_to_be_32(CONTROLLER_GROUP_ADDR);
	ipv4_hdr->dst_addr = en->dst_ip;

	/* encode fastpass payload */
	data_len = fpproto_encode_packet(pd, payload_ptr, FASTPASS_MAX_PAYLOAD,
			en->controller_ip, en->dst_ip, 26);
	if (data_len < 0) {
		comm_log_error_encoding_packet(en->dst_ip, en - end_nodes, data_len);
	}

	/* adjust packet size */
	ipv4_length = sizeof(struct ipv4_hdr) + data_len;
	// ipv4_length = RTE_MAX(46u, ipv4_length);
	rte_pktmbuf_append(m, ETHER_HDR_LEN + ipv4_length);
	ipv4_hdr->total_length = rte_cpu_to_be_16(ipv4_length);

	ipv4_hdr->hdr_checksum = 0;
#ifdef NO_HW_CHECKSUM
	ipv4_hdr->hdr_checksum = fp_fold(fp_csum_partial(ipv4_hdr, 4*0x5, 0));
#else
	// Activate IP checksum offload for packet
	m->ol_flags |= PKT_TX_IP_CKSUM;
	m->pkt.vlan_macip.f.l2_len = sizeof(struct ether_hdr);
	m->pkt.vlan_macip.f.l3_len = sizeof(struct ipv4_hdr);
#endif

	return m;
}

/**
 * \brief Performs an allocation for a single request packet, sends
 * 		a reply to the requester
 *
 * 	returns true if the packet was a watchdog packet
 *
 * Takes ownership of mbuf memory - either sends it or frees it.
 * @param portid: the port out of which to send the packet
 */
static inline bool
comm_rx(struct rte_mempool* pktmbuf_pool, struct rte_mbuf *m, uint8_t portid,
		uint8_t tx_queue)
{
	struct ether_hdr *eth_hdr;
	struct ipv4_hdr *ipv4_hdr;
	u8 *req_pkt;
	uint32_t req_src;
	struct end_node_state *en;
	uint16_t ether_type;
	uint16_t ip_total_len;
	uint16_t ip_hdr_len;
	bool saw_watchdog_packet = false;

	eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);
	ipv4_hdr = (struct ipv4_hdr *)(rte_pktmbuf_mtod(m, unsigned char *)
			     + sizeof(struct ether_hdr));
	req_pkt = (rte_pktmbuf_mtod(m, unsigned char *)
			     + sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr));

	ether_type = rte_be_to_cpu_16(eth_hdr->ether_type);

	comm_log_rx_pkt(rte_pktmbuf_data_len(m));

	if (unlikely(ether_type == ETHER_TYPE_ARP)) {
		print_arp(m, portid);
		send_gratuitous_arp(pktmbuf_pool, portid, tx_queue, controller_ip());
		goto cleanup; // Disregard ARP
	}

	if (unlikely(ether_type != ETHER_TYPE_IPv4)) {
		comm_log_rx_non_ipv4_packet(portid);
		goto cleanup;
	}

	if (unlikely(ipv4_hdr->next_proto_id == IPPROTO_FASTPASS_WATCHDOG)) {
		comm_log_rx_watchdog_packet(portid);
		saw_watchdog_packet = true;
		goto cleanup; /* discard packet */
	}

	if (unlikely(ipv4_hdr->next_proto_id == IPPROTO_FASTPASS_KILL)) {
		/* this is a kill packet. exit the arbiter. */
		rte_exit(EXIT_SUCCESS, "Received kill packet");
	}

	if (unlikely(ipv4_hdr->next_proto_id != IPPROTO_FASTPASS)) {
		comm_log_rx_ip_non_fastpass_pkt(portid);
		goto cleanup;
	}

	ip_total_len = rte_be_to_cpu_16(ipv4_hdr->total_length);
	ip_hdr_len = ipv4_hdr->version_ihl & 0xF;

	if (unlikely(sizeof(struct ether_hdr) + ip_total_len > rte_pktmbuf_data_len(m))) {
		comm_log_rx_truncated_pkt(ip_total_len, rte_pktmbuf_data_len(m),
				ipv4_hdr->src_addr);
		goto cleanup;
	}

	if (unlikely(ip_hdr_len < 5 || ip_total_len < 4 * ip_hdr_len)) {
		comm_log_rx_truncated_pkt(ip_total_len, rte_pktmbuf_data_len(m),
				ipv4_hdr->src_addr);
		goto cleanup;
	}

#if (CLASSIFY_BY_IP == 1)
	/* obtain ID from IPv4 address */
	/* arbiter only support IPv4 right now, not IPv6 */
	req_src = fp_map_ip_to_id(ntohl(ipv4_hdr->src_addr));
#else
	/* obtain ID from MAC address */
	uint64_t mac_addr;
	mac_addr = ((u64)ntohs(*(__be16 *)&eth_hdr->s_addr.addr_bytes[0]) << 32)
 			  | ntohl(*(__be32 *)&eth_hdr->s_addr.addr_bytes[2]);
	req_src = fp_map_mac_to_id(mac_addr);
#endif

	/* each end_node_state is per endpoint (rather than per flow) */
	en = &end_nodes[req_src];

	/* copy most recent ethernet and IP addresses, for return packets */
	ether_addr_copy(&eth_hdr->s_addr, &en->dst_ether);
	en->dst_ip = ipv4_hdr->src_addr;
	en->controller_ip = ipv4_hdr->dst_addr;


	COMM_DEBUG("at %lu controller got packet src_ip=0x%"PRIx32
			" src_node=%u dst=0x%"PRIx32" ip_len=%u\n", rte_get_timer_cycles(),
			ipv4_hdr->src_addr, req_src, ipv4_hdr->dst_addr, ip_total_len);


	if (req_src < MAX_NODES) {
		fpproto_handle_rx_complete(&end_nodes[req_src].conn, req_pkt,
				ip_total_len - 4 * (ipv4_hdr->version_ihl & 0xF),
				ipv4_hdr->src_addr, ipv4_hdr->dst_addr);
	}

cleanup:
	/* free the request packet */
	rte_pktmbuf_free(m);
	return saw_watchdog_packet;
}

/*
 * Read packets from RX queues
 */
static inline bool do_rx_burst(struct lcore_conf_t* qconf, uint8_t portid,
		uint8_t rx_queue, uint8_t tx_queue, struct rte_mempool* pktmbuf_pool)
{
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	int i, j, nb_rx;
	uint64_t rx_time;
	uint64_t deadline_monotonic;
	bool saw_watchdog = false;
	bool res;

	deadline_monotonic = rte_get_timer_cycles() + RX_BURST_DEADLINE_SEC * rte_get_timer_hz();

	nb_rx = rte_eth_rx_burst(portid, rx_queue, pkts_burst, MAX_PKT_BURST);
	rx_time = fp_get_time_ns();

	/* Prefetch first packets */
	for (j = 0; j < PREFETCH_OFFSET && j < nb_rx; j++) {
		rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[j], void *));
	}

	/* Prefetch and handle already prefetched packets */
	for (j = 0; j < (nb_rx - PREFETCH_OFFSET); j++) {
		rte_prefetch0(
				rte_pktmbuf_mtod(pkts_burst[ j + PREFETCH_OFFSET], void *));
		if (rte_get_timer_cycles() < deadline_monotonic) {
			res = comm_rx(pktmbuf_pool, pkts_burst[j], portid, tx_queue);
			saw_watchdog = saw_watchdog || res;
		} else {
			/* deadline passed, drop on the floor */
			rte_pktmbuf_free(pkts_burst[j]);
			comm_log_dropped_rx_passed_deadline();
		}
	}

	/* handle remaining prefetched packets */
	for (; j < nb_rx; j++) {
		res = saw_watchdog || comm_rx(pktmbuf_pool, pkts_burst[j], portid,
										tx_queue);
		saw_watchdog = saw_watchdog || res;
	}

	comm_log_processed_batch(nb_rx, rx_time);

	return saw_watchdog;
}

/* Extract the fields from an admitted edge in an admitted struct into src,
 * dst, and flags. Behavior depends on the algirhtm used (emulation, etc.). */
static inline
void get_admitted_fields(struct admitted_traffic *admitted, uint16_t index,
		uint16_t *src, uint16_t *dst, uint8_t *flags) {

#if (defined(EMULATION_ALGO) || defined (BENCHMARK_ALGO))
	struct emu_admitted_edge *edge;
	edge = get_admitted_edge(admitted, index);
	*src = edge->src;
	*dst = edge->dst;
	*flags = edge->flags;
#else
	uint16_t dst_encoding;

	*src = admitted->edges[index].src;
	dst_encoding = admitted->edges[index].dst;
	/* the dst id is the encoding with path bits removed */
	*dst = dst_encoding & DST_MASK;
	*flags = (dst_encoding & PATH_MASK) >> PATH_SHIFT;
#endif
}

/**
 * Fill in parts of @alloc that depend on the algorithm used, from the
 * specified edge in @admitted.
 */
static inline
void fill_algo_fields_in_alloc(struct pending_alloc *alloc,
		struct admitted_traffic *admitted, uint16_t index) {
#if defined(EMULATION_ALGO)
	struct emu_admitted_edge *edge;
	edge = get_admitted_edge(admitted, index);
	alloc->id = edge->id;
	memcpy(&alloc->data[0], &edge->data[0], emu_alloc_data_bytes());
#endif
}

/**
 * Record the allocations received in @q_admitted and trigger a report to each
 * source endpoint that got a new allocation.
 */
static inline void process_allocated_traffic_one_q(
		struct comm_core_state *core, struct rte_ring *q_admitted,
		struct rte_mempool *admitted_traffic_mempool)
{
	int rc;
	int i, j;
	struct admitted_traffic* admitted[MAX_ADMITTED_PER_LOOP];
        uint16_t partition;
	uint64_t current_timeslot;
	struct end_node_state *en;
	struct pending_alloc_queue *pending_q;
	struct pending_alloc *alloc;
	uint16_t src;
	uint16_t dst;
	uint8_t flags;
	uint16_t tslot;
	uint16_t thrown_alloc;

	/* Process newly allocated timeslots */
	rc = rte_ring_dequeue_burst(q_admitted, (void **) &admitted[0],
								MAX_ADMITTED_PER_LOOP);
	if (unlikely(rc < 0)) {
		/* error in dequeuing.. should never happen?? */
		comm_log_dequeue_admitted_failed(rc);
		return;
	}

	for (i = 0; i < rc; i++) {
		partition = get_admitted_partition(admitted[i]);
		current_timeslot = ++core->latest_timeslot[partition];
		comm_log_got_admitted_tslot(get_num_admitted(admitted[i]),
					    current_timeslot, partition);
		for (j = 0; j < get_size(admitted[i]); j++) {
			/* extract src, dst, and flags from this admitted edge */
			get_admitted_fields(admitted[i], j, &src, &dst, &flags);

			/* get the source endpoint's structure */
			en = &end_nodes[src];
			pending_q = &en->pending_allocs;

			/* is the timeslot queue full? aka is head one more than tail? */
			if (pending_q->head == pending_q->tail + 1) {
				tslot = pending_q->allocs[wnd_pos(pending_q->head)].timeslot;
				thrown_alloc = pending_q->allocs[wnd_pos(pending_q->head)].dst;

				/* throw away that timeslot */
				pending_q->head++;

				/* also log it */
				comm_log_alloc_overflowed_queue(tslot, current_timeslot, src,
						thrown_alloc);
			}

			/* add the allocation to the queue of pending allocs */
			alloc = &pending_q->allocs[wnd_pos(pending_q->tail++)];
			alloc->dst = dst;
			alloc->flags = flags & FLAGS_MASK;
			alloc->timeslot = (current_timeslot >> 4) & 0xFFFF;
			fill_algo_fields_in_alloc(alloc, admitted[i], j);
			en->alloc_to_dst[dst]++;

			/* trigger_report will make sure a TX is triggered */
			trigger_report(en, &en->report_queue, dst);
		}
	}
	/* free memory */
	rte_mempool_put_bulk(admitted_traffic_mempool, (void **) admitted, rc);
}

static inline uint64_t process_allocated_traffic(struct comm_core_state *core,
		struct rte_ring **q_admitted,
		struct rte_mempool *admitted_traffic_mempool) {
#ifdef EMULATION_ALGO
	uint16_t i;
	for (i = 0; i < ALGO_N_CORES; i++)
		process_allocated_traffic_one_q(core, q_admitted[i],
				admitted_traffic_mempool);
#else
	process_allocated_traffic_one_q(core, q_admitted[0],
			admitted_traffic_mempool);
#endif
}

/* check statically that the window is not too long, because fill_packet_alloc
 * cannot handle gaps larger than 256 */
struct __static_check_wnd_size {
	uint8_t check_FASTPASS_WND_is_not_too_big_for__fill_packet_alloc[256 - FASTPASS_WND_LEN];
};

/**
 * Fills pending total alloc reports to end-node @en into the packet desc @pd
 */
static inline void fill_packet_report(struct comm_core_state *core,
		struct fpproto_pktdesc *pd, struct end_node_state *en)
{
	struct pending_alloc_queue *pending_q = &en->pending_allocs;
	uint32_t unsent_allocs;

	pd->n_areq = 0;

	while (!report_empty(&en->report_queue)
			&& pd->n_areq < FASTPASS_PKT_MAX_AREQ) {
		uint16_t node = report_pop(&en->report_queue);
		pd->areq[pd->n_areq].src_dst_key = node;

		/* AREQ should not include allocs that have not been sent, due to
		 * limits on the number of allocs per control packet */
		unsent_allocs = pending_q->tail - pending_q->head;
		pd->areq[pd->n_areq].tslots = en->alloc_to_dst[node] - unsent_allocs;
		pd->n_areq++;
	}

	/* if report queue is still not empty, we should trigger another packet */
	if (!report_empty(&en->report_queue))
		trigger_request(en);
}

/**
 * Extracts allocations from the end-node @en into the packet desc @pd
 */
static inline void fill_packet_alloc(struct comm_core_state *core,
		struct fpproto_pktdesc *pd, struct end_node_state *en)
{
	uint16_t n_dsts = 0;
	uint16_t n_tslot = 0;
	struct pending_alloc_queue *pending_q = &en->pending_allocs;
	struct pending_alloc *cur_alloc;
	uint16_t dst;
	uint16_t i;

	/* check if there are allocs that need to be conveyed */
	if (pending_q->head == pending_q->tail)
		goto out;

	cur_alloc = &pending_q->allocs[wnd_pos(pending_q->head)];
	pd->base_tslot = cur_alloc->timeslot;

next_alloc:
	/* find the destination for this flow */
	dst = cur_alloc->dst;

	if (core->alloc_enc_space[dst] == 0) {
		/* this is the first time seeing dst, need to add it to pd->dsts */
		if (n_dsts == 15) {
			/* too many destinations already, we're done */
			goto cleanup;
		} else {
			/* get the next slot in the pd->dsts array */
			pd->dsts[n_dsts] = dst;
			core->alloc_enc_space[dst] = n_dsts + 1;
			n_dsts++;
		}
	}

	/* encode the allocation byte
	 * upper 4 bits for destination index, lower 4 bits for flags */
	pd->tslot_desc[n_tslot] = (core->alloc_enc_space[dst] << 4) |
			(cur_alloc->flags & FLAGS_MASK);
#if defined(EMULATION_ALGO)
	pd->emu_tslot_desc[n_tslot].id = cur_alloc->id;
	memcpy(&pd->emu_tslot_desc[n_tslot].data[0], &cur_alloc->data[0],
			emu_alloc_data_bytes());
#endif
	n_tslot++;

	/* remove the timeslot from the queue */
	pending_q->head++;

	if (likely((pending_q->tail != pending_q->head)
				&& (n_tslot <= FASTPASS_PKT_MAX_ALLOC_TSLOTS - 2))) {
		cur_alloc = &pending_q->allocs[wnd_pos(pending_q->head)];
		goto next_alloc;
	}

cleanup:
	/* we set core->alloc_enc_space back to zeros */
	for (i = 0; i < n_dsts; i++) {
		dst = pd->dsts[i];
		core->alloc_enc_space[dst] = 0;
	}

out:
	pd->used_alloc_tslot = n_tslot;

	/* pad to even n_tslot */
	if (n_tslot & 1) {
		pd->tslot_desc[n_tslot] = 0;
#if defined(EMULATION_ALGO)
		pd->emu_tslot_desc[n_tslot].id = 0;
#endif
		n_tslot++;
	}

	pd->n_dsts = n_dsts;
	pd->alloc_tslot = n_tslot;
	assert((pd->alloc_tslot & 1) == 0);
}

/**
 * Transmit a report of allocations and ACKs to the endpoint @en.
 */
static inline void tx_end_node(struct end_node_state *en, uint8_t portid,
		uint8_t tx_queue, struct ether_addr *src_mac_addr,
		struct rte_mempool* pktmbuf_pool)
{
	const unsigned lcore_id = rte_lcore_id();
	struct comm_core_state *core = &ccore_state[lcore_id];
	uint32_t node_ind = en - end_nodes;
	struct rte_mbuf *out_pkt;
	struct fpproto_pktdesc *pd;
	u64 now;

	/* clear the trigger - needs to be here so functions below can trigger
	 * more TX packets */
	pacer_reset(&en->tx_pacer);

	/* prepare to send */
	fpproto_prepare_to_send(&en->conn);

	/* allocate pktdesc */
	pd = fpproto_pktdesc_alloc();
	if (unlikely(pd == NULL)) {
		comm_log_pktdesc_alloc_failed(node_ind);
		/* retry later */
		trigger_request(en);
		return;
	}

	/* fill in allocated timeslots */
	fill_packet_alloc(core, pd, en);
	/* fill in report of allocated timeslots */
	fill_packet_report(core, pd, en);

	/* we want this packet's reliability to be tracked */
	now = rte_get_timer_cycles();
	fpproto_commit_packet(&en->conn, pd, now);

	/* make the packet */
	out_pkt = make_packet(en, pd, src_mac_addr, pktmbuf_pool);
	if (unlikely(out_pkt == NULL))
		return; /* pd committed, will get retransmitted on timeout */

	/* log sent packet */
	comm_log_tx_pkt(node_ind, now, rte_pktmbuf_data_len(out_pkt));

	/* send on port */
	send_packet_via_queue(out_pkt, portid, tx_queue);
}

/* handles reception; returns true if packet is watchdog, false otherwise */
static inline bool
watchdog_rx(struct rte_mbuf *m, uint8_t portid)
{
	struct ether_hdr *eth_hdr;
	struct ipv4_hdr *ipv4_hdr;
	uint16_t ether_type;
	bool retval = false;

	eth_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);
	ipv4_hdr = (struct ipv4_hdr *)(rte_pktmbuf_mtod(m, unsigned char *)
			     + sizeof(struct ether_hdr));

	ether_type = rte_be_to_cpu_16(eth_hdr->ether_type);

	comm_log_rx_pkt(rte_pktmbuf_data_len(m));

	if (unlikely(ether_type != ETHER_TYPE_IPv4)) {
		comm_log_rx_non_ipv4_packet(portid);
		goto cleanup;
	}

	if (unlikely(ipv4_hdr->next_proto_id != IPPROTO_FASTPASS_WATCHDOG))
		goto cleanup; /* discard packet */

	comm_log_rx_watchdog_packet(portid);
	retval = true;

cleanup:
	/* free the request packet */
	rte_pktmbuf_free(m);
	return retval;
}

void watchdog_loop(struct comm_core_cmd * cmd)
{
	const unsigned lcore_id = rte_lcore_id();
	struct lcore_conf_t *	qconf = &lcore_conf[lcore_id];
	struct comm_core_state *core = &ccore_state[lcore_id];
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	int i, j, nb_rx;
	uint64_t now;
	uint8_t portid;
	uint8_t rx_queue, tx_queue;
	struct rte_mempool* pktmbuf_pool;
	struct rte_mempool* admitted_traffic_mempool;

    now = rte_get_timer_cycles();
    core->last_rx_watchdog = now;

    portid = cmd->port_id;
	rx_queue = cmd->rx_queue_id;
	tx_queue = cmd->tx_queue_id;
	pktmbuf_pool = cmd->tx_pktmbuf_pool;
	admitted_traffic_mempool = cmd->admitted_traffic_mempool;

    while (core->last_rx_watchdog - now < WATCHDOG_TRIGGER_THRESHOLD_SEC * rte_get_timer_hz()) {
        now = rte_get_timer_cycles();

		nb_rx = rte_eth_rx_burst(portid, rx_queue, pkts_burst, MAX_PKT_BURST);

		/* Prefetch and handle already prefetched packets */
		for (j = 0; j < nb_rx; j++) {
			if (watchdog_rx(pkts_burst[j], portid) == true)
				core->last_rx_watchdog = rte_get_timer_cycles();
		}

		comm_log_processed_batch(nb_rx, now);

    	/* Still need to process newly allocated timeslots, which would be empty */
		process_allocated_traffic(core, cmd->q_allocated,
				admitted_traffic_mempool);

		/* send IGMP */
		if (now - core->last_igmp > IGMP_SEND_INTERVAL_SEC * rte_get_timer_hz()) {
			core->last_igmp = now;
			send_gratuitous_arp(pktmbuf_pool, portid, tx_queue, controller_ip());
			send_igmp(pktmbuf_pool, portid, tx_queue, controller_ip());
		}

		/* Flush queued packets */
		for (i = 0; i < n_enabled_port; i++)
			send_queued_packets(portid, tx_queue);
    }

    /* no watchdog too long - watchdog should fire! */
}

void exec_comm_core(struct comm_core_cmd * cmd)
{
	int i;
	uint8_t portid, rx_queue, tx_queue;
	struct lcore_conf_t *qconf;
	const unsigned lcore_id = rte_lcore_id();
	struct comm_core_state *core = &ccore_state[lcore_id];
	struct list_head lst = LIST_HEAD_INIT(lst);
	struct end_node_state *en;
	uint64_t now;
	struct fp_timer *tim;
	bool saw_watchdog;
	struct rte_mempool* pktmbuf_pool;
	struct rte_mempool* admitted_traffic_mempool;

	core->last_igmp = rte_get_timer_cycles();
    core->last_tx_watchdog = rte_get_timer_cycles();

	qconf = &lcore_conf[lcore_id];

	comm_log_init(&comm_core_logs[lcore_id]);

	COMM_DEBUG("starting, on lcore %d, current timeslot %lu\n", rte_lcore_id(),
			core->latest_timeslot[0]);

	portid = cmd->port_id;
	rx_queue = cmd->rx_queue_id;
	tx_queue = cmd->tx_queue_id;
	pktmbuf_pool = cmd->tx_pktmbuf_pool;
	admitted_traffic_mempool = cmd->admitted_traffic_mempool;

	RTE_LOG(INFO, BENCHAPP, "comm_core -- lcoreid=%u portid=%hhu rx_queue=%hhu tx_queue=%hhu tx_pktmbuf_pool=%p q_admitted=%p admitted_traffic_mempool=%p\n",
			rte_lcore_id(), portid, rx_queue, tx_queue, pktmbuf_pool,
			cmd->q_allocated, admitted_traffic_mempool);
	send_gratuitous_arp(pktmbuf_pool, portid, tx_queue, controller_ip());

	while (rte_get_timer_cycles() < cmd->start_time);

	send_gratuitous_arp(pktmbuf_pool, portid, tx_queue, controller_ip());
	if (RUN_WITH_BACKUP)
		send_igmp(pktmbuf_pool, portid, tx_queue, controller_ip());

	fp_init_timers(&core->timeout_timers, rte_get_timer_cycles());
	fp_init_timers(&core->tx_timers, rte_get_timer_cycles());

	/* MAIN LOOP */
	while (1) {
		/* read packets from RX queues */
		saw_watchdog = do_rx_burst(qconf, portid, rx_queue, tx_queue,
									pktmbuf_pool);
		if (saw_watchdog && !I_AM_MASTER) {
			watchdog_loop(cmd);
			continue;
		}

		/* process retrans timers */
		now = rte_get_timer_cycles();
		fp_timer_get_expired(&core->timeout_timers, now, &lst);
		while ((tim = list_pop(&lst, struct fp_timer, node)) != NULL) {
			/* get pointer to end_node_state */
			en = container_of(tim, struct end_node_state, timeout_timer);

			/* log */
			comm_log_retrans_timer_expired(en - end_nodes, now);

			/* call the handler */
			fpproto_handle_timeout(&en->conn, now);
		}

		/* Process newly allocated timeslots */
		process_allocated_traffic(core, cmd->q_allocated, admitted_traffic_mempool);

		/* Process the spent demands, launching a new demand for demands where
		 * backlog increased while the original demand was being allocated */
		handle_spent_demands(g_admissible_status());

		/* RX, retrans timers, and new traffic might push traffic into the
		 * q_head buffer; flush it now. */
		flush_backlog(g_admissible_status());

		/* process tx timers */
		fp_timer_get_expired(&core->tx_timers, now, &lst);
		while ((tim = list_pop(&lst, struct fp_timer, node)) != NULL) {
			/* get pointer to end_node_state */
			en = container_of(tim, struct end_node_state, tx_timer);

			/* do the TX */
			tx_end_node(en, portid, tx_queue, &cmd->eth_addr, pktmbuf_pool);
		}

		/* send IGMP if using a backup */
		if (RUN_WITH_BACKUP &&
		    now - core->last_igmp > IGMP_SEND_INTERVAL_SEC * rte_get_timer_hz()) {
			core->last_igmp = now;
			send_igmp(pktmbuf_pool, portid, tx_queue, controller_ip());
		}

		/* send watchdog if using an arbiter*/
		if (RUN_WITH_BACKUP &&
		    now - core->last_tx_watchdog > WATCHDOG_PACKET_GAP_SEC * rte_get_timer_hz()) {
			send_watchdog(pktmbuf_pool, portid, tx_queue, controller_ip());
			core->last_tx_watchdog = now;
			comm_log_sent_watchdog();
		}


        /* Flush queued packets */
		send_queued_packets(portid, tx_queue);

	}
}

void comm_dump_stat(uint16_t node_id, struct conn_log_struct *conn_log)
{
	int i;
	uint64_t ctr;
	struct end_node_state *en = &end_nodes[node_id];

	uint64_t now = rte_get_timer_cycles();
	fpproto_update_internal_stats(&en->conn);
	fpproto_dump_stats(&en->conn, &conn_log->stat);
	conn_log->next_retrans_gap = en->timeout_timer.time * TIMER_GRANULARITY - now;
	conn_log->next_tx_gap = en->tx_timer.time * TIMER_GRANULARITY - now;
	conn_log->pacer_gap = pacer_next_event(&en->tx_pacer) - now;

	/* dump demands */
	ctr = 0;
	for (i = 0; i < MAX_FLOWS; i++)
		ctr += en->demands[i];
	conn_log->demands = ctr;
}

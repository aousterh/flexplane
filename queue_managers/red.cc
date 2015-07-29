/*
 * red.cc
 *
 * Created on: December 21, 2014
 *     Author: hari
 */

#include "queue_managers/red.h"
#include "../graph-algo/random.h"

#define RED_PORT_CAPACITY 128
#define RED_QUEUE_CAPACITY 4096

REDQueueManager::REDQueueManager(PacketQueueBank *bank,
		uint32_t n_queues_total, struct red_args *red_params) :
		m_bank(bank), m_red_params(*red_params) {
	uint32_t i;

	if (bank == NULL)
		throw std::runtime_error("bank should be non-NULL");
	/* initialize other state */
	seed_random(&random_state, time(NULL));

	 /* shift out thresholds to maintain precision. min_th, max_th, and q_avg
	  * are maintained as shifted out by wq_shift (i.e. multiplied by
	  * 2^wq_shift) */
	m_red_params.min_th = m_red_params.min_th << m_red_params.wq_shift;
	m_red_params.max_th = m_red_params.max_th << m_red_params.wq_shift;

	/* initialize per-queue parameters */
	m_q_avg.reserve(n_queues_total);
	m_count_since_last.reserve(n_queues_total);
	for (i = 0; i < n_queues_total; i++) {
		m_q_avg[i] = 0;             // follows Floyd's 1993 paper pseudocode
		m_count_since_last[i] = -1; // follows Floyd's 1993 paper pseudocode
	}
}

void REDQueueManager::enqueue(struct emu_packet *pkt, uint32_t port,
		uint32_t queue, uint64_t cur_time) {
	uint32_t qlen = m_bank->occupancy(port, queue);
	//    printf("RED qlen %d q_avg %d count_since_last %d\n", qlen, q_avg, count_since_last);
	if (qlen >= m_red_params.q_capacity) {
		/* no space to enqueue, drop this packet */
		//        printf("REDenq: force drop qlen %d capacity%d\n", qlen, m_red_params.q_capacity);
		mark_or_drop(pkt, RED_FORCEDROP, port, queue);
		return;
	} else {
		if (red_rules(pkt, qlen, port, queue, cur_time) != RED_DROPPKT) {
			m_bank->enqueue(port, queue, pkt);
		}
	}
}

uint8_t REDQueueManager::red_rules(struct emu_packet *pkt, uint32_t qlen,
		uint32_t port, uint32_t queue, uint64_t cur_time) {
	uint32_t q_index;
	uint32_t qlen_shifted;

	q_index = m_bank->flat_index(port, queue);
	qlen_shifted = qlen << m_red_params.wq_shift;

	// note that the EWMA weight is specified as a bit shift factor
	if (!m_bank->empty(port, queue)) {
		if (qlen_shifted >= m_q_avg[q_index]) {
			m_q_avg[q_index] += (qlen_shifted - m_q_avg[q_index]) >>
					m_red_params.wq_shift;
		} else {
			m_q_avg[q_index] -= (m_q_avg[q_index] - qlen_shifted) >>
					m_red_params.wq_shift;
		}
	} else {
		/* if packet arrives to an empty queue, must handle idle slots since it
		 * became empty */
		uint64_t time_diff = cur_time - m_bank->last_empty_time(port, queue);

		/* q_avg = (1-w_q)^(cur_time - q_time)*q_avg */
		/* TODO: do this exponentiation more efficiently by using a table
		 * lookup */
		for (; time_diff > 0 && m_q_avg[q_index] >= (1 << m_red_params.wq_shift);
				time_diff--) {
			m_q_avg[q_index] -= m_q_avg[q_index] >> m_red_params.wq_shift;
		}
	}

	//    printf("red_rules: qlen %d q_avg %d count_since_last %d\n", qlen, q_avg, count_since_last);

	float p_a, p_b;
	bool accept = true;

	if (m_q_avg[q_index] > m_red_params.max_th) {
		accept = mark_or_drop(pkt, 0, port, queue);
	} else if (m_q_avg[q_index] > m_red_params.min_th) { // in (q_min, q_max]: probabilistic drop/mark
		p_b = m_red_params.max_p *
				(float) (m_q_avg[q_index] - m_red_params.min_th) /
				(m_red_params.max_th - m_red_params.min_th);
		p_a = p_b / (1 - m_count_since_last[q_index] * p_b);
		if (p_a > 1.0 || p_a < 0.0) {
			p_a = 1;
		}

		uint16_t randint = random_int(&random_state, RANDRANGE_16);
		//	printf("p_b %.6f p_a %.6f p_a*RANDRANGE_16 %d randint %d\n", p_b, p_a, (uint16_t) (p_a*RANDRANGE_16), randint);
		// mark_or_drop with probability p_a
		if (randint <= (uint16_t) (p_a * RANDRANGE_16)) {
			accept = mark_or_drop(pkt, false, port, queue);
		}
	}
	m_count_since_last[q_index]++;
	// in all other cases, q_avg <= q_min, so just return
	return accept;
}

uint8_t REDQueueManager::mark_or_drop(struct emu_packet *pkt, bool force_drop,
		uint32_t port, uint32_t queue) {
	uint32_t q_index = m_bank->flat_index(port, queue);

	m_count_since_last[q_index] = -1;
	if (force_drop || !(m_red_params.ecn)) {
		//        printf("RED dropping pkt\n");
		m_dropper->drop(pkt, port);
		return RED_DROPPKT;
	} else {
		/* mark the ECN bit */
		//        printf("RED marking pkt\n");
		m_dropper->mark_ecn(pkt, port);
		return RED_ACCEPTMARKED;
	}
}

/**
 * All ports of a REDRouter run RED. We don't currently support routers with 
 * different ports running different QMs or schedulers.
 */
REDRouter::REDRouter(struct red_args *red_params, uint32_t rack_index) :
		m_bank(EMU_ENDPOINTS_PER_RACK, 1, RED_QUEUE_CAPACITY),
		m_rt(16, rack_index, EMU_ENDPOINTS_PER_RACK, 0),
		m_cla(),
		m_qm(&m_bank, EMU_ENDPOINTS_PER_RACK, red_params),
		m_sch(&m_bank),
		REDRouterBase(&m_rt, &m_cla, &m_qm, &m_sch, EMU_ENDPOINTS_PER_RACK) {
}

void REDRouter::assign_to_core(Dropper *dropper,
		struct emu_admission_core_statistics *stat) {
	m_qm.assign_to_core(dropper, stat);
}

struct queue_bank_stats *REDRouter::get_queue_bank_stats() {
	return m_bank.get_queue_bank_stats();
}

REDRouter::~REDRouter() {}

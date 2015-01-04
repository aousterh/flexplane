/*
 * EthernetDevice.h
 *
 *  Created on: Dec 18, 2014
 *      Author: yonch
 */

#ifndef ETHERNETDEVICE_H_
#define ETHERNETDEVICE_H_

#include <rte_ethdev.h>
#include "util/getter_setter.h"

namespace dpdk {

class eth_conf;
class eth_txconf;
class eth_rxconf;

class EthernetDevice {
public:
	/** see rte_eth_dev_configure */
	EthernetDevice(uint8_t port_id, uint16_t nb_rx_q, uint16_t nb_tx_q,
		      const eth_conf &dev_conf);
	virtual ~EthernetDevice();

	/** see rte_eth_macaddr_get */
	struct ether_addr get_macaddr();

	/** see rte_eth_tx_queue_setup */
	void tx_queue_setup(uint16_t tx_queue_id,
					  uint16_t nb_tx_desc, unsigned int socket_id,
					  eth_txconf &tx_conf);

	/** see rte_eth_rx_queue_setup */
	void rx_queue_setup(uint16_t rx_queue_id, uint16_t nb_rx_desc,
						unsigned int socket_id, eth_rxconf &rx_conf,
					    struct rte_mempool *mb_pool);

	/** see rte_eth_dev_start */
	void start();

	/** see rte_eth_promiscuous_enable */
	void promiscuous_enable();

	/** see rte_eth_link_get_nowait */
	struct rte_eth_link link_get_nowait();

private:
	uint8_t m_port_id;
};

/**
 * see: struct rte_eth_thresh
 */
class eth_thresh {
public:
	eth_thresh(struct rte_eth_thresh *thresh) : m(thresh) {}
#define GET_SET(attr, type) GETTER_SETTER(eth_thresh, attr, type, m->attr)
	GET_SET(pthresh, uint8_t);
	GET_SET(hthresh, uint8_t);
	GET_SET(wthresh, uint8_t);
#undef GET_SET
private:
	struct rte_eth_thresh *m;
};

/**
 * See struct rte_eth_txconf
 */
class eth_txconf {
public:
	eth_txconf();
	eth_thresh tx_thresh() { return eth_thresh(&m.tx_thresh); }

#define GET_SET(attr, type) GETTER_SETTER(eth_txconf, attr, type, m.attr)
	GET_SET(tx_rs_thresh, uint16_t);
	GET_SET(tx_free_thresh, uint16_t);
	GET_SET(txq_flags, uint32_t);
	GET_SET(start_tx_per_q, uint8_t);
#undef GET_SET
private:
	friend EthernetDevice;
	struct rte_eth_txconf m;
};

/**
 * See struct rte_eth_txconf
 */
class eth_rxconf {
public:
	eth_rxconf();
	eth_thresh rx_thresh() { return eth_thresh(&m.rx_thresh); }

#define GET_SET(attr, type) GETTER_SETTER(eth_rxconf, attr, type, m.attr)
	GET_SET(rx_free_thresh, uint16_t);
	GET_SET(rx_drop_en, uint8_t);
	GET_SET(start_rx_per_q, uint8_t);
#undef GET_SET
private:
	friend EthernetDevice;
	struct rte_eth_rxconf m;
};

/**
 *  See: struct rte_eth_rxmode
 */
class rxmode {
public:
	rxmode(struct rte_eth_rxmode *rxmode) : m(rxmode) {}
#define GET_SET(attr, type) GETTER_SETTER(rxmode, attr, type, m->attr)
	GET_SET(mq_mode, enum rte_eth_rx_mq_mode);
	GET_SET(max_rx_pkt_len, uint32_t);
	GET_SET(split_hdr_size, uint16_t);
	GET_SET(header_split, uint8_t);
	GET_SET(hw_ip_checksum, uint8_t);
	GET_SET(hw_vlan_filter, uint8_t);
	GET_SET(hw_vlan_strip, uint8_t);
	GET_SET(hw_vlan_extend, uint8_t);
	GET_SET(jumbo_frame, uint8_t);
	GET_SET(hw_strip_crc, uint8_t);
	GET_SET(enable_scatter, uint8_t);
#undef GET_SET
private:
	struct rte_eth_rxmode *m;
};

/**
 * See: struct rte_eth_rss_conf
 */
class rssconf {
public:
	rssconf(struct rte_eth_rss_conf *rss_conf) : m(rss_conf) {}
#define GET_SET(attr, type) GETTER_SETTER(rssconf, attr, type, m->attr)
	GET_SET(rss_hf, uint64_t);
#undef GET_SET
private:
	struct rte_eth_rss_conf *m;
};

/**
 * See: struct rte_eth_txmode
 */
class txmode {
public:
	txmode(struct rte_eth_txmode *txmode) : m(txmode) {}
#define GET_SET(attr, type) GETTER_SETTER(txmode, attr, type, m->attr)
	GET_SET(mq_mode, enum rte_eth_tx_mq_mode);
#undef GET_SET
private:
	struct rte_eth_txmode *m;
};


class eth_conf {
public:
	eth_conf();

	rxmode &rx();
	rssconf &rss_conf();
	txmode &tx();

#define GET_SET(fn_name, type, attr) GETTER_SETTER(eth_conf, fn_name, type, m_conf.attr)
	GET_SET(link_speed, uint16_t, link_speed);
#undef GET_SET

private:
	friend EthernetDevice;
	struct rte_eth_conf m_conf;
	rxmode m_rxmode;
	rssconf m_rssconf;
	txmode m_txmode;
};

} /* namespace dpdk */

#endif /* ETHERNETDEVICE_H_ */

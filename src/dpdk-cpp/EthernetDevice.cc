/*
 * EthernetDevice.cpp
 *
 *  Created on: Dec 18, 2014
 *      Author: yonch
 */

#include "EthernetDevice.h"

#include <sstream>
#include <stdexcept>

dpdk::EthernetDevice::EthernetDevice(uint8_t port_id, uint16_t nb_rx_q,
		uint16_t nb_tx_q, const eth_conf& dev_conf)
	: m_port_id(port_id)
{
	int ret;

	ret = rte_eth_dev_configure(port_id, nb_rx_q, nb_tx_q, &dev_conf.m_conf);

	if (ret != 0) {
		std::stringstream msg;
		msg << "Could not configure port " << int(port_id) << ", ret=" << ret;
		throw std::runtime_error(msg.str());
	}
}

dpdk::EthernetDevice::~EthernetDevice() {}

struct ether_addr dpdk::EthernetDevice::get_macaddr()
{
	struct ether_addr ret;
	rte_eth_macaddr_get(m_port_id, &ret);
	return ret;
}

void dpdk::EthernetDevice::tx_queue_setup(uint16_t tx_queue_id,
		uint16_t nb_tx_desc, unsigned int socket_id,
		eth_txconf &tx_conf)
{
	int ret;
	ret = rte_eth_tx_queue_setup(m_port_id, tx_queue_id, nb_tx_desc,
			socket_id, &tx_conf.m);

	if (ret != 0) {
		std::stringstream msg;
		msg << "tx_queue_setup port=" << m_port_id << " queue="<< tx_queue_id
			<< "failed with ret=" << ret;
		throw std::runtime_error(msg.str());
	}
}

void dpdk::EthernetDevice::rx_queue_setup(uint16_t rx_queue_id, uint16_t nb_rx_desc,
		unsigned int socket_id, eth_rxconf& rx_conf,
		struct rte_mempool* mb_pool)
{
	int ret;
	ret = rte_eth_rx_queue_setup(m_port_id, rx_queue_id, nb_rx_desc,
			socket_id, &rx_conf.m, mb_pool);

	if (ret != 0) {
		std::stringstream msg;
		msg << "rx_queue_setup port=" << m_port_id << " queue="<< rx_queue_id
			<< "failed with ret=" << ret;
		throw std::runtime_error(msg.str());
	}
}

void dpdk::EthernetDevice::start()
{
	int ret = rte_eth_dev_start(m_port_id);

	if (ret != 0) {
		std::stringstream msg;
		msg << "rte_eth_dev_start port=" << m_port_id
			<< "failed with ret=" << ret;
		throw std::runtime_error(msg.str());
	}
}

void dpdk::EthernetDevice::promiscuous_enable()
{
	rte_eth_promiscuous_enable(m_port_id);
}

struct rte_eth_link dpdk::EthernetDevice::link_get_nowait()
{
	struct rte_eth_link link;

	memset(&link, 0, sizeof(link));
	rte_eth_link_get_nowait(m_port_id, &link);

	return link;
}

dpdk::eth_txconf::eth_txconf()
{
	memset(&m, 0, sizeof(struct rte_eth_txconf));
	/* try to put sensible defaults */
	tx_thresh()
			.pthresh(36)
			.hthresh(0)
			.wthresh(0);

	(*this)
			.tx_free_thresh(0)
			.tx_rs_thresh(0);
}

dpdk::eth_rxconf::eth_rxconf()
{
	memset(&m, 0, sizeof(struct rte_eth_rxconf));
	rx_thresh()
			.pthresh(8)
			.hthresh(8)
			.wthresh(4);
	(*this)
			.rx_free_thresh(64)
			.rx_drop_en(0);

}

dpdk::eth_conf::eth_conf()
	: m_rxmode(&m_conf.rxmode),
	  m_rssconf(&m_conf.rx_adv_conf.rss_conf),
	  m_txmode(&m_conf.txmode)
{
	memset(&m_conf, 0, sizeof(struct rte_eth_conf));

	/* try to put sensible defaults */
	(*this).rx()
			.max_rx_pkt_len(ETHER_MAX_LEN)
			.split_hdr_size(0)
			.header_split(0)
			.hw_ip_checksum(1)
			.hw_vlan_filter(0)
			.jumbo_frame(0)
			.hw_strip_crc(0);

	(*this).rss_conf()
			.rss_hf(ETH_RSS_IPV4);

	(*this).tx()
			.mq_mode(ETH_DCB_NONE);
}


dpdk::rxmode &dpdk::eth_conf::rx() {
	return m_rxmode;
}

dpdk::rssconf &dpdk::eth_conf::rss_conf() {
	return m_rssconf;
}

dpdk::txmode &dpdk::eth_conf::tx() {
	return m_txmode;
}

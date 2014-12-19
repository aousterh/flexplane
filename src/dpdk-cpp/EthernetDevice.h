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

class EthernetDeviceConfig;

class EthernetDevice {
public:
	EthernetDevice();
	virtual ~EthernetDevice();

};



class EthernetDeviceConfig {
public:
	EthernetDeviceConfig();
	virtual ~EthernetDeviceConfig();

#define GET_SET(fn_name, attr) GETTER_SETTER(EthernetDeviceConfig, fn_name, struct rte_eth_conf, m_conf, attr)

	GET_SET(link_speed, link_speed);
	GET_SET(rx_split_hdr_size,rxmode.split_hdr_size = 0,
	GET_SET(rx_header_split,rxmode.header_split   = 0, /* Header Split disabled */
	GET_SET(rx_hw_ip_checksum,rxmode.hw_ip_checksum = 1, /* IP checksum offload enabled */
	GET_SET(rx_hw_vlan_filter,rxmode.hw_vlan_filter = 0, /* VLAN filtering disabled */
	GET_SET(rx_jumbo_frame,rxmode.jumbo_frame    = 1, /* Jumbo Frame Support enabled */
	GET_SET(rx_max_rx_pkt_len,rxmode.max_rx_pkt_len = 9000, /* Jumbo Frame MAC pkt length */
	GET_SET(rx_hw_strip_crc,rxmode.hw_strip_crc   = 0, /* CRC stripped by hardware */

#undef GET_SET

private:
	struct rte_eth_conf m_conf;
};

} /* namespace dpdk */

#endif /* ETHERNETDEVICE_H_ */

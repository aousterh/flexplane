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

#define GET_SET(fn_name, type, attr) GETTER_SETTER(EthernetDeviceConfig, fn_name, type, m_conf.attr)

	GET_SET(link_speed, uint16_t, link_speed);
	GET_SET(rx_split_hdr_size, uint16_t, rxmode.split_hdr_size);
	GET_SET(rx_header_split, uint8_t ,rxmode.header_split);
	GET_SET(rx_hw_ip_checksum, uint8_t ,rxmode.hw_ip_checksum);
	GET_SET(rx_hw_vlan_filter, uint8_t ,rxmode.hw_vlan_filter);
	GET_SET(rx_jumbo_frame, uint8_t ,rxmode.jumbo_frame);
	GET_SET(rx_max_rx_pkt_len, uint32_t ,rxmode.max_rx_pkt_len);
	GET_SET(rx_hw_strip_crc, uint8_t ,rxmode.hw_strip_crc);

#undef GET_SET

private:
	struct rte_eth_conf m_conf;
};

} /* namespace dpdk */

#endif /* ETHERNETDEVICE_H_ */

/*
 * emu_topology.h
 *
 *  Created on: July 24, 2015
 *      Author: aousterh
 */

#ifndef EMU_TOPOLOGY_H_
#define EMU_TOPOLOGY_H_

#include <inttypes.h>

/**
 * Configuration of emulated topology.
 */
struct emu_topo_config {
	uint16_t	num_racks;
	uint16_t	rack_shift;
};

/* The number of endpoints per rack, from the rack shift. */
static inline uint16_t endpoints_per_rack(struct emu_topo_config *topo_config)
{
	return (1 << topo_config->rack_shift);
}

/* The number of endpoints in each endpoint group. */
static inline uint16_t endpoints_per_epg(struct emu_topo_config *topo_config) {
	return endpoints_per_rack(topo_config);
}

/* The number of ToRs (top of rack switches) in the network. */
static inline uint16_t num_tors(struct emu_topo_config *topo_config) {
	return topo_config->num_racks;
}

/* The number of core routers in the network (routers in the tier above ToR).
 * */
static inline uint16_t num_core_routers(struct emu_topo_config *topo_config) {
	if (topo_config->num_racks == 1)
		return 0;
	else
		return 1; /* for now */
}

/* The total number of routers in the emulated topology. */
static inline uint16_t num_routers(struct emu_topo_config *topo_config) {
	return num_tors(topo_config) + num_core_routers(topo_config);
}

/* The total number of endpoints in the topology. */
static inline uint16_t num_endpoints(struct emu_topo_config *topo_config) {
	return topo_config->num_racks * endpoints_per_rack(topo_config);
}

/* The number of endpoint groups. */
static inline uint16_t num_endpoint_groups(struct emu_topo_config *topo_config)
{
	return topo_config->num_racks;
}

/* The number of ports on each ToR. */
static inline uint16_t tor_ports(struct emu_topo_config *topo_config) {
	if (topo_config->num_racks == 1)
		return endpoints_per_rack(topo_config); /* downward-facing ports */
	else
		return 2 * endpoints_per_rack(topo_config); /* up and downward ports */
}

/* Mask of links used to send to uplink core routers. */
static inline uint32_t tor_uplink_mask(struct emu_topo_config *topo_config) {
	if (topo_config->num_racks == 1)
		return 0; /* no uplinks */
	else
		return (1 << topo_config->rack_shift) - 1;
}

/* The number of ports on each Core router. */
static inline uint16_t core_router_ports(struct emu_topo_config *topo_config) {
	if (topo_config->num_racks == 2)
		return num_endpoints(topo_config);
	else if (topo_config->num_racks == 3)
		return 16 * 3;
	else
		return 0;
}

/* Endpoint groups per comm core. */
static inline uint16_t epgs_per_comm(struct emu_topo_config *topo_config) {
	return num_endpoint_groups(topo_config);
}

/* Number of packet queues needed for this topology. */
static inline uint16_t num_packet_qs(struct emu_topo_config *topo_config) {
	return 3 * num_endpoint_groups(topo_config) + num_routers(topo_config);
}

#endif /* EMU_TOPOLOGY_H_ */

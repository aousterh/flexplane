/*
 * config.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define DROP_ON_FAILED_ENQUEUE

#ifndef ALGO_N_CORES
#define ALGO_N_CORES			1
//#define ALGO_N_CORES			2
//#define ALGO_N_CORES			3
//#define ALGO_N_CORES			4
//#define ALGO_N_CORES			5
#endif

/* maximums */
#define EMU_MAX_OUTPUTS_PER_RTR	8
#define EMU_MAX_ENDPOINT_GROUPS	8
#define EMU_MAX_ROUTERS			8
#define EMU_MAX_EPGS_PER_COMM	EMU_MAX_ENDPOINT_GROUPS
#define EMU_MAX_PACKET_QS		(3 * EMU_MAX_ENDPOINT_GROUPS + EMU_MAX_ROUTERS)
#define EMU_MAX_ALGO_CORES		10

#endif /* CONFIG_H_ */

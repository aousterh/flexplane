/*
 * config.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define EMU_RACK_SHIFT			5
#define EMU_ENDPOINTS_PER_RACK	(1 << EMU_RACK_SHIFT)
#define EMU_NUM_ROUTERS			1
#define EMU_NUM_ENDPOINTS		(EMU_ENDPOINTS_PER_RACK * EMU_NUM_ROUTERS)
#define EMU_NUM_ENDPOINT_GROUPS	1
#define EMU_ENDPOINTS_PER_EPG	(EMU_NUM_ENDPOINTS / EMU_NUM_ENDPOINT_GROUPS)
#define EMU_ADMITS_PER_ADMITTED	(2 * EMU_NUM_ENDPOINTS)

#ifndef ALGO_N_CORES
#define ALGO_N_CORES			2
#endif

#endif /* CONFIG_H_ */

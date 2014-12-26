/*
 * config.h
 *
 *  Created on: June 23, 2014
 *      Author: aousterh
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define EMU_ROUTER_NUM_PORTS	32
#define EMU_NUM_ROUTERS			1
#define EMU_NUM_ENDPOINTS		(EMU_ROUTER_NUM_PORTS * EMU_NUM_ROUTERS)
#define EMU_NUM_ENDPOINT_GROUPS	1
#define EMU_MAX_DROPS			(EMU_NUM_ENDPOINTS)

#endif /* CONFIG_H_ */

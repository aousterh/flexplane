/*
 * emulation_c_compat.cc
 *
 *  Created on: Jan 25, 2015
 *      Author: yonch
 */


#include "emulation_c_compat.h"
#include "emulation.h"

void emu_add_backlog(void* emu, uint16_t src, uint16_t dst, uint16_t flow,
		uint32_t amt, uint16_t start_id, u8* areq_data)
{
	((Emulation *)emu)->add_backlog(src, dst, flow, amt, start_id, areq_data);
}

void emu_reset_sender(void* emu, uint16_t src)
{
	((Emulation *)emu)->reset_sender(src);
}

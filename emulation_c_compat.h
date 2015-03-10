/*
 * emulation_c_compat.h
 *
 *  Created on: Jan 25, 2015
 *      Author: yonch
 */

#ifndef EMULATION_C_COMPAT_H_
#define EMULATION_C_COMPAT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void emu_add_backlog(void *emu, uint16_t src, uint16_t dst, uint16_t flow,
		uint32_t amt, uint16_t start_id, uint8_t *areq_data);

void emu_reset_sender(void *emu, uint16_t src);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* EMULATION_C_COMPAT_H_ */

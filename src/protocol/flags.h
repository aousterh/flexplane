/*
 * flags.h
 *
 *  Created on: August 27, 2014
 *      Author: aousterh
 */

#ifndef FLAGS_H_
#define FLAGS_H_

#include "platform/generic.h"

/* This file contains information about encoding/decoding flags in Fastpass
 * control packets. */

#define FLAGS_MASK	0xF

/* flags for emulation */
#define EMU_FLAGS_NONE	0
#define EMU_FLAGS_DROP	1

#endif /* FLAGS_H_ */

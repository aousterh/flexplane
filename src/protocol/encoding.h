/*
 * encoding.h
 *
 *  Created on: August 27, 2014
 *      Author: aousterh
 */

#ifndef ENCODING_H_
#define ENCODING_H_

/* This file contains information about encoding/decoding payloads in Fastpass
 * control packets. */

#if ((defined(PARALLEL_ALGO) && defined(PIPELINED_ALGO)) || \
     (defined(PARALLEL_ALGO) && defined(EMULATION_ALGO)) ||  \
     (defined(EMULATION_ALGO) && defined(PIPELINED_ALGO)))
#error "Multiple ALGOs are defined"
#endif
#if !(defined(PARALLEL_ALGO) || defined(PIPELINED_ALGO) || \
      defined(EMULATION_ALGO))
#error "No ALGO is defined"
#endif

#if (defined(PARALLEL_ALGO) || defined(PIPELINED_ALGO))
/* uppermost 2 bits encode the path, remaining bits encode dst id */
#define NUM_PATHS 4  // if not 4, NUM_GRAPHS and related code must be modified
#define DST_MASK 0x3FFF  // 2^PATH_SHIFT - 1
#define PATH_MASK (0xFFFF & ~DST_MASK)
#define PATH_SHIFT 14
#define FLAGS_MASK	PATH_MASK
#endif

#ifdef EMULATION_ALGO
/* uppermost bit encodes drop info, remaining bits encode dst id */
#define FLAGS_MASK	0x8000
#define DST_MASK   	0x7FFF

/* TODO: add support for path selection with emulation */
#define NUM_PATHS 1
#define PATH_MASK 0
#define PATH_SHIFT 0
#endif

#if ((FLAGS_MASK | DST_MASK) != 0xFFFF)
#error "Invalid MASKs - some bits unused"
#endif

#if ((FLAGS_MASK & DST_MASK) != 0x0)
#error "Invalid MASKs - some bits used multiple times"
#endif

#endif /* ENCODING_H_ */

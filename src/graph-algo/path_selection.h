/*
 * path_selection.h
 *
 *  Created on: January 2, 2014
 *      Author: aousterh
 */

#ifndef PATH_SELECTION_H_
#define PATH_SELECTION_H_

#include "../protocol/encoding.h"

#include <stdbool.h>

/* dummy struct declaration */
struct admitted_traffic;

// Selects paths for traffic in admitted and writes the path ids
// to the most significant bits of the destination ip addrs
void select_paths(struct admitted_traffic *admitted, uint8_t num_racks);

// Returns true if the assignment of paths is valid; false otherwise
bool paths_are_valid(struct admitted_traffic *admitted, uint8_t num_racks);

#endif /* PATH_SELECTION_H_ */

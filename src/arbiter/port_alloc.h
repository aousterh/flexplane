/*
 * port_alloc.h
 *
 *  Created on: Sep 29, 2013
 *      Author: yonch
 */

#ifndef PORT_ALLOC_H_
#define PORT_ALLOC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Allocates an RX queue for portid on lcore.
 *
 * @param lcore: the lcore that's allocated a queue
 * @param portid: the port whose RX queue is allocated
 * @return 0 on success, negative on error
 */
int conf_alloc_rx_queue(uint16_t lcore, uint8_t portid);

/**
 * \brief Allocates an TX queue for portid on lcore.
 *
 * @param lcore: the lcore that's allocated a queue
 * @param portid: the port whose TX queue is allocated
 * @return 0 on success, negative on error
 */
int conf_alloc_tx_queue(uint16_t lcore, uint8_t portid);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PORT_ALLOC_H_ */

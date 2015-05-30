/**
 * @file       uart.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef UART_H_
#define UART_H_

/*================================ include ==================================*/

#include "types.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

typedef void (* uart_cb_t)(void);

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void uart_init(void);
void uart_deinit(void);

void uart_enable_interrupts(void);
void uart_disable_interrupts(void);

void uart_register_rx_cb(uart_cb_t callback);
void uart_register_tx_cb(uart_cb_t callback);

void uart_cancel_rx_cb(void);
void uart_cancel_tx_cb(void);

void uart_send_byte(uint8_t byte);
uint8_t uart_receive_byte(void);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* UART_H_ */

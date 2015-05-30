/**
 * @file       radio.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef RADIO_H_
#define RADIO_H_

/*================================ include ==================================*/

#include "types.h"
#include "packet_buffer.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

typedef void (*radio_cb_t)(void);

typedef enum {
    RADIO_OFF             = 0x00,
    RADIO_SLEEP           = 0x01,
    RADIO_IDLE            = 0x02,
    RADIO_TX_ENABLING     = 0x03,
    RADIO_TX_ENABLED      = 0x04,
    RADIO_TX_TRANSMITTING = 0x05,
    RADIO_TX_DONE         = 0x06,
    RADIO_RX_ENABLING     = 0x07,
    RADIO_RX_ENABLED      = 0x08,
    RADIO_RX_RECEIVING    = 0x09,
    RADIO_RX_DONE         = 0x0a,
    RADIO_ERROR           = 0x0b
} radio_state_t;

typedef struct {
    radio_state_t current_state;
    radio_cb_t rx_init;
    radio_cb_t tx_init;
    radio_cb_t rx_done;
    radio_cb_t tx_done;
} radio_vars_t;

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void radio_init(void);
void radio_idle(void);
void radio_receive(void);
void radio_transmit(void);
void radio_reset(void);

void radio_set_rx_cb(radio_cb_t rx_init_cb, radio_cb_t rx_done_cb);
void radio_set_tx_cb(radio_cb_t tx_init_cb, radio_cb_t tx_done_cb);

void radio_cancel_rx_cb(void);
void radio_cancel_tx_cb(void);

void radio_enable_interrupts(void);
void radio_disable_interrupts(void);

void radio_set_channel(uint8_t channel);

void radio_set_power(uint8_t power);

void radio_get_packet(packet_buffer_t* queue_entry);
void radio_put_packet(packet_buffer_t* queue_entry);

void radio_read_rssi(int8_t* rssi);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* RADIO_H_ */

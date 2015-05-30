/**
 * @file       wor.c
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

/*================================ include ==================================*/

#include "wor.h"

#include "scheduler.h"
#include "library.h"

#include "debug.h"
#include "leds.h"
#include "radio.h"
#include "uart.h"

/*================================ define ===================================*/

#define WOR_TX_DURATION             ( 65536UL )
#define WOR_TX_PERIOD               ( 32UL )
#define WOR_TX_PACKETS              ( WOR_TX_DURATION / WOR_TX_PERIOD )

#define WOR_RX_PERIOD               ( 32768 )
#define WOR_RX_DURATION             ( 64 )

#if MAC_DEVICE == MAC_GATEWAY
#define WOR_PREPARE                 ( 1 )
#define WOR_PROCESS                 ( 3 )
#define WOR_LIFS                    ( 32 )
#define WOR_DRIFT                   ( 64 )
#elif MAC_DEVICE == MAC_NODE
#define WOR_PREPARE                 ( 1 )
#define WOR_PROCESS                 ( 3 )
#define WOR_LIFS                    ( 32 )
#define WOR_DRIFT                   ( 64 )
#else
#error "MAC_DEVICE not defined."
#endif

#define WOR_RADIO_CHANNEL           ( 26 )

/*================================ typedef ==================================*/

typedef struct {
    wor_cb_t wor_cb;

    mac_time_t tx_duration;
    mac_time_t tx_period;
    mac_time_t tx_packets;

    mac_time_t rx_period;
    mac_time_t rx_duration;

    mac_channel_t wor_channel;
} wor_vars_t;

// Packet = 1 length + 5 payload + 2 crc = 8 bytes
typedef struct __attribute__((__packed__)) {
    mac_type_t mac_type;                     // 1 B
    mac_packet_t mac_packet;                 // 1 B
    mac_time_t mac_time;                     // 2 B
    mac_channel_t mac_channel;               // 1 B
} wor_packet_t;

/*=============================== variables =================================*/

wor_vars_t wor_vars;

/*=============================== prototypes ================================*/

void wor_start(void);
void wor_rx_init(void);
void wor_tx_init(void);
void wor_rx_done(void);
void wor_tx_done(void);
void wor_timeout(void);
void wor_done(void);

/*================================= public ==================================*/

void wor_init(void) {
    // Initialize the memory of the variables
    memset(&wor_vars, 0, sizeof(wor_vars_t));
}

void wor_set_cb(wor_cb_t callback) {
    // Setup the WOR callback
    wor_vars.wor_cb = callback;
}

void wor_cancel_cb(void) {
    // Clear the WOR callback
    wor_vars.wor_cb = NULL;
}

/*================================ private ==================================*/

// If the device type is GATEWAY
#if (MAC_DEVICE == MAC_GATEWAY)

void wor_config(void) {
    // Setup the WOR variables
    wor_vars.tx_duration = (mac_time_t)(WOR_TX_DURATION - 1);
    wor_vars.tx_period   = WOR_TX_PERIOD;
    wor_vars.tx_packets  = WOR_TX_PACKETS;
    wor_vars.wor_channel = WOR_RADIO_CHANNEL;

    // Update the MAC variables
    mac_vars.mac_packet  = MAC_PACKET_WOR;
    mac_vars.mac_time    = wor_vars.tx_duration;
    mac_vars.mac_channel = mac_vars.mac_channel;

    // Schedule the task to start the WOR
    scheduler_push(wor_start, TASK_PRIO_MAX);
}

void wor_start(void) {
    wor_packet_t* wor_packet = NULL;
    virtual_timer_width_t ticks;

    debug_system_on();
    debug_user_on();

    // Obtain a queue entry
    mac_vars.queue_mac_tx = packet_buffer_get();
    wor_packet = (wor_packet_t*) mac_vars.queue_mac_tx->payload;
    mac_vars.queue_mac_tx->length = sizeof(wor_packet_t);

    // Prepare the WOR
    wor_packet->mac_type = mac_vars.mac_type;
    wor_packet->mac_packet = mac_vars.mac_packet;
    wor_packet->mac_time = mac_vars.mac_time;
    wor_packet->mac_channel = mac_vars.mac_channel;

    // Wake up the radio
    radio_idle();

    // Set the radio to the WOR channel
    radio_set_channel(wor_vars.wor_channel);

    // Set the radio transmit callbacks
    radio_set_tx_cb(wor_tx_init, wor_tx_done);
    radio_enable_interrupts();

    // Put the WOR in the radio and transmit it
    radio_put_packet(mac_vars.queue_mac_tx);
    radio_transmit();

    // Wait for the duration of a FBP
    ticks = wor_vars.tx_period - MAC_RADIO_IDLE_TX - WOR_PREPARE;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, wor_done, TASK_PRIO_MAX);

    debug_user_off();
}

void wor_tx_init(void) {
    debug_radio_on();
}

void wor_tx_done(void) {
    debug_radio_off();
}

void wor_done(void) {
    virtual_timer_width_t ticks;

    debug_user_on();

    // Put the radio back to idle just in case
    radio_idle();
    radio_cancel_tx_cb();

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_tx);
    mac_vars.queue_mac_tx = NULL;

    // Update WOR status
    wor_vars.tx_packets  -= 1;
    wor_vars.tx_duration -= wor_vars.tx_period;

    // Update the MAC variables
    mac_vars.mac_time = wor_vars.tx_duration;

    // Determine next action to take
    if (wor_vars.tx_packets != 0) {
        // Continue transmitting WOR
        ticks = VIRTUAL_TIMER_KICK_NOW;
        virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, wor_start, TASK_PRIO_MAX);
    } else {
        // Wait LIFS to start the FBP
        ticks = 16 * WOR_LIFS;
        virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, wor_vars.wor_cb, TASK_PRIO_MAX);
    }

    debug_user_off();
    debug_system_off();
}

#endif /* MAC_DEVICE == MAC_GATEWAY */

// If the device type is END NODE
#if (MAC_DEVICE == MAC_NODE)

void wor_config(void) {
    // Setup the WOR variables
    wor_vars.rx_duration = WOR_RX_DURATION;
    wor_vars.rx_period   = WOR_RX_PERIOD;
    wor_vars.wor_channel = WOR_RADIO_CHANNEL;

    // Schedule the task to start the MAC
    scheduler_push(wor_start, TASK_PRIO_MAX);
}

void wor_start(void) {
    virtual_timer_width_t ticks;

    debug_system_on();
    debug_user_on();

    // Wake up the radio
    radio_idle();
    
    // Set the radio to the WOR channel
    radio_set_channel(wor_vars.wor_channel);

    // Set the radio receive callbacks
    radio_set_rx_cb(wor_rx_init, wor_rx_done);
    radio_enable_interrupts();

    // Go to timeout if the radio timer expires and we got nothing
    ticks = wor_vars.rx_duration;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, wor_timeout, TASK_PRIO_MAX);

    // Put the radio to receive
    radio_receive();

    debug_user_off();
}

void wor_rx_init(void) {
    debug_radio_on();
}

void wor_rx_done(void) {
    wor_packet_t* wor_packet = NULL;

    // Get the packet from the radio
    mac_vars.queue_mac_rx = packet_buffer_get();
    radio_get_packet(mac_vars.queue_mac_rx);

    // Check if the received packet is correct
    if (mac_vars.queue_mac_rx->crc) {
        // Convert the packet to a WOR packet
        wor_packet = (wor_packet_t *) mac_vars.queue_mac_rx->payload;
        if (wor_packet->mac_packet == MAC_PACKET_WOR) {
            // Update the WOR variables
            mac_set_type(wor_packet->mac_type);
            mac_set_channel(wor_packet->mac_channel);
            mac_set_time(wor_packet->mac_time);

            // Cancel radio receive callbacks
            radio_cancel_rx_cb();

            // Sleep the radio
            radio_idle();
        }
    }

    debug_radio_off();
}

void wor_timeout(void) {
    virtual_timer_width_t ticks;

    debug_user_on();

    // Put the radio back to idle just in case
    radio_idle();
    radio_cancel_rx_cb();

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_rx);
    mac_vars.queue_mac_rx = NULL;

    if (mac_vars.mac_type == MAC_TYPE_NONE) {
        // Start again if we timed out
        ticks = wor_vars.rx_period - wor_vars.rx_duration - 2 * MAC_RADIO_IDLE_RX;
        virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, wor_start, TASK_PRIO_MAX);
    } else {
        // Start again if we timed out
        ticks = VIRTUAL_TIMER_KICK_NOW;
        virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, wor_done, TASK_PRIO_MAX);
    }

    debug_user_off();
}

void wor_done(void) {
    virtual_timer_width_t ticks;

    debug_user_on();

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_rx);
    mac_vars.queue_mac_rx = NULL;

    // Schedule to start FBP
    ticks = mac_vars.mac_time + 12 * WOR_LIFS;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, wor_vars.wor_cb, TASK_PRIO_MAX);

    debug_user_off();
    debug_system_off();
}

#endif

/**
 * @file       fsa.c
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

/*================================ include ==================================*/

#include "mac.h"
#include "fsa.h"

#include "scheduler.h"
#include "library.h"

#include "board.h"
#include "cpu.h"
#include "debug.h"
#include "ieee-addr.h"
#include "leds.h"
#include "radio.h"
#include "uart.h"

/*================================ define ===================================*/

#define FSA_FBP_DURATION                ( 32 )
#define FSA_DATA_DURATION               ( 152 )
#define FSA_ACK_DURATION                ( 32 )
#define FSA_SIFS_DURATION               ( 16 )
#define FSA_LIFS_DURATION               ( 32 )
#define FSA_SLOT_DURATION               ( 1 * FSA_DATA_DURATION + \
                                          1 * FSA_ACK_DURATION  + \
                                          2 * FSA_SIFS_DURATION   \
                                        ) // 152 + 32 + 2 * 16 = 216

#if (MAC_DEVICE == MAC_GATEWAY)
#define FSA_FBP_PREPARE                 ( 1 )
#define FSA_FBP_PROCESS                 ( 1 )
#define FSA_DATA_PREPARE                ( 1 )
#define FSA_DATA_PROCESS                ( 1 )
#define FSA_ACK_PREPARE                 ( 1 )
#define FSA_ACK_PROCESS                 ( 1 )
#elif (MAC_DEVICE == MAC_NODE)
#define FSA_FBP_PREPARE                 ( 1 )
#define FSA_FBP_PROCESS                 ( 2 )
#define FSA_DATA_PREPARE                ( 4 )
#define FSA_DATA_PROCESS                ( 1 )
#define FSA_ACK_PREPARE                 ( 1 )
#define FSA_ACK_PROCESS                 ( 1 )
#else
#error "MAC_DEVICE not defined."
#endif

#define FSA_RADIO_CHANNEL               ( 26 )
#define FSA_RSSI_THRESHOLD              ( -85 )
#define FSA_UNSYNC_ERRORS               ( 16 )

/*================================ typedef ==================================*/

typedef uint8_t fsa_slot_count_t;

/**
 * Packet structure for FSA operation
 */
typedef struct {
    mac_packet_t mac_packet;        ///< Type of packet we received
    mac_address_t mac_address;      ///< Local address of the node
    mac_seq_number_t seq_number;    ///< Sequence number of the packet

    uint8_t packet_success;         ///< Packet was received successfully

    uint8_t slot_total;             ///< Total number of slots in the frame
    uint8_t slot_count;             ///< Current slot in the frame
    uint8_t slot_selected;          ///< Slot that we selected in the frame
    uint8_t slot_remaining;         ///< Number of slots remaining in the frame

    mac_data_state_t data_state;    ///< State of the received packet
    mac_address_t data_address;     ///< Origin of the received packet
    mac_rssi_t rssi_status;         ///< RSSI status of the received packet

    int8_t data_rssi;               ///< RSSI value of the received packet
    int8_t data_rssi_threshold;     ///< RSSI value to consider packets as collided
} fsa_vars_t;

/**
 * Packet structure for FSA statistics
 */
typedef struct {
    uint8_t fsa_total;              ///< Number of transmitted packets
    uint8_t unsync_error;           ///< Number of unsynchronization errors
} fsa_stats_t;

/**
 * Packet structure to allow FSA debugging over serial
 * Length = 7 bytes
 */
typedef struct __attribute__((__packed__)) {
    uint8_t  mac_type;              ///< MAC type (1 byte)
    uint8_t  slot_count;            ///< Current slot in the frame (1 byte)
    uint8_t  data_state;            ///< State of the received packet (1 byte)
    uint16_t data_address;          ///< The address of the packet source (2 bytes)
    int8_t   rssi_status;           ///< RSSI we received the packet (1 byte)
    uint8_t  fsa_total;             ///< Number of transmitted packets (1 byte)
} fsa_debug_serial_t;

/**
 * Packet structure for FBP (FeedBack Packet) packets
 * Length = 1 size + 10 payload + 2 crc = 13 bytes
 * Time   = 13 bytes @ 250 kbps = 0,416 ms = 13,63 ticks @ 32.768 kHz -> 24 ticks
 */
typedef struct __attribute__((__packed__)) {
    uint8_t  mac_type;              ///< MAC type (1 byte)
    uint8_t  mac_packet;            ///< Packet type (1 byte)
    uint16_t source;                ///< Packet soruce (2 bytes)
    uint16_t destination;           ///< Packet destination (2 bytes)
    uint16_t seq_number;            ///< Sequence number (2 bytes)
    uint8_t  next_channel;          ///< Next channel (1 bytes)
    uint8_t  slot_count;            ///< Number of slots per frame (1 byte)
} fsa_fbp_t;

/**
 * Packet structure for DATA packets
 * Length = 1 size + 125 payload + 2 crc = 128 bytes
 * Time   = 128 bytes @ 250 kbps = 4,096 ms = 134,25 ticks @ 32.768 kHz -> 152 ticks
 */
typedef struct __attribute__((__packed__)) {
    uint8_t  mac_type;              ///< MAC type (1 byte)
    uint8_t  mac_packet;            ///< Packet type (1 byte)
    uint16_t source;                ///< Paclet source (2 bytes)
    uint16_t destination;           ///< Packet destination (2 bytes)
    uint8_t  fsa_total;             ///< Number of transmitted packets (1 bytes)
    uint8_t  data[118];             ///< Packet payload (118 bytes)
} fsa_data_t;

/**
 * Packet structure for ACK (Acknowledgement) packets
 * Length = 1 size + 7 payload + 2 crc = 10 bytes
 * Time = 10 bytes @ 250 kbps = 0,320 ms = 10,48 ticks @ 32.768 kHz -> 16 ticks
 */
typedef struct __attribute__((__packed__)) {
    uint8_t  mac_type;              ///< MAC type (1 byte)
    uint8_t  mac_packet;            ///< Packet type (1 byte)
    uint16_t source;                ///< Packet source (2 byte)
    uint16_t destination;           ///< Packet destination(2 byte)
    uint8_t  data_state;            ///< Packet status (1 byte)
} fsa_ack_t;

/*=============================== variables =================================*/

fsa_vars_t fsa_vars;
fsa_stats_t fsa_stats;
fsa_debug_serial_t fsa_debug_serial;

/*=============================== prototypes ================================*/

static void fsa_fbp_init(void);
static void fsa_fbp_done(void);
static void fsa_data_init(void);
static void fsa_data_done(void);
static void fsa_ack_init(void);
static void fsa_ack_done(void);

static void fsa_vars_init(void);
static void fsa_vars_reset(void);

#if (MAC_DEVICE == MAC_GATEWAY)
static void fsa_fbp_tx_init(void);
static void fsa_fbp_tx_done(void);
static void fsa_data_rx_init(void);
static void fsa_data_rx_rssi(void);
static void fsa_data_rx_done(void);
static void fsa_ack_tx_init(void);
static void fsa_ack_tx_done(void);
static void fsa_vars_log(void);
#elif (MAC_DEVICE == MAC_NODE)
static void fsa_fbp_rx_init(void);
static void fsa_fbp_rx_done(void);
static void fsa_data_tx_init(void);
static void fsa_data_tx_done(void);
static void fsa_ack_rx_init(void);
static void fsa_ack_rx_done(void);
static void fsa_vars_update(fsa_fbp_t* fsa_fbp);
#endif

/*================================= public ==================================*/

/**
 * @brief Function to initialize FSA operation
 */
void fsa_init(void) {
    // Initialize the memory of the variables
    memset(&fsa_vars, 0, sizeof(fsa_vars_t));
    memset(&fsa_stats, 0, sizeof(fsa_stats_t));
    memset(&fsa_debug_serial, 0, sizeof(fsa_debug_serial_t));

    // Initialize FSA variables
    fsa_vars_init();

    // Set the device address
    ieee_addr_get_eui16((uint16_t*) &fsa_vars.mac_address);
}

/**
 * @brief Function to start FSA operation
 */
void fsa_start(void) {
    debug_user_on();

    // Schedule the task to start the MAC
    scheduler_push(fsa_fbp_init, TASK_PRIO_MAX);

    debug_user_off();
}

/*================================ private ==================================*/

// If the device type is GATEWAY
#if (MAC_DEVICE == MAC_GATEWAY)

static void fsa_fbp_init(void) {
    virtual_timer_width_t ticks;
    fsa_fbp_t* fsa_fbp = NULL;

    debug_system_on();
    debug_user_on();

    // Restore the local FSA variables
    fsa_vars_reset();

    // Obtain a queue entry and populate it
    mac_vars.queue_mac_tx = packet_buffer_get();
    fsa_fbp = (fsa_fbp_t *) mac_vars.queue_mac_tx->payload;
    mac_vars.queue_mac_tx->length = sizeof(fsa_fbp_t);

    // Prepare the FBP
    fsa_fbp->mac_type = MAC_TYPE_FSA;
    fsa_fbp->mac_packet = MAC_PACKET_FBP;
    fsa_fbp->source = fsa_vars.mac_address;
    fsa_fbp->destination = MAC_ADDR_BCAST;
    fsa_fbp->seq_number = fsa_vars.seq_number;
    fsa_fbp->slot_count = fsa_vars.slot_total;
    fsa_fbp->next_channel = mac_vars.mac_channel;

    // Set the radio transmit callback
    radio_set_tx_cb(fsa_fbp_tx_init, fsa_fbp_tx_done);

    // Put the FBP in the radio and transmit it
    radio_put_packet(mac_vars.queue_mac_tx);
    radio_transmit();

    // Wait for the duration of a FBP
    ticks = FSA_FBP_DURATION;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_fbp_done, TASK_PRIO_MAX);

    debug_user_off();
}

static void fsa_fbp_tx_init(void) {
    debug_radio_on();
}

static void fsa_fbp_tx_done(void) {
    debug_radio_off();
}

static void fsa_fbp_done(void) {
    virtual_timer_width_t ticks;

    debug_user_on();

    // Put the radio back to IDLE just in case
    radio_idle();
    radio_cancel_tx_cb();

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_tx);
    mac_vars.queue_mac_tx = NULL;

    // Update the sequence number
    fsa_vars.seq_number += 1;

    // Wait LIFS to start the DATA
    ticks = FSA_LIFS_DURATION - MAC_RADIO_IDLE_RX - FSA_DATA_PREPARE;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_data_init, TASK_PRIO_MAX);

    debug_user_off();
    debug_system_off();
}

static void fsa_data_init(void) {
    virtual_timer_width_t ticks;

    debug_system_on();
    debug_user_on();

    // Set the radio receive callbacks
    radio_set_rx_cb(fsa_data_rx_init, fsa_data_rx_done);

    // Put the radio to receive
    radio_receive();

    // Wait for the duration of a DATA packet
    ticks = (FSA_DATA_DURATION >> 1) - (3 * FSA_DATA_PROCESS);
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_data_rx_rssi, TASK_PRIO_MAX);

    debug_user_off();
}

static void fsa_data_rx_init(void) {
    debug_radio_on();
}

static void fsa_data_rx_rssi(void) {
    virtual_timer_width_t ticks;

    debug_user_on();

    // Read and convert the RSSI
    radio_read_rssi(&fsa_vars.data_rssi);

    // Wait for the duration of the RSSI event
    ticks = (FSA_DATA_DURATION >> 1) - (1 * FSA_DATA_PROCESS);
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_data_done, TASK_PRIO_MAX);

    debug_user_off();
}

static void fsa_data_rx_done(void) {
    // Get the packet from the radio
    mac_vars.queue_mac_rx = packet_buffer_get();
    radio_get_packet(mac_vars.queue_mac_rx);

    debug_radio_off();
}

static void fsa_data_done(void) {
    virtual_timer_width_t ticks;
    fsa_data_t* fsa_data = NULL;

    debug_user_on();

    // Put the radio back to IDLE just in case
    radio_idle();
    radio_cancel_rx_cb();

    // Update the data slot counter
    fsa_vars.slot_count += 1;

    // Check if the RSSI is above the threshold
    if (fsa_vars.data_rssi > fsa_vars.data_rssi_threshold) {
        fsa_vars.rssi_status = MAC_RSSI_ABOVE;
    } else {
        fsa_vars.rssi_status = MAC_RSSI_BELOW;
    }

    // Check if the received packet is correct
    if (mac_vars.queue_mac_rx->crc) {
        // Convert the packet to a data packet
        fsa_data = (fsa_data_t *) mac_vars.queue_mac_rx->payload;
        if (fsa_data->mac_type == MAC_TYPE_FSA &&
            fsa_data->mac_packet == MAC_PACKET_DATA) {
            fsa_vars.data_state = MAC_DATA_SUCCESS;
            fsa_vars.data_address = fsa_data->source;
            fsa_stats.fsa_total = fsa_data->fsa_total;
        }
    } else {
        if (fsa_vars.rssi_status == MAC_RSSI_ABOVE) {
            fsa_vars.data_state = MAC_DATA_ERROR;
            fsa_vars.data_address = MAC_ADDR_NONE;
        } else {
            fsa_vars.data_state = MAC_DATA_EMPTY;
            fsa_vars.data_address = MAC_ADDR_NONE;
        }
    }

    // Update the debug variables
    fsa_vars_log();

    // Register the message and schedule a task to push it
    serial_push_msg(SERIAL_MOTE2PC_DATA, (uint8_t *) &fsa_debug_serial, sizeof(fsa_debug_serial_t));

    // Reset the number of received packets
    fsa_stats.fsa_total = 0;

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_rx);
    mac_vars.queue_mac_rx = NULL;

    // Wait SIFS to start ACK
    ticks = FSA_SIFS_DURATION - MAC_RADIO_IDLE_TX - FSA_ACK_PREPARE;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_ack_init, TASK_PRIO_MAX);

    debug_user_off();
    debug_system_off();
}

static void fsa_ack_init(void) {
    virtual_timer_width_t ticks;
    fsa_ack_t* fsa_ack = NULL;

    debug_system_on();
    debug_user_on();

    // Obtain a queue entry and populate it
    mac_vars.queue_mac_tx = packet_buffer_get();
    fsa_ack = (fsa_ack_t *) mac_vars.queue_mac_tx->payload;
    mac_vars.queue_mac_tx->length = sizeof(fsa_ack_t);

    // Prepare the ACK
    fsa_ack->mac_type = MAC_TYPE_FSA;
    fsa_ack->mac_packet = MAC_PACKET_ACK;
    fsa_ack->source = fsa_vars.mac_address;
    fsa_ack->destination = fsa_vars.data_address;
    fsa_ack->data_state = fsa_vars.data_state;

    // Set the radio transmit callback
    radio_set_tx_cb(fsa_ack_tx_init, fsa_ack_tx_done);

    // Put the ACK packet in the radio and transmit it
    radio_put_packet(mac_vars.queue_mac_tx);
    radio_transmit();

    // Wait for the duration of a ACK
    ticks = FSA_ACK_DURATION;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_ack_done, TASK_PRIO_MAX);

    debug_user_off();
}

static void fsa_ack_tx_init(void) {
    debug_radio_on();
}

static void fsa_ack_tx_done(void) {
    debug_radio_off();
}

static void fsa_ack_done(void) {
    virtual_timer_width_t ticks;

    debug_user_on();

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_tx);
    mac_vars.queue_mac_tx = NULL;

    // Put the radio back to IDLE just in case
    radio_idle();
    radio_cancel_tx_cb();

    // Reset the data related properties
    fsa_vars.data_state = MAC_DATA_EMPTY;
    fsa_vars.data_address = MAC_ADDR_NONE;

    // Update the data remaining counter
    fsa_vars.slot_remaining--;

    // If there are more data slots remaining
    if (fsa_vars.slot_remaining) {
        ticks = FSA_SIFS_DURATION - MAC_RADIO_IDLE_RX - FSA_DATA_PREPARE - FSA_ACK_PROCESS;
        virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_data_init, TASK_PRIO_MAX);
    } else { // Otherwise we go back to transmit a FBP
        ticks = FSA_SIFS_DURATION - MAC_RADIO_IDLE_TX - FSA_FBP_PREPARE - FSA_ACK_PROCESS;
        virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_fbp_init, TASK_PRIO_MAX);
    }

    debug_user_off();
    debug_system_off();
}

static void fsa_vars_init(void) {
    fsa_vars.data_rssi_threshold = FSA_RSSI_THRESHOLD;
}

static void fsa_vars_reset(void) {
    fsa_vars.slot_count     = 0;
    fsa_vars.slot_total     = mac_vars.mac_slots;
    fsa_vars.slot_remaining = mac_vars.mac_slots;
}

static void fsa_vars_log(void) {
    fsa_debug_serial.mac_type     = MAC_TYPE_FSA;
    fsa_debug_serial.slot_count   = fsa_vars.slot_count;
    fsa_debug_serial.data_state   = fsa_vars.data_state;
    fsa_debug_serial.data_address = fsa_vars.data_address;
    fsa_debug_serial.rssi_status  = fsa_vars.rssi_status;
    fsa_debug_serial.fsa_total    = fsa_stats.fsa_total;
}

#endif /* MAC_DEVICE == MAC_GATEWAY */

// If the device type is END NODE
#if (MAC_DEVICE == MAC_NODE)

virtual_timer_id_t virtual_timer_id;

static void fsa_fbp_init(void) {
    virtual_timer_width_t ticks;

    debug_system_on();
    debug_user_on();

    // Restore the local FSA variables
    fsa_vars_reset();

    // Register the radio callbacks
    radio_set_rx_cb(fsa_fbp_rx_init, fsa_fbp_rx_done);

    // Register and start the radio timer callback
    ticks = FSA_FBP_DURATION << 4;
    virtual_timer_id = virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_fbp_done, TASK_PRIO_MAX);

    // Put the radio to receive
    radio_receive();

    debug_user_off();
}

static void fsa_fbp_rx_init(void) {
    virtual_timer_width_t ticks;

    debug_radio_on();

    // Stop the old virtual timer
    virtual_timer_stop(virtual_timer_id);

    // Start the virtual timer callback
    ticks = FSA_FBP_DURATION - MAC_RADIO_PHY_HEADER - MAC_RADIO_IDLE_RX - 3; // ToDo: Check
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_fbp_done, TASK_PRIO_MAX);
}

static void fsa_fbp_rx_done(void) {
    fsa_fbp_t* fsa_fbp = NULL;

    // Get the packet from the radio
    mac_vars.queue_mac_rx = packet_buffer_get();
    radio_get_packet(mac_vars.queue_mac_rx);

    // Check if the received packet is correct
    if (mac_vars.queue_mac_rx->crc) {
        // Convert the packet to a FBP
        fsa_fbp = (fsa_fbp_t *) mac_vars.queue_mac_rx->payload;

        // If packet is FSA and FBP
        if (fsa_fbp->mac_type == MAC_TYPE_FSA &&
            fsa_fbp->mac_packet == MAC_PACKET_FBP) {
            // Update the local FSA variables
            fsa_vars_update(fsa_fbp);
        }
    }

    debug_radio_off();
}

static void fsa_fbp_done(void) {
    virtual_timer_width_t ticks;

    debug_user_on();

    // Put the radio back to IDLE just in case
    radio_idle();
    radio_cancel_rx_cb();

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_rx);
    mac_vars.queue_mac_rx = NULL;

    // If we receive a FBP
    if (fsa_vars.mac_packet == MAC_PACKET_FBP) {
        // We synchronize upon receiving a FBP
        mac_toggle_synchronized(MAC_STATE_SYNC);

        // We reset the unsynchronized errors variable
        fsa_stats.unsync_error = FSA_UNSYNC_ERRORS;

        // Select the slot at which the data packet is transmitted
        fsa_vars.slot_selected = random_get() % fsa_vars.slot_total;
        fsa_vars.slot_remaining = (fsa_vars.slot_total - fsa_vars.slot_selected - 1);

        // Decide action based on selected slot
        ticks = FSA_LIFS_DURATION - MAC_RADIO_IDLE_TX - FSA_DATA_PREPARE;
        ticks += fsa_vars.slot_selected * (FSA_SLOT_DURATION) + 1;
        virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_data_init, TASK_PRIO_MAX);
    } else { // If we receive anything else
        // Notify we have lost synchronization
        mac_toggle_synchronized(MAC_STATE_UNSYNC);

        // Cancel the radio callbacks
        radio_cancel_rx_cb();
        radio_cancel_tx_cb();

        // Decrement the unsynchronized errors variable
        fsa_stats.unsync_error--;

        // Register and start the appropriate radio timer callback
        if (fsa_stats.unsync_error == 0) { // Lost synchronization
            radio_reset();
            cpu_reset();
        } else { // Maintain synchronization
            // ticks = FSA_SLOT_DURATION - FSA_FBP_DURATION - MAC_RADIO_IDLE_RX;
            // virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_fbp_init, TASK_PRIO_MAX);
            ticks = VIRTUAL_TIMER_KICK_NOW;
            virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_fbp_init, TASK_PRIO_MAX);
        }
    }

    debug_user_off();
    debug_system_off();
}

static void fsa_data_init(void) {
    virtual_timer_width_t ticks;
    fsa_data_t* fsa_data = NULL;

    debug_system_on();
    debug_user_on();

    // Obtain a queue entry
    mac_vars.queue_mac_tx = packet_buffer_get();
    fsa_data = (fsa_data_t *) mac_vars.queue_mac_tx->payload;
    mac_vars.queue_mac_tx->length = sizeof(fsa_data_t);

    // Create the DATA packet
    fsa_data->mac_type = MAC_TYPE_FSA;
    fsa_data->mac_packet = MAC_PACKET_DATA;
    fsa_data->destination = MAC_ADDR_BCAST;
    fsa_data->source = fsa_vars.mac_address;
    fsa_data->fsa_total = fsa_stats.fsa_total;

    // Fill in the DATA packet
    uint16_t random = random_get();
    for (uint8_t i = 0; i < sizeof(fsa_data->data); i++) {
        fsa_data->data[i] = random >> 0;
    }

    // Register the radio callback
    radio_set_tx_cb(fsa_data_tx_init, fsa_data_tx_done);

    // Put the DATA in the radio and transmit it
    radio_put_packet(mac_vars.queue_mac_tx);
    radio_transmit();

    // Register and start the radio timer callback
    ticks = FSA_DATA_DURATION - FSA_DATA_PREPARE;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_data_done, TASK_PRIO_MAX);

    debug_user_off();
}

static void fsa_data_tx_init(void) {
    debug_radio_on();
}

static void fsa_data_tx_done(void) {
    debug_radio_off();
}

static void fsa_data_done(void) {
    virtual_timer_width_t ticks;

    debug_user_on();

    // Put the radio back to IDLE just in case
    radio_idle();
    radio_cancel_tx_cb();

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_tx);
    mac_vars.queue_mac_tx = NULL;

    // Register and start the radio timer callback
    ticks = FSA_SIFS_DURATION - MAC_RADIO_IDLE_RX - FSA_DATA_PROCESS;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_ack_init, TASK_PRIO_MAX);

    debug_system_off();
    debug_user_off();
}

static void fsa_ack_init(void) {
    virtual_timer_width_t ticks;

    debug_system_on();
    debug_user_on();

    // Register the radio callbacks
    radio_set_rx_cb(fsa_ack_rx_init, fsa_ack_rx_done);

    // Put the radio to receive
    radio_receive();

    // Start the radio timer callback
    ticks = FSA_ACK_DURATION - FSA_ACK_PREPARE;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_ack_done, TASK_PRIO_MAX); // ToDo: Check

    debug_user_off();
}

static void fsa_ack_rx_init(void) {
    debug_radio_on();
}

static void fsa_ack_rx_done(void) {
    fsa_ack_t* fsa_ack = NULL;

    // Get the packet from the radio
    mac_vars.queue_mac_rx = packet_buffer_get();
    radio_get_packet(mac_vars.queue_mac_rx);

    // Check if the received packet is correct
    if (mac_vars.queue_mac_rx->crc) {
        // Convert the packet to a FBP
        fsa_ack = (fsa_ack_t*) mac_vars.queue_mac_rx->payload;
        if (fsa_ack->mac_type == MAC_TYPE_FSA &&
            fsa_ack->mac_packet == MAC_PACKET_ACK) {
            if (fsa_ack->data_state == MAC_DATA_SUCCESS &&
                    fsa_ack->destination == fsa_vars.mac_address) {
                fsa_vars.packet_success = true;
            } else {
                fsa_vars.packet_success = false;
            }
        }
    } else {
        // Notify we have lost synchronization
        mac_toggle_synchronized(MAC_STATE_UNSYNC);
    }

    debug_radio_off();
}

static void fsa_ack_done(void) {
    virtual_timer_width_t ticks;

    debug_user_on();

    // Put the radio back to IDLE just in case
    radio_idle();
    radio_cancel_rx_cb();

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_rx);
    mac_vars.queue_mac_rx = NULL;

    // Start the radio timer callback
    ticks  = FSA_SIFS_DURATION - MAC_RADIO_IDLE_RX - FSA_ACK_PROCESS;
    ticks += fsa_vars.slot_remaining * FSA_SLOT_DURATION;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, fsa_fbp_init, TASK_PRIO_MAX); // ToDo: Check

    debug_user_off();
    debug_system_off();
}

static void fsa_vars_init(void) {
    fsa_vars.mac_packet     = MAC_PACKET_NONE;
    fsa_vars.slot_count     = 0;
    fsa_vars.packet_success = false;
}

static void fsa_vars_reset(void) {
    fsa_vars.mac_packet     = MAC_PACKET_NONE;
    fsa_vars.slot_count     = 0;
    fsa_vars.packet_success = false;
}

static void fsa_vars_update(fsa_fbp_t* fsa_fbp) {
    // Update the FSA variables
    fsa_vars.mac_packet = fsa_fbp->mac_packet;
    fsa_vars.slot_total = fsa_fbp->slot_count;

    // Update the MAC variables
    mac_vars.mac_channel = fsa_fbp->next_channel;
}

#endif /* MAC_DEVICE == MAC_NODE */

/**
 * @file       dq.c
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
#include "dq.h"

#include "scheduler.h"
#include "library.h"

#include "cpu.h"
#include "debug.h"
#include "ieee-addr.h"
#include "leds.h"
#include "radio.h"
#include "uart.h"

/*================================ define ===================================*/

#define DQ_FBP_DURATION                 ( 44 )
#define DQ_ARP_DURATION                 ( 24 )
#define DQ_DATA_DURATION                ( 152 )
#define DQ_SIFS_DURATION                ( 16 )
#define DQ_LIFS_DURATION                ( 32 )
#define DQ_SLOT_DURATION                ( 1 * DQ_FBP_DURATION  + \
                                          1 * DQ_LIFS_DURATION + \
                                          1 * DQ_ARP_DURATION  + \
                                          1 * DQ_SIFS_DURATION + \
                                          1 * DQ_ARP_DURATION  + \
                                          1 * DQ_SIFS_DURATION + \
                                          1 * DQ_ARP_DURATION  + \
                                          1 * DQ_SIFS_DURATION + \
                                          1 * DQ_DATA_DURATION + \
                                          1 * DQ_SIFS_DURATION   \
                                        ) // 44 + 32 + 3 * 16 + 3 * 24 + 152 + 16 = 364
#define DQ_ARP_COUNT                    ( 3 )

#if MAC_DEVICE == MAC_GATEWAY
#define DQ_FBP_PREPARE                  ( 2 )
#define DQ_FBP_PROCESS                  ( 2 )
#define DQ_ARP_PREPARE                  ( 2 )
#define DQ_ARP_PROCESS                  ( 1 )
#define DQ_DATA_PREPARE                 ( 1 )
#define DQ_DATA_PROCESS                 ( 2 )
#elif MAC_DEVICE == MAC_NODE
#define DQ_FBP_PREPARE				    ( 1 )
#define DQ_FBP_PROCESS                  ( 3 )
#define DQ_ARP_PREPARE                  ( 1 )
#define DQ_ARP_PROCESS                  ( 1 )
#define DQ_DATA_PREPARE                 ( 1 )
#define DQ_DATA_PROCESS                 ( 1 )
#else
#error "MAC_DEVICE not defined."
#endif

#define DQ_RADIO_CHANNEL                ( 26 )
#define DQ_RSSI_THRESHOLD               ( -85 )
#define DQ_UNSYNC_ERRORS                ( 8 )

/*================================ typedef ==================================*/

typedef uint8_t dq_arp_count_t;

typedef uint16_t dq_arp_random_t;
typedef uint16_t dq_crq_length_t;
typedef uint16_t dq_dtq_length_t;

typedef enum {
    DQ_ARP_EMPTY     = 0x00,
    DQ_ARP_COLLISION = 0x01,
    DQ_ARP_SUCCESS   = 0x02,
} dq_arp_state_t;

typedef enum {
    DQ_ARP_SLOT_0    = 0x00,
    DQ_ARP_SLOT_1    = 0x01,
    DQ_ARP_SLOT_2    = 0x02,
    DQ_ARP_SLOT_SIZE = 0x03
} dq_arp_slot_t;

typedef enum {
    DQ_ARP_RSSI_NONE  = 0x00,
    DQ_ARP_RSSI_BELOW = 0x01,
    DQ_ARP_RSSI_ABOVE = 0x02
} dq_arp_rssi_t;

typedef enum {
    DQ_DATA_EMPTY   = 0x00,
    DQ_DATA_ERROR   = 0x01,
    DQ_DATA_SUCCESS = 0x02
} dq_data_state_t;

typedef enum {
    DQ_NONE = 0x00,
    DQ_WOR  = 0x01,
    DQ_FBP  = 0x02,
    DQ_ARP  = 0x03,
    DQ_DATA = 0x04
} dq_packet_type_t;

/**
 * Packet structure for DQ operation
 */
typedef struct {
    dq_packet_type_t packet_type;   ///<

    mac_address_t mac_address;      ///< Local address of the node
    mac_seq_number_t seq_number;    ///< Sequence number of the packet
    mac_channel_t next_channel;     ///< The next channel

    uint8_t arp_count;              ///<
    uint8_t arp_selected;           ///<
    uint8_t arp_transmitted;        ///<

    int8_t arp_rssi;                ///<
    int8_t arp_rssi_threshold;      ///<
    dq_arp_random_t arp_random;     ///<

    uint8_t arp_total;              ///<
    uint8_t crq_wait;               ///<
    uint8_t dtq_wait;               ///<

    uint8_t unsync_error;           ///< The number of unsynchronization errors

    dq_crq_length_t crq_local;      ///< The local value of the CRQ
    dq_crq_length_t pcrq_local;     ///< The local pointer to the CRQ
    dq_dtq_length_t dtq_local;      ///< The local value of the DTQ
    dq_dtq_length_t pdtq_local;     ///< The local pointer to the DTQ

    dq_arp_state_t arp1_state;      ///< The state of ARP #1
    dq_arp_random_t arp1_random;    ///< The value of the ARP #1
    dq_arp_rssi_t arp1_rssi;        ///< The RSSI of ARP #1
    dq_arp_state_t arp2_state;      ///< The state of ARP #2
    dq_arp_random_t arp2_random;    ///< The value of the ARP #2
    dq_arp_rssi_t arp2_rssi;        ///< The value of the ARP #2
    dq_arp_state_t arp3_state;      ///< The state of ARP #3
    dq_arp_random_t arp3_random;    ///< The value of the ARP #3
    dq_arp_rssi_t arp3_rssi;        ///< The value of the ARP #3

    dq_data_state_t data_state;     ///<
    mac_address_t data_address;     ///<

    dq_crq_length_t crq_global;     ///< The global value of the CRQ
    dq_dtq_length_t dtq_global;     ///< The global value of the DTQ
} dq_vars_t;

/**
 * Packet structure to allow DQ debugging over serial
 */
typedef struct __attribute__((__packed__)) {
    mac_type_t mac_type;            ///<

    dq_arp_state_t arp1_state;      ///< The state of ARP1
    dq_arp_state_t arp2_state;      ///< The state of ARP2
    dq_arp_state_t arp3_state;      ///< The state of ARP3
    dq_data_state_t data_state;     ///< The state of DATA packet

    int8_t arp1_rssi;               ///< The RSSI of ARP1
    int8_t arp2_rssi;               ///< The RSSI of ARP2
    int8_t arp3_rssi;               ///< The RSSI of ARP3

    uint8_t arp_total;              ///< The number of transmitted ARPs
    uint8_t crq_wait;               ///< The number of slots in the CRQ queue
    uint8_t dtq_wait;               ///< The number of slots in the DTQ queue

    dq_arp_random_t arp1_random;    ///< The address of the node in ARP1
    dq_arp_random_t arp2_random;    ///< The address of the node in ARP2
    dq_arp_random_t arp3_random;    ///< The address of the node in ARP3
    mac_address_t data_address;     ///< The address of the node in DATA packet

    dq_crq_length_t crq_local;      ///< The local value of the CRQ
    dq_crq_length_t crq_global;     ///< The global value of the CRQ
    dq_crq_length_t pcrq_local;     ///< The pointer to the position in the CRQ

    dq_dtq_length_t dtq_local;      ///< The local value of the DTQ
    dq_dtq_length_t dtq_global;     ///< The global value of the DTQ
    dq_dtq_length_t pdtq_local;     ///< The pointer to the position in the DTQ
} dq_debug_serial_t;

/**
 * Packet structure for ARP (Access Request Packet) packets
 * Length = 1 size + 4 payload + 2 crc = 7 bytes
 * Time   = 7 bytes @ 250 kbps = 0,224 ms = 7,34 ticks @ 32.768 kHz -> 16 ticks
 */
typedef struct __attribute__((__packed__)) {
    uint8_t  mac_type;              ///< (1 byte)
    uint8_t  packet_type;           ///< (1 byte)
    uint16_t random_number;         ///< (2 byte)
} dq_arp_t;

/**
 * Packet structure for DATA packets
 * Length = 1 size + 125 payload + 2 crc = 128 bytes
 * Time   = 128 bytes @ 250 kbps = 4,096 ms = 134,25 ticks @ 32.768 kHz -> 152 ticks
 */
typedef struct __attribute__((__packed__)) {
    uint8_t  mac_type;              ///< (1 byte)
    uint8_t  packet_type;           ///< (1 byte)
    uint16_t source;                ///< (2 byte)
    uint16_t destination;           ///< (2 byte)
    uint8_t  arp_total;             ///< (1 byte)
    uint8_t  crq_wait;              ///< (1 byte)
    uint8_t  dtq_wait;              ///< (1 byte)
    uint8_t  data[116];             ///< (116 byte)
} dq_data_t;

/**
 * Packet structure for FBP (FeedBack Packet) packets
 * Length = 1 size + 24 payload + 2 crc = 27 bytes
 * Time   = 27 bytes @ 250 kbps = 0,864 ms = 28,31 ticks @ 32.768 kHz -> 32 ticks
 */
typedef struct __attribute__((__packed__)) {
    uint8_t  mac_type;              ///< (1 byte)
    uint8_t  packet_type;           ///< (1 byte)
    uint16_t source;                ///< (2 byte)
    uint16_t destination;           ///< (2 byte)
    uint16_t seq_number;            ///< (2 byte)
    uint8_t  arp1_state;            ///< (1 byte)
    uint16_t arp1_random;           ///< (2 byte)
    uint8_t  arp2_state;            ///< (1 byte)
    uint16_t arp2_random;           ///< (2 byte)
    uint8_t  arp3_state;            ///< (1 byte)
    uint16_t arp3_random;           ///< (2 byte)
    uint8_t  data_state;            ///< (1 byte)
    uint16_t crq_global;            ///< (2 byte)
    uint16_t dtq_global;            ///< (2 byte)
    uint8_t  arp_count;             ///< (1 byte)
    uint8_t  next_channel;          ///< (1 byte)
} dq_fbp_t;

/*=============================== variables =================================*/

dq_vars_t dq_vars;
dq_debug_serial_t dq_debug_serial;

/*=============================== prototypes ================================*/

static void dq_fbp_init(void);
static void dq_fbp_done(void);
static void dq_arp_init(void);
static void dq_arp_done(void);
static void dq_data_init(void);
static void dq_data_done(void);

#if (MAC_DEVICE == MAC_GATEWAY)
static void dq_arp_rx_init(void);
static void dq_arp_rx_rssi(void);
static void dq_arp_rx_done(void);
static void dq_data_rx_init(void);
static void dq_data_rx_done(void);
static void dq_fbp_tx_init(void);
static void dq_fbp_tx_done(void);

static void dq_vars_reset(void);
static void dq_vars_log(void);
#elif (MAC_DEVICE == MAC_NODE)
static void dq_fbp_rx_init(void);
static void dq_fbp_rx_done(void);
static void dq_arp_tx_init(void);
static void dq_arp_tx_done(void);
static void dq_data_tx_init(void);
static void dq_data_tx_done(void);

static void dq_arp_vars_set(void);
static void dq_arp_vars_reset(void);
static void dq_data_vars_reset(void);

static void dq_vars_update(dq_fbp_t* dq_fbp);

static bool dq_rtr_check(void);
static bool dq_dtr_check(void);
static bool dq_qdr_check(void);
static void dq_qdr_update(void);
#endif

static void dq_qdr_rules(void);

/*================================= public ==================================*/

/**
 * @brief Function to initialize DQ operation
 */
void dq_init(void) {
    // Initialize the memory of the variables
    memset(&dq_vars, 0, sizeof(dq_vars_t));
    memset(&dq_debug_serial, 0, sizeof(dq_debug_serial_t));

    // Set the device address
    ieee_addr_get_eui16((uint16_t*) &dq_vars.mac_address);
}

/**
 * @brief Funtion to start DQ operation
 */
void dq_start(void) {
    // Schedule the task to start the MAC
    scheduler_push(dq_fbp_init, TASK_PRIO_MAX);
}

/*================================ private ==================================*/

// If the device type is GATEWAY
#if (MAC_DEVICE == MAC_GATEWAY)

static void dq_fbp_init(void) {
    dq_fbp_t* dq_fbp = NULL;

    debug_system_on();
    debug_user_on();

    // Obtain a queue entry
    mac_vars.queue_mac_tx = packet_buffer_get();
    dq_fbp = (dq_fbp_t *) mac_vars.queue_mac_tx->payload;
    mac_vars.queue_mac_tx->length = sizeof(dq_fbp_t);

    // Prepare the FBP
    dq_fbp->packet_type = DQ_FBP;
    dq_fbp->source = dq_vars.mac_address;
    dq_fbp->destination = MAC_ADDR_BCAST;
    dq_fbp->seq_number = dq_vars.seq_number;
    dq_fbp->arp1_state = dq_vars.arp1_state;
    dq_fbp->arp1_random = dq_vars.arp1_random;
    dq_fbp->arp2_state = dq_vars.arp2_state;
    dq_fbp->arp2_random = dq_vars.arp2_random;
    dq_fbp->arp3_state = dq_vars.arp3_state;
    dq_fbp->arp3_random = dq_vars.arp3_random;
    dq_fbp->data_state = dq_vars.data_state;
    dq_fbp->crq_global = dq_vars.crq_global;
    dq_fbp->dtq_global = dq_vars.dtq_global;
    dq_fbp->arp_count = dq_vars.arp_count;
    dq_fbp->next_channel = dq_vars.next_channel;

    // Set the radio transmit callback
    radio_set_tx_cb(dq_fbp_tx_init, dq_fbp_tx_done);

    // Put the FBP in the radio and transmit it
    radio_put_packet(mac_vars.queue_mac_tx);
    radio_transmit();

    // Wait for the duration of a FBP
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, DQ_FBP_DURATION, dq_fbp_done, TASK_PRIO_MAX);

    debug_user_off();
}

static void dq_fbp_tx_init(void) {
    debug_radio_on();
}

static void dq_fbp_tx_done(void) {
    debug_radio_off();
}

static void dq_fbp_done(void) {
    virtual_timer_width_t ticks;

    debug_user_on();

    // Put the radio back to IDLE just in case
    radio_idle();
    radio_cancel_tx_cb();

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_tx);
    mac_vars.queue_mac_tx = NULL;

    // Register the message and schedule a task to push it
    serial_push_msg(SERIAL_MOTE2PC_DATA, (uint8_t *)&dq_debug_serial, sizeof(dq_debug_serial_t));

    // Reset the ALP variables
    dq_vars_reset();

    // Wait SIFS to start the ARP
    ticks = DQ_SIFS_DURATION - MAC_RADIO_IDLE_RX - DQ_FBP_PREPARE - DQ_FBP_PROCESS;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_arp_init, TASK_PRIO_MAX);

    debug_user_off();
    debug_system_off();
}

static void dq_arp_init(void) {
    virtual_timer_width_t ticks;

    debug_system_on();
    debug_user_on();

    // Set the radio receive callbacks
    radio_set_rx_cb(dq_arp_rx_init, dq_arp_rx_done);

    // Put the radio to receive
    radio_receive();

    // Wait for the duration of an ARP
    ticks = (DQ_ARP_DURATION >> 1);
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_arp_rx_rssi, TASK_PRIO_MAX);

    debug_user_off();
}

static void dq_arp_rx_init(void) {
    debug_radio_on();
}

static void dq_arp_rx_rssi(void) {
    virtual_timer_width_t ticks;

    debug_user_on();

    // Read and convert the RSSI
    radio_read_rssi(&dq_vars.arp_rssi);

    // Wait for the duration of an ARP
    ticks = (DQ_ARP_DURATION >> 1) - 1;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_arp_done, TASK_PRIO_MAX);

    debug_user_off();
}

static void dq_arp_rx_done(void) {
    // Get the packet from the radio
    mac_vars.queue_mac_rx = packet_buffer_get();
    radio_get_packet(mac_vars.queue_mac_rx);

    debug_radio_off();
}

static void dq_arp_done(void) {
    dq_arp_t* dq_arp = NULL;
    dq_arp_state_t* current_arp_state = NULL;
    dq_arp_rssi_t* current_arp_rssi = NULL;
    uint16_t* current_arp_random = NULL;
    virtual_timer_width_t ticks;
    uint8_t current_arp = 0;

    debug_user_on();

    // Put the radio back to IDLE just in case
    radio_idle();
    radio_cancel_rx_cb();

    // Know which ARP we are currently processing and point to it
    current_arp = DQ_ARP_COUNT - dq_vars.arp_count;
    switch(current_arp) {
        case 0:
            current_arp_state = &dq_vars.arp1_state;
            current_arp_rssi = &dq_vars.arp1_rssi;
            current_arp_random = &dq_vars.arp1_random;
            break;
        case 1:
            current_arp_state = &dq_vars.arp2_state;
            current_arp_rssi = &dq_vars.arp2_rssi;
            current_arp_random = &dq_vars.arp2_random;
            break;
        case 2:
            current_arp_state = &dq_vars.arp3_state;
            current_arp_rssi = &dq_vars.arp3_rssi;
            current_arp_random = &dq_vars.arp3_random;
            break;
        default:
            break;
    }

    // Check if the RSSI is above the threshold
    if (dq_vars.arp_rssi > dq_vars.arp_rssi_threshold) {
        *current_arp_rssi = DQ_ARP_RSSI_ABOVE;
    } else {
        *current_arp_rssi = DQ_ARP_RSSI_BELOW;
    }

    // Check if the received packet is correct
    if (mac_vars.queue_mac_rx->crc) {
        // Convert the packet to a ARP packet
        dq_arp = (dq_arp_t *) mac_vars.queue_mac_rx->payload;
        if (dq_arp->packet_type == DQ_ARP) {
            *current_arp_state = DQ_ARP_SUCCESS;
            *current_arp_random = dq_arp->random_number;
        } else {
            *current_arp_state = DQ_ARP_COLLISION;
            *current_arp_random = 0;
        }
    } else {
        if (*current_arp_rssi == DQ_ARP_RSSI_ABOVE) {
            *current_arp_state = DQ_ARP_COLLISION;
            *current_arp_random = 0;
        } else {
            *current_arp_state = DQ_ARP_EMPTY;
            *current_arp_random = 0;
        }
    }

    // Store the RSSI to send it through UART
    *current_arp_rssi = dq_vars.arp_rssi;

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_rx);
    mac_vars.queue_mac_rx = NULL;

    // Update the ARP counters
    dq_vars.arp_count--;

    // Schedule the next action, ARP or DATA
    if (dq_vars.arp_count == 0) {
        ticks = DQ_SIFS_DURATION - MAC_RADIO_IDLE_TX - DQ_ARP_PREPARE - DQ_ARP_PROCESS;
        virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_data_init, TASK_PRIO_MAX);
    } else {
        ticks = DQ_SIFS_DURATION - MAC_RADIO_IDLE_RX - 2*DQ_ARP_PREPARE - DQ_ARP_PROCESS;
        virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_arp_init, TASK_PRIO_MAX);
    }

    debug_user_off();
    debug_system_off();
}

static void dq_data_init(void) {
    virtual_timer_width_t ticks;

    debug_system_on();
    debug_user_on();

    // Set the radio receive callbacks
    radio_set_rx_cb(dq_data_rx_init, dq_data_rx_done);

    // Put the radio to receive
    radio_receive();

    // Wait for the duration of a DATA packet
    ticks = DQ_DATA_DURATION;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_data_done, TASK_PRIO_MAX);

    debug_user_off();
}

static void dq_data_rx_init(void) {
    debug_radio_on();
}

static void dq_data_rx_done(void) {
    dq_data_t* dq_data = NULL;

    // Get the packet from the radio
    mac_vars.queue_mac_rx = packet_buffer_get();
    radio_get_packet(mac_vars.queue_mac_rx);

    // Check if the received packet is correct
    if (mac_vars.queue_mac_rx->crc) {
        // Convert the packet to a data packet
        dq_data = (dq_data_t *) mac_vars.queue_mac_rx->payload;

        if (dq_data->packet_type == DQ_DATA) {
            dq_vars.data_state   = DQ_DATA_SUCCESS;
            dq_vars.data_address = dq_data->source;
            dq_vars.arp_total    = dq_data->arp_total;
            dq_vars.crq_wait     = dq_data->crq_wait;
            dq_vars.dtq_wait     = dq_data->dtq_wait;
        }
    } else {
        dq_vars.data_state   = DQ_DATA_ERROR;
        dq_vars.data_address = 0x00;
    }

    debug_radio_off();
}

static void dq_data_done(void) {
    bsp_timer_width_t ticks;

    debug_user_on();

    // Put the radio back to IDLE just in case
    radio_idle();
    radio_cancel_rx_cb();

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_rx);
    mac_vars.queue_mac_rx = NULL;

    // Apply the QDR rules to the local CRQ and DTQ counters
    dq_qdr_rules();

    // Update the CRQ and DTQ global counters based on the QDR rules
    dq_vars.crq_global = dq_vars.crq_local;
    dq_vars.dtq_global = dq_vars.dtq_local;

    // Move to the next channel previous to change it
    // radio_set_channel(dq_vars.next_channel);

    // Update the sequence number and next channel
    dq_vars.next_channel = mac_next_channel();
    dq_vars.arp_count    = DQ_ARP_COUNT;
    dq_vars.seq_number  += 1;

    // Update the debug variables
    dq_vars_log();

    // Wait LIFS to start FBP
    ticks = DQ_LIFS_DURATION - MAC_RADIO_IDLE_TX - DQ_DATA_PREPARE - DQ_DATA_PROCESS;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_fbp_init, TASK_PRIO_MAX);

    debug_user_off();
    debug_system_off();
}

static void dq_vars_reset(void) {
    dq_vars.arp_count = DQ_ARP_COUNT;

    dq_vars.arp1_state  = DQ_ARP_EMPTY;
    dq_vars.arp1_rssi   = DQ_ARP_RSSI_NONE;
    dq_vars.arp1_random = 0;

    dq_vars.arp2_state  = DQ_ARP_EMPTY;
    dq_vars.arp2_rssi   = DQ_ARP_RSSI_NONE;
    dq_vars.arp2_random = 0;

    dq_vars.arp3_state  = DQ_ARP_EMPTY;
    dq_vars.arp3_rssi   = DQ_ARP_RSSI_NONE;
    dq_vars.arp3_random = 0;

    dq_vars.arp_rssi_threshold = DQ_RSSI_THRESHOLD;

    dq_vars.data_state   = DQ_DATA_EMPTY;
    dq_vars.data_address = 0;

    dq_vars.arp_total = 0;
    dq_vars.dtq_wait  = 0;
    dq_vars.crq_wait  = 0;
}

static void dq_vars_log(void) {
    dq_debug_serial.mac_type = MAC_TYPE_DQ;

    dq_debug_serial.arp1_state  = dq_vars.arp1_state;
    dq_debug_serial.arp1_random = dq_vars.arp1_random;
    dq_debug_serial.arp1_rssi   = dq_vars.arp1_rssi;
    dq_debug_serial.arp2_state  = dq_vars.arp2_state;
    dq_debug_serial.arp2_random = dq_vars.arp2_random;
    dq_debug_serial.arp2_rssi   = dq_vars.arp2_rssi;
    dq_debug_serial.arp3_state  = dq_vars.arp3_state;
    dq_debug_serial.arp3_random = dq_vars.arp3_random;
    dq_debug_serial.arp3_rssi   = dq_vars.arp3_rssi;

    dq_debug_serial.data_state   = dq_vars.data_state;
    dq_debug_serial.data_address = dq_vars.data_address;

    dq_debug_serial.arp_total = dq_vars.arp_total;
    dq_debug_serial.crq_wait  = dq_vars.crq_wait;
    dq_debug_serial.dtq_wait  = dq_vars.dtq_wait;

    dq_debug_serial.crq_local  = dq_vars.crq_local;
    dq_debug_serial.crq_global = dq_vars.crq_global;
    dq_debug_serial.pcrq_local = dq_vars.pcrq_local;

    dq_debug_serial.dtq_global = dq_vars.dtq_global;
    dq_debug_serial.dtq_local  = dq_vars.dtq_global;
    dq_debug_serial.pdtq_local = dq_vars.pdtq_local;
}
#endif /* MAC_DEVICE == MAC_GATEWAY */

// If the device type is END NODE
#if (MAC_DEVICE == MAC_NODE)

virtual_timer_id_t virtual_timer_id;

static void dq_fbp_init(void) {
    virtual_timer_width_t ticks;

    debug_system_on();
    debug_user_on();

    // Restore the variable values
    dq_vars.packet_type = DQ_NONE;

    // Register the radio callbacks
    radio_set_rx_cb(dq_fbp_rx_init, dq_fbp_rx_done);

    // Register and start the radio timer callback
    ticks = DQ_FBP_DURATION << 4;
    virtual_timer_id = virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_fbp_done, TASK_PRIO_MAX);

    // Put the radio to receive
    radio_receive();

    debug_user_off();
}

static void dq_fbp_rx_init(void) {
    virtual_timer_width_t ticks;

    debug_radio_on();

    // Start the radio timer callback
    ticks = DQ_FBP_DURATION - 2 * MAC_RADIO_PHY_HEADER - MAC_RADIO_IDLE_RX,
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_fbp_done, TASK_PRIO_MAX);

    // Stop the old virtual timer
    virtual_timer_stop(virtual_timer_id);
}

static void dq_fbp_rx_done(void) {
    dq_fbp_t* dq_fbp = NULL;

    // Get the packet from the radio
    mac_vars.queue_mac_rx = packet_buffer_get();
    radio_get_packet(mac_vars.queue_mac_rx);

    // Check if the received packet is correct
    if (mac_vars.queue_mac_rx->crc) {
        // Convert the packet to a FBP
        dq_fbp = (dq_fbp_t *) mac_vars.queue_mac_rx->payload;

        // If we really got a FBP
        if (dq_fbp->packet_type == DQ_FBP) {
            // Update the local ALP variables
            dq_vars_update(dq_fbp);
        }
    }

    debug_radio_off();
}

static void dq_fbp_done(void) {
    virtual_timer_width_t ticks;

    debug_user_on();

    // Put the radio back to IDLE just in case
    radio_idle();
    radio_cancel_rx_cb();

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_rx);
    mac_vars.queue_mac_rx = NULL;

    // If we are unsynchronized get the DTQ and CRQ values
    if (mac_vars.mac_state == MAC_STATE_UNSYNC) {
        // Reset the ARP and DATA-related variables
        dq_arp_vars_reset();
        dq_data_vars_reset();
    }

    // If we received a FBP
    if (dq_vars.packet_type == DQ_FBP) {
        // We synchronize upon receiving a FBP
        mac_toggle_synchronized(MAC_STATE_SYNC);

        // We reset the unsynchronized errors variable
        dq_vars.unsync_error = DQ_UNSYNC_ERRORS;

        // Apply the QDR rules
        dq_qdr_rules();

        // Check if the DTQ and CRQ values are NOT consistent
        if (!dq_qdr_check()) {
            // Notify we have lost synchronization
            mac_toggle_synchronized(MAC_STATE_UNSYNC);

            // Cancels the radio callbacks
            radio_cancel_rx_cb();
            radio_cancel_tx_cb();

            // Register and start the radio timer callback
            ticks = DQ_SLOT_DURATION - DQ_FBP_DURATION - 2 * MAC_RADIO_IDLE_RX - DQ_FBP_PREPARE - DQ_FBP_PROCESS;
            virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_fbp_init, TASK_PRIO_MAX);
        } else {
            // Update the DTQ and CRQ
            dq_qdr_update();

            // Update the CRQ and DTQ wait states
            if (dq_vars.crq_local != 0) {
                dq_vars.crq_wait++;
            } else if (dq_vars.dtq_local != 0) {
                dq_vars.dtq_wait++;
            }

            // Check if we are allowed to transmit an ARP and we have to
            if (dq_rtr_check()) {
                // Set the number of ARP and select one at random
                dq_arp_vars_set();

                // Register and start the radio timer callback
                if (dq_vars.arp_count == dq_vars.arp_selected) {
                    ticks = DQ_SIFS_DURATION - MAC_RADIO_IDLE_TX - DQ_FBP_PREPARE - DQ_FBP_PROCESS;
                    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_arp_init, TASK_PRIO_MAX);
                } else {
                    ticks = DQ_SIFS_DURATION - DQ_FBP_PREPARE - DQ_FBP_PROCESS;
                    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_arp_init, TASK_PRIO_MAX);
                }
            } else if (dq_dtr_check()) { // Otherwise check if we are allowed to transmit a DATA
                // Reset the ARP-related variables
                dq_arp_vars_reset();

                // Register and start the radio timer callback
                ticks = 4 * DQ_SIFS_DURATION + 3 * DQ_ARP_DURATION - MAC_RADIO_IDLE_TX - DQ_FBP_PREPARE - DQ_FBP_PROCESS;
                virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_data_init, TASK_PRIO_MAX);
            } else { // Otherwise we jump to the next FBP
                // Reset the ARP-related variables
                dq_arp_vars_reset();

                // Register and start the radio timer callback
                ticks = DQ_SLOT_DURATION - DQ_FBP_DURATION - 2 * MAC_RADIO_IDLE_RX - DQ_FBP_PREPARE - DQ_FBP_PROCESS;
                virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_fbp_init, TASK_PRIO_MAX);
            }
        }
    } else { // If the packet is not a FBP
        // Notify we have lost synchronization
        mac_toggle_synchronized(MAC_STATE_UNSYNC);

        // Cancel the radio callbacks
        radio_cancel_rx_cb();
        radio_cancel_tx_cb();

        // Decrement the unsynchronized errors variable
        dq_vars.unsync_error--;

        // Register and start the appropriate radio timer callback
        if (dq_vars.unsync_error == 0) { // Lost synchronization
            radio_reset();
            cpu_reset();
        } else { // Maintain synchronization
            // ticks = DQ_SLOT_DURATION - DQ_FBP_DURATION - 2 * MAC_RADIO_IDLE_RX;
            // virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_fbp_init, TASK_PRIO_MAX);
            ticks = VIRTUAL_TIMER_KICK_NOW;
            virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_fbp_init, TASK_PRIO_MAX);
        }
    }

    debug_user_off();
    debug_system_off();
}

static void dq_arp_init(void) {
    dq_arp_t* dq_arp = NULL;
    virtual_timer_width_t ticks;

    debug_system_on();
    debug_user_on();

    // If this is the ARP slot we selected
    if (dq_vars.arp_count == dq_vars.arp_selected) {
        // Obtain a queue entry
        mac_vars.queue_mac_tx = packet_buffer_get();
        dq_arp = (dq_arp_t *) mac_vars.queue_mac_tx->payload;
        mac_vars.queue_mac_tx->length = sizeof(dq_arp_t);

        // Configure the ARP
        dq_arp->packet_type = DQ_ARP;
        dq_arp->random_number = dq_vars.arp_random;

        // Set the radio callbacks
        radio_set_tx_cb(dq_arp_tx_init, dq_arp_tx_done);

        // Account for the transmitted ARP
        dq_vars.arp_total += 1;

        // Put the ARP in the radio and transmit it
        radio_put_packet(mac_vars.queue_mac_tx);
        radio_transmit();
    }

    // Register and start the radio timer callback
    ticks = DQ_ARP_DURATION;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_arp_done, TASK_PRIO_MAX);

    debug_user_off();
}

static void dq_arp_tx_init(void) {
    debug_radio_on();
}

static void dq_arp_tx_done(void) {
    debug_radio_off();
}

static void dq_arp_done(void) {
    virtual_timer_width_t ticks;

    debug_user_on();

    // Put the radio back to IDLE just in case
    radio_idle();
    radio_cancel_tx_cb();

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_tx);
    mac_vars.queue_mac_tx = NULL;

    // Count the elapsed ARP
    dq_vars.arp_count++;

    // Register and start the radio timer callback
    if (dq_vars.arp_count == DQ_ARP_COUNT) { // This is the last ARP
        ticks = DQ_SIFS_DURATION + DQ_DATA_DURATION + DQ_LIFS_DURATION - 2 * MAC_RADIO_IDLE_RX - DQ_ARP_PREPARE - DQ_ARP_PROCESS;
        virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_fbp_init, TASK_PRIO_MAX);
    } else if (dq_vars.arp_count == dq_vars.arp_selected) { // This is the selected ARP
        ticks = DQ_SIFS_DURATION - MAC_RADIO_IDLE_TX - DQ_ARP_PREPARE - DQ_ARP_PROCESS;
        virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_arp_init, TASK_PRIO_MAX);
    } else { // This is NOT the selected ARP
        ticks = DQ_SIFS_DURATION - DQ_ARP_PREPARE - DQ_ARP_PROCESS;
        virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_arp_init, TASK_PRIO_MAX);
    }

    debug_system_off();
    debug_user_off();
}

static void dq_data_init(void) {
    dq_data_t* dq_data = NULL;
    virtual_timer_width_t ticks;

    debug_system_on();
    debug_user_on();

    // Obtain a queue entry
    mac_vars.queue_mac_tx = packet_buffer_get();
    dq_data = (dq_data_t *) mac_vars.queue_mac_tx->payload;
    mac_vars.queue_mac_tx->length = sizeof(dq_data_t);

    // Configure the DATA
    dq_data->mac_type = MAC_TYPE_DQ;
    dq_data->packet_type = DQ_DATA;
    dq_data->destination = MAC_ADDR_BCAST;
    dq_data->source = dq_vars.mac_address;
    dq_data->arp_total = dq_vars.arp_total;
    dq_data->crq_wait = dq_vars.crq_wait;
    dq_data->dtq_wait = dq_vars.dtq_wait;

    // Fill in the DATA packet
    uint16_t random = random_get();
    for (uint8_t i = 0; i < sizeof(dq_data->data); i++) {
        dq_data->data[i] = random >> 0;
    }

    // Reset the ARP, DTQ and CRQ counters
    dq_vars.arp_total = 0;
    dq_vars.dtq_wait  = 0;
    dq_vars.crq_wait  = 0;

    // Register the radio callback
    radio_set_tx_cb(dq_data_tx_init, dq_data_tx_done);

    // Put the DATA in the radio and transmit it
    radio_put_packet(mac_vars.queue_mac_tx);
    radio_transmit();

    // Register and start the radio timer callback
    ticks = DQ_DATA_DURATION;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_data_done, TASK_PRIO_MAX);

    debug_user_off();
}

static void dq_data_tx_init(void) {
    debug_radio_on();
}

static void dq_data_tx_done(void) {
    debug_radio_off();
}

static void dq_data_done(void) {
    virtual_timer_width_t ticks;

    debug_user_on();

    // Put the radio back to IDLE just in case
    radio_idle();
    radio_cancel_tx_cb();

    // Free the queue entry
    packet_buffer_release(mac_vars.queue_mac_tx);
    mac_vars.queue_mac_tx = NULL;

    // Move to the next channel
    // radio_set_channel(dq_vars.next_channel);

    // For single packet experiments, reset the board
    // radio_reset();
    // board_reset();

    // Register and start the radio timer callback
    ticks = DQ_LIFS_DURATION - 2 * MAC_RADIO_IDLE_RX - DQ_DATA_PREPARE - DQ_DATA_PROCESS;
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, ticks, dq_fbp_init, TASK_PRIO_MAX);

    debug_user_off();
    debug_system_off();
}

static void dq_arp_vars_set(void) {
    dq_vars.arp_selected    = random_get() % DQ_ARP_SLOT_SIZE;
    dq_vars.arp_transmitted = true;
    dq_vars.arp_count       = 0;
    // dq_vars.arp_random   = random_get();
    dq_vars.arp_random      = dq_vars.mac_address;
}

static void dq_arp_vars_reset(void) {
    dq_vars.arp_selected    = 0;
    dq_vars.arp_transmitted = false;
    dq_vars.arp_count       = 0;
    dq_vars.arp_random      = 0;
}

static void dq_data_vars_reset(void) {
    dq_vars.crq_local  = dq_vars.crq_global;
    dq_vars.dtq_local  = dq_vars.dtq_global;
    dq_vars.pcrq_local = 0;
    dq_vars.pdtq_local = 0;
}

static void dq_vars_update(dq_fbp_t* dq_fbp) {
    dq_vars.packet_type  = dq_fbp->packet_type;
    dq_vars.next_channel = dq_fbp->next_channel;
    dq_vars.seq_number   = dq_fbp->seq_number;
    dq_vars.arp_count    = dq_fbp->arp_count;

    dq_vars.arp1_state  = dq_fbp->arp1_state;
    dq_vars.arp1_random = dq_fbp->arp1_random;
    dq_vars.arp2_state  = dq_fbp->arp2_state;
    dq_vars.arp2_random = dq_fbp->arp2_random;
    dq_vars.arp3_state  = dq_fbp->arp3_state;
    dq_vars.arp3_random = dq_fbp->arp3_random;

    dq_vars.data_state = dq_fbp->data_state;

    dq_vars.crq_global = dq_fbp->crq_global;
    dq_vars.dtq_global = dq_fbp->dtq_global;
}

static bool dq_dtr_check(void) {
    // If the node is at the head of the DTQ it can transmit in DATA
    if (dq_vars.pdtq_local == 1) {
        return true;
    } else { // Otherwise the node is not allowed to transmit in the DATA
        return false;
    }
}

static bool dq_rtr_check(void) {
    // If no collisions are pending and node does not occupy any position in the CRQ or DTQ
    if (dq_vars.crq_local == 0 && dq_vars.pcrq_local == 0 && dq_vars.pdtq_local == 0) {
        return true;
    } else if (dq_vars.pcrq_local == 1) { // If the node is at the head of the CRQ
        return true;
    } else { // Otherwise the node is not allowed to transmit in the ARP
        return false;
    }
}

static bool dq_qdr_check(void) {
    // Perform a sanity check of the CRQ and DTQ variables
    if (dq_vars.crq_local != dq_vars.crq_global ||
        dq_vars.dtq_local != dq_vars.dtq_global) {
        return false;
    }
    return true;
}

static void dq_qdr_update(void) {
    dq_arp_state_t* current_arp_state = NULL;
    uint16_t* current_arp_random = NULL;
    uint8_t total_success = 0;
    uint8_t relative_success = 0;
    uint8_t total_collision = 0;
    uint8_t relative_collision = 0;

    // Update the pDTQ if DATA was successful or empty
    if ((dq_vars.pdtq_local > 0) &&
        (dq_vars.data_state != DQ_DATA_ERROR)) {
        dq_vars.pdtq_local -= 1;
    }

    // Update the pCRQ to account for the collision resolution attempt
    if (dq_vars.pcrq_local > 0) {
        dq_vars.pcrq_local -= 1;
    }

    // Know which ARP we are currently processing and point to it
    switch (dq_vars.arp_selected) {
        case DQ_ARP_SLOT_0:
            current_arp_state = &dq_vars.arp1_state;
            current_arp_random = &dq_vars.arp1_random;
            relative_success = 1;
            relative_collision = 1;
            break;
        case DQ_ARP_SLOT_1:
            current_arp_state = &dq_vars.arp2_state;
            current_arp_random = &dq_vars.arp2_random;
            if (dq_vars.arp1_state == DQ_ARP_SUCCESS) {
                relative_success = 2;
            } else {
                relative_success = 1;
            }
            if (dq_vars.arp1_state == DQ_ARP_COLLISION) {
                relative_collision = 2;
            } else {
                relative_collision = 1;
            }
            break;
        case DQ_ARP_SLOT_2:
            current_arp_state = &dq_vars.arp3_state;
            current_arp_random = &dq_vars.arp3_random;
            if (dq_vars.arp1_state == DQ_ARP_SUCCESS && dq_vars.arp2_state == DQ_ARP_SUCCESS) {
                relative_success = 3;
            } else if (dq_vars.arp1_state == DQ_ARP_SUCCESS || dq_vars.arp2_state == DQ_ARP_SUCCESS) {
                relative_success = 2;
            } else {
                relative_success = 1;
            }
            if (dq_vars.arp1_state == DQ_ARP_COLLISION && dq_vars.arp2_state == DQ_ARP_COLLISION) {
                relative_collision = 3;
            } else if (dq_vars.arp1_state == DQ_ARP_COLLISION || dq_vars.arp2_state == DQ_ARP_COLLISION) {
                relative_collision = 2;
            } else {
                relative_collision = 1;
            }
            break;
        default:
            break;
    }

    // Increase DTQ and CRQ by one for each success/collision ARP
    total_success = 0;
    total_collision = 0;

    // Check ARP1 state
    if (dq_vars.arp1_state == DQ_ARP_SUCCESS) {
        total_success += 1;
    } else if (dq_vars.arp1_state == DQ_ARP_COLLISION) {
        total_collision += 1;
    }

    // Check ARP2 state
    if (dq_vars.arp2_state == DQ_ARP_SUCCESS) {
        total_success += 1;
    } else if (dq_vars.arp2_state == DQ_ARP_COLLISION) {
        total_collision += 1;
    }

    // Check ARP3 state
    if (dq_vars.arp3_state == DQ_ARP_SUCCESS) {
        total_success += 1;
    } else if (dq_vars.arp3_state == DQ_ARP_COLLISION) {
        total_collision += 1;
    }

    // Update the pDTQ and pCRQ according to the FBP status
    if (dq_vars.arp_transmitted) {
        // If ARP success enter the DTQ
        if (*current_arp_state == DQ_ARP_SUCCESS &&
            dq_vars.arp_random == *current_arp_random) {
            // Calculate the position in the DTQ
            dq_vars.pdtq_local = dq_vars.dtq_local + relative_success - total_success;
        } else { // Otherwise mark the ARP as collision for further processing
            *current_arp_state = DQ_ARP_COLLISION;
        }

        // If ARP collision enter the CRQ
        if (*current_arp_state == DQ_ARP_COLLISION) {
            // Calculate the poisition in the CRQ
            dq_vars.pcrq_local = dq_vars.crq_local + relative_collision - total_collision;
        }
    }
}

#endif /* MAC_DEVICE == MAC_NODE */

static void dq_qdr_rules(void) {
    // Decrease CRQ to account for the collision resolution attempt
    if (dq_vars.crq_local > 0) {
        dq_vars.crq_local -= 1;
    }

    // Decrease DTQ by one for each success or empty DATA
    if (dq_vars.dtq_local > 0 &&
        dq_vars.data_state != DQ_DATA_ERROR) {
        dq_vars.dtq_local -= 1;
    }

    // Increase DTQ and CRQ by one for each success/collision ARP
    if (dq_vars.arp1_state == DQ_ARP_SUCCESS) {
        dq_vars.dtq_local += 1;
    } else if (dq_vars.arp1_state == DQ_ARP_COLLISION) {
        dq_vars.crq_local += 1;
    }
    if (dq_vars.arp2_state == DQ_ARP_SUCCESS) {
        dq_vars.dtq_local += 1;
    } else if (dq_vars.arp2_state == DQ_ARP_COLLISION) {
        dq_vars.crq_local += 1;
    }
    if (dq_vars.arp3_state == DQ_ARP_SUCCESS) {
        dq_vars.dtq_local += 1;
    } else if (dq_vars.arp3_state == DQ_ARP_COLLISION) {
        dq_vars.crq_local += 1;
    }
}

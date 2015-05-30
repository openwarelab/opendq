/**
 * @file       serial.c
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */
 
/*================================ include ==================================*/

#include "serial.h"

#include "board.h"
#include "debug.h"
#include "ieee-addr.h"
#include "leds.h"
#include "uart.h"

#include "hdlc.h"

#include "scheduler.h"

/*================================ define ===================================*/

#define SERIAL_TX_BUFFER_SIZE       ( 128 )
#define SERIAL_RX_BUFFER_SIZE		( 128 )

/*================================ typedef ==================================*/

typedef enum {
    SERIAL_STATUS_OFF   = 0x00,
    SERIAL_STATUS_READY = 0x01,
    SERIAL_STATUS_RX    = 0x02,
    SERIAL_STATUS_TX    = 0x03,
    SERIAL_STATUS_ERROR = 0x04
} serial_status_t;

typedef struct {
    serial_cb_t serial_cb;
    task_prio_t task_prio;
} serial_task_t;

typedef struct {
    // General
    serial_status_t status;

    // EUI-16 address
    uint16_t eui16;

    // PC2MOTE tasks
    serial_task_t serial_task_pc2mote_start;
    serial_task_t serial_task_pc2mote_stop;

    // MOTE2PC tasks
    serial_task_t serial_task_mote2pc_data;

    // Receive
    uint8_t  rx_command;
    uint8_t* rx_data_ptr;
    uint8_t  rx_data_len;

    // Transmit
    uint8_t  tx_command;
    uint8_t* tx_data_ptr;
    uint8_t  tx_data_len;

    // Receive buffer
    uint8_t  rx_buffer[HDLC_HEADER_SIZE + SERIAL_RX_BUFFER_SIZE + HDLC_FOOTER_SIZE];
    uint8_t* rx_buffer_ptr;
    uint8_t  rx_buffer_len;

    // Transmit buffer
    uint8_t  tx_buffer[HDLC_HEADER_SIZE + SERIAL_RX_BUFFER_SIZE + HDLC_FOOTER_SIZE];
    uint8_t* tx_buffer_ptr;
    uint8_t  tx_buffer_len;
} serial_vars_t;

/*=============================== variables =================================*/

static serial_vars_t serial_vars;

/*=============================== prototypes ================================*/

static void serial_tx_init(void);
static void serial_tx_byte(void);
static void serial_tx_done(void);

static void serial_rx_init(void);
static void serial_rx_byte(void);
static void serial_rx_done(void);

/*================================= public ==================================*/

void serial_init(void) {
    // Initialize the memory of the variables
    memset(&serial_vars, 0, sizeof(serial_vars_t));

    // Update the serial status
    serial_vars.status = SERIAL_STATUS_OFF;

    // Recover the EUI16 adress
    ieee_addr_get_eui16((uint16_t*) &serial_vars.eui16);

    // Register the UART interface callbacks
    uart_register_rx_cb(serial_rx_byte);
    uart_register_tx_cb(serial_tx_byte);

    // Enable the UART interrupts
    uart_enable_interrupts();

    // Update the serial status
    serial_vars.status = SERIAL_STATUS_READY;
}

void serial_register_pc2mote_cb(serial_pc2mote_t serial_pc2mote, serial_cb_t serial_cb, task_prio_t task_prio) {
    switch(serial_pc2mote) {
        case SERIAL_PC2MOTE_START:
            serial_vars.serial_task_pc2mote_start.serial_cb = serial_cb;
            serial_vars.serial_task_pc2mote_start.task_prio = task_prio;
            break;
        case SERIAL_PC2MOTE_STOP:
            serial_vars.serial_task_pc2mote_stop.serial_cb = serial_cb;
            serial_vars.serial_task_pc2mote_stop.task_prio = task_prio;
            break;
        default:
            break;
    }
}

void serial_register_mote2pc_cb(serial_mote2pc_t serial_mote2pc, serial_cb_t serial_cb, task_prio_t task_prio) {
    switch(serial_mote2pc) {
        case SERIAL_MOTE2PC_DATA:
            serial_vars.serial_task_mote2pc_data.serial_cb = serial_cb;
            serial_vars.serial_task_mote2pc_data.task_prio = task_prio;
            break;
        default:
            break;
    }
}

bool serial_is_busy(void) {
    return (serial_vars.status == SERIAL_STATUS_TX ||
            serial_vars.status == SERIAL_STATUS_RX);
}

void serial_reset(void) {
    // Disable the UART interface
    uart_disable_interrupts();
    uart_cancel_rx_cb();
    uart_cancel_tx_cb();
    uart_deinit();
}

void serial_push_msg(uint8_t command, uint8_t* data, uint8_t size) {
    // Store the HDLC command
    serial_vars.tx_command = command;

    // Store the variables and check buffer overflow
    serial_vars.tx_data_ptr = data;
    serial_vars.tx_data_len = (size > SERIAL_TX_BUFFER_SIZE ? SERIAL_TX_BUFFER_SIZE : size);

    // Push a task to the scheduler to init the transmission
    scheduler_push(serial_tx_init, TASK_PRIO_MIN);
}

void serial_parse_msg(serial_packet_t* serial_packet) {
    uint8_t* buffer = NULL;

    // Copy the command
    serial_packet->type = *serial_vars.rx_buffer_ptr++;
    serial_vars.rx_buffer_len -= 1;

    // Copy the address
    serial_packet->address  = (*serial_vars.rx_buffer_ptr++ << 8);
    serial_vars.rx_buffer_len -= 1;
    serial_packet->address |= (*serial_vars.rx_buffer_ptr++ << 0);
    serial_vars.rx_buffer_len -= 1;

    // Check for buffer overflow
    serial_packet->length = (serial_vars.rx_buffer_len > serial_packet->length ? serial_packet->length : serial_vars.rx_buffer_len);

    // Point to the data buffer
    buffer = serial_packet->data;

    // Copy the contents from the UART buffer to the other buffer
    while (serial_vars.rx_buffer_len--) {
        *buffer++ = *serial_vars.rx_buffer_ptr++;
    }

    // Change to idle status
    serial_vars.status = SERIAL_STATUS_READY;
}

/*================================ private ==================================*/

static void serial_rx_init(void) {
    // Initialize the receive pointer and size
    serial_vars.rx_buffer_ptr = serial_vars.rx_buffer;
    serial_vars.rx_buffer_len = 1;

    // Initialize the HDLC frame
    hdlc_open_rx(serial_vars.rx_buffer_ptr, &serial_vars.rx_buffer_len);

    // Change to receive mode
    serial_vars.status = SERIAL_STATUS_RX;
}

static void serial_rx_byte(void) {
    uint8_t byte, status;

    // If we are in transmit mode
    if (serial_vars.status == SERIAL_STATUS_TX) {
        return;
    }

    // Receive a byte from the UART
    byte = uart_receive_byte();

    // If we are at the beginning of an HDLC frame
    if (serial_vars.status == SERIAL_STATUS_READY) {
        // Initialize the reception
        serial_rx_init();
    }

    // Put a byte in the HDLC
    status = hdlc_put_rx(byte);

    // If the HDLC process is finished or error
    if (status == HDLC_STATUS_DONE ||
        status == HDLC_STATUS_ERROR) {
        serial_rx_done();
    }
}

static void serial_rx_done(void) {
    serial_cb_t serial_cb = NULL;
    task_prio_t task_prio;
    uint8_t pc2mote_type;

    if (serial_vars.status == SERIAL_STATUS_RX) {
        if (hdlc_close_rx() == HDLC_CRC_CORRECT) {
            // Get the packet type and update length
            pc2mote_type = *serial_vars.rx_buffer_ptr;

            switch (pc2mote_type) {
                case SERIAL_PC2MOTE_START:
                    serial_cb = serial_vars.serial_task_pc2mote_start.serial_cb;
                    task_prio = serial_vars.serial_task_pc2mote_start.task_prio;
                    break;
                case SERIAL_PC2MOTE_STOP:
                    serial_cb = serial_vars.serial_task_pc2mote_stop.serial_cb;
                    task_prio = serial_vars.serial_task_pc2mote_stop.task_prio;
                    break;
                default:
                    while (true);
                    break;
            }

            if (serial_cb != NULL) {
                scheduler_push(serial_cb, task_prio);
                return;
            }
        }
    }

    // Change to ready mode
    serial_vars.status = SERIAL_STATUS_READY;
}

static void serial_tx_init(void) {
    uint16_t node_address;

    // Disable UART interrupts
    uart_disable_interrupts();

    // Initialize the transmit pointer and size
    serial_vars.tx_buffer_ptr = serial_vars.tx_buffer;
    serial_vars.tx_buffer_len = 0;

    // Initialize the HDLC frame
    hdlc_open_tx(serial_vars.tx_buffer_ptr, &serial_vars.tx_buffer_len);

    // Copy the command
    hdlc_put_tx(serial_vars.tx_command);

    // Copy the node address
    node_address = serial_vars.eui16;
    hdlc_put_tx((node_address >> 8) & 0xFF);
    hdlc_put_tx((node_address >> 0) & 0xFF);

    // Copy the payload data
    while (serial_vars.tx_data_len--) {
        hdlc_put_tx(*serial_vars.tx_data_ptr++);
    }

    // Finalize the HDLC frame
    hdlc_close_tx();

    // Enable UART interrupts
    uart_enable_interrupts();

    // Change to transmit mode and push first byte
    serial_vars.status = SERIAL_STATUS_TX;
    serial_tx_byte();
}

static void serial_tx_byte(void) {
    // If we are in transmit mode
    if (serial_vars.status == SERIAL_STATUS_TX) {
        // If there is more data pending
        if (serial_vars.tx_buffer_len--) {
            uart_send_byte(*serial_vars.tx_buffer_ptr++);
        } else { // Otherwise
            serial_tx_done();
        }
    }
}

static void serial_tx_done(void) {
    // Change to idle status
    serial_vars.status = SERIAL_STATUS_READY;
}

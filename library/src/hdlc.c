/**
 * @file       hdlc.c
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

/*================================ include ==================================*/

#include "hdlc.h"

#include "crc16.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

typedef struct {
    hdlc_status_t hdlc_status;

    uint8_t last_byte;
    uint8_t is_escaping;

    // Receive buffer
    uint8_t* rx_buffer_ptr;
    uint8_t* rx_buffer_len;

    // Transmit buffer
    uint8_t* tx_buffer_ptr;
    uint8_t* tx_buffer_len;
} hdlc_vars_t;

/*=============================== variables =================================*/

static hdlc_vars_t hdlc_vars;

/*=============================== prototypes ================================*/

static void hdlc_parse_rx(uint8_t byte);

/*================================= public ==================================*/

void hdlc_init(void) {
    // Initialize the memory of the variables
    memset(&hdlc_vars, 0, sizeof(hdlc_vars_t));

    // Update the HDLC status
    hdlc_vars.hdlc_status = HDLC_STATUS_IDLE;
}

void hdlc_open_rx(uint8_t * buffer, uint8_t * size) {
    // Reset the HDLC driver status
    hdlc_vars.hdlc_status = HDLC_STATUS_IDLE;
    hdlc_vars.last_byte = 0;
    hdlc_vars.is_escaping = false;

    // Reset the RX buffer variables
    hdlc_vars.rx_buffer_ptr = buffer;
    hdlc_vars.rx_buffer_len = size;

    // Initialize the CRC engine
    crc16_init();
}

hdlc_status_t hdlc_put_rx(uint8_t byte) {
    // Start of HDLC frame
    if (hdlc_vars.hdlc_status == HDLC_STATUS_IDLE  &&
        hdlc_vars.last_byte == HDLC_FLAG &&
        byte != HDLC_FLAG) {

        // Update the HDLC status
        hdlc_vars.hdlc_status = HDLC_STATUS_BUSY;

        // Put the byte in the buffer
        hdlc_parse_rx(byte);
    // Middle of HDLC frame
    } else if(hdlc_vars.hdlc_status == HDLC_STATUS_BUSY &&
              byte != HDLC_FLAG) {

        // Put the byte in the buffer
        hdlc_parse_rx(byte);
    // End of HDLC frame
    } else if (hdlc_vars.hdlc_status == HDLC_STATUS_BUSY &&
               byte == HDLC_FLAG) {

        // Update the HDLC status
        hdlc_vars.hdlc_status = HDLC_STATUS_DONE;
    }

    // Store the last byte
    hdlc_vars.last_byte = byte;

    return hdlc_vars.hdlc_status;
}

hdlc_crc_t hdlc_close_rx(void) {
    // Check if the CRC values are the same
    if (crc16_check() == true) {
        // Account for the CRC bytes
        *hdlc_vars.rx_buffer_len -= 2;
        return HDLC_CRC_CORRECT;
    } else {
        return HDLC_CRC_INCORRECT;
    }
}

void hdlc_open_tx(uint8_t * buffer, uint8_t * size) {
    // Reset the TX buffer variables
    hdlc_vars.tx_buffer_ptr = buffer;
    hdlc_vars.tx_buffer_len = size;

    // Initialize the CRC engine
    crc16_init();

    // Initialize the TX buffer
    *hdlc_vars.tx_buffer_ptr++ = HDLC_FLAG;
    *hdlc_vars.tx_buffer_len += 1;
}

void hdlc_put_tx(uint8_t byte) {
    // Push a byte into the CRC engine
    crc16_push(byte);

    // Check that we do not transmit a FLAG or ESCAPE character
    if (byte == HDLC_FLAG || byte == HDLC_ESCAPE) {
       *hdlc_vars.tx_buffer_ptr++ = HDLC_ESCAPE;
       *hdlc_vars.tx_buffer_len += 1;
       byte ^= HDLC_ESCAPE_MASK;
    }

    // Copy the character into the TX buffer
    *hdlc_vars.tx_buffer_ptr++ = byte;
    *hdlc_vars.tx_buffer_len += 1;
}

void hdlc_close_tx(void) {
    uint16_t crc;

    // Finish computing the CRC value
    crc = crc16_get();

    // Add the CRC value to the TX buffer
    hdlc_put_tx((crc >> 8) & 0xFF);
    hdlc_put_tx((crc >> 0) & 0xFF);

    // Add the HDLC flag to the TX buffer
    *hdlc_vars.tx_buffer_ptr = HDLC_FLAG;
    *hdlc_vars.tx_buffer_len += 1;
}

/*================================ private ==================================*/

static void hdlc_parse_rx(uint8_t byte) {
    if (byte == HDLC_ESCAPE) {
        hdlc_vars.is_escaping = true;
    } else {
        // If we had an escape character
        if (hdlc_vars.is_escaping == true) {
            byte = byte ^ HDLC_ESCAPE_MASK;
            hdlc_vars.is_escaping = false;
        }

        // Copy the character into the RX buffer
        *hdlc_vars.rx_buffer_ptr++ = byte;
        *hdlc_vars.rx_buffer_len += 1;

        // Push a byte into the CRC engine
        crc16_push(byte);
    }
}

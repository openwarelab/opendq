/**
 * @file       packet_buffer.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef PACKET_BUFFER_H_
#define PACKET_BUFFER_H_

/*================================ include ==================================*/

#include "types.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

typedef enum {
    PACKET_STATUS_FREE  = 0x00,
    PACKET_STATUS_TAKEN = 0x01
} packet_status_t;

typedef struct {
    packet_status_t status;

    uint8_t* payload;               // Pointer to the paypload
    uint8_t  length;                // Length of the payload
    int8_t   rssi;                  // The RSSI value of the received packet
    uint8_t  lqi;                   // The LQI value of the received packet
    uint8_t  crc;                   // The CRC value of the received packet

    uint8_t buffer[128];            // The buffer where to store the packet (1B LENGTH + 127B DATA + 2B CRC)
    uint8_t size;                   // The size occupied in the buffer
} packet_buffer_t;

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void packet_buffer_init(void);

packet_buffer_t* packet_buffer_get(void);
void packet_buffer_release(packet_buffer_t* packet_buffer);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* PACKET_BUFFER_H_ */

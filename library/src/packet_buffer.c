/**
 * @file       packet_buffer.c
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

/*================================ include ==================================*/

#include "packet_buffer.h"

#include "cpu.h"
#include "leds.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

#define PACKET_BUFFER_SIZE				( 16 )

/*=============================== variables =================================*/

static packet_buffer_t packet_buffer[PACKET_BUFFER_SIZE];

/*=============================== prototypes ================================*/

static void packet_buffer_reset(packet_buffer_t* queue_entry);

/*================================= public ==================================*/

void packet_buffer_init(void) {
    packet_buffer_t* scratch = NULL;

    // Initialize the queue entries
    for (uint32_t i = 0; i < PACKET_BUFFER_SIZE; i++) {
        scratch = &packet_buffer[i];
        packet_buffer_reset(scratch);
    }
}

packet_buffer_t* packet_buffer_get(void) {
    packet_buffer_t* scratch = NULL;

    // Disable interrupts
    cpu_disable_interrupts();

    // Loop through the array to find an empty space
    for (uint32_t i = 0; i < PACKET_BUFFER_SIZE; i++) {
        // Updated the queue entry
        scratch = &packet_buffer[i];
        if (scratch->status == PACKET_STATUS_FREE) {
            scratch->status = PACKET_STATUS_TAKEN;
            break;
        }
    }

    // If the queue is full
    if (scratch == NULL) {
        leds_error_on();
        while (true);
    }

    // Enable the interrupts
    cpu_enable_interrupts();

    // Return the queue entry
    return scratch;
}

void packet_buffer_release(packet_buffer_t* packet_buffer) {
    if (packet_buffer != NULL) {
        // Disable interrupts
        cpu_disable_interrupts();

        // Reset the queue entry
        packet_buffer_reset(packet_buffer);

        // Enable interrupts
        cpu_enable_interrupts();
    }
}

/*================================ private ==================================*/

static void packet_buffer_reset(packet_buffer_t* packet_buffer) {
    packet_buffer->status = PACKET_STATUS_FREE;

    packet_buffer->payload = packet_buffer->buffer;
    packet_buffer->length  = 0;
    packet_buffer->rssi    = 0;
    packet_buffer->lqi     = 0;
    packet_buffer->crc     = 0;

    packet_buffer->size    = sizeof(packet_buffer->buffer);
}

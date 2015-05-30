/**
 * @file       main.c
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

/*================================ include ==================================*/

#include "types.h"

#include "board.h"
#include "cpu.h"
#include "debug.h"
#include "leds.h"
#include "radio.h"
#include "virtual_timer.h"

#include "library.h"
#include "scheduler.h"

#include "mac.h"
#include "wor.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

static void start_cmd(void);
static void start_cb(void);
static void reset_cmd(void);
static void reset_cb(void);

/*================================= public ==================================*/

void start_cmd(void) {
    // Register the MAC callback
    wor_set_cb(mac_start);

    // Start the WOR task
    scheduler_push(wor_config, TASK_PRIO_MAX);
}

void reset_cmd(void) {
    // Wait until UART finishes
    while(serial_is_busy())
        ;

    // Reset the board
    cpu_reset();
}

void reset_cb(void) {
    // Wait until UART finishes
    while(serial_is_busy())
        ;

    // Send a reset message to the computer
    serial_push_msg(SERIAL_MOTE2PC_RESET, NULL, 0);

    // Start a timer to reset
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, 10, reset_cmd, TASK_PRIO_MAX);
}

void start_cb(void) {
    static uint8_t buffer[64];
    static serial_packet_t serial_packet;
    uint32_t experiment_duration = 0;
    uint8_t* data = NULL;

    // Setup the serial packet
    serial_packet.data = buffer;
    serial_packet.length = sizeof(buffer);

    // Parse the serial message
    serial_parse_msg(&serial_packet);

    // Copy the pointer to the buffer
    data = serial_packet.data;

    // Configure the MAC based on the serial message
    mac_vars.mac_type = (mac_type_t)(*data++);
    mac_vars.mac_slots = (mac_slots_t)(*data++);

    // Configure the experiment duration
    experiment_duration  = (*data++ << 8);
    experiment_duration |= (*data++);
    experiment_duration *= 33;

    // Push the task to start
    scheduler_push(start_cmd, TASK_PRIO_MAX);

    // Start a timer to finish
    virtual_timer_start(VIRTUAL_TIMER_TYPE_ONE_SHOT, experiment_duration, reset_cb, TASK_PRIO_MAX);
}

int main(void) {
    board_init();
    library_init();
    mac_init();
    scheduler_init();

    // Register a serial callback
    serial_register_pc2mote_cb(SERIAL_PC2MOTE_START, start_cb, TASK_PRIO_MAX);

    // Start the scheduler
    scheduler_start();
}

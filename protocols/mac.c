/**
 * @file       mac.c
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
#include "fsa.h"
#include "wor.h"

#include "library.h"
#include "scheduler.h"

#include "debug.h"
#include "leds.h"
#include "radio.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

mac_vars_t mac_vars;

/*=============================== prototypes ================================*/

/*================================= public ==================================*/

/* Function to initialize the MAC layer */
void mac_init(void) {
    // Initialize the memory of the variables
    memset(&mac_vars, 0, sizeof(mac_vars_t));

    // Initialize the MAC modules
    fsa_init();
    dq_init();
    wor_init();
}

void mac_start(void) {
    task_cb_t mac_task = NULL;

    debug_user_on();

    // Wake up the radio
    radio_idle();

    // Put the radio on the default channel
    radio_set_channel(mac_vars.mac_channel);

    // Select which MAC to start
    switch (mac_vars.mac_type) {
        case(MAC_TYPE_FSA):
            mac_task = fsa_start;
            break;
        case(MAC_TYPE_DQ):
            mac_task = dq_start;
            break;
        default:
            leds_error_on();
            while(true);
            break;
    }

    // Push the MAC task to the scheduler
    scheduler_push(mac_task, TASK_PRIO_MAX);

    debug_user_off();
}

void mac_set_type(mac_type_t type) {
    mac_vars.mac_type = type;
}

void mac_set_channel(mac_channel_t channel) {
    mac_vars.mac_channel = channel;
}

void mac_set_time(mac_time_t time) {
    mac_vars.mac_time = time;
}

void mac_toggle_synchronized(mac_state_t mac_state) {
    mac_vars.mac_state = mac_state;

    if (mac_state == MAC_STATE_SYNC) {
        leds_user_on();
    } else {
        leds_user_off();
    }
}

uint8_t mac_next_channel(void) {
    uint8_t next_channel;

    // Generate a random number between 0 and 14
    // next_channel = 11 + (uint8_t)random_get() % 15;
    next_channel = MAC_DEFAULT_CHANNEL;

    return next_channel;
}

/*================================ private ==================================*/

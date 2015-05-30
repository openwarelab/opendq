/**
 * @file       virtual_timer.c
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

/*================================ include ==================================*/

#include "virtual_timer.h"

#include "board.h"
#include "bsp_timer.h"
#include "leds.h"

/*================================ define ===================================*/

#define VIRTUAL_TIMER_MAX_TICKS                 ( 0xFFFFFFFF )
#define VIRTUAL_TIMER_MAX_TIMERS                ( 16 )
#define VIRTUAL_TIMER_MAX_TIMERS_ERROR          ( 255 )

/*================================ typedef ==================================*/

typedef enum {
    VIRTUAL_TIMER_MODE_OFF = 0x00,
    VIRTUAL_TIMER_MODE_ON  = 0x01
} virtual_timer_mode_t;

typedef struct {
    virtual_timer_mode_t  mode;
    virtual_timer_t       buffer[VIRTUAL_TIMER_MAX_TIMERS];
    virtual_timer_width_t timeout;
} virtual_timer_vars_t;

/*=============================== variables =================================*/

static virtual_timer_vars_t virtual_timer_vars;

/*=============================== prototypes ================================*/

static void virtual_timer_interrupt(void);
static void virtual_timer_reset(virtual_timer_id_t vtimer_id);

/*================================= public ==================================*/

void virtual_timer_init(void) {
    // Initialize the memory of the variables
    memset(&virtual_timer_vars, 0, sizeof(virtual_timer_vars_t));

    // Initialize the vtimer entries
    for (uint8_t i = 0; i < VIRTUAL_TIMER_MAX_TIMERS; i++) {
        virtual_timer_reset(i);
    }

    // Initially, the virtual timer is off
    virtual_timer_vars.mode = VIRTUAL_TIMER_MODE_OFF;
}

virtual_timer_id_t virtual_timer_start(virtual_timer_type_t type, virtual_timer_width_t ticks, task_cb_t callback, task_prio_t priority) {
    virtual_timer_id_t id;

    // Find an unused timer
    for (id = 0; id < VIRTUAL_TIMER_MAX_TIMERS &&
        virtual_timer_vars.buffer[id].status != VIRTUAL_TIMER_STATUS_STOPPED; id++);

    // If a position is available register the timer, otherwise return error
    if (id < VIRTUAL_TIMER_MAX_TIMERS) {
        // Update the timer variables
        virtual_timer_vars.buffer[id].status     = VIRTUAL_TIMER_STATUS_RUNNING;
        virtual_timer_vars.buffer[id].type       = type;
        virtual_timer_vars.buffer[id].ticks      = ticks;
        virtual_timer_vars.buffer[id].ticks_left = ticks;
        virtual_timer_vars.buffer[id].callback   = callback;
        virtual_timer_vars.buffer[id].priority   = priority;

        // If the virtual timer is not running or the expire time is shorter, reconfigure it
        if (virtual_timer_vars.mode == VIRTUAL_TIMER_MODE_OFF ||
            virtual_timer_vars.buffer[id].ticks_left < virtual_timer_vars.timeout) {

            // Update the current timeout
            virtual_timer_vars.timeout = virtual_timer_vars.buffer[id].ticks_left;

            // If the timer is not running, start it
            if (virtual_timer_vars.mode == VIRTUAL_TIMER_MODE_OFF) {
                // Start the bsp_timer
                bsp_timer_set_cb(virtual_timer_interrupt);
                bsp_timer_enable_interrupts();

                // Set the virtual timer to on
                virtual_timer_vars.mode = VIRTUAL_TIMER_MODE_ON;
            }

            // Schedule a system timer
            bsp_timer_start(virtual_timer_vars.timeout);
        }
    } else {
        leds_error_on();
        while(true);
    }

    return id;
}

void virtual_timer_stop(virtual_timer_id_t vtimer_id) {
    virtual_timer_reset(vtimer_id);
}

/*================================ private ==================================*/

static void virtual_timer_interrupt(void) {
    virtual_timer_id_t id;
    virtual_timer_width_t ticks;

    // Initialize the variable to its maximum value
    ticks = VIRTUAL_TIMER_MAX_TICKS;

    // Update the timers
    for (id = 0; id < VIRTUAL_TIMER_MAX_TIMERS; id++) {
        // If the timer is running
        if (virtual_timer_vars.buffer[id].status == VIRTUAL_TIMER_STATUS_RUNNING) {
            // Update the number of ticks left
            if (virtual_timer_vars.buffer[id].ticks_left  > virtual_timer_vars.timeout) {
                virtual_timer_vars.buffer[id].ticks_left -= virtual_timer_vars.timeout;
            } else {
                virtual_timer_vars.buffer[id].ticks_left = 0;
            }

            // If the timer has expired, push it to the scheduler and decide what's next
            if (virtual_timer_vars.buffer[id].ticks_left == 0) {
                scheduler_push(virtual_timer_vars.buffer[id].callback, virtual_timer_vars.buffer[id].priority);
                // If the timer is periodic, restart the number of ticks left, otherwise remove it
                if (virtual_timer_vars.buffer[id].type == VIRTUAL_TIMER_TYPE_PERIODIC) {
                    virtual_timer_vars.buffer[id].ticks_left = virtual_timer_vars.buffer[id].ticks;
                } else if (virtual_timer_vars.buffer[id].type == VIRTUAL_TIMER_TYPE_ONE_SHOT) {
                    virtual_timer_reset(id);
                } else {
                    leds_error_on();
                    while(true);
                }
            }
        }
    }

    // Calculate the next expire timer
    for (id = 0; id < VIRTUAL_TIMER_MAX_TIMERS; id++) {
        if (virtual_timer_vars.buffer[id].status == VIRTUAL_TIMER_STATUS_RUNNING &&
            virtual_timer_vars.buffer[id].ticks_left < ticks) {
            ticks = virtual_timer_vars.buffer[id].ticks_left;
        }
    }

    // If there is no next expire timer
    if (ticks == VIRTUAL_TIMER_MAX_TICKS) {
        virtual_timer_vars.mode = VIRTUAL_TIMER_MODE_OFF;
        bsp_timer_stop();
    } else {
        // Schedule a system timer
        virtual_timer_vars.timeout = ticks;
        bsp_timer_start(virtual_timer_vars.timeout);
    }
}

static void virtual_timer_reset(virtual_timer_id_t vtimer_id) {
    virtual_timer_vars.buffer[vtimer_id].status     = VIRTUAL_TIMER_STATUS_STOPPED;
    virtual_timer_vars.buffer[vtimer_id].type       = VIRTUAL_TIMER_TYPE_NONE;
    virtual_timer_vars.buffer[vtimer_id].ticks      = 0;
    virtual_timer_vars.buffer[vtimer_id].ticks_left = 0;
    virtual_timer_vars.buffer[vtimer_id].callback   = NULL;
    virtual_timer_vars.buffer[vtimer_id].priority   = TASK_PRIO_NONE;
}

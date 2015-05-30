/**
 * @file       bsp_timer.c
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

/*================================ include ==================================*/

#include "cc2538_include.h"

#include "bsp_timer.h"

/*================================ define ===================================*/

#define BSP_TIMER_MINIMUM_TICKS         ( 5 )

/*================================ typedef ==================================*/

typedef struct {
    bsp_timer_cb_t callback;
} bsp_timer_vars_t;

/*=============================== variables =================================*/

static bsp_timer_vars_t bsp_timer_vars;

/*=============================== prototypes ================================*/

void bsp_timer_interrupt(void);

/*================================= public ==================================*/

void bsp_timer_init(void) {
    // Initialize the memory of the bsp_timer variables
    memset(&bsp_timer_vars, 0, sizeof(bsp_timer_vars_t));

    // Allow the bsp_timer to wake upfrom LPM2
    GPIOIntWakeupEnable(GPIO_IWE_SM_TIMER);
}

void bsp_timer_reset(void) {
    bsp_timer_cancel_cb();
    bsp_timer_disable_interrupts();
}

void bsp_timer_set_cb(bsp_timer_cb_t callback) {
    bsp_timer_vars.callback = callback;
}

void bsp_timer_cancel_cb(void) {
    bsp_timer_vars.callback = NULL;
}

void bsp_timer_enable_interrupts(void) {
    IntPrioritySet(INT_SMTIM, (7 << 5));
    IntEnable(INT_SMTIM);
}

void bsp_timer_disable_interrupts(void) {
    IntDisable(INT_SMTIM);
}

void bsp_timer_start(bsp_timer_width_t delay_ticks) {
    uint32_t current_ticks;

    // Get the current number of ticks
    current_ticks = SleepModeTimerCountGet();

    if (delay_ticks < BSP_TIMER_MINIMUM_TICKS) {
        IntPendSet(INT_SMTIM);
    } else {
        // Set the timer compare value
        SleepModeTimerCompareSet(current_ticks + delay_ticks);
    }
}

void bsp_timer_stop(void) {
    bsp_timer_cancel_cb();
    bsp_timer_disable_interrupts();
}

bsp_timer_width_t bsp_timer_get(void) {
    bsp_timer_width_t current_ticks;

    // Get the current number of ticks
    current_ticks = SleepModeTimerCountGet();

    return current_ticks;
}

bool bsp_timer_expired(bsp_timer_width_t future) {
    bsp_timer_width_t current;
    int32_t remaining;

    current = SleepModeTimerCountGet();

    remaining = (int32_t) (future - current);

    if (remaining > 0) {
        return false;
    } else {
        return true;
    }
}

/*================================ private ==================================*/

/*=============================== interrupt =================================*/

void bsp_timer_interrupt(void) {
    // Clear the pending interrupt
    IntPendClear(INT_SMTIM);

    // Execute the callback function
    if (bsp_timer_vars.callback != NULL) {
        bsp_timer_vars.callback();
    }
}

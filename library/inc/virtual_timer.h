/**
 * @file       virtual_timer.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef VIRTUAL_TIMER_H_
#define VIRTUAL_TIMER_H_

/*================================ include ==================================*/

#include "types.h"

#include "bsp_timer.h"

#include "scheduler.h"

/*================================ define ===================================*/

#define VIRTUAL_TIMER_KICK_NOW                  ( 0 )

/*================================ typedef ==================================*/

typedef bsp_timer_width_t virtual_timer_width_t;

typedef uint8_t virtual_timer_id_t;

typedef enum {
    VIRTUAL_TIMER_STATUS_STOPPED = 0x00,
    VIRTUAL_TIMER_STATUS_RUNNING = 0x01
} virtual_timer_status_t;

typedef enum {
    VIRTUAL_TIMER_TYPE_NONE     = 0x00,
    VIRTUAL_TIMER_TYPE_PERIODIC = 0x01,
    VIRTUAL_TIMER_TYPE_ONE_SHOT = 0x02
} virtual_timer_type_t;

typedef struct virtual_timer_t {
    virtual_timer_status_t status;
    virtual_timer_type_t   type;
    virtual_timer_width_t  ticks;
    virtual_timer_width_t  ticks_left;
    task_cb_t              callback;
    task_prio_t            priority;
} virtual_timer_t;

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

/*================================= public ==================================*/

void virtual_timer_init(void);
virtual_timer_id_t virtual_timer_start(virtual_timer_type_t vtimer_type, virtual_timer_width_t vtimer_ticks, task_cb_t task_callback, task_prio_t task_priority);
void virtual_timer_stop(virtual_timer_id_t vtimer_id);

/*================================ private ==================================*/

#endif /* VIRTUAL_TIMER_H_ */

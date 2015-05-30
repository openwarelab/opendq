/**
 * @file       scheduler.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

/*================================ include ==================================*/

#include "types.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

typedef void (* task_cb_t)(void);

typedef enum {
   TASK_PRIO_NONE = 0x00,
   TASK_PRIO_MIN  = 0x01,
   TASK_PRIO_MED  = 0x02,
   TASK_PRIO_MAX  = 0x03
} task_prio_t;

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void scheduler_init(void);
void scheduler_start(void);
void scheduler_push(task_cb_t callback, task_prio_t priority);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* SCHEDULER_H_ */

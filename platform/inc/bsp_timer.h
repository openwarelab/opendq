/**
 * @file       bsp_timer.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef BSP_TIMER_H_
#define BSP_TIMER_H_

/*================================ include ==================================*/

#include "types.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

typedef void (* bsp_timer_cb_t)(void);

typedef uint32_t bsp_timer_width_t;

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void bsp_timer_init(void);
void bsp_timer_reset(void);
void bsp_timer_set_cb(bsp_timer_cb_t callback);
void bsp_timer_cancel_cb(void);
void bsp_timer_enable_interrupts(void);
void bsp_timer_disable_interrupts(void);
void bsp_timer_start(bsp_timer_width_t delay_ticks);
void bsp_timer_stop(void);
bsp_timer_width_t bsp_timer_get(void);
bool bsp_timer_expired(bsp_timer_width_t future);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* BSP_TIMER_H_ */

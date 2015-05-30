/**
 * @file       debug.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef DEBUG_H_
#define DEBUG_H_

/*================================ include ==================================*/

#include "types.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void debug_init(void);

void debug_clocks_on(void);
void debug_clocks_off(void);
void debug_clocks_toggle(void);

void debug_system_on(void);
void debug_system_off(void);
void debug_system_toggle(void);

void debug_radio_on(void);
void debug_radio_off(void);
void debug_radio_toggle(void);

void debug_error_on(void);
void debug_error_off(void);
void debug_error_toggle(void);

void debug_user_on(void);
void debug_user_off(void);
void debug_user_toggle(void);

void debug_isr_on(void);
void debug_isr_off(void);
void debug_isr_toggle(void);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* DEBUG_H_ */

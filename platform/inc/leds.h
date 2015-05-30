/**
 * @file       leds.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef LEDS_H_
#define LEDS_H_

/*================================ include ==================================*/

#include "types.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void leds_init(void);

void leds_all_on(void);
void leds_all_off(void);

void leds_system_on(void);
void leds_system_off(void);
void leds_system_toggle(void);

void leds_radio_on(void);
void leds_radio_off(void);
void leds_radio_toggle(void);

void leds_error_on(void);
void leds_error_off(void);
void leds_error_toggle(void);

void leds_user_on(void);
void leds_user_off(void);
void leds_user_toggle(void);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* LEDS_H_ */

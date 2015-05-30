/**
 * @file       wor.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef WOR_H_
#define WOR_H_

/*================================ include ==================================*/

#include "mac.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

typedef void (* wor_cb_t)(void);

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void wor_init(void);
void wor_config(void);
void wor_set_cb(wor_cb_t callback);
void wor_cancel_cb(void);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* WOR_H_ */

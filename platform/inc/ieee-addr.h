/**
 * @file       ieee-addr.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef IEEE_ADDR_H_
#define IEEE_ADDR_H_

/*================================ include ==================================*/

#include "types.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void ieee_addr_init(void);
void ieee_addr_get_eui16(uint16_t* address);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* IEEE_ADDR_H_ */

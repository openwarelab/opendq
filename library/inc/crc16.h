/**
 * @file       crc16.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef CRC16_H_
#define CRC16_H_

/*================================ include ==================================*/

#include "types.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void crc16_init(void);
void crc16_push(uint8_t byte);
uint16_t crc16_get(void);
bool crc16_check(void);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* CRC16_H_ */

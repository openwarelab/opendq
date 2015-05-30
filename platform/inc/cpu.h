/**
 * @file       cpu.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef CPU_H_
#define CPU_H_

/*================================ include ==================================*/

#include "types.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void cpu_init(void);

void cpu_wait(void);
void cpu_sleep(void);
void cpu_reset(void);

void cpu_enable_interrupts(void);
void cpu_disable_interrupts(void);

void cpu_delay_us(uint32_t delay_us);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* CPU_H_ */

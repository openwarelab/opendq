/**
 * @file       gpio.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef GPIO_H_
#define GPIO_H_

/*================================ include ==================================*/

#include "types.h"

/*================================ define ===================================*/

#define GPIO_A_PORT                 ( 0 ) // GPIO_A
#define GPIO_B_PORT                 ( 1 ) // GPIO_B
#define GPIO_C_PORT                 ( 2 ) // GPIO_C
#define GPIO_D_PORT                 ( 3 ) // GPIO_D

/*================================ typedef ==================================*/

typedef void (* gpio_callback_t)(void);

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void gpio_init(void);

void gpio_config_output(uint8_t port, uint8_t pin);
void gpio_config_input(uint8_t port, uint8_t pin, uint8_t edge);

void gpio_register_callback(uint8_t port, uint8_t pin, gpio_callback_t callback);
void gpio_clear_callback(uint8_t port, uint8_t pin);

void gpio_enable_interrupt(uint8_t port, uint8_t pin);
void gpio_disable_interrupt(uint8_t port, uint8_t pin);

void gpio_on(uint8_t port, uint8_t pin);
void gpio_off(uint8_t port, uint8_t pin);
void gpio_toggle(uint8_t port, uint8_t pin);
bool gpio_read(uint8_t port, uint8_t pin);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* GPIO_H_ */

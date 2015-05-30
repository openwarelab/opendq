/**
 * @file       debug.c
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

/*================================ include ==================================*/

#include "cc2538_include.h"

#include "openmote-cc2538.h"

#include "debug.h"
#include "gpio.h"

/*================================ define ===================================*/

#define DEBUG               ( 1 )

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

/*================================= public ==================================*/

#if (DEBUG == 1)
void debug_init(void) {
    gpio_config_output(GPIO_DEBUG_AD0_PORT, GPIO_DEBUG_AD0_PIN);
    gpio_config_output(GPIO_DEBUG_AD1_PORT, GPIO_DEBUG_AD1_PIN);
    gpio_config_output(GPIO_DEBUG_AD2_PORT, GPIO_DEBUG_AD2_PIN);
    gpio_config_output(GPIO_DEBUG_AB2_PORT, GPIO_DEBUG_AB2_PIN);
    gpio_config_output(GPIO_DEBUG_AB3_PORT, GPIO_DEBUG_AB3_PIN);
    gpio_config_output(GPIO_DEBUG_AB5_PORT, GPIO_DEBUG_AB5_PIN);

    gpio_off(GPIO_DEBUG_AD0_PORT, GPIO_DEBUG_AD0_PIN);
    gpio_off(GPIO_DEBUG_AD1_PORT, GPIO_DEBUG_AD1_PIN);
    gpio_off(GPIO_DEBUG_AD2_PORT, GPIO_DEBUG_AD2_PIN);
    gpio_off(GPIO_DEBUG_AB2_PORT, GPIO_DEBUG_AB2_PIN);
    gpio_off(GPIO_DEBUG_AB3_PORT, GPIO_DEBUG_AB3_PIN);
    gpio_off(GPIO_DEBUG_AB5_PORT, GPIO_DEBUG_AB5_PIN);
}

void debug_clocks_on(void) {
    gpio_on(GPIO_DEBUG_AD0_PORT, GPIO_DEBUG_AD0_PIN);
}

void debug_clocks_off(void) {
    gpio_off(GPIO_DEBUG_AD0_PORT, GPIO_DEBUG_AD0_PIN);
}

void debug_clocks_toggle(void) {
    gpio_toggle(GPIO_DEBUG_AD0_PORT, GPIO_DEBUG_AD0_PIN);
}

void debug_system_on(void) {
    gpio_on(GPIO_DEBUG_AD1_PORT, GPIO_DEBUG_AD1_PIN);
}

void debug_system_off(void) {
    gpio_off(GPIO_DEBUG_AD1_PORT, GPIO_DEBUG_AD1_PIN);
}

void debug_system_toggle(void) {
    gpio_toggle(GPIO_DEBUG_AD1_PORT, GPIO_DEBUG_AD1_PIN);
}

void debug_radio_on(void) {
    gpio_on(GPIO_DEBUG_AD2_PORT, GPIO_DEBUG_AD2_PIN);
}

void debug_radio_off(void) {
    gpio_off(GPIO_DEBUG_AD2_PORT, GPIO_DEBUG_AD2_PIN);
}

void debug_radio_toggle(void) {
    gpio_toggle(GPIO_DEBUG_AD2_PORT, GPIO_DEBUG_AD2_PIN);
}

void debug_error_on(void) {
    gpio_on(GPIO_DEBUG_AB3_PORT, GPIO_DEBUG_AB3_PIN);
}

void debug_error_off(void) {
    gpio_off(GPIO_DEBUG_AB3_PORT, GPIO_DEBUG_AB3_PIN);
}

void debug_error_toggle(void) {
    gpio_toggle(GPIO_DEBUG_AB3_PORT, GPIO_DEBUG_AB3_PIN);
}

void debug_user_on(void) {
    gpio_on(GPIO_DEBUG_AB2_PORT, GPIO_DEBUG_AB2_PIN);
}

void debug_user_off(void) {
    gpio_off(GPIO_DEBUG_AB2_PORT, GPIO_DEBUG_AB2_PIN);
}

void debug_user_toggle(void) {
    gpio_toggle(GPIO_DEBUG_AB2_PORT, GPIO_DEBUG_AB2_PIN);
}

void debug_isr_on(void) {
    gpio_on(GPIO_DEBUG_AB5_PORT, GPIO_DEBUG_AB5_PIN);
}

void debug_isr_off(void) {
    gpio_off(GPIO_DEBUG_AB5_PORT, GPIO_DEBUG_AB5_PIN);
}

void debug_isr_toggle(void) {
    gpio_toggle(GPIO_DEBUG_AB5_PORT, GPIO_DEBUG_AB5_PIN);
}

#else //  (DEBUG == 0)

void debug_init(void) {
}

void debug_clocks_on(void) {
}

void debug_clocks_off(void) {
}

void debug_clocks_toggle(void) {
}

void debug_system_on(void) {
}

void debug_system_off(void) {
}

void debug_system_toggle(void) {
}

void debug_radio_on(void) {
}

void debug_radio_off(void) {
}

void debug_radio_toggle(void) {
}

void debug_error_on(void) {
}

void debug_error_off(void) {
}

void debug_error_toggle(void) {
}

void debug_user_on(void) {
}

void debug_user_off(void) {
}

void debug_user_toggle(void) {
}

void debug_isr_on(void) {
}

void debug_isr_off(void) {
}

void debug_isr_toggle(void) {
}

#endif

/*================================ private ==================================*/

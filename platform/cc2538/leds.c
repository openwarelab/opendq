/**
 * @file       leds.c
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

#include "gpio.h"
#include "leds.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

/*================================= public ==================================*/

void leds_init(void) {
    gpio_config_output(LED_RED_PORT, LED_RED_PIN);
    gpio_config_output(LED_GREEN_PORT, LED_GREEN_PIN);
    gpio_config_output(LED_ORANGE_PORT, LED_ORANGE_PIN);
    gpio_config_output(LED_YELLOW_PORT, LED_YELLOW_PIN);

    gpio_off(LED_RED_PORT, LED_RED_PIN);
    gpio_off(LED_GREEN_PORT, LED_GREEN_PIN);
    gpio_off(LED_ORANGE_PORT, LED_ORANGE_PIN);
    gpio_off(LED_YELLOW_PORT, LED_YELLOW_PIN);
}

void leds_all_on(void) {
    leds_system_on();
    leds_error_on();
    leds_radio_on();
    leds_user_on();
}

void leds_all_off(void) {
    leds_system_off();
    leds_error_off();
    leds_radio_off();
    leds_user_off();
}

void leds_system_on(void) {
    gpio_on(LED_GREEN_PORT, LED_GREEN_PIN);
}

void leds_system_off(void) {
    gpio_off(LED_GREEN_PORT, LED_GREEN_PIN);
}

void leds_system_toggle(void) {
    gpio_toggle(LED_GREEN_PORT, LED_GREEN_PIN);
}

void leds_radio_on(void) {
    gpio_on(LED_ORANGE_PORT, LED_ORANGE_PIN);
}

void leds_radio_off(void) {
    gpio_off(LED_ORANGE_PORT, LED_ORANGE_PIN);
}

void leds_radio_toggle(void) {
    gpio_toggle(LED_ORANGE_PORT, LED_ORANGE_PIN);
}

void leds_error_on(void) {
    gpio_on(LED_RED_PORT, LED_RED_PIN);
}

void leds_error_off(void) {
    gpio_off(LED_RED_PORT, LED_RED_PIN);
}

void leds_error_toggle(void) {
    gpio_toggle(LED_RED_PORT, LED_RED_PIN);
}

void leds_user_on(void) {
    gpio_on(LED_YELLOW_PORT, LED_YELLOW_PIN);
}

void leds_user_off(void) {
    gpio_off(LED_YELLOW_PORT, LED_YELLOW_PIN);
}

void leds_user_toggle(void) {
    gpio_toggle(LED_YELLOW_PORT, LED_YELLOW_PIN);
}

/*================================ private ==================================*/

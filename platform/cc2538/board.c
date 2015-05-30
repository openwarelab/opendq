/**
 * @file       board.c
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

#include "board.h"
#include "bsp_timer.h"
#include "cpu.h"
#include "debug.h"
#include "flash.h"
#include "gpio.h"
#include "ieee-addr.h"
#include "leds.h"
#include "radio.h"
#include "random.h"
#include "uart.h"

#include "types.h"

/*================================ define ===================================*/

#define BOARD_TICKS_PER_US              ( 31 )

// Defines the CCA Flash address
#define CCA_FLASH_ADDRESS               ( 0x0027F800 )

#define BOARD_ENABLE_FLASH_ERASE        ( TRUE )
#define BUTTON_USER_PORT                ( GPIO_C_PORT )
#define BUTTON_USER_PIN                 ( GPIO_PIN_3 )
#define BUTTON_USER_EDGE                ( GPIO_FALLING_EDGE )
#define BUTTON_DELAY_US                 ( 100000 )
#define BUTTON_DELAY_TICKS              ( BUTTON_DELAY_US / BOARD_TICKS_PER_US )

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

static void board_enable_flash_erase(void);
static void board_perform_flash_erase(void);

/*================================= public ==================================*/

void board_init(void) {
    // Initialize the CPU
    cpu_init();

    // Initialize the board subsystems
    gpio_init();
    leds_init();
    debug_init();

    // Initialize the random module
    random_init();

    // Initialize the IEEE address
    ieee_addr_init();

    // Initialize the IEEE 802.15.4 radio
    radio_init();

    // Initialize the bsp and radio timers
    bsp_timer_init();

    // Initialize the communication interfaces
    uart_init();

    // Allow the user button to trigger a flash erase
    board_enable_flash_erase();
}
/*================================ private ==================================*/

static void board_enable_flash_erase(void) {
#if (BOARD_ENABLE_FLASH_ERASE == TRUE)
    uint32_t delay_ticks;

    gpio_config_input(BUTTON_USER_PORT, BUTTON_USER_PIN, BUTTON_USER_EDGE);

    // Calculate timeout
    delay_ticks  = BUTTON_DELAY_TICKS;
    delay_ticks += bsp_timer_get();

    // Wait until timeout
    while (!bsp_timer_expired(delay_ticks));

    gpio_register_callback(BUTTON_USER_PORT, BUTTON_USER_PIN, &board_perform_flash_erase);
    gpio_enable_interrupt(BUTTON_USER_PORT, BUTTON_USER_PIN);
#endif
}

static void board_perform_flash_erase(void) {
    cpu_disable_interrupts();
    flash_erase_page(CCA_FLASH_ADDRESS);
    cpu_reset();
}

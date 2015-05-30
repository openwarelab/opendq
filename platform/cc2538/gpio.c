/**
 * @file       gpio.c
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

#include "gpio.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

static gpio_callback_t gpio_callbacks[32];

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void gpio_a_interrupt(void);
void gpio_b_interrupt(void);
void gpio_c_interrupt(void);
void gpio_d_interrupt(void);

uint32_t gpio_get_base(uint8_t port);
void gpio_notify_callback(uint8_t port, uint8_t mask);

/*================================= public ==================================*/

void gpio_init(void) {
    // Initialize the gpio callbacks
    memset(gpio_callbacks, 0, sizeof(gpio_callbacks));

    // Register the GPIO interrupt handlers
    GPIOPortIntRegister(GPIO_A_BASE, gpio_a_interrupt);
    GPIOPortIntRegister(GPIO_B_BASE, gpio_b_interrupt);
    GPIOPortIntRegister(GPIO_C_BASE, gpio_c_interrupt);
    GPIOPortIntRegister(GPIO_D_BASE, gpio_d_interrupt);
}

void gpio_config_output(uint8_t port, uint8_t pin) {
    uint32_t base;
    base = gpio_get_base(port);
    GPIOPinTypeGPIOOutput(base, pin);
}

void gpio_config_input(uint8_t port, uint8_t pin, uint8_t edge) {
    uint32_t base;
    base = gpio_get_base(port);
    GPIOPinTypeGPIOInput(base, pin);
    GPIOIntTypeSet(base, pin, edge);
}

void gpio_register_callback(uint8_t port, uint8_t pin, gpio_callback_t callback) {
    gpio_callbacks[(port << 3) + pin] = callback;
}

void gpio_clear_callback(uint8_t port, uint8_t pin) {
    gpio_callbacks[(port << 3) + pin] = NULL;
}

void gpio_enable_interrupt(uint8_t port, uint8_t pin) {
    uint32_t base;
    base = gpio_get_base(port);

    GPIOPinIntClear(base, pin);
    GPIOPowIntClear(base, pin);

    GPIOPinIntEnable(base, pin);
    GPIOPowIntEnable(base, pin);
}

void gpio_disable_interrupt(uint8_t port, uint8_t pin) {
    uint32_t base;
    base = gpio_get_base(port);
    GPIOPinIntDisable(base, pin);
    GPIOPowIntDisable(base, pin);
}

void gpio_on(uint8_t port, uint8_t pin) {
    uint32_t base;
    base = gpio_get_base(port);
    GPIOPinWrite(base, pin, pin);
}

void gpio_off(uint8_t port, uint8_t pin) {
    uint32_t base;
    base = gpio_get_base(port);
    GPIOPinWrite(base, pin, 0);
}

void gpio_toggle(uint8_t port, uint8_t pin) {
    uint32_t base;
    uint32_t status;

    base = gpio_get_base(port);

    status = GPIOPinRead(base, pin);

    if (status & pin) {
        GPIOPinWrite(base, pin, 0);
    } else {
        GPIOPinWrite(base, pin, pin);
    }
}

bool gpio_read(uint8_t port, uint8_t pin) {
    uint32_t state;
    state = GPIOPinRead(port, pin);
    return (bool)(state & pin);
}

/*================================ private ==================================*/

uint32_t gpio_get_base(uint8_t port)
{
    uint32_t base;
    switch(port) {
        case GPIO_A_PORT:
            base = GPIO_A_BASE;
            break;
        case GPIO_B_PORT:
            base = GPIO_B_BASE;
            break;
        case GPIO_C_PORT:
            base = GPIO_C_BASE;
            break;
        case GPIO_D_PORT:
            base = GPIO_D_BASE;
            break;
        default:
            while (true);
            break;
    }
    return base;
}

void gpio_notify_callback(uint8_t port, uint8_t mask) {
    gpio_callback_t* callback;

    for (uint8_t i = 0; i < 8; i++) {
        if (mask & (1 << i)) {
            callback = &gpio_callbacks[(port << 3) + (1 << i)];
            if((*callback) != NULL) {
                (*callback)();
            }
        }
    }
}

/*=============================== interrupt =================================*/

void gpio_a_interrupt(void) {
    uint8_t pin_status;
    uint8_t pow_status;
    uint8_t status;

    pin_status = (uint8_t) GPIOPinIntStatus(GPIO_A_BASE, true);
    pow_status = (uint8_t) GPIOPowIntStatus(GPIO_A_BASE, true);

    GPIOPinIntClear(GPIO_A_BASE, pin_status);
    GPIOPowIntClear(GPIO_A_BASE, pow_status);

    status = pin_status | pow_status;

    gpio_notify_callback(GPIO_A_PORT, status);
}

void gpio_b_interrupt(void) {
    uint8_t pin_status;
    uint8_t pow_status;
    uint8_t status;

    pin_status = (uint8_t) GPIOPinIntStatus(GPIO_B_BASE, true);
    pow_status = (uint8_t) GPIOPowIntStatus(GPIO_B_BASE, true);

    GPIOPinIntClear(GPIO_B_BASE, pin_status);
    GPIOPowIntClear(GPIO_B_BASE, pow_status);

    status = pin_status | pow_status;

    gpio_notify_callback(GPIO_B_PORT, status);
}

void gpio_c_interrupt(void) {
    uint8_t pin_status;
    uint8_t pow_status;
    uint8_t status;

    pin_status = (uint8_t) GPIOPinIntStatus(GPIO_C_BASE, true);
    pow_status = (uint8_t) GPIOPowIntStatus(GPIO_C_BASE, true);

    GPIOPinIntClear(GPIO_C_BASE, pin_status);
    GPIOPowIntClear(GPIO_C_BASE, pow_status);

    status = pin_status | pow_status;

    gpio_notify_callback(GPIO_C_PORT, status);
}

void gpio_d_interrupt(void) {
    uint8_t pin_status;
    uint8_t pow_status;
    uint8_t status;

    pin_status = (uint8_t) GPIOPinIntStatus(GPIO_D_BASE, true);
    pow_status = (uint8_t) GPIOPowIntStatus(GPIO_D_BASE, true);

    GPIOPinIntClear(GPIO_D_BASE, pin_status);
    GPIOPowIntClear(GPIO_D_BASE, pow_status);

    status = pin_status | pow_status;

    gpio_notify_callback(GPIO_D_PORT, status);
}

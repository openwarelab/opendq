/**
 * @file       cpu.c
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

#include "cpu.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

static void cpu_gpio_init(void);
static void cpu_clocks_init(void);

/*================================= public ==================================*/

void cpu_init(void) {
    cpu_gpio_init();
    cpu_clocks_init();
}

void cpu_wait(void) {
    SysCtrlSleep();
}

void cpu_sleep(void) {
    SysCtrlDeepSleep();
}

void cpu_reset(void) {
    SysCtrlReset();
}

void cpu_enable_interrupts(void) {
    IntMasterEnable();
}

void cpu_disable_interrupts(void) {
    IntMasterDisable();
}

void cpu_delay_us(uint32_t delay_us) {
    SysCtrlDelay(delay_us);
}

/*================================ private ==================================*/

static void cpu_gpio_init(void) {
    GPIOPinTypeGPIOOutput(GPIO_A_BASE, 0xFF);
    GPIOPinTypeGPIOOutput(GPIO_B_BASE, 0xFF);
    GPIOPinTypeGPIOOutput(GPIO_C_BASE, 0xFF);
    GPIOPinTypeGPIOOutput(GPIO_D_BASE, 0xFF);

    GPIOPinWrite(GPIO_A_BASE, 0xFF, 0x00);
    GPIOPinWrite(GPIO_B_BASE, 0xFF, 0x00);
    GPIOPinWrite(GPIO_C_BASE, 0xFF, 0x00);
    GPIOPinWrite(GPIO_D_BASE, 0xFF, 0x00);
}

static void cpu_clocks_init(void) {
    /**
     * Configure the 32 kHz pins, PD6 and PD7, for crystal operation
     * By default they are configured as GPIOs
     */
    GPIODirModeSet(GPIO_D_BASE, 0x40, GPIO_DIR_MODE_IN);
    GPIODirModeSet(GPIO_D_BASE, 0x80, GPIO_DIR_MODE_IN);
    IOCPadConfigSet(GPIO_D_BASE, 0x40, IOC_OVERRIDE_ANA);
    IOCPadConfigSet(GPIO_D_BASE, 0x80, IOC_OVERRIDE_ANA);

    /**
     * Set the real-time clock to use the 32.768 kHz external crystal
     * Set the system clock to use the 32 MHz external crystal at 32 MHz
     */
    SysCtrlClockSet(true, false, SYS_CTRL_SYSDIV_32MHZ);

    /**
     * Set the IO clock to operate at 32 MHz
     * This way peripherals can run while the system clock is gated
     */
    SysCtrlIOClockSet(SYS_CTRL_SYSDIV_32MHZ);

    /**
     * Wait until the 32 MHz oscillator becomes stable
     */
    while (!((HWREG(SYS_CTRL_CLOCK_STA)) & (SYS_CTRL_CLOCK_STA_XOSC_STB)));
}

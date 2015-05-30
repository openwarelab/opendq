/**
 * @file       uart.c
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

#include "uart.h"

/*================================ define ===================================*/

#define UART_PERIPHERAL         ( SYS_CTRL_PERIPH_UART0 )
#define UART_BASE               ( UART0_BASE )
#define UART_CLOCK              ( UART_CLOCK_PIOSC )
#define UART_INTERRUPT          ( INT_UART0 )
#define UART_RX_PORT            ( GPIO_A_BASE )
#define UART_RX_PIN             ( GPIO_PIN_0 )
#define UART_RX_IOC             ( IOC_UARTRXD_UART0 )
#define UART_TX_PORT            ( GPIO_A_BASE )
#define UART_TX_PIN             ( GPIO_PIN_1 )
#define UART_TX_IOC             ( IOC_MUX_OUT_SEL_UART0_TXD )
#define UART_BAUDRATE           ( 115200 )
#define UART_CONFIG             ( UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE )
#define UART_INT_MODE           ( UART_TXINT_MODE_EOT )

/*================================ typedef ==================================*/

typedef struct {
    uart_cb_t uart_rx_cb;
    uart_cb_t uart_tx_cb;
} uart_vars_t;

/*=============================== variables =================================*/

static uart_vars_t uart_vars;

/*=============================== prototypes ================================*/

/*================================= public ==================================*/

/**
 * Initialize the UART interface
 */
void uart_init(void) {
    // Initialize the memory of the uart variables
    memset(&uart_vars, 0, sizeof(uart_vars_t));

    // Enable peripheral except in deep sleep modes (e.g. LPM1, LPM2, LPM3)
    SysCtrlPeripheralEnable(UART_PERIPHERAL);
    SysCtrlPeripheralSleepEnable(UART_PERIPHERAL);
    SysCtrlPeripheralDeepSleepDisable(UART_PERIPHERAL);

    // Disable peripheral previous to configuring it
    UARTDisable(UART_PERIPHERAL);

    // Set IO clock as UART clock source
    UARTClockSourceSet(UART_BASE, UART_CLOCK);

    // Configure the UART RX and TX pins
    IOCPinConfigPeriphInput(UART_RX_PORT, UART_RX_PIN, UART_RX_IOC);
    IOCPinConfigPeriphOutput(UART_TX_PORT, UART_TX_PIN, UART_TX_IOC);

    // Configure the UART GPIOs
    GPIOPinTypeUARTInput(UART_RX_PORT, UART_RX_PIN);
    GPIOPinTypeUARTOutput(UART_TX_PORT, UART_TX_PIN);

    // Configure the UART
    UARTConfigSetExpClk(UART_BASE, SysCtrlIOClockGet(), UART_BAUDRATE, UART_CONFIG);

    // Disable FIFO as we only use a one-byte buffer
    UARTFIFODisable(UART_BASE);

    // Raise an interrupt at the end of transmission
    UARTTxIntModeSet(UART_BASE, UART_INT_MODE);

    // Enable UART hardware
    UARTEnable(UART_BASE);
}

/**
 * Deinitialize the UART interface
 */
void uart_deinit(void) {
    // Wait until UART is not busy
    while(UARTBusy(UART_BASE))
        ;

    // Disable UART hardware
    UARTDisable(UART_BASE);

    // Configure the pins as outputs
    GPIOPinTypeGPIOOutput(UART_RX_PORT, UART_RX_PIN);
    GPIOPinTypeGPIOOutput(UART_TX_PORT, UART_TX_PIN);

    // Pull the pins to ground
    GPIOPinWrite(UART_RX_PORT, UART_RX_PIN, 0);
    GPIOPinWrite(UART_TX_PORT, UART_TX_PIN, 0);
}

void uart_enable_interrupts(void) {
    // Enable the UART peripheral interrupts
    UARTIntEnable(UART_BASE, UART_INT_TX | UART_INT_RX | UART_INT_RT);

    // Set the UART peripheral interrupt priority
    IntPrioritySet(UART_INTERRUPT, (5 << 5));

    // Enable the UART global interrupt
    IntEnable(UART_INTERRUPT);
}

void uart_disable_interrupts(void) {
    // Disable the UART peripheral interrupts
    UARTIntDisable(UART_BASE, UART_INT_TX | UART_INT_RX | UART_INT_RT);

    // Disable the UART global interrupt
    IntDisable(UART_INTERRUPT);
}

void uart_register_rx_cb(uart_cb_t callback) {
    uart_vars.uart_rx_cb = callback;
}

void uart_register_tx_cb(uart_cb_t callback) {
    uart_vars.uart_tx_cb = callback;
}

void uart_cancel_rx_cb(void) {
    uart_vars.uart_rx_cb = NULL;
}

void uart_cancel_tx_cb(void) {
    uart_vars.uart_tx_cb = NULL;
}

void uart_send_byte(uint8_t byte) {
    UARTCharPutNonBlocking(UART_BASE, byte);
}

void uart_send_bytes(uint8_t* buffer, uint32_t length) {
    // Write n bytes to the UART
    for (uint32_t i = 0; i < length; i++) {
        UARTCharPut(UART_BASE, *buffer++);
    }

    // Wait until it is complete
    while(UARTBusy(UART_BASE))
        ;
}

uint8_t uart_receive_byte(void) {
    int32_t byte;
    byte = UARTCharGetNonBlocking(UART_BASE);
    return (uint8_t)(byte & 0xFF);
}

uint8_t uart_receive_bytes(uint8_t* buffer, uint32_t length) {
    uint32_t data;

    // Read n bytes from the UART
    for (uint32_t i = 0; i < length; i++) {
        data = UARTCharGet(UART_BASE);
        *buffer++ = (uint8_t)data;
    }

    // Wait until it is complete
    while(UARTBusy(UART_BASE))
        ;

    return 0;
}

void uart_rx_interrupt(void) {
    if (uart_vars.uart_rx_cb != NULL) {
        uart_vars.uart_rx_cb();
    }
}

void uart_tx_interrupt(void) {
    if (uart_vars.uart_tx_cb != NULL) {
        uart_vars.uart_tx_cb();
    }
}

/*================================ private ==================================*/

void uart0_interrupt(void) {
    uint32_t status;

    // Read interrupt source
    status = UARTIntStatus(UART_BASE, true);

    // Process TX interrupt
    if (status & UART_INT_TX) {
        UARTIntClear(UART_BASE, UART_INT_TX);
        uart_tx_interrupt();
    }

    // Process RX interrupt
    if ((status & UART_INT_RX) || (status & UART_INT_RT)) {
        UARTIntClear(UART_BASE, UART_INT_RX | UART_INT_RT);
        uart_rx_interrupt();
    }
}

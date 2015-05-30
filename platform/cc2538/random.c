/**
 * @file       random.c
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

#include "random.h"

/*================================ define ===================================*/

// Defines for the CSP (Command Strobe Processor)
#define CC2538_RF_CSP_OP_ISRXON         ( 0xE3 )
#define CC2538_RF_CSP_OP_ISRFOFF        ( 0xEF )

// Send an RX ON command strobe to the CSP
#define CC2538_RF_CSP_ISRXON() do {   \
 HWREG(RFCORE_SFR_RFST) = CC2538_RF_CSP_OP_ISRXON; \
} while(0)

// Send a RF OFF command strobe to the CSP
#define CC2538_RF_CSP_ISRFOFF() do { \
 HWREG(RFCORE_SFR_RFST) = CC2538_RF_CSP_OP_ISRFOFF; \
} while(0)

// Defines for the ADC
#define SOC_ADC_ADCCON1_RCTRL0          ( 0x00000004 )
#define SOC_ADC_ADCCON1_RCTRL1          ( 0x00000008 )

/*================================ typedef ==================================*/

typedef struct {
    uint16_t shift_reg;
} random_vars_t;

/*=============================== variables =================================*/

static random_vars_t random_vars;

/*=============================== prototypes ================================*/

static uint16_t random_seed(void);

/*================================= public ==================================*/

void random_init(void) {
    // Initialize the memory of the random variables
    memset(&random_vars, 0, sizeof(random_vars_t));

    // Initialize the shift register
    random_vars.shift_reg = random_seed();
}

uint16_t random_get(void) {
    uint16_t random_value = 0;

    // Galois shift register with taps: 16, 14, 13, 11
    // Characteristic polynomial: x^16 + x^14 + x^13 + x^11 + 1
    for (uint8_t i = 0; i < 16; i++) {
        random_value |= (random_vars.shift_reg & 0x01) << i;
        random_vars.shift_reg = (random_vars.shift_reg >> 1)
                ^ (-(int16_t) (random_vars.shift_reg & 1) & 0xB400);
    }

    return random_value;
}

/*================================ private ==================================*/

static uint16_t random_seed(void) {
    uint16_t seed = 0;

    // Enable the radio clock
    HWREG(SYS_CTRL_RCGCRFC) = 1;

    // Wait until the radio is clocked
    while (HWREG(SYS_CTRL_RCGCRFC) != 1);

    // Turn the radio on and receive
    CC2538_RF_CSP_ISRXON();

    // Wait until the RSSI valid signal is high
    while (!(HWREG(RFCORE_XREG_RSSISTAT) & RFCORE_XREG_RSSISTAT_RSSI_VALID));

    // Sample the radio until the seed is valid (0x0000 and 0x8003 are not valid)
    while (seed == 0x0000 || seed == 0x8003) {
        for (uint8_t i = 0; i < 16; i++) {
            seed  |= (HWREG(RFCORE_XREG_RFRND) & RFCORE_XREG_RFRND_IRND);
            seed <<= 1;
        }
    }

    // Turn the radio off
    CC2538_RF_CSP_ISRFOFF();

    return seed;
}

/*=============================== interrupt =================================*/

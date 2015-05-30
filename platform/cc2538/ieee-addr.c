/**
 * @file       ieee-addr.c
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

#include "ieee-addr.h"

/*================================ define ===================================*/

// Defines for the EUI64 address
#define CC2538_EUI64_ADDRESS_HI_H               ( 0x0028002F )
#define CC2538_EUI64_ADDRESS_HI_L               ( 0x0028002C )
#define CC2538_EUI64_ADDRESS_LO_H               ( 0x0028002B )
#define CC2538_EUI64_ADDRESS_LO_L               ( 0x00280028 )

/*================================ typedef ==================================*/

typedef struct {
    uint8_t eui64_addr[8];
} ieee_addr_vars_t;

/*=============================== variables =================================*/

static ieee_addr_vars_t ieee_addr_vars;

/*=============================== prototypes ================================*/

static void ieee_addr_init_eui64(uint8_t* address);

/*================================= public ==================================*/

void ieee_addr_init(void) {
    ieee_addr_init_eui64(ieee_addr_vars.eui64_addr);
}

void ieee_addr_get_eui16(uint16_t* address) {
    *address = (ieee_addr_vars.eui64_addr[6] << 8) | (ieee_addr_vars.eui64_addr[7] << 0);
}

/*================================ private ==================================*/

static void ieee_addr_init_eui64(uint8_t* address) {
    uint8_t* eui64_flash;

    eui64_flash = (uint8_t*) CC2538_EUI64_ADDRESS_LO_H;
    while (eui64_flash >= (uint8_t*) CC2538_EUI64_ADDRESS_LO_L) {
        *address++ = *eui64_flash--;
    }

    eui64_flash = (uint8_t*) CC2538_EUI64_ADDRESS_HI_H;
    while (eui64_flash >= (uint8_t*) CC2538_EUI64_ADDRESS_HI_L) {
        *address++ = *eui64_flash--;
    }
}

/*=============================== interrupt =================================*/

/**
 * @file       library.c
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

/*================================ include ==================================*/

#include "config.h"

#include "library.h"

#include "board.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

/*================================= public ==================================*/

/* Function to initialize the library */
void library_init(void) {
    // Initialize the queue manager
    packet_buffer_init();

    // Initialize the virtual timer
    virtual_timer_init();

    // Initialize the serial port
    serial_init();
}

/*================================ private ==================================*/

/**
 * @file       main.c
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

/*================================ include ==================================*/

#include "types.h"

#include "board.h"
#include "leds.h"

#include "library.h"
#include "scheduler.h"

#include "wor.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

/*================================= public ==================================*/
int main(void) {
    // Initialize the basic components
    board_init();
    library_init();
    mac_init();
    scheduler_init();

    // Push the WOR task to the scheduler
    wor_set_cb(mac_start);
    scheduler_push(wor_config, TASK_PRIO_MAX);

    // Start the scheduler
    scheduler_start();
}

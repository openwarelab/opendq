/**
 * @file       serial.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef SERIAL_H_
#define SERIAL_H_

/*================================ include ==================================*/

#include "types.h"
#include "scheduler.h"

/*================================ define ===================================*/

/*================================ typedef ==================================*/

typedef void (* serial_cb_t)(void);

typedef struct {
    uint8_t type;
    uint16_t address;
    uint8_t* data;
    uint8_t length;
} serial_packet_t;

typedef enum {
    SERIAL_MOTE2PC_DATA  = (uint8_t) 'D',
    SERIAL_MOTE2PC_RESET = (uint8_t) 'R'
} serial_mote2pc_t;

typedef enum {
    SERIAL_PC2MOTE_START = (uint8_t) 'A',
    SERIAL_PC2MOTE_STOP  = (uint8_t) 'O'
} serial_pc2mote_t;

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void serial_init(void);
void serial_reset(void);
bool serial_is_busy(void);

void serial_register_mote2pc_cb(serial_mote2pc_t serial_mote2pc, serial_cb_t serial_cb, task_prio_t task_prio);
void serial_register_pc2mote_cb(serial_pc2mote_t serial_pc2mote, serial_cb_t serial_cb, task_prio_t task_prio);

void serial_push_msg(uint8_t command, uint8_t* message, uint8_t size);
void serial_parse_msg(serial_packet_t* serial_packet);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* SERIAL_H_ */

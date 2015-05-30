/**
 * @file       hdlc.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef HDLC_H_
#define HDLC_H_

/*================================ include ==================================*/

#include "types.h"

/*================================ define ===================================*/

#define HDLC_HEADER_SIZE            ( 4 ) // Flag (1) + Command (1) + Address (2)
#define HDLC_FOOTER_SIZE            ( 3 ) // CRC (2) + Flag (1)

#define HDLC_FLAG                   ( 0x7E )
#define HDLC_ESCAPE                 ( 0x7D )
#define HDLC_ESCAPE_MASK            ( 0x20 )

/*================================ typedef ==================================*/

typedef enum {
    HDLC_STATUS_IDLE  = 0x00,
    HDLC_STATUS_BUSY  = 0x01,
    HDLC_STATUS_DONE  = 0x02,
    HDLC_STATUS_ERROR = 0x03
} hdlc_status_t;

typedef enum {
    HDLC_CRC_INCORRECT = 0x00,
    HDLC_CRC_CORRECT   = 0x01
} hdlc_crc_t;

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

void hdlc_open_rx(uint8_t* buffer, uint8_t* size);
hdlc_status_t hdlc_put_rx(uint8_t byte);
hdlc_crc_t hdlc_close_rx(void);

void hdlc_open_tx(uint8_t* buffer, uint8_t* size);
void hdlc_put_tx(uint8_t byte);
void hdlc_close_tx(void);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* HDLC_H_ */

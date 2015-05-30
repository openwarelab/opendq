/**
 * @file       mac.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef MAC_H_
#define MAC_H_

/*================================ include ==================================*/

#include "config.h"

#include "types.h"
#include "packet_buffer.h"

/*================================ define ===================================*/

#define MAC_NONE                        ( 0x00 )
#define MAC_FSA                         ( 0x01 )
#define MAC_DQ                          ( 0x02 )

#define MAC_GATEWAY                     ( 0x01 )
#define MAC_NODE                        ( 0x02 )

#define MAC_RADIO_IDLE_TX               ( 6 ) // 192 us
#define MAC_RADIO_TX_IDLE               ( 0 )
#define MAC_RADIO_IDLE_RX               ( 6 ) // 192 us
#define MAC_RADIO_RX_IDLE               ( 0 )
#define MAC_RADIO_PHY_HEADER            ( 4 ) // 128 us

#define MAC_DEFAULT_CHANNEL             ( 26 )

/*================================ typedef ==================================*/

typedef enum {
    MAC_STATE_UNSYNC = 0x00,
    MAC_STATE_SYNC   = 0x01
} mac_state_t;

typedef enum {
    MAC_TYPE_NONE = 0x00,
    MAC_TYPE_FSA  = 0x01,
    MAC_TYPE_DQ   = 0x02,
} mac_type_t;

typedef enum {
    MAC_PACKET_NONE = 0x00,
    MAC_PACKET_WOR  = 0x01,
    MAC_PACKET_ARP  = 0x02,
    MAC_PACKET_FBP  = 0x03,
    MAC_PACKET_DATA = 0x04,
    MAC_PACKET_ACK  = 0x05
} mac_packet_t;

typedef enum {
    MAC_ARP_EMPTY   = 0x00,
    MAC_ARP_ERROR   = 0x01,
    MAC_ARP_SUCCESS = 0x02
} mac_arp_state_t;

typedef enum {
    MAC_DATA_EMPTY   = 0x00,
    MAC_DATA_ERROR   = 0x01,
    MAC_DATA_SUCCESS = 0x02
} mac_data_state_t;

typedef enum {
    MAC_RSSI_NONE  = 0x00,
    MAC_RSSI_BELOW = 0x01,
    MAC_RSSI_ABOVE = 0x02
} mac_rssi_t;

typedef enum {
    MAC_ADDR_NONE  = 0x0000,
    MAC_ADDR_BCAST = 0xFFFF
} mac_addr_type_t;

typedef uint16_t mac_address_t;
typedef uint16_t mac_seq_number_t;
typedef uint16_t mac_time_t;
typedef uint8_t  mac_channel_t;
typedef uint8_t  mac_slots_t;

typedef struct {
    mac_type_t mac_type;
    mac_packet_t mac_packet;
    mac_state_t mac_state;

    mac_time_t mac_time;
    mac_slots_t mac_slots;
    mac_channel_t mac_channel;

    packet_buffer_t* queue_mac_rx;
    packet_buffer_t* queue_mac_tx;
} mac_vars_t;

/*=============================== variables =================================*/

extern mac_vars_t mac_vars;

/*=============================== prototypes ================================*/

void mac_init(void);
void mac_start(void);
void mac_set_type(mac_type_t type);
void mac_set_channel(mac_channel_t channel);
void mac_set_time(mac_time_t time);
void mac_toggle_synchronized(mac_state_t mac_state);
uint8_t mac_next_channel(void);

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* MAC_H_ */

/**
 * @file       openmote-cc2538.h
 * @author     Pere Tuset-Peiro (peretuset@openmote.com)
 * @version    v0.1
 * @date       May 2015
 * @brief
 *
 * @copyright  Copyright 2015, OpenMote Technologies, S.L.
 *             This file is licensed under the GNU General Public License v2.
 */

#ifndef OPENMOTE_CC2538_
#define OPENMOTE_CC2538_

/*================================ include ==================================*/

/*================================ define ===================================*/

#define LED_RED_PORT            ( GPIO_C_PORT )
#define LED_RED_PIN             ( GPIO_PIN_4 )

#define LED_ORANGE_PORT         ( GPIO_C_PORT )
#define LED_ORANGE_PIN          ( GPIO_PIN_5 )

#define LED_YELLOW_PORT         ( GPIO_C_PORT )
#define LED_YELLOW_PIN          ( GPIO_PIN_6 )

#define LED_GREEN_PORT          ( GPIO_C_PORT )
#define LED_GREEN_PIN           ( GPIO_PIN_7 )

#define GPIO_DEBUG_AD0_PORT     ( GPIO_D_PORT ) // AD0/DIO0, CLOCKS
#define GPIO_DEBUG_AD0_PIN      ( GPIO_PIN_3 )

#define GPIO_DEBUG_AD1_PORT     ( GPIO_D_PORT ) // AD1/DIO1, SYSTEM
#define GPIO_DEBUG_AD1_PIN      ( GPIO_PIN_2 )

#define GPIO_DEBUG_AD2_PORT     ( GPIO_D_PORT ) // AD2/DIO2, RADIO
#define GPIO_DEBUG_AD2_PIN      ( GPIO_PIN_1 )

#define GPIO_DEBUG_AB2_PORT     ( GPIO_B_PORT ) // DTR/DI8, ISR
#define GPIO_DEBUG_AB2_PIN      ( GPIO_PIN_2 )

#define GPIO_DEBUG_AB3_PORT     ( GPIO_B_PORT ) // PWM1, ERROR
#define GPIO_DEBUG_AB3_PIN      ( GPIO_PIN_3 )

#define GPIO_DEBUG_AB5_PORT     ( GPIO_B_PORT ) // DO8, USER
#define GPIO_DEBUG_AB5_PIN      ( GPIO_PIN_5 )

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

/*=============================== prototypes ================================*/

/*================================= public ==================================*/

/*================================ private ==================================*/

#endif /* OPENMOTE_CC2538_ */

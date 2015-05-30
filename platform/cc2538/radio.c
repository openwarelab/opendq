/**
 * @file       radio.c
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

#include "debug.h"
#include "radio.h"

/*================================ define ===================================*/

// Defines for the transmit power
#define CC2538_RF_TX_POWER_DEFAULT              ( 0xD5 )

// Defines for the channel
#define CC2538_RF_CHANNEL_MIN                   ( 11 )
#define CC2538_RF_CHANNEL_MAX                   ( 26 )
#define CC2538_RF_CHANNEL_DEFAULT               ( 17 )
#define CC2538_RF_CHANNEL_SPACING               ( 5 )

// Defines for the RSSI
#define CC2538_RF_RSSI_OFFSET                   ( 73 )

// Defines for the CRC and LQI
#define CC2538_RF_CRC_BITMASK                   ( 0x80 )
#define CC2538_RF_LQI_BITMASK                   ( 0x7F )

// Defines for the packet
#define CC2538_RF_MAX_PACKET_LEN                ( 127 )
#define CC2538_RF_MIN_PACKET_LEN                ( 3 )

// Defines for the CCA (Clear Channel Assessment)
#define CC2538_RF_CCA_CLEAR                     ( 0x01 )
#define CC2538_RF_CCA_BUSY                      ( 0x00 )
#define CC2538_RF_CCA_THRESHOLD                 ( 0xF8 )

// Defines for the CSP (Command Strobe Processor)
#define CC2538_RF_CSP_OP_ISRXON                 ( 0xE3 )
#define CC2538_RF_CSP_OP_ISTXON                 ( 0xE9 )
#define CC2538_RF_CSP_OP_ISTXONCCA              ( 0xEA )
#define CC2538_RF_CSP_OP_ISRFOFF                ( 0xEF )
#define CC2538_RF_CSP_OP_ISFLUSHRX              ( 0xED )
#define CC2538_RF_CSP_OP_ISFLUSHTX              ( 0xEE )

// Send an RX ON command strobe to the CSP
#define CC2538_RF_CSP_ISRXON() do {   \
 HWREG(RFCORE_SFR_RFST) = CC2538_RF_CSP_OP_ISRXON; \
} while(0)

// Send a TX ON command strobe to the CSP
#define CC2538_RF_CSP_ISTXON() do { \
 HWREG(RFCORE_SFR_RFST) = CC2538_RF_CSP_OP_ISTXON; \
} while(0)

// Send a RF OFF command strobe to the CSP
#define CC2538_RF_CSP_ISRFOFF() do { \
 HWREG(RFCORE_SFR_RFST) = CC2538_RF_CSP_OP_ISRFOFF; \
} while(0)

// Flush the RX FIFO
#define CC2538_RF_CSP_ISFLUSHRX() do { \
  HWREG(RFCORE_SFR_RFST) = CC2538_RF_CSP_OP_ISFLUSHRX; \
  HWREG(RFCORE_SFR_RFST) = CC2538_RF_CSP_OP_ISFLUSHRX; \
} while(0)

// Flush the TX FIFO
#define CC2538_RF_CSP_ISFLUSHTX() do { \
  HWREG(RFCORE_SFR_RFST) = CC2538_RF_CSP_OP_ISFLUSHTX; \
  HWREG(RFCORE_SFR_RFST) = CC2538_RF_CSP_OP_ISFLUSHTX; \
} while(0)

/*================================ typedef ==================================*/

/*=============================== variables =================================*/

radio_vars_t radio_vars;

/*=============================== prototypes ================================*/

/*================================= public ==================================*/

void radio_init(void) {
    /* Initialize the memory of the radio variables */
    memset(&radio_vars, 0, sizeof(radio_vars_t));

    /* Enable peripheral except in deep sleep modes (e.g. LPM1, LPM2, LPM3) */
    SysCtrlPeripheralEnable(SYS_CTRL_PERIPH_RFC);
    SysCtrlPeripheralSleepEnable(SYS_CTRL_PERIPH_RFC);
    SysCtrlPeripheralDeepSleepDisable(SYS_CTRL_PERIPH_RFC);

    /* Adjust for optimal radio performance */
    HWREG(RFCORE_XREG_MDMCTRL1)  = 0x14;
    HWREG(RFCORE_XREG_RXCTRL)    = 0x3F;

    /* Adjust current in synthesizer */
    HWREG(RFCORE_XREG_FSCTRL)    = 0x55;

    /* Tune sync word detection by requiring two zero symbols before the sync word */
    HWREG(RFCORE_XREG_MDMCTRL0)  = 0x85;

    /* Adjust current in VCO */
    HWREG(RFCORE_XREG_FSCAL1)    = 0x01;

    /* Adjust target value for AGC control loop */
    HWREG(RFCORE_XREG_AGCCTRL1)  = 0x15;

    /* Tune ADC performance */
    HWREG(RFCORE_XREG_ADCTEST0)  = 0x10;
    HWREG(RFCORE_XREG_ADCTEST1)  = 0x0E;
    HWREG(RFCORE_XREG_ADCTEST2)  = 0x03;

    /* Update CCA register to -81 dB */
    HWREG(RFCORE_XREG_CCACTRL0)  = 0xF8;

    /* Set transmit anti-aliasing filter bandwidth */
    HWREG(RFCORE_XREG_TXFILTCFG) = 0x09;

    /* Set AGC target value */
    HWREG(RFCORE_XREG_AGCCTRL1)  = 0x15;

    /* Set bias currents */
    HWREG(ANA_REGS_O_IVCTRL)     = 0x0B;

    /* Disable the CSPT register compare function */
    HWREG(RFCORE_XREG_CSPT)      = 0xFFUL;

    /* Enable automatic CRC calculation and RSSI append */
    HWREG(RFCORE_XREG_FRMCTRL0)  = RFCORE_XREG_FRMCTRL0_AUTOCRC;

    /* Disable frame filtering */
    HWREG(RFCORE_XREG_FRMFILT0) &= ~RFCORE_XREG_FRMFILT0_FRAME_FILTER_EN;

    /* Disable source address matching and autopend */
    HWREG(RFCORE_XREG_SRCMATCH)  = 0;

    /* Set maximum FIFOP threshold */
    HWREG(RFCORE_XREG_FIFOPCTRL) = CC2538_RF_MAX_PACKET_LEN;

    /* Flush transmit and receive */
    CC2538_RF_CSP_ISFLUSHRX();
    CC2538_RF_CSP_ISFLUSHTX();

    /* Set default transmit power and channel */
    HWREG(RFCORE_XREG_TXPOWER)   = CC2538_RF_TX_POWER_DEFAULT;
    HWREG(RFCORE_XREG_FREQCTRL)  = CC2538_RF_CHANNEL_MIN;

    /* Update the radio state */
    radio_vars.current_state = RADIO_OFF;
}

void radio_idle(void) {
    /* Wait for ongoing TX to complete (e.g. this could be an outgoing ACK) */
    while (HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_TX_ACTIVE)
        ;

    /* Don't turn off if we are off as this will trigger a Strobe Error */
    if (HWREG(RFCORE_XREG_RXENABLE) != 0) {
        /* Turn off the radio */
        CC2538_RF_CSP_ISRFOFF();
    }

    /* Update the radio state */
    radio_vars.current_state = RADIO_IDLE;
}

void radio_receive(void) {
    /* Flush the RX buffer */
    CC2538_RF_CSP_ISFLUSHRX();

    /* Set the radio state to receive */
    radio_vars.current_state = RADIO_RX_ENABLING;

    /* Enable receive mode */
    CC2538_RF_CSP_ISRXON();

    /* Busy-wait until radio really listening */
    while(!((HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_RX_ACTIVE)))
        ;

    /* Set the radio state to receive */
    radio_vars.current_state = RADIO_RX_ENABLED;
}

void radio_transmit(void) {
    /* Make sure we are not transmitting already */
    while(HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_TX_ACTIVE)
        ;

    /* Set the radio state to transmit */
    radio_vars.current_state = RADIO_TX_ENABLING;

    /* Enable transmit mode */
    CC2538_RF_CSP_ISTXON();

    /* Busy-wait until radio really transmitting */
    while(!((HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_TX_ACTIVE)))
        ;

    /* Set the radio state to transmit */
    radio_vars.current_state = RADIO_TX_ENABLED;
}

void radio_reset(void) {
    /* Wait for ongoing TX to complete (e.g. this could be an outgoing ACK) */
    while (HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_TX_ACTIVE)
        ;

    /* Flush the RX and TX buffers */
    CC2538_RF_CSP_ISFLUSHRX();
    CC2538_RF_CSP_ISFLUSHTX();

    /* Don't turn off if we are off since this will trigger a Strobe Error */
    if (HWREG(RFCORE_XREG_RXENABLE) != 0) {
        /* Turn off the radio */
        CC2538_RF_CSP_ISRFOFF();
    }

    /* Update the radio state */
    radio_vars.current_state = RADIO_OFF;
}

void radio_set_rx_cb(radio_cb_t rx_init_cb, radio_cb_t rx_done_cb) {
    radio_vars.rx_init = rx_init_cb;
    radio_vars.rx_done = rx_done_cb;
}

void radio_set_tx_cb(radio_cb_t tx_init_cb, radio_cb_t tx_done_cb) {
    radio_vars.tx_init = tx_init_cb;
    radio_vars.tx_done = tx_done_cb;
}

void radio_cancel_rx_cb(void) {
    radio_vars.rx_init = NULL;
    radio_vars.rx_done = NULL;
}

void radio_cancel_tx_cb(void) {
    radio_vars.tx_init = NULL;
    radio_vars.tx_done = NULL;
}

void radio_enable_interrupts(void) {
    /* Enable RF interrupts 0, RXPKTDONE, SFD and FIFOP only -- see page 751  */
    HWREG(RFCORE_XREG_RFIRQM0) |= ((0x06 | 0x02 | 0x01) << RFCORE_XREG_RFIRQM0_RFIRQM_S) & RFCORE_XREG_RFIRQM0_RFIRQM_M;

    /* Enable RF interrupts 1, TXDONE only */
    HWREG(RFCORE_XREG_RFIRQM1) |= ((0x02) << RFCORE_XREG_RFIRQM1_RFIRQM_S) & RFCORE_XREG_RFIRQM1_RFIRQM_M;

    /* Enable RF error interrupts */
    // HWREG(RFCORE_XREG_RFERRM) = RFCORE_XREG_RFERRM_RFERRM_M;

    /* Set the RF interrupt interrupt priority */
    IntPrioritySet(INT_RFCORERTX, (6 << 5));
    // IntPrioritySet(INT_RFCOREERR, (7 << 5));

    /* Enable radio interrupts */
    IntEnable(INT_RFCORERTX);
    // IntEnable(INT_RFCOREERR);
}

void radio_disable_interrupts(void) {
    /* Disable RF interrupts 0, RXPKTDONE, SFD and FIFOP only -- see page 751  */
    HWREG(RFCORE_XREG_RFIRQM0) = 0;

    /* Disable RF interrupts 1, TXDONE only */
    HWREG(RFCORE_XREG_RFIRQM1) = 0;

    /* Disable the radio interrupts */
    IntDisable(INT_RFCORERTX);
    // IntDisable(INT_RFCOREERR);
}

void radio_set_channel(uint8_t channel) {
    /* Check that the channel is within bounds */
    if (!(channel < CC2538_RF_CHANNEL_MIN) || !(channel > CC2538_RF_CHANNEL_MAX))
    {
        /* Changes to FREQCTRL take effect after the next recalibration */
        HWREG(RFCORE_XREG_FREQCTRL) = (CC2538_RF_CHANNEL_MIN +
                                      (channel - CC2538_RF_CHANNEL_MIN) * CC2538_RF_CHANNEL_SPACING);
    }
}

void radio_set_power(uint8_t power) {
    /* Set the radio transmit power */
    HWREG(RFCORE_XREG_TXPOWER) = power;
}

/* Gets a packet from the radio buffer */
void radio_get_packet(packet_buffer_t* packet_buffer) {
    uint8_t packet_length;
    uint8_t scratch;

    if (radio_vars.current_state != RADIO_RX_DONE) {
        return;
    }

    /* Check the packet length (first byte) */
    packet_length = HWREG(RFCORE_SFR_RFDATA);

    /* Check if packet is too long or too short */
    if ((packet_length > CC2538_RF_MAX_PACKET_LEN) ||
        (packet_length <= CC2538_RF_MIN_PACKET_LEN)) {
        /* Flush the RX buffer */
        CC2538_RF_CSP_ISFLUSHRX();

        return;
    }

    /* Account for the CRC bytes */
    packet_length -= 2;

    /* Check if the packet fits in the buffer */
    if (packet_length > packet_buffer->size) {
        /* Flush the RX buffer */
        CC2538_RF_CSP_ISFLUSHRX();

        return;
    }

    /* Copy the RX buffer to the buffer (except for the CRC) */
    for (uint8_t i = 0; i < packet_length; i++) {
        packet_buffer->payload[i] = HWREG(RFCORE_SFR_RFDATA);
    }

    /* Update the packet length */
    packet_buffer->length = packet_length;

    /* Update the packet RSSI */
    packet_buffer->rssi = ((int8_t) (HWREG(RFCORE_SFR_RFDATA)) - CC2538_RF_RSSI_OFFSET);

    /* Update the packet CRC and RSSI */
    scratch            = HWREG(RFCORE_SFR_RFDATA);
    packet_buffer->crc = scratch & CC2538_RF_CRC_BITMASK;
    packet_buffer->lqi = scratch & CC2538_RF_LQI_BITMASK;

    /* Flush the RX buffer */
    CC2538_RF_CSP_ISFLUSHRX();

    /* Set the radio state to idle */
    radio_vars.current_state = RADIO_IDLE;
}

/* Puts a packet to the radio buffer */
void radio_put_packet(packet_buffer_t* packet_buffer) {
    uint8_t packet_length;

    /* Make sure previous transmission is not still in progress */
    while (HWREG(RFCORE_XREG_FSMSTAT1) & RFCORE_XREG_FSMSTAT1_TX_ACTIVE)
        ;

    /* Check if the radio state is correct */
    if (radio_vars.current_state != RADIO_IDLE) {
        return;
    }

    /* Account for the CRC bytes */
    packet_length = packet_buffer->length + 2;

    /* Check if packet is too long */
    if ((packet_length >  CC2538_RF_MAX_PACKET_LEN) ||
        (packet_length <= CC2538_RF_MIN_PACKET_LEN)) {
        return;
    }

    /* Flush the TX buffer */
    CC2538_RF_CSP_ISFLUSHTX();

    /* Append the PHY length to the TX buffer */
    HWREG(RFCORE_SFR_RFDATA) = packet_length;

    /* Append the packet payload to the TX buffer */
    for (uint8_t i = 0; i < packet_length; i++) {
        HWREG(RFCORE_SFR_RFDATA) = packet_buffer->buffer[i];
    }
}

void radio_read_rssi(int8_t* rssi) {
    // Wait until the RSSI is valid
    while(!(HWREG(RFCORE_XREG_RSSISTAT) & RFCORE_XREG_RSSISTAT_RSSI_VALID));

    // Read the RSSI value
    *rssi = ((int8_t) (HWREG(RFCORE_XREG_RSSI)) - CC2538_RF_RSSI_OFFSET);
}

/*================================ private ==================================*/

void rf_core_interrupt(void) {
    uint32_t irq_status0, irq_status1;

    debug_isr_on();

    /* Read RFCORE_STATUS */
    irq_status0 = HWREG(RFCORE_SFR_RFIRQF0);
    irq_status1 = HWREG(RFCORE_SFR_RFIRQF1);

    /* Clear interrupt flags */
    HWREG(RFCORE_SFR_RFIRQF0) = 0;
    HWREG(RFCORE_SFR_RFIRQF1) = 0;

    /* STATUS0 Register: Start of frame event */
    if ((irq_status0 & RFCORE_SFR_RFIRQF0_SFD) == RFCORE_SFR_RFIRQF0_SFD) {
        if (radio_vars.current_state == RADIO_RX_ENABLED &&
            radio_vars.rx_init != NULL) {
            radio_vars.current_state = RADIO_RX_RECEIVING;
            radio_vars.rx_init();
        }
        else if (radio_vars.current_state == RADIO_TX_ENABLED &&
                 radio_vars.tx_init != NULL) {
            radio_vars.current_state = RADIO_TX_TRANSMITTING;
            radio_vars.tx_init();
        }
        else {
            // radio_idle();
        }
    }

    /* STATUS0 Register: End of frame event */
    if (((irq_status0 & RFCORE_SFR_RFIRQF0_RXPKTDONE) ==  RFCORE_SFR_RFIRQF0_RXPKTDONE)) {
        if (radio_vars.current_state == RADIO_RX_RECEIVING &&
            radio_vars.rx_done != NULL) {
            radio_vars.current_state = RADIO_RX_DONE;
            radio_vars.rx_done();
        }
        else {
            // radio_idle();
        }
    }

    /* STATUS0 Register: FIFO is full event */
    if (((irq_status0 & RFCORE_SFR_RFIRQF0_FIFOP) ==  RFCORE_SFR_RFIRQF0_FIFOP)) {
        // radio_idle();
    }

    /* STATUS1 Register: End of frame event */
    if (((irq_status1 & RFCORE_SFR_RFIRQF1_TXDONE) == RFCORE_SFR_RFIRQF1_TXDONE)) {
        if (radio_vars.current_state == RADIO_TX_TRANSMITTING &&
            radio_vars.tx_done != NULL) {
            radio_vars.current_state = RADIO_TX_DONE;
            radio_vars.tx_done();
        }
        else {
            // radio_idle();
        }
    }

    debug_isr_off();
}

void rf_error_interrupt(void) {
    uint32_t irq_error;

    /* Read RFERR_STATUS */
    irq_error = HWREG(RFCORE_SFR_RFERRF);

    /* Clear interrupt flags and mask */
    HWREG(RFCORE_SFR_RFERRF) = 0;
    HWREG(RFCORE_XREG_RFERRM) = 0;
}

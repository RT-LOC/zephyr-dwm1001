/*! ----------------------------------------------------------------------------
 *  @file    ex_01e_tx_with_cca.c
 *  @brief   Here we implement a simple Clear Channel Assessment (CCA) mechanism
 *           before frame transmission. The CCA can be used to avoid collisions
 *           with other frames on the air. See Note 1 for more details.
 *
 *           Note this is not doing CCA the way a continuous carrier radio would do it by
 *           looking for energy/carrier in the band. It is only looking for preamble so
 *           will not detect PHR or data phases of the frame. In a UWB data network it
 *           is advised to also do a random back-off before re-transmission in the event
 *           of not receiving acknowledgement to a data frame transmission.
 *
 *           This example has been designed to operate with Continuous Frame example (CF).
 *           The CF device will fill the air with frames which will be detected by the CCA
 *           and thus the CCA will cancel the transmission and will use back off to try
 *           sending again at later stage. This example will actually get to send when the
 *           CCA preamble detection overlaps with the data portion of the continuous TX
 *           or inter frame period. Note, the Continuous Frame example actually stops after 30s
 *           interval (thus the user should toggle the reset button on the unit running
 *           CF example to restart it if they wish to continue observing this pseudo CCA
 *           experiencing an environment of high air-utilisation). Thus the radio configuration
 *           used here matches that of CF example.
 * @attention
 *
 * Copyright 2018 (c) Decawave Ltd, Dublin, Ireland.
 * Copyright 2019 (c) Frederic Mes, RTLOC.
 *
 * All rights reserved.
 *
 * @author Decawave
 */
#ifdef EX_01E_DEF
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port.h"

/* Example application name and version to display on console. */
#define APP_NAME "TX + CCA  v1.1"

/* Default communication configuration. */
static dwt_config_t config = {                                                                        
    5,               /* Channel number. */
    DWT_PRF_64M,     /* Pulse repetition frequency. */                                                
    DWT_PLEN_128,    /* Preamble length. Used in TX only. */                                           
    DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */                           
    9,               /* TX preamble code. Used in TX only. */                                         
    9,               /* RX preamble code. Used in RX only. */
    1,               /* 0 to use standard SFD, 1 to use non-standard SFD. */                          
    DWT_BR_6M8,      /* Data rate. */
    DWT_PHRMODE_EXT, /* PHY header mode. */
    (129)            /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */           
};

/* The frame sent in this example is an 802.15.4e standard blink. It is a 12-byte frame composed of the following fields:
 *     - byte 0: frame type (0xC5 for a blink).
 *     - byte 1: sequence number, incremented for each new frame.
 *     - byte 2 -> 9: device ID, see NOTE 2 below.
 *     - byte 10/11: frame check-sum, automatically set by DW1000.  */
static uint8 tx_msg[] = {0xC5, 0, 'D', 'E', 'C', 'A', 'W', 'A', 'V', 'E', 0, 0};
/* Index to access to sequence number of the blink frame in the tx_msg array. */
#define BLINK_FRAME_SN_IDX 1

/* Inter-frame delay period, in milliseconds.
 * this example will try to transmit a frame every 100 ms*/
#define TX_DELAY_MS 100

/* initial backoff period when failed to transmit a frame due to preamble detection */
#define INITIAL_BACKOFF_PERIOD 400 /* This constant would normally smaller (e.g. 1ms), however here it is set to 400 ms so that
                                    * user can see (on console) the report that the CCA detects a preamble on the air occasionally.
                                    * and is doing a TX back-off.
                                    */

int tx_sleep_period; /* Sleep period until the next TX attempt */
int next_backoff_interval = INITIAL_BACKOFF_PERIOD; /* Next backoff in the event of busy channel detection by this pseudo CCA algorithm */

/* String used to display measured distance on console (16 characters maximum). */
char console_str[17] = {0};

/* holds copy of status register */
uint32 status_reg = 0;

/**
 * Application entry point.
 */
int dw_main(void)
{

    /* Display application name on console. */
    printk(APP_NAME);

    /* Configure DW1000 SPI */
    openspi();

    /* Reset and initialise DW1000. See NOTE 3 below.
     * For initialisation, DW1000 clocks must be temporarily set to crystal speed. After initialisation SPI rate can be increased for optimum
     * performance. */
    reset_DW1000(); /* Target specific drive of RSTn line into DW1000 low for a period. */
    port_set_dw1000_slowrate();
    if (dwt_initialise(DWT_LOADNONE) == DWT_ERROR)
    {
        printk("INIT FAILED");
        while (1)
        { };
    }
    port_set_dw1000_fastrate();

    /* Configure DW1000. See NOTE 4 below. */
    dwt_configure(&config);

    /* Can enable LEDs on EVB1000 and TX/RX states output for debug */
    dwt_setleds(DWT_LEDS_ENABLE);
    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

    /* Configure preamble timeout to 3 PACs; if no preamble detected in this time we assume channel is clear. See Note 5*/
    dwt_setpreambledetecttimeout(3);

    /* Loop forever sending frames periodically. */
    while(1)
    {
        /* local variables for console output, this holdes the result of the most recent channel assessment y pseudo CCA algorithm,
        * 1 (channel is clear) or 0 (preamble was detected) */
        int channel_clear;

        /* Write frame data to DW1000 and prepare transmission. See NOTE 6 below.*/
        dwt_writetxdata(sizeof(tx_msg), tx_msg, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(sizeof(tx_msg), 0, 0); /* Zero offset in TX buffer, no ranging. */

        /* Activate RX to perform CCA. */
        dwt_rxenable(DWT_START_RX_IMMEDIATE);
        /* Start transmission. Will be delayed (the above RX command has to finish first)
         *  until we get the preamble timeout or canceled by TRX OFF if a preamble is detected. */
        dwt_starttx(DWT_START_TX_IMMEDIATE);

        /* Poll DW1000 until preamble timeout or detection. See NOTE 7 below. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXPRD | SYS_STATUS_RXPTO)));

        if (status_reg & SYS_STATUS_RXPTO)
        {
            channel_clear = 1;
            /* Poll DW1000 until frame sent, see Note 8 below. */
            while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS));

            tx_sleep_period = TX_DELAY_MS; /* sent a frame - set interframe period */
            next_backoff_interval = INITIAL_BACKOFF_PERIOD; /* set initial backoff period */
            /* Increment the blink frame sequence number (modulo 256). */
            tx_msg[BLINK_FRAME_SN_IDX]++;
        }
        else
        {
            /* if DW IC detects the preamble, as we don't want to receive a frame we TRX OFF
               and wait for a backoff_period before trying to transmit again */
            dwt_forcetrxoff();

            tx_sleep_period = next_backoff_interval; /* set the TX sleep period */
            next_backoff_interval++; /* If failed to transmit, increase backoff and try again.
                               * In a real implementation the back-off is typically a randomised period
                               * whose range is an exponentially related to the number of successive failures.
                               * See https://en.wikipedia.org/wiki/Exponential_backoff */
            channel_clear = 0;
        }

        /* Note in order to see cca_result of 0 on the console, the backoff period is artificially set to 400 ms */
        sprintf(console_str, "CCA=%d   %d  \n", channel_clear, tx_sleep_period);
        printk(console_str);

        /* Execute a delay between transmissions. */
        Sleep(tx_sleep_period);

    }
}
#endif
/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. For Wireless Sensor Networks application, most of the MAC protocols rely on Clear Channel Assessment (CCA) to avoid collisions with other frames
 *    in the air. This consists in sampling the air for a short period to see if the medium is idle before transmitting. For most radios this involves
 *    looking for the RF carrier, but for UWB where this is not possible, one approach is to just look for preamble to avoid conflicting transmissions,
 *    since any sending of preamble during data will typically not disturb those receivers who are demodulating in data mode.
 *    The idea then is to sample the air for a small amount of time to see if a preamble can be detected, then if preamble is not seen the transmission
 *    is initiated, otherwise we defer the transmission typically for a random back-off period after which transmission is again attempted with CCA.
 *    Note: we return to idle for the back-off period and do not receive the frame whose preamble was detected, since the MAC (and upper layer) wants
 *    to transmit and not receive at this time.
 *    This example has been designed to operate with example 4b - Continuous Frame. The 4b device will fill the air with frames which will be detected by the CCA
 *    and thus the CCA will cancel the transmission and will use back off to try sending again at later stage.
 *    This example will actually get to send when the CCA preamble detection overlaps with the data portion of the continuous TX or inter frame period,
 *    Note the Continuous Frame example actually stops after 30s interval (thus the user should toggle the reset button on the unit running example 4b
 *    to restart it if they wish to continue observing this pseudo CCA experiencing an environment of high air-utilisation).
 * 2. The device ID is a hard coded constant in the blink to keep the example simple but for a real product every device should have a unique ID.
 *    For development purposes it is possible to generate a DW1000 unique ID by combining the Lot ID & Part Number values programmed into the
 *    DW1000 during its manufacture. However there is no guarantee this will not conflict with someone else�s implementation. We recommended that
 *    customers buy a block of addresses from the IEEE Registration Authority for their production items. See "EUI" in the DW1000 User Manual.
 * 3. In this example, LDE microcode is not loaded upon calling dwt_initialise(). This will prevent the IC from generating an RX timestamp. If
 *    time-stamping is required, DWT_LOADUCODE parameter should be used. See two-way ranging examples (e.g. examples 5a/5b).
 * 4. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW1000 OTP memory.
 * 5  The preamble timeout of 3 PACs is recommended as sufficient for this CCA example for all modes and data rates. The PAC size should be different
 *    when is different for different preamble configurations, as per User Manual guidelines.
 * 6. dwt_writetxdata() takes the full size of tx_msg as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
 *    automatically appended by the DW1000. This means that our tx_msg could be two bytes shorter without losing any data (but the sizeof would not
 *    work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()).
 * 7. We use polled mode of operation here to keep the example as simple as possible but the RXPRD and RXPTO status events can be used to generate an interrupt.
 *    Please refer to DW1000 User Manual for more details on "interrupts".
 * 8. We use polled mode of operation here to keep the example as simple as possible but the TXFRS status event can be used to generate an interrupt.
 *    Please refer to DW1000 User Manual for more details on "interrupts".
 * 9. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *    DW1000 API Guide for more details on the DW1000 driver functions.
 ****************************************************************************************************************************************************/

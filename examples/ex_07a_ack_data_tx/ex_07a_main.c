/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   Automatically acknowledged data TX example code
 *
 *           This is a simple code example that sends a frame and then turns on the DW1000 receiver to receive a response, expected to be an ACK
 *           frame as sent by the companion simple example "ACK DATA RX" example code. After the ACK is received, this application proceeds to the
 *           sending of the next frame (increasing the frame sequence number). If the expected valid ACK is not received, the application immediately
 *           retries to send the frame (keeping the same frame sequence number).
 *
 * @attention
 *
 * Copyright 2016 (c) Decawave Ltd, Dublin, Ireland.
 * Copyright 2019 (c) Frederic Mes, RTLOC.
 *
 * All rights reserved.
 *
 * @author Decawave
 */
#ifdef EX_07A_DEF
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port.h"

/* Example application name and version to display on console. */
#define APP_NAME "ACK DATA TX v1.1"

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

/* The frame sent in this example is a data frame encoded as per the IEEE 802.15.4-2011 standard. It is a 21-byte frame composed of the following
 * fields:
 *     - byte 0/1: frame control (0x8861 to indicate a data frame using 16-bit addressing and requesting ACK).
 *     - byte 2: sequence number, incremented for each new frame.
 *     - byte 3/4: PAN ID (0xDECA)
 *     - byte 5/6: destination address, see NOTE 2 below.
 *     - byte 7/8: source address, see NOTE 2 below.
 *     - byte 9 to 18: MAC payload, see NOTE 1 below.
 *     - byte 19/20: frame check-sum, automatically set by DW1000. */
static uint8 tx_msg[] = {0x61, 0x88, 0, 0xCA, 0xDE, 'X', 'R', 'X', 'T', 'm', 'a', 'c', 'p', 'a', 'y', 'l', 'o', 'a', 'd', 0, 0};
/* Index to access the sequence number and frame control fields in frames sent and received. */
#define FRAME_FC_IDX 0
#define FRAME_SN_IDX 2
/* ACK frame control value. */
#define ACK_FC_0 0x02
#define ACK_FC_1 0x00

/* Inter-frame delay period, in milliseconds. */
#define TX_DELAY_MS 1000

/* Receive response timeout, expressed in UWB microseconds (UUS, 1 uus = 512/499.2 �s). See NOTE 3 below. */
#define RX_RESP_TO_UUS 2200

/* Buffer to store received frame. See NOTE 4 below. */
#define ACK_FRAME_LEN 5
static uint8 rx_buffer[ACK_FRAME_LEN];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;

/* Hold copy of frame length of frame received (if good) so that it can be examined at a debug breakpoint. */
static uint16 frame_len = 0;

/* ACK status for last transmitted frame. */
static int tx_frame_acked = 0;

/* Counters of frames sent, frames ACKed and frame retransmissions. See NOTE 1 below. */
static uint32 tx_frame_nb = 0;
static uint32 tx_frame_ack_nb = 0;
static uint32 tx_frame_retry_nb = 0;

/**
 * Application entry point.
 */
int dw_main(void)
{
    /* Display application name on console. */
    printk(APP_NAME);

    /* Configure DW1000 SPI */
    openspi();

    /* Reset and initialise DW1000.
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

    /* Configure DW1000. See NOTE 5 below. */
    dwt_configure(&config);

    /* Configure DW1000 LEDs */
    dwt_setleds(1);
    
    /* Set delay to turn reception on immediately after transmission of the frame. See NOTE 6 below. */
    dwt_setrxaftertxdelay(0);

    /* Set RX frame timeout for the response. */
    dwt_setrxtimeout(RX_RESP_TO_UUS);

    /* Loop forever transmitting data. */
    while (1)
    {
        /* TESTING BREAKPOINT LOCATION #1 */

        /* Write frame data to DW1000 and prepare transmission. See NOTE 7 below.*/
        dwt_writetxdata(sizeof(tx_msg), tx_msg, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(sizeof(tx_msg), 0, 0); /* Zero offset in TX buffer, no ranging. */

        /* Start transmission, indicating that a response is expected so that reception is enabled immediately after the frame is sent. */
        dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

        /* We assume that the transmission is achieved normally, now poll for reception of a frame or error/timeout. See NOTE 8 below. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
        { };

        if (status_reg & SYS_STATUS_RXFCG)
        {
            /* Clear good RX frame event in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);

            /* A frame has been received, check frame length is correct for ACK, then read and verify the ACK. */
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
            if (frame_len == ACK_FRAME_LEN)
            {
                dwt_readrxdata(rx_buffer, frame_len, 0);

                /* Check if it is the expected ACK. */
                if ((rx_buffer[FRAME_FC_IDX] == ACK_FC_0) && (rx_buffer[FRAME_FC_IDX + 1] == ACK_FC_1)
                    && (rx_buffer[FRAME_SN_IDX] == tx_msg[FRAME_SN_IDX]))
                {
                    tx_frame_acked = 1;
                }
            }
        }
        else
        {
            /* Clear RX error/timeout events in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
        }

        /* Update number of frames sent. */
        tx_frame_nb++;

        if (tx_frame_acked)
        {
            /* Execute a delay between transmissions. See NOTE 1 below. */
            Sleep(TX_DELAY_MS);

            /* Increment the sent frame sequence number (modulo 256). */
            tx_msg[FRAME_SN_IDX]++;

            /* Update number of frames acknowledged. */
            tx_frame_ack_nb++;
        }
        else
        {
            /* Update number of retransmissions. */
            tx_frame_retry_nb++;
        }

        /* Reset acknowledged frame flag. */
        tx_frame_acked = 0;
    }
}
#endif
/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. This example can be turned into a high speed data transfer test by removing the delay executed between two successful transmissions. The
 *    communication configuration and MAC payload size in the message can be changed to see the effect of the different parameters on the throughput
 *    (which can be computed using the different counters provided in the application). For example using the debugger to stop at the start of the
 *    while loop, and then timing from the "GO" for a few minutes before breaking in again, and examining the frame counters.
 * 2. Source and destination addresses are hard coded constants to keep the example simple but for a real product every device should have a unique ID.
 *    For development purposes it is possible to generate a DW1000 unique ID by combining the Lot ID & Part Number values programmed into the DW1000
 *    during its manufacture. However there is no guarantee this will not conflict with someone else�s implementation. We recommended that customers
 *    buy a block of addresses from the IEEE Registration Authority for their production items. See "EUI" in the DW1000 User Manual.
 * 3. This timeout is for complete reception of a frame, i.e. timeout duration must take into account the length of the expected frame. Here the value
 *    is arbitrary but chosen large enough to make sure that there is enough time to receive a complete ACK frame sent by the "ACK DATA RX" example
 *    at the 110k data rate used (around 2 ms).
 * 4. In this example, receive buffer is set to the exact size of the only frame we want to handle but 802.15.4 UWB standard maximum frame length is
 *    127 bytes. DW1000 also supports an extended frame length (up to 1023 bytes long) mode which is not used in this example..
 * 5. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW1000 OTP memory.
 * 6. TX to RX delay is set to 0 to activate reception immediately after transmission, as the companion "ACK DATA RX" example is configured to send
 *    the ACK immediately after reception of the frame sent by this example application.
 * 7. dwt_writetxdata() takes the full size of tx_msg as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
 *    automatically appended by the DW1000. This means that our tx_msg could be two bytes shorter without losing any data (but the sizeof would not
 *    work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()).
 * 8. We use polled mode of operation here to keep the example as simple as possible but all status events can be used to generate interrupts. Please
 *    refer to DW1000 User Manual for more details on "interrupts".
 * 9. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *    DW1000 API Guide for more details on the DW1000 driver functions.
 ****************************************************************************************************************************************************/

/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   Wake up a low-power listening node (i.e. the companion example to this).
 *
 *           This example is a companion to example 9a "Low-power listening RX". It sends a wake-up sequence of back-to-back frames, designed to wake
 *           the Low-Power Listening Receiver (companion example), and then waits for a response from the awakened device to show that the wake-up was
 *           successful (in a real application this response might be the start of a longer interaction). The code then sleeps for a set time period
 *           representing the time until we want to wake-up a device again (in a real application wake-ups would typically be based on an infrequent
 *           need to communicate with a low-power listening node). In this example we alternate the destination address every other wake-up to show
 *           that the low-power listening Receiver (companion example) awakes but does not respond when the wake-up is not for it.
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
#ifdef EX_08B_DEF
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port.h"

/* Example application name and version to display on console. */
#define APP_NAME "LPLISTEN TX v1.1"

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

/* This example will send a wake-up sequence composed of several frames sent back-to-back. These frames are data frames encoded as per the MAC layer
 * definition in the IEEE 802.15.4-2011 standard. This wake-up sequence frame is a 14-byte frame composed of the following fields:
 *     - byte 0/1: frame control (0x8841 to indicate a data frame using 16-bit addressing).
 *     - byte 2: sequence number, incremented for each new frame.
 *     - byte 3/4: PAN ID (0xDECA).
 *     - byte 5/6: 16-bit destination address. See NOTE 2 below.
 *     - byte 7/8: 16-bit source address. See NOTE 2 below.
 *     - byte 9: byte 1 of MAC payload with a proprietary message encoding, here we use octet 0xE0 to indicate this is a message of the wake-up sequence.
 *     - byte 10/11: bytes 2,3 of MAC payload (for this proprietary message) is the number of frames remaining to be sent in this wake-up sequence.
 *     - byte 12/13: frame check-sum, automatically set by DW1000.  */
static uint8 wus_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'X', 'R', 'X', 'T', 0xE0, 0, 0, 0, 0};
/* Defines to access different fields in an ISO/IEC 24730-62:2013 standard data frame. */
#define DATA_FRAME_SEQ_NB_IDX 2
#define DATA_FRAME_DEST_ADDR_IDX 5
#define DATA_FRAME_WUS_CNTDWN_IDX 10

/* Number of frames composing the wake-up sequence. See NOTE 3 below. */
#define WUS_FRAME_NB 1350

/* Interaction period (after wake-up sequence) maximum duration, in UWB microseconds (equivalent to 50 ms). */
#define INTERACTION_PERIOD_MAX_TIME_UUS 48750

/* Delay between wake-up sequences, in milliseconds. See NOTE 4 below. */
#define TX_DELAY_MS 5000

/* Buffer to store received frame. See NOTE 5 below. */
#define RX_BUF_LEN 127
static uint8 rx_buffer[RX_BUF_LEN];

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;

/**
 * Application entry point.
 */
int dw_main(void)
{
    /* Display application name on console. */
    printk(APP_NAME);

    /* Configure DW1000 SPI */
    openspi();

    /* Reset and initialise DW1000. See NOTE 6 below.
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

    /* Configure DW1000. See NOTE 7 below. */
    dwt_configure(&config);

    /* Set frame wait timeout for wake-up sequence response from companion example. */
    dwt_setrxtimeout(INTERACTION_PERIOD_MAX_TIME_UUS);

    /* Configure DW1000 LEDs */
    dwt_setleds(1);
    
    /* Outer loop, (periodically) sending wake-up sequence, consisting of many back-to-back frames, and then waiting for response from a woken-up
     * device. Then idling/sleeping until we want to wake-up a device again. */
    while(1)
    {
        uint16 frame_counter = WUS_FRAME_NB;
        /* Loop to send the wake-up frames one after another, until we have sent all WUS_FRAME_NB of them. See NOTE 8 below. */
        while (frame_counter--)
        {
            /* Write frame counter in the frame. */
            wus_msg[DATA_FRAME_WUS_CNTDWN_IDX] = frame_counter & 0xFF;
            wus_msg[DATA_FRAME_WUS_CNTDWN_IDX + 1] = frame_counter >> 8;

            /* Write frame data to DW1000 and prepare transmission. See NOTE 9 below.*/
            dwt_writetxdata(sizeof(wus_msg), wus_msg, 0); /* Zero offset in TX buffer. */
            dwt_writetxfctrl(sizeof(wus_msg), 0, 0); /* Zero offset in TX buffer, no ranging. */

            /* Start transmission. */
            dwt_starttx(DWT_START_TX_IMMEDIATE);

            /* Poll DW1000 until TX frame sent event set. See NOTE 10 below.
             * STATUS register is 5 bytes long but we are not interested in the high byte here, so we read a more manageable 32-bits with this API
             * call. */
            while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS))
            { };

            /* Clear TX frame sent event. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

            /* Increment the frame sequence number (modulo 256). */
            wus_msg[DATA_FRAME_SEQ_NB_IDX]++;
        }

        /* Having completed the sending of the wake-up sequence, we now activate the receiver to await the response from the awakened device (in a
         * real application this response might be the start of an interaction and not just a single message). */
        dwt_rxenable(DWT_START_RX_IMMEDIATE);

        /* Poll for reception of a frame or error/timeout. See NOTE 10 below.
         * STATUS register is 5 bytes long but we are not interested in the high byte here, so we read a more manageable 32-bits with this API call. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
        { };

        if (status_reg & SYS_STATUS_RXFCG)
        {
            uint32 frame_len;

            /* Clear RX good frame event in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);

            /* A frame has been received, read it into the local buffer. */
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_MASK;
            if (frame_len <= RX_BUF_LEN)
            {
                /* TESTING BREAKPOINT LOCATION #1 */

                dwt_readrxdata(rx_buffer, frame_len, 0);
            }
        }
        else
        {
            /* Clear RX error/timeout events in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
        }

        /* Execute a delay before sending the next wake-up sequence. */
        Sleep(TX_DELAY_MS);

        /* Change destination address to a dummy address every second wake-up sequence so that we can exercise the companion "low-power listening"
         * example, through the case where the wake-up is for it, and the case where the wake-up relates to another node. */
        if (wus_msg[DATA_FRAME_DEST_ADDR_IDX] == 'X') /* For simplicity we just change one octet of the address. */
        {
            wus_msg[DATA_FRAME_DEST_ADDR_IDX] = 0;
        }
        else
        {
            wus_msg[DATA_FRAME_DEST_ADDR_IDX] = 'X';
        }
    }
}
#endif
/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. When using low-power listening mode, communication parameters have to be chosen carefully so that the wake-up sequence can be detected with good
 *    reliability by the listener and, at the same time, allow it not to burn too much power when receiving a frame. The configuration used in this
 *    example is a good compromise between these needs: the preamble is long enough to ease the detection of frames and the use of 6.8 Mbps data rate
 *    makes the data part of each frame quite short.
 * 2. Source and destination addresses are hard coded constants in this example to keep it simple but for a real product every device should have a
 *    unique ID. Here, 16-bit addressing is used to keep the messages as short as possible but, in an actual application, this should be done only
 *    after an exchange of specific messages used to define those short addresses for each device participating to the ranging exchange.
 * 3. The total duration of the wake-up sequence must be long enough to make sure it overlaps at least one companion example�s listening period. The
 *    number of frame here has been determined accordingly to the "long sleep" phase duration hard-coded in companion example.
 * 4. In a real application the frequency of wake-ups would typically be much larger, i.e. dependent on some unpredictable infrequent need to
 *    communicate to a low-power listening node.
 * 5. In this example, maximum frame length is set to 127 bytes which is 802.15.4 UWB standard maximum frame length. DW1000 supports an extended
 *    frame length (up to 1023 bytes long) mode which is not used in this example.
 * 6. In this example, LDE microcode is not loaded upon calling dwt_initialise(). This will prevent the IC from generating an RX timestamp. If
 *    time-stamping is required, DWT_LOADUCODE parameter should be used. See two-way ranging examples (e.g. examples 5a/5b).
 * 7. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW1000 OTP memory.
 * 8. Frames composing the wake-up sequence are sent back-to-back with the shortest inter-frame spacing (IFS) possible. Here, IFS is typically
 *    determined by the time the host processor needs to process the loop and issue the next TX start. This could be improved by optimising the code
 *    executed by the loop, especially by taking advantage of the TX buffer offset feature of the DW1000: several frames can be written in the buffer
 *    at once and sent one by one just by changing the TX buffer offset the DW1000 will read at when sending the frame. Later frames to send can be
 *    written in the TX buffer while waiting for the current frame to be sent, as long as they are not written to the part of the TX buffer the DW1000
 *    is reading for its current transmission.
 * 9. dwt_writetxdata() takes the full size of tx_msg as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
 *    automatically appended by the DW1000. This means that our tx_msg could be two bytes shorter without losing any data (but the sizeof would not
 *    work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()).
 * 10. We use polled mode of operation here to keep the example as simple as possible but the status events can be used to generate an interrupt.
 *     Please refer to DW1000 User Manual for more details on "interrupts".
 * 11. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *     DW1000 API Guide for more details on the DW1000 driver functions.
 ****************************************************************************************************************************************************/

/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   Automatically Acknowledged data RX example code
 *
 *           This is a simple code example that turns on the DW1000 receiver to receive a frame, (expecting the frame as sent by the companion simple
 *           example "ACK DATA TX"). The DW1000 is configured so that when a correctly addressed data frame is received with the ACK request (AR) bit
 *           set in the frame control field, the DW1000 will automatically respond with an ACK frame. The code loops after each frame reception to
 *           await another frame.
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
#ifdef EX_07B_DEF
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port.h"

/* Example application name and version to display on console. */
#define APP_NAME "ACK DATA RX v1.1"

/* Default communication configuration. We use here EVK1000's default mode (mode 3). */
static dwt_config_t config = {
    2,               /* Channel number. */
    DWT_PRF_64M,     /* Pulse repetition frequency. */
    DWT_PLEN_1024,   /* Preamble length. Used in TX only. */
    DWT_PAC32,       /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    1,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_110K,     /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    (1025 + 64 - 32) /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
};

/* PAN ID/EUI/short address. See NOTE 1 and 2 below. */
static uint16 pan_id = 0xDECA;
static uint8 eui[] = {'A', 'C', 'K', 'D', 'A', 'T', 'R', 'X'};
static uint16 short_addr = 0x5258; /* "RX" */

/* Buffer to store received frame. See NOTE 3 below. */
#define FRAME_LEN_MAX 127
static uint8 rx_buffer[FRAME_LEN_MAX];

/* ACK request bit mask in DATA and MAC COMMAND frame control's first byte. */
#define FCTRL_ACK_REQ_MASK 0x20

/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;

/* Hold copy of frame length of frame received (if good) so that it can be examined at a debug breakpoint. */
static uint16 frame_len = 0;

/**
 * Application entry point.
 */
int dw_main(void)
{
    /* Display application name on console. */
    printk(APP_NAME);

    /* Configure DW1000 SPI */
    openspi();

    /* Reset and initialise DW1000. See NOTE 4 below.
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

    /* Set PAN ID, EUI and short address. See NOTE 2 below. */
    dwt_setpanid(pan_id);
    dwt_seteui(eui);
    dwt_setaddress16(short_addr);

    /* Configure frame filtering. Only data frames are enabled in this example. Frame filtering must be enabled for Auto ACK to work. */
    dwt_enableframefilter(DWT_FF_DATA_EN);

    /* Activate auto-acknowledgement. Time is set to 0 so that the ACK is sent as soon as possible after reception of a frame. */
    dwt_enableautoack(0);

    /* Configure DW1000 LEDs */
    dwt_setleds(1);
    
    /* Loop forever receiving frames. */
    while (1)
    {
        /* Activate reception immediately. See NOTE 6 below. */
        dwt_rxenable(0);

        /* Poll until a frame is properly received or an RX error occurs. See NOTE 7 below.
         * STATUS register is 5 bytes long but we are not interested in the high byte here, so we read a more manageable 32-bits with this API call. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_ERR)))
        { };

        if (status_reg & SYS_STATUS_RXFCG)
        {
            /* Clear good RX frame event in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);

            /* A frame has been received, read it into the local buffer. */
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
            if (frame_len <= FRAME_LEN_MAX)
            {
                dwt_readrxdata(rx_buffer, frame_len, 0);
            }

            /* TESTING BREAKPOINT LOCATION #1 */

            /* Since the auto ACK feature is enabled, an ACK should be sent if the received frame requests it, so we await the ACK TX completion
             * before taking next action. See NOTE 8 below. */
            if (rx_buffer[0] & FCTRL_ACK_REQ_MASK)
            {
                /* Poll DW1000 until confirmation of transmission of the ACK frame. */
                while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & SYS_STATUS_TXFRS))
                { };

                /* Clear TXFRS event. */
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
            }
        }
        else
        {
            /* Clear RX error events in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
        }
    }
}
#endif
/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. PAN ID, EUI and short address are hard coded constants to keep the example simple but for a real product every device should have a unique ID.
 *    For development purposes it is possible to generate a DW1000 unique ID by combining the Lot ID & Part Number values programmed into the DW1000
 *    during its manufacture. However there is no guarantee this will not conflict with someone else�s implementation. We recommended that customers
 *    buy a block of addresses from the IEEE Registration Authority for their production items. See "EUI" in the DW1000 User Manual.
 * 2. EUI64 is not actually used in this example but the DW1000 is set up with this dummy value, to have it set to something. This would be required
 *    for a real application, i.e. because short addresses (and PAN ID) are typically assigned by a PAN coordinator.
 * 3. In this example, maximum frame length is set to 127 bytes which is 802.15.4 UWB standard maximum frame length. DW1000 supports an extended frame
 *    length (up to 1023 bytes long) mode which is not used in this example.
 * 4. In this example, LDE microcode is not loaded upon calling dwt_initialise(). This will prevent the IC from generating an RX timestamp. If
 *    time-stamping is required, DWT_LOADUCODE parameter should be used. See two-way ranging examples (e.g. examples 5a/5b).
 * 5. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *    the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW1000 OTP memory.
 * 6. Manual reception activation is performed here but DW1000 offers several features that can be used to handle more complex scenarios or to
 *    optimise system's overall performance (e.g. timeout after a given time, automatic re-enabling of reception in case of errors, etc.).
 * 7. We use polled mode of operation here to keep the example as simple as possible but all status events can be used to generate interrupts. Please
 *    refer to DW1000 User Manual for more details on "interrupts".
 * 8. This is the purpose of the AAT bit in DW1000's STATUS register but because of an issue with the operation of AAT, it is simpler to directly
 *    check in the frame control if the ACK request bit is set. Please refer to DW1000 User Manual for more details on Auto ACK feature and the AAT
 *    bit.
 * 9. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *    DW1000 API Guide for more details on the DW1000 driver functions.
 ****************************************************************************************************************************************************/

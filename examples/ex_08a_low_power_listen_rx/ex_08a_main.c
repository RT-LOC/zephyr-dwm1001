/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   RX using low-power listening mode example code
 *
 *           This example application implements a scheme called "low-power listening". Low-power listening is a feature whereby the DW1000 is
 *           predominantly in the SLEEP state but wakes periodically for a very short time to sample the air for a preamble sequence. If a preamble is
 *           detected, the reception of the whole frame is performed normally by the DW1000. The device can then decide to interrupt low-power
 *           listening to enter any interaction mode needed. Low-power listening is then best used to infrequently wake-up a device among a population.
 *           A device using low-power listening can be woken-up using a "wake-up sequence" formed by several standard frames sent back-to-back.
 *
 *           Low-power listening scheme is formed by the repetition of the 4 following phases:
 *               - SLEEP state phase ("long sleep")
 *               - first RX ON phase
 *               - SNOOZE state phase ("short sleep")
 *               - second RX ON phase. This second phase is needed because there is a probability that the first RX ON phase happens during the
 *                 transmission of the SFD/PHR/DATA part of the current frame in the wake-up sequence or during the IFS between two frames. If the
 *                 "short sleep" duration is correctly defined depending on communication configuration and frame length, this second RX ON phase will
 *                 ensure that a preamble in the wake-up sequence can be detected.
 *
 *           See "Low-Power Listening" section in User Manual for more details.
 *
 *           This example sets up low-power listening mode and awaits to be woken-up by a wake-up sequence as sent by the companion example 9b
 *           "Low-power listening TX". When such a frame is received, this example checks if it is the intended recipient of the wake-up sequence. If
 *           so, it sleeps until the end of the wake-up sequence and then takes part in a subsequent interaction period (in this example this
 *           interaction is just a single frame transmission). Then after completing the interaction it reenters the low-power listening state. If the
 *           received wake-up sequence is addressed to another node, we sleep for a period sufficiently long that the wake up sequence and subsequent
 *           interaction are complete before we reactivate the low-power listening.
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
#ifdef EX_08A_DEF
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port.h"

/* Example application name and version to display on console. */
#define APP_NAME "LPLISTEN RX v1.1"

/* Default communication configuration. See NOTE 1 below. */
static dwt_config_t config = {
    2,                   /* Channel number. */
    DWT_PRF_16M,         /* Pulse repetition frequency. */
    DWT_PLEN_1024,       /* Preamble length. Used in TX only. */
    DWT_PAC16,           /* Preamble acquisition chunk size. Used in RX only. */
    3,                   /* TX preamble code. Used in TX only. */
    3,                   /* RX preamble code. Used in RX only. */
    0,                   /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_6M8,          /* Data rate. */
    DWT_PHRMODE_STD,     /* PHY header mode. */
    (1024 + 1 + 8 - 16)  /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
};

/* Crystal frequency, in hertz. */
#define XTAL_FREQ_HZ 38400000

/* RX ON time, expressed in multiples of PAC size.
 * The IC automatically adds 1 PAC so the RX ON time of 2 here gives 3 PAC times and, since the configuration (above) specifies DWT_PAC16, we get an
 * RX ON time of 3*16 symbols, or around 48 �s. See NOTE 2 below. */
#define LPL_RX_SNIFF_TIME 2

/* Snooze ("short sleep") time, expressed in multiples of 512/19.2 �s (~26.7 �s).
 * The IC automatically adds 1 to the value set so the snooze time of 4 here gives 5*512/19.2 �s (~133 �s). See NOTE 2 below. */
#define LPL_SHORT_SLEEP_SNOOZE_TIME 4

/* "Long sleep" time, in milliseconds. See NOTE 3 below. */
#define LONG_SLEEP_TIME_MS 1500

/* Interaction period (after wake-up sequence) maximum duration, in milliseconds. */
#define INTERACTION_PERIOD_MAX_TIME_MS 50

/* This example will respond to "Low-Power Listening TX" example's wake-up sequence with a data frame encoded as per the MAC layer definition in the
 * IEEE 802.15.4-2011 standard. This response is a 12-byte frame composed of the following fields:
 *     - byte 0/1: frame control (0x8841 to indicate a data frame using 16-bit addressing).
 *     - byte 2: sequence number, incremented for each new frame.
 *     - byte 3/4: PAN ID (0xDECA).
 *     - byte 5/6: 16-bit destination address. See NOTE 4 below.
 *     - byte 7/8: 16-bit source address. See NOTE 4 below.
 *     - byte 9: MAC payload with a proprietary message encoding, here we use a single octet 0xE1 to indicate our response to a wake-up sequence.
 *               See NOTE 5 below.
 *     - byte 10/11: frame check-sum, automatically set by DW1000.  */
static uint8 interaction_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'X', 'T', 'X', 'R', 0xE1, 0, 0};
/* Defines to access different fields in an ISO/IEC 24730-62:2013 standard data frame. */
#define DATA_FRAME_SEQ_NB_IDX 2
#define DATA_FRAME_PAN_ID_IDX 3
#define DATA_FRAME_DEST_ADDR_IDX 5
#define DATA_FRAME_SRC_ADDR_IDX 7
#define DATA_FRAME_APP_FCODE_IDX 9
#define DATA_FRAME_WUS_CNTDWN_IDX 10

/* Wake-up sequence frame duration including IFS, in microseconds. */
#define WUS_FRAME_TIME_US 1130

/* Buffer to store received frame. See NOTE 6 below. */
#define WUS_FRAME_LEN 14
static uint8 rx_buffer[WUS_FRAME_LEN];

/* Frame received flag, shared by main loop and RX callback.
 * This global static variable is used as the mechanism to signal events to the background main loop from the interrupt handler callback. */
static int rx_frame = 0;

/* Dummy buffer for DW1000 wake-up SPI read. See NOTE 7 below. */
#define DUMMY_BUFFER_LEN 600
static uint8 dummy_buffer[DUMMY_BUFFER_LEN];

/* Count the number of times low-power listening has been interrupted because of a frame that was not part of the expected wake-up sequence. This can
 * be examined at a debug breakpoint. */
static uint32 non_wus_frame_rx_nb = 0;

/* Declaration of static functions.
 * We use interrupts to handle the wake-up activation in low power listening. This RX callback does the main handling.*/
static void rx_ok_cb(const dwt_cb_data_t *cb_data);

/**
 * Application entry point.
 */
int dw_main(void)
{
    uint32 lp_osc_freq, sleep_cnt;


    /* Display application name on console. */
    printk(APP_NAME);

    /* Configure DW1000 SPI */
    openspi();

    /* Install the low-power listening ISR handler.
     * This is an interrupt service routine part of the driver that is specific to correctly handling the low-power listening wake-up. */
    port_set_deca_isr(dwt_lowpowerlistenisr);

    /* Reset and initialise DW1000. See NOTE 8 and 9 below.
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

    /* This is put here for testing, so that we can see the receiver ON/OFF pattern using an oscilloscope. */
    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

    /* Configure DW1000. See NOTE 10 below. */
    dwt_configure(&config);

    /* Configure DW1000 LEDs */
    dwt_setleds(1);

    /* Calibrate and configure sleep count. This has to be done with DW1000 clocks set to crystal speed.
     * This will define the duration of the "long sleep" phase. */
    port_set_dw1000_slowrate();
    lp_osc_freq = (XTAL_FREQ_HZ / 2) / dwt_calibratesleepcnt();
    sleep_cnt = ((LONG_SLEEP_TIME_MS * lp_osc_freq) / 1000) >> 12;
    dwt_configuresleepcnt(sleep_cnt);
    port_set_dw1000_fastrate();

    /* Configure sleep mode to allow low-power listening to operate properly. */
    dwt_configuresleep(DWT_PRESRV_SLEEP | DWT_CONFIG | DWT_RX_EN, DWT_WAKE_SLPCNT | DWT_SLP_EN);

    /* Set snooze time. This will define the duration of the "short sleep" phase. */
    dwt_setsnoozetime(LPL_SHORT_SLEEP_SNOOZE_TIME);

    /* Set preamble detect timeout. This will define the duration of the reception phases. */
    dwt_setpreambledetecttimeout(LPL_RX_SNIFF_TIME);

    /* Register RX call-back. */
    dwt_setcallbacks(NULL, &rx_ok_cb, NULL, NULL);

    /* Enable wanted interrupts (RX good frames only). */
    dwt_setinterrupt(DWT_INT_RFCG, 1);

    /* Enable low-power listening mode. */
    dwt_setlowpowerlistening(1);

    /* Go to sleep to trigger low-power listening mode. */
    dwt_entersleep();

    /* Loop forever receiving frames. */
    while (1)
    {
        uint16 wus_end_frame_nb;
        uint32 wus_end_time_ms;

        /* Wait for a frame to be received.
         * The user should look at the "rx_ok_cb" function below to see the next piece of RX handling before reading on through the rest of the main
         * line code here. */
        while (!rx_frame)
        { };

        /* Configure DW1000 for next sleep phases so we can go to DEEPSLEEP and wake with SPI CS wake-up. */
        dwt_configuresleep(DWT_PRESRV_SLEEP | DWT_CONFIG, DWT_WAKE_CS | DWT_SLP_EN);

        /* Read received frame's countdown to the end of the wake-up sequence. */
        wus_end_frame_nb = (rx_buffer[DATA_FRAME_WUS_CNTDWN_IDX + 1] << 8) + rx_buffer[DATA_FRAME_WUS_CNTDWN_IDX];
        wus_end_time_ms = (wus_end_frame_nb * WUS_FRAME_TIME_US) / 1000;

        /* Check that the wake-up sequence is destined to this application. */
        if ((rx_buffer[DATA_FRAME_DEST_ADDR_IDX + 1] == 'R') && (rx_buffer[DATA_FRAME_DEST_ADDR_IDX] == 'X'))
        {
            /* TESTING BREAKPOINT LOCATION #1 */

            /* Put the DW1000 to sleep. */
            dwt_entersleep();

            /* Wait for the end of the wake-up sequence. */
            Sleep(wus_end_time_ms);

            /* Wake DW1000 up. See NOTE 7 below. */
            dwt_spicswakeup(dummy_buffer, DUMMY_BUFFER_LEN);

            /* Write interaction message data to DW1000 and prepare transmission. See NOTE 11 below. */
            dwt_writetxdata(sizeof(interaction_msg), interaction_msg, 0); /* Zero offset in TX buffer. */
            dwt_writetxfctrl(sizeof(interaction_msg), 0, 0); /* Zero offset in TX buffer, no ranging. */

            /* Start transmission. */
            dwt_starttx(DWT_START_TX_IMMEDIATE);

            /* Poll DW1000 until TX frame sent event set. See NOTE 12 below.
             * STATUS register is 5 bytes long but we are not interested in the high byte here, so we read a more manageable 32-bits with this API
             * call. */
            while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS))
            { };

            /* Clear TX frame sent event. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

            /* Increment the frame sequence number (modulo 256). */
            interaction_msg[DATA_FRAME_SEQ_NB_IDX]++;
        }
        else
        {
            /* TESTING BREAKPOINT LOCATION #2 */

            /* Compute the time until the end of the interaction period (after end of the wake-up sequence). */
            uint32 sleep_time_ms = wus_end_time_ms + INTERACTION_PERIOD_MAX_TIME_MS;

            /* Put the DW1000 to sleep. */
            dwt_entersleep();

            /* Wait for the end of the interaction period. */
            Sleep(sleep_time_ms);

            /* Wake DW1000 up. See NOTE 7 below. */
            dwt_spicswakeup(dummy_buffer, DUMMY_BUFFER_LEN);
        }

        /* Go back to low-power listening.
         * Sleep mode must be reconfigured to allow low-power listening to operate properly as it has been modified earlier. */
        dwt_configuresleep(DWT_PRESRV_SLEEP | DWT_CONFIG | DWT_RX_EN, DWT_WAKE_SLPCNT | DWT_SLP_EN);
        dwt_setlowpowerlistening(1);
        dwt_entersleep();
        rx_frame = 0;
    };
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn rx_ok_callback()
 *
 * @brief Call-back to process RX good frames events
 *
 * @param  cb_data  call-back data
 *
 * @return  none
 */
static void rx_ok_cb(const dwt_cb_data_t *cb_data)
{
    if (cb_data->datalength == WUS_FRAME_LEN)
    {
        /* A frame of correct length to be a wake-up message has been received, copy it to our local buffer. */
        dwt_readrxdata(rx_buffer, cb_data->datalength, 0);

        /* Validate the frame is addressed to us from the expected sender and has the encoding of one of the wake-up sequence messages we expect.
         * Then signal the arrival of the wake-up message to the background main loop by setting the rx_frame event flag. */
        if ((rx_buffer[DATA_FRAME_PAN_ID_IDX + 1] == 0xDE) && (rx_buffer[DATA_FRAME_PAN_ID_IDX] == 0xCA)
            && (rx_buffer[DATA_FRAME_SRC_ADDR_IDX + 1] == 'T') && (rx_buffer[DATA_FRAME_SRC_ADDR_IDX] == 'X')
            && (rx_buffer[DATA_FRAME_APP_FCODE_IDX] == 0xE0))
        {
            rx_frame = 1;
        }
    }

    /* If the frame is not from the expected wake-up sequence, go back to low-power listening. */
    if (!rx_frame)
    {
        dwt_setlowpowerlistening(1); /* No need to reconfigure sleep mode here as it has not been modified since wake-up. */
        dwt_entersleep();
        non_wus_frame_rx_nb++;
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
 * 2. For the same reasons than in NOTE 1 above, timings of the reception/short sleep phase have to be chosen carefully. Typically, reception time
 *    should be at least 3 PACs to be able to detect preambles with good reliability and short sleep time should be equal to (or slightly more than)
 *    the length of the (SFD + PHR + DATA) part of the frames composing the wake-up sequence in addition to the IFS between these frames. This is
 *    needed to ensure that, if the first reception phase hits in the SFD/PHR/DATA/IFS part of a wake-up sequence's frame and thus does not detect it,
 *    the second reception phase will hit a preamble. This is how the timings have been defined here.
 * 3. The sleep counter is 16 bits wide but represents the upper 16 bits of a 28 bits counter. Thus the granularity of this counter is 4096 counts.
 *    Combined with the frequency of the internal RING oscillator being typically between 7 and 13 kHz, this means that the time granularity that we
 *    get when using the timed sleep feature is typically between 315 and 585 ms. As the sleep time calculated is rounded down to the closest integer
 *    number of sleep counts, this means that the actual sleep time can be significantly less than the one defined here.
 * 4. Source and destination addresses are hard coded constants in this example to keep it simple but for a real product every device should have a
 *    unique ID. Here, 16-bit addressing is used to keep the messages as short as possible but, in an actual application, this should be done only
 *    after an exchange of specific messages used to define those short addresses for each device participating to the ranging exchange.
 * 5. While a single unacknowledged transmission might be okay for some applications, in most a more involved interaction would typically occur after
 *    a wake-up.
 * 6. In this example, receive buffer is set to the exact size of the only frame we want to handle but 802.15.4 UWB standard maximum frame length is
 *    127 bytes. DW1000 also supports an extended frame length (up to 1023 bytes long) mode which is not used in this example.
 * 7. When using SPI chip select line to wake DW1000 up (by maintaining it low for at least 500 us), we need a buffer to collect the data that DW1000
 *    outputs during the corresponding dummy SPI transaction. The length of the transaction, and then the time for which the SPI chip select is held
 *    low, is determined by the buffer length given to dwt_spicswakeup() so this length must be chosen high enough so that the DW1000 has enough time
 *    to wake up.
 * 8. In this example, LDE microcode is not loaded upon calling dwt_initialise(). This will prevent the IC from generating an RX timestamp. If
 *    time-stamping is required, DWT_LOADUCODE parameter should be used. See two-way ranging examples (e.g. examples 5a/5b).
 * 9. As the DW1000 is not woken by the reset line, we could get to this point with it asleep which means that it will not be possible to initialise
 *    it properly. But, because of the low-power listening mode configuration, it is very complex to handle the case by trying to wake the DW1000 up
 *    before initialisation. The best solution remains to power off the DW1000 when the user wants to reset it. In the case of this example running on
 *    an EVB1000 board, this means powering off the whole board.
 * 10. In a real application, for optimum performance within regulatory limits, it may be necessary to set TX pulse bandwidth and TX power, (using
 *     the dwt_configuretxrf API call) to per device calibrated values saved in the target system or the DW1000 OTP memory.
 * 11. dwt_writetxdata() takes the full size of tx_msg as a parameter but only copies (size - 2) bytes as the check-sum at the end of the frame is
 *     automatically appended by the DW1000. This means that our tx_msg could be two bytes shorter without losing any data (but the sizeof would not
 *     work anymore then as we would still have to indicate the full length of the frame to dwt_writetxdata()).
 * 12. We use polled mode of operation here to keep the example as simple as possible but it would also be possible to use the DW1000 interrupt
 *     triggered by TXFRS event, depending on what is the best fit for the actual system's architecture.
 * 13. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *     DW1000 API Guide for more details on the DW1000 driver functions.
 ****************************************************************************************************************************************************/

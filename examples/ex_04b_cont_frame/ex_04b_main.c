/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   Continuous frame mode example code
 *
 *           This example application enables continuous frame mode to transmit frames without interruption for 2 minutes before stopping all
 *           operation. The frames are sent at a rate of one per millisecond. This is consistent with regulatory approvals when smart TX power
 *           can give boost to frames shorter than 1 ms.
 *
 * @attention
 *
 * Copyright 2015 (c) Decawave Ltd, Dublin, Ireland.
 * Copyright 2019 (c) Frederic Mes, RTLOC.
 *
 * All rights reserved.
 *
 * @author Decawave
 */
#ifdef EX_04B_DEF
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port.h"

/* Example application name and version to display on console. */
#define APP_NAME "CONT FRAME v1.3"

/* Start-to-start delay between frames, expressed in quarters of the 499.2 MHz fundamental frequency (around 8 ns). See NOTE 1 below. */
#define CONT_FRAME_PERIOD 124800

/* Continuous frame duration, in milliseconds. */
#define CONT_FRAME_DURATION_MS 120000

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

/* Recommended TX power and Pulse Generator delay values for Channel 5 and 64 MHz PRF as selected in the configuration above. See NOTE 2 below. */
static dwt_txconfig_t txconfig = {
    0xC0,            /* PG delay. */
    0x25456585,      /* TX power. */
};

/* The frame sent in this example is an 802.15.4e standard blink. It is a 12-byte frame composed of the following fields:
 *     - byte 0: frame type (0xC5 for a blink).
 *     - byte 1: sequence number, put to 0.
 *     - byte 2 -> 9: device ID, hard coded constant in this example for simplicity.
 *     - byte 10/11: frame check-sum, automatically set by DW1000 in a normal transmission and set to 0 here for simplicity.
 * See NOTE 1 below. */
static uint8 tx_msg[] = {0xC5, 0, 'D', 'E', 'C', 'A', 'W', 'A', 'V', 'E', 0, 0};


/**
 * Application entry point.
 */
int dw_main(void)
{

    /* Display application name on console. */
    printk(APP_NAME);

    /* Configure DW1000 SPI */
    openspi();

    /* During initialisation and continuous frame mode activation, DW1000 clocks must be set to crystal speed so SPI rate have to be lowered and will
     * not be increased again in this example. */
    port_set_dw1000_slowrate();

    /* Reset and initialise DW1000. See NOTE 3 below. */
    reset_DW1000(); /* Target specific drive of RSTn line into DW1000 low for a period. */
    if (dwt_initialise(DWT_LOADNONE) == DWT_ERROR)
    {
        printk("INIT FAILED");
        while (1)
        { };
    }

    /* Configure DW1000. */
    dwt_configure(&config);
    dwt_configuretxrf(&txconfig);

    /* Configure DW1000 LEDs */
    dwt_setleds(1);
    
    /* Activate continuous frame mode. */
    dwt_configcontinuousframemode(CONT_FRAME_PERIOD);

    /* Once configured, continuous frame must be started like a normal transmission. */
    dwt_writetxdata(sizeof(tx_msg), tx_msg, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_msg), 0, 0); /* Zero offset in TX buffer, no ranging. */
    dwt_starttx(DWT_START_TX_IMMEDIATE);

    /* Wait for the required period of repeated transmission. */
    Sleep(CONT_FRAME_DURATION_MS);

    /* Software reset of the DW1000 to deactivate continuous frame mode and go back to default state. Initialisation and configuration should be run
     * again if one wants to get the DW1000 back to normal operation. */
    dwt_softreset();

    /* End here. */
    while (1)
    { };
}
#endif
/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. Continuous frame mode is typically used to tune transmit power for regulatory purposes. This example has been designed to reproduce the use case
 *    of a tag blinking at a high rate: the blink length is around 180 �s in this configuration, emitted once per millisecond. In this configuration,
 *    the frame's transmission power can be increased while still complying with regulations. For more details about the management of TX power, the
 *    user is referred to DW1000 User Manual.
 * 2. The user is referred to DW1000 User Manual for references values applicable to different channels and/or PRF. Those reference values may need to
 *    be tuned for optimum performance and regulatory approval depending on the target product design.
 *    In this example the frame is set up with 128 preamble symbols and 12 octets of data at 6.8Mb/s giving a resultant frame length of 178 ms.
 *    This allows the BOOSTP250 value to be used by the IC when smart power mode is enabled (which it is by default in the DW1000).
 *    The TX power configuration value 0x25456585 used here sets the BOOSTP250 field to 0x45 which is 6 dB above the BOOSTNORM value of 0x85.
 *    Please refer to the DW1000 User Manual for more details about TX power and smart power.
 * 3. In this example, LDE microcode is not loaded upon calling dwt_initialise(). This will prevent the IC from generating an RX timestamp. If
 *    time-stamping is required, DWT_LOADUCODE parameter should be used. See two-way ranging examples (e.g. examples 5a/5b).
 * 4. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *    DW1000 API Guide for more details on the DW1000 driver functions.
 ****************************************************************************************************************************************************/

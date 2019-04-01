/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   Continuous wave mode example code
 *
 *           This example code activates continuous wave mode on channel 5 for 2 minutes before stopping operation.
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
#ifdef EX_04A_DEF
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port.h"

/* Example application name and version to display on console. */
#define APP_NAME "CONT WAVE v1.3"

/* Continuous wave duration, in milliseconds. */
#define CONT_WAVE_DURATION_MS 120000

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

/* Recommended TX power and Pulse Generator delay values for the mode defined above. */
static dwt_txconfig_t txconfig = {
    0xC0,            /* PG delay. */
    0x25456585,      /* TX power. */
};

/**
 * Application entry point.
 */
int dw_main(void)
{
    /* Display application name on console. */
    printk(APP_NAME);
    
    /* Configure DW1000 SPI */
    openspi();

    /* During initialisation and continuous wave mode activation, DW1000 clocks must be set to crystal speed so SPI rate have to be lowered and will
     * not be increased again in this example. */
    port_set_dw1000_slowrate();

    /* Reset and initialise DW1000. See NOTE 1 below. */
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
    
    /* Activate continuous wave mode. */
    dwt_configcwmode(config.chan);

    /* Wait for the wanted duration of the continuous wave transmission. */
    Sleep(CONT_WAVE_DURATION_MS);



    /* Software reset of the DW1000 to deactivate continuous wave mode and go back to default state. Initialisation and configuration should be run
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
 * 1. In this example, LDE microcode is not loaded upon calling dwt_initialise(). This will prevent the IC from generating an RX timestamp. If
 *    time-stamping is required, DWT_LOADUCODE parameter should be used. See two-way ranging examples (e.g. examples 5a/5b).
 * 2. The user is referred to DecaRanging ARM application (distributed with EVK1000 product) for additional practical example of usage, and to the
 *    DW1000 API Guide for more details on the DW1000 driver functions.
 ****************************************************************************************************************************************************/

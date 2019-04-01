/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   TX Bandwidth and Power Compensation Reference Measurement example code
 *
 *       This example application takes reference measurements from the DW1000 for the bandwidth and power settings, to be
 *       used for the example 09b (bandwidth and power compensation). These reference measurements are used as a base for
 *       the adjustments done during compensation. The measurements to be taken are the temperature and the contents of
 *       the TX_POWER, PG_DELAY and PGC_STATUS registers. These measurements will be output on the console.
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
#ifdef EX_09A_DEF
#include <stdio.h>
#include <string.h>

#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port.h"

/* Example application name and version to display on console. */
#define APP_NAME "BW PWR REF v1.2"

/* String to display on console string */
char console_str[16] = {0};

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

/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth and power of the spectrum at the current
 * temperature. These values can be calibrated prior to taking reference measurements. See note 1 below. */
static dwt_txconfig_t txconfig = {
    0xC0,            /* PG delay. */
    0x25456585,      /* TX power. */
};

/**
 * Application entry point.
 */
int dw_main(void)
{
    uint16 ref_pgcount;
    int ref_temp;
    uint32 ref_power;
    uint8 ref_pgdelay;

    /* Display application name on console. */
    printk(APP_NAME);

    /* Configure DW1000 SPI */
    openspi();

    /* During initialisation and continuous frame mode activation, DW1000 clocks must be set to crystal speed so SPI rate has to be lowered and will
     * not be increased again in this example. */
    port_set_dw1000_slowrate();

    /* Reset and initialise DW1000. See NOTE 2 below. */
    reset_DW1000(); /* Target specific drive of RSTn line into DW1000 low for a period. */
    if (dwt_initialise(DWT_READ_OTP_TMP) == DWT_ERROR)
    {
        printk("INIT FAILED");
        while (1)
        { };
    }

    /* Configure DW1000. */
    dwt_configure(&config);
    /* Configure the TX frontend with the desired operational settings */
    dwt_configuretxrf(&txconfig);

    /* Read DW1000 IC temperature for temperature compensation procedure. See NOTE 3 */
    ref_temp = (dwt_readtempvbat(1) & 0xFF00) >> 8;

    ref_pgcount = dwt_calcpgcount(txconfig.PGdly);
    ref_power = txconfig.power;
    ref_pgdelay = txconfig.PGdly;

    /* Software reset of the DW1000 to deactivate continuous frame mode and go back to default state. Initialisation and configuration should be run
     * again if one wants to get the DW1000 back to normal operation. */
    dwt_softreset();

    /* Configure DW1000 LEDs */
    dwt_setleds(1);
    
    /* End here. */
    /* Display the temperature, power register, PG_DELAY register and PGC_STATUS register on the console*/
    while (1)
    {
        sprintf(console_str, "Raw Temp: %x  ", ref_temp);
        printk(console_str);
        deca_sleep(2000);
        sprintf(console_str, "Power: %x   ", (unsigned)ref_power);
        printk(console_str);
        deca_sleep(2000);
        sprintf(console_str, "PG_DELAY: %02x   ", (unsigned)ref_pgdelay);
        printk(console_str);
        deca_sleep(2000);
        sprintf(console_str, "PG_COUNT: %x   ", (unsigned)ref_pgcount);
        printk(console_str);
        deca_sleep(2000);

    };
}
#endif
/*****************************************************************************************************************************************************
 * NOTES:
 *
 * 1. The reference measurements are made after optimising the transmit spectrum bandwidth and power level to maximise the use of the allowed spectrum
 *    mask (the mask used was the IEEE 802.15.4a mask). This optimisation needs to be carried out once, perhaps in a production test environment, and
 *    the reference measurements to be stored are the temperature at which the optimisation is made, the contents of the TX_POWER [1E] register and
 *    the contents of the PG_DELAY [2A:0B] register and the contents of the PG_COUNT [2A:08] register. For more information, see App Note APS024
 * 2. In this example, LDE microcode is not loaded upon calling dwt_initialise(). This will prevent the IC from generating an RX timestamp. If
 *    time-stamping is required, DWT_LOADUCODE parameter should be used. See two-way ranging examples (e.g. examples 5a/5b).
 * 3. The temperature is read from the DW1000 using this API call. The temperature is in the MSB, we use the raw value here.
 ****************************************************************************************************************************************************/

/*! ----------------------------------------------------------------------------
 * @file    port.c
 * @brief   HW specific definitions and functions for portability
 *
 * @attention
 *
 * Copyright 2016 (c) DecaWave Ltd, Dublin, Ireland.
 * Copyright 2019 (c) Frederic Mes, RTLOC. 
 *
 *
 * All rights reserved.
 *
 * @author DecaWave
 */

#include "port.h"
#include "deca_device_api.h"

//zephyr includes
#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
// #include <spi.h>

/****************************************************************************//**
 *
 *                              APP global variables
 *
 *******************************************************************************/

/****************************************************************************//**
 *
 *                  Port private variables and function prototypes
 *
 *******************************************************************************/
static volatile uint32_t signalResetDone;

/****************************************************************************//**
 *
 *                              Time section
 *
 *******************************************************************************/

/* @fn    portGetTickCnt
 * @brief wrapper for to read a SysTickTimer, which is incremented with
 *        CLOCKS_PER_SEC frequency.
 *        The resolution of time32_incr is usually 1/1000 sec.
 * */
unsigned long
portGetTickCnt(void)
{
    //TODO
    return 0;
}


/* @fn    usleep
 * @brief precise usleep() delay
 * */
#pragma GCC optimize ("O0")
int usleep(unsigned long usec)
{
    int i,j;
#pragma GCC ivdep
    for(i=0;i<usec;i++)
    {
#pragma GCC ivdep
        for(j=0;j<2;j++)
        {
            // __NOP();
            // __NOP();
        }
    }
    return 0;
}


/* @fn    Sleep
 * @brief Sleep delay in ms using SysTick timer
 * */
void
Sleep(uint32_t x)
{
    k_sleep(x);
}

/****************************************************************************//**
 *
 *                              END OF Time section
 *
 *******************************************************************************/

/****************************************************************************//**
 *
 *                              Configuration section
 *
 *******************************************************************************/

/* @fn    peripherals_init
 * */
int peripherals_init (void)
{
    //TODO
    return 0;
}


/* @fn    spi_peripheral_init
 * */
void spi_peripheral_init()
{
    openspi();
}

/****************************************************************************//**
 *
 *                          End of configuration section
 *
 *******************************************************************************/

/****************************************************************************//**
 *
 *                          DW1000 port section
 *
 *******************************************************************************/

/* @fn      reset_DW1000
 * @brief   DW_RESET pin on DW1000 has 2 functions
 *          In general it is output, but it also can be used to reset the digital
 *          part of DW1000 by driving this pin low.
 *          Note, the DW_RESET pin should not be driven high externally.
 * */
void reset_DW1000(void)
{
    //TODO:drive the RSTn pin low
 
    usleep(1);

    //put the pin back to output open-drain (not active)
    setup_DW1000RSTnIRQ(0);

    Sleep(2);
}

/* @fn      setup_DW1000RSTnIRQ
 * @brief   setup the DW_RESET pin mode
 *          0 - output Open collector mode
 *          !0 - input mode with connected EXTI0 IRQ
 * */
void setup_DW1000RSTnIRQ(int enable)
{
    //TODO
}


/* @fn      led_off
 * @brief   switch off the led from led_t enumeration
 * */
void led_off (led_t led)
{
    //TODO
    switch (led)
    {
        case 0:
            break;
        case 1:
            break;
        default:
            // do nothing for undefined led number
            break;
    }
}

/* @fn      led_on
 * @brief   switch on the led from led_t enumeration
 * */
void led_on (led_t led)
{
    //TODO
    switch (led)
    {
        case 0:
            break;
        case 1:
            break;
        default:
            // do nothing for undefined led number
            break;
    }
}


/* @fn      port_wakeup_dw1000
 * @brief   "slow" waking up of DW1000 using DW_CS only
 * */
void port_wakeup_dw1000(void)
{
    //TODO
}

/* @fn      port_wakeup_dw1000_fast
 * @brief   waking up of DW1000 using DW_CS and DW_RESET pins.
 *          The DW_RESET signalling that the DW1000 is in the INIT state.
 *          the total fast wakeup takes ~2.2ms and depends on crystal startup time
 * */
void port_wakeup_dw1000_fast(void)
{
    //TODO
}



/* @fn      port_set_dw1000_slowrate
 * @brief   set 2MHz
 * */
void port_set_dw1000_slowrate(void)
{
    set_spi_speed_slow();
}

/* @fn      port_set_dw1000_fastrate
 * @brief   set 8MHz
 * */
void port_set_dw1000_fastrate(void)
{
    //TODO
    set_spi_speed_fast();
}


/****************************************************************************//**
 *
 *                          End APP port section
 *
 *******************************************************************************/



/****************************************************************************//**
 *
 *                              IRQ section
 *
 *******************************************************************************/


/* @fn      process_deca_irq
 * @brief   main call-back for processing of DW1000 IRQ
 *          it re-enters the IRQ routing and processes all events.
 *          After processing of all events, DW1000 will clear the IRQ line.
 * */
void process_deca_irq(void)
{
    //TODO
}


/* @fn      port_DisableEXT_IRQ
 * @brief   wrapper to disable DW_IRQ pin IRQ
 *          in current implementation it disables all IRQ from lines 5:9
 * */
void port_DisableEXT_IRQ(void)
{
    //TODO
}

/* @fn      port_EnableEXT_IRQ
 * @brief   wrapper to enable DW_IRQ pin IRQ
 *          in current implementation it enables all IRQ from lines 5:9
 * */
void port_EnableEXT_IRQ(void)
{
    //TODO
}


/* @fn      port_GetEXT_IRQStatus
 * @brief   wrapper to read a DW_IRQ pin IRQ status
 * */
uint32_t port_GetEXT_IRQStatus(void)
{
    //TODO
    return 0;
}


/* @fn      port_CheckEXT_IRQ
 * @brief   wrapper to read DW_IRQ input pin state
 * */
uint32_t port_CheckEXT_IRQ(void)
{
    //TODO
    return 0;
}


/****************************************************************************//**
 *
 *                              END OF IRQ section
 *
 *******************************************************************************/

/* DW1000 IRQ handler definition. */
// static port_deca_isr_t port_deca_isr = NULL;

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn port_set_deca_isr()
 *
 * @brief This function is used to install the handling function for DW1000 IRQ.
 *
 * NOTE:
 *
 * @param deca_isr function pointer to DW1000 interrupt handler to install
 *
 * @return none
 */
void port_set_deca_isr(port_deca_isr_t deca_isr)
{
    //TODO
}


/****************************************************************************//**
 *
 *******************************************************************************/


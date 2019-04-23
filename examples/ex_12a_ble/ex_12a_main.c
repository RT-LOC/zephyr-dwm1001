/**
 * Copyright (c) 2019 - Frederic Mes, RTLOC
 * 
 * This file is part of Zephyr-DWM1001.
 *
 *   Zephyr-DWM1001 is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Zephyr-DWM1001 is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Zephyr-DWM1001.  If not, see <https://www.gnu.org/licenses/>.
 * 
 */

/*! ----------------------------------------------------------------------------
 *  @file   ex_12a_main.c
 *  @brief  Example of usage BLE - DPS Gatt Profile.
 *          In this example 2 (fake) distances are outputted every 300ms. 
 *  @author RTLOC
 */

#include <zephyr.h>

#include <stdio.h>
#include <string.h>

#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port.h"

#include "ble_dwm1001.h"

/* Example application name and version to display on console. */
#define APP_HEADER "\nDWM1001 & Zephyr\n"
#define APP_NAME "Example 12a - BLE DPS Profile (faked distances)\n"
#define APP_VERSION "Version - 1.0.0\n"
#define APP_VERSION_NUM 0x010000
#define APP_LINE "=================\n"

#define APP_UID 0xDECA0000000000C1
#define APP_HW  1

static float distance1 = 0.0f;
static float distance2 = 3.0f;

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn main()
 *
 * @brief Application entry point.
 *
 * @param  none
 *
 * @return none
 */
int dw_main(void)
{
    /* Display application name on console. */
    printk(APP_HEADER);
    printk(APP_NAME);
    printk(APP_VERSION);
    printk(APP_LINE);

    /* BLE Configuration */
    ble_reps_t *ble_reps; 
    uint8_t ble_buf[120] = {0};
    ble_reps = (ble_reps_t *)(&ble_buf[0]);

    ble_device_info_t devinfo;
    memset(&devinfo, 0, sizeof(ble_device_info_t));
    devinfo.uid = APP_UID;
    devinfo.hw_ver = APP_HW;
    devinfo.fw1_ver = APP_VERSION_NUM;

    ble_dwm1001_set_devinfo(&devinfo);
    ble_dwm1001_enable();


    /* Loop forever responding to ranging requests. */
    while (1)
    {
        /* Increase distances */
        distance1 += 1.0f;
        distance2 += 2.0f;

        if(distance1 > 150.0f)
            distance1 = 1.0f;

        if(distance2 > 100.0f)
            distance2 = 3.0f;

        /* Display faked distance on console. */
        printk("dist: %3.2f, dist2: %3.2f m\n", distance1, distance2);

        /* Fill reports */
        ble_reps->cnt = 2;
        ble_reps->ble_rep[0].node_id = 0x01;
        ble_reps->ble_rep[0].dist = distance1;
        ble_reps->ble_rep[0].tqf = 0;

        ble_reps->ble_rep[1].node_id = 0x06;
        ble_reps->ble_rep[1].dist = distance2;
        ble_reps->ble_rep[1].tqf = 0;

        /* Send to BLE layer */
        ble_dwm1001_dps(ble_buf, 1 + sizeof(ble_rep_t)*ble_reps->cnt);  
        
        /* Sleep for 0.3s */
        Sleep(300); 
    }
}
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
 *  @file   ble_dwm1001.h
 *  @brief  BLE driver header for dwm1001.
 *  @author RTLOC
 */

#ifndef __BLE_DWM1001_H__
#define __BLE_DWM1001_H__

#include "dps.h"

/* BLE Report */
struct ble_rep {
	uint16_t node_id;
	float dist;
	uint8_t tqf;
}__attribute__((__packed__));
typedef struct ble_rep ble_rep_t;

struct ble_reps {
    uint8_t cnt;
	ble_rep_t ble_rep[10];
}__attribute__((__packed__));
typedef struct ble_reps ble_reps_t;


int ble_dwm1001_enable(void);
void ble_dwm1001_dps(uint8_t *tx, uint16_t len);
void ble_dwm1001_set_devinfo(ble_device_info_t *devinfo_new);

#endif /* __BLE_DWM1001_H__ */ 

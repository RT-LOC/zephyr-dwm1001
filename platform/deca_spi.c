/*! ----------------------------------------------------------------------------
 * @file    deca_spi.c
 * @brief   SPI access functions
 *
 * @attention
 *
 * Copyright 2015 (c) DecaWave Ltd, Dublin, Ireland.
 * Copyright 2019 (c) Frederic Mes, RTLOC.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */

#include "deca_spi.h"
#include "deca_device_api.h"
#include "port.h"

//zephyr includes
#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <spi.h>

struct device *spi;
struct spi_config spi_cfg;

uint8_t tx_buf[255];
uint8_t rx_buf[255];

struct spi_buf bufs[2];

struct spi_buf_set tx; 
struct spi_buf_set rx;

/****************************************************************************//**
 *
 *                              DW1000 SPI section
 *
 *******************************************************************************/
/*! ------------------------------------------------------------------------------------------------------------------
 * Function: openspi()
 *
 * Low level abstract function to open and initialise access to the SPI device.
 * returns 0 for success, or -1 for error
 */
int openspi()
{
	spi = device_get_binding(DT_SPI_1_NAME);
	if (!spi) {
		printk("Could not find SPI driver\n");
		return -1;
	}
	spi_cfg.operation = SPI_WORD_SET(8);
	spi_cfg.frequency = 256000;

	memset(&tx_buf[0], 0, 255);
	memset(&rx_buf[0], 0, 255);
	bufs[0].buf = &tx_buf[0];
	bufs[1].buf = &rx_buf[0];	
	tx.buffers = &bufs[0];
	rx.buffers = &bufs[1];
	tx.count = 1;
	rx.count = 1;
    return 0;
} // end openspi()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: closespi()
 *
 * Low level abstract function to close the the SPI device.
 * returns 0 for success, or -1 for error
 */
int closespi(void)
{
    //TODO
    return 0;
} // end closespi()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: writetospi()
 *
 * Low level abstract function to write to the SPI
 * Takes two separate byte buffers for write header and write data
 * returns 0 for success
 */
int writetospi(uint16 headerLength,
               const    uint8 *headerBuffer,
               uint32 bodyLength,
               const    uint8 *bodyBuffer)
{
    decaIrqStatus_t  stat ;
    stat = decamutexon() ;

	memcpy(&tx_buf[0], headerBuffer, headerLength);
	memcpy(&tx_buf[headerLength], bodyBuffer, bodyLength);

    bufs[0].len = headerLength+bodyLength;
    bufs[1].len = headerLength+bodyLength;

    spi_transceive(spi, &spi_cfg, &tx, &rx);
    decamutexoff(stat);

    return 0;
} // end writetospi()


/*! ------------------------------------------------------------------------------------------------------------------
 * Function: readfromspi()
 *
 * Low level abstract function to read from the SPI
 * Takes two separate byte buffers for write header and read data
 * returns the offset into read buffer where first byte of read data may be found,
 * or returns 0
 */
int readfromspi(uint16 headerLength,
                const uint8 *headerBuffer,
                uint32 readlength,
                uint8 *readBuffer)
{
    decaIrqStatus_t  stat ;
    stat = decamutexon() ;

	memset(&tx_buf[0], 0, headerLength+readlength);
	memcpy(&tx_buf[0], headerBuffer, headerLength);

    bufs[0].len = headerLength+readlength;
    bufs[1].len = headerLength+readlength;
    spi_transceive(spi, &spi_cfg, &tx, &rx);

	memcpy(readBuffer, rx_buf+headerLength, readlength);

    decamutexoff(stat);

    return 0;
} // end readfromspi()

/****************************************************************************//**
 *
 *                              END OF DW1000 SPI section
 *
 *******************************************************************************/


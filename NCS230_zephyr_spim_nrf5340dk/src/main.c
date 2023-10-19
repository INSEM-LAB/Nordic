/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/spi.h>

const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(spi3));

static struct spi_config spi_cfg = {
	.operation = SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB,
	.frequency = 1000000,
	.slave = 0, 
};

void serial_flash_spi_test( struct spi_cs_control *cs)
{
	int err;
	static uint8_t tx_buffer[1];
	static uint8_t rx_buffer[4];	

	const struct spi_buf tx_buf = {
		.buf = tx_buffer,
		.len = sizeof(tx_buffer)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	struct spi_buf rx_buf = {
		.buf = rx_buffer,
		.len = sizeof(rx_buffer),
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1	
	};

	spi_cfg.cs = cs;

	/* 1 byte manufacturer ID & 2 bytes device ID */
	tx_buffer[0] = 0x9F; //RDID(Read identification)

	err = spi_transceive(dev, &spi_cfg, &tx, &rx);
	if (err) {
		printk("SPI error: %d\n", err);
	} else {
		/* Connect MISO to MOSI for loopback */
		printk("TX sent: %x\n", tx_buffer[0]);
		printk("RX recv: %x %x %x %x\n", rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3]);
	}	
}

void main(void)
{
	struct spi_cs_control cs_ctrl = (struct spi_cs_control){
		.gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(spi3), cs_gpios),
		.delay = 0u,
	};

	printk("HW SPI test with SPI3\n");

	if (!device_is_ready(dev)) {
    	return;
	}

	while(1)
	{
		serial_flash_spi_test(&cs_ctrl);
		k_sleep(K_MSEC(1000));		
	}
}

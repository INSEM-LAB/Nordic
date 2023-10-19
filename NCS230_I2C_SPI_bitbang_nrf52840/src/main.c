/*
 * Copyright (c) 2021 Marc Reilly, Creative Product Design
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>

#define SW_I2C_NODE	DT_NODELABEL(gpio_i2c0)
#define SW_SPI_NODE	DT_NODELABEL(gpio_spi0)

void serial_flash_spi_test(const struct device *dev, struct spi_cs_control *cs)
{
	struct spi_config config;

	config.frequency = 1000000;
	config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	config.slave = 0;
	config.cs = cs;

	enum { datacount = 4 };
	uint8_t buff[1] = { 0x9F };
	uint8_t rxdata[datacount];

	struct spi_buf tx_buf[1] = {
		{.buf = buff, .len = 1},
	};
	struct spi_buf rx_buf[1] = {
		{.buf = rxdata, .len = datacount},
	};

	struct spi_buf_set tx_set = { .buffers = tx_buf, .count = 1 };
	struct spi_buf_set rx_set = { .buffers = rx_buf, .count = 1 };

	int ret = spi_transceive(dev, &config, &tx_set, &rx_set);

    /* MX25R6435F RDID read */
	printk("tx (i)  : %02x \n", buff[0]);
	printk("rx (i)  : %02x %02x %02x %02x \n", rxdata[0], rxdata[1], rxdata[2], rxdata[3]);
}

void main(void)
{
    int ret;
    uint8_t value;

	const struct device *const i2c_dev = DEVICE_DT_GET(SW_I2C_NODE);
    const struct device *const spi_dev = DEVICE_DT_GET(SW_SPI_NODE);

	struct spi_cs_control cs_ctrl = (struct spi_cs_control){
		.gpio = GPIO_DT_SPEC_GET(SW_SPI_NODE, cs_gpios),
		.delay = 0u,
	};

	if (!device_is_ready(i2c_dev)) {
		printk("%s: device not ready.\n", i2c_dev->name);
		return;
	}

	if (!device_is_ready(spi_dev)) {
		printk("%s: device not ready.\n", spi_dev->name);
		return;
	}

    k_sleep(K_MSEC(1000));

    /* VL53L0X sensor used to test a bitbang I2C. */
    printk("\nSoftware I2C testing of VL53L0X\n");
    ret = i2c_reg_read_byte(i2c_dev, 0x29, 0xc0, &value);
    printk("value : %02x\r\n", value);
    k_sleep(K_MSEC(100));

    ret = i2c_reg_read_byte(i2c_dev, 0x29, 0xc1, &value);
    printk("value : %02x\r\n", value);
    k_sleep(K_MSEC(100));

    ret = i2c_reg_read_byte(i2c_dev, 0x29, 0xc2, &value);
    printk("value : %02x\r\n", value);
    k_sleep(K_MSEC(100));

	printk("\nSoftware SPI testing of MX25R6435F\n");
    serial_flash_spi_test(spi_dev, &cs_ctrl);
}

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

#define I2C_NODE DT_NODELABEL(arduino_i2c)

void main(void)
{
    int ret;
    uint8_t value;
    uint8_t data[3]={0};

	const struct device *const i2c_dev = DEVICE_DT_GET(I2C_NODE);

	if (!device_is_ready(i2c_dev)) {
		printk("%s: device not ready.\n", i2c_dev->name);
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

    ret = i2c_burst_read(i2c_dev, 0x29, 0xc0, data, sizeof(data));
    printk("value : %02x %02x %02x\r\n", data[0], data[1], data[2]);
    k_sleep(K_MSEC(100));    
}

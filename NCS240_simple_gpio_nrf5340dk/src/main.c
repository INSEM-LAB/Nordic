/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

void main(void)
{
	int err;
	static const struct device *gpio_dev;

	printk("Hello World! %s\n", CONFIG_BOARD);

	gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

	err = gpio_pin_configure(gpio_dev, 29, GPIO_OUTPUT);
	if (err) {
		printk("GPIO_0 config error: %d", err);
	}

	err = gpio_pin_set(gpio_dev, 29, 1);
	if (err) {
		printk("GPIO_0 set error: %d", err);
	}

	while(1)
	{
		gpio_pin_toggle(gpio_dev,29);
		k_sleep(K_MSEC(1000));
	}
}

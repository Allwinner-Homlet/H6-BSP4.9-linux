/*
 * drivers/misc/sunxi-rf/sunxi-rfkill.h
 *
 * Copyright (c) 2014 softwinner.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SUNXI_RFKILL_H
#define __SUNXI_RFKILL_H

#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/rfkill.h>

struct sunxi_bt_platdata {
	struct regulator *bt_power;
	struct regulator *io_regulator;
	struct clk *lpo;
	int gpio_bt_rst;
	char *bt_power_name;
	char *io_regulator_name;

	int power_state;
	struct rfkill *rfkill;
	struct platform_device *pdev;
};

struct sunxi_wlan_platdata {
	int bus_index;
	struct regulator *wlan_power;
	struct regulator *io_regulator;
	struct clk *lpo;
	int gpio_wlan_regon;
	int gpio_wlan_hostwake;
	int gpio_chip_en;

	char *wlan_power_name;
	char *io_regulator_name;

	int power_state;
	struct platform_device *pdev;
};

extern void sunxi_wl_chipen_set(int dev, int on_off);
extern void sunxi_wlan_set_power(bool on_off);
extern int  sunxi_wlan_get_bus_index(void);
extern int  sunxi_wlan_get_oob_irq(void);
extern int  sunxi_wlan_get_oob_irq_flags(void);
extern int  enable_gpio_wakeup_src(int para);

#endif /* SUNXI_RFKILL_H */

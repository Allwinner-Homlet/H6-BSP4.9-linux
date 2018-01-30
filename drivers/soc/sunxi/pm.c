/*
 * drivers/soc/sunxi/pm.c
 * (C) Copyright 2017-2023
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * fanqinghua <fanqinghua@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/of.h>
#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/sunxi-gpio.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/arisc/arisc.h>
#include <asm/cpuidle.h>
#include <linux/power/axp_depend.h>
#include <asm/io.h>
#include <asm/cacheflush.h>
#include "pm.h"

static unsigned long time_to_wakeup_ms;
module_param_named(time_to_wakeup_ms, time_to_wakeup_ms,
	ulong, S_IRUGO | S_IWUSR);

static super_standby_t *super_standby_para;
static unsigned int standby_space_size;
static DEFINE_SPINLOCK(data_lock);
static unsigned int sys_mask;


int init_sys_pwr_dm(void)
{

	axp_add_sys_pwr_dm("vdd-cpua");
	/*axp_add_sys_pwr_dm("vdd-cpub"); */
	axp_add_sys_pwr_dm("vcc-dram");
	/*axp_add_sys_pwr_dm("vdd-gpu"); */
	axp_add_sys_pwr_dm("vdd-sys");
	/*axp_add_sys_pwr_dm("vdd-vpu"); */
	axp_add_sys_pwr_dm("vdd-cpus");
	/*axp_add_sys_pwr_dm("vdd-drampll"); */
	axp_add_sys_pwr_dm("vcc-lpddr");
	/*axp_add_sys_pwr_dm("vcc-adc"); */
	axp_add_sys_pwr_dm("vcc-pl");
	/*axp_add_sys_pwr_dm("vcc-pm"); */
	axp_add_sys_pwr_dm("vcc-io");
	/*axp_add_sys_pwr_dm("vcc-cpvdd"); */
	/*axp_add_sys_pwr_dm("vcc-ldoin"); */
	axp_add_sys_pwr_dm("vcc-pll");
	/*axp_add_sys_pwr_dm("vcc-pc"); */

	sys_mask = axp_get_sys_pwr_dm_mask();

	pr_info("after inited: sys_mask config = 0x%x.\n", sys_mask);

	return 0;
}

int enable_gpio_wakeup_src(int para)
{
	spin_lock(&data_lock);
	super_standby_para->event |= CPUS_WAKEUP_GPIO;
	if (para >= AXP_PIN_BASE) {
		super_standby_para->gpio_enable_bitmap |=
			(WAKEUP_GPIO_AXP((para - AXP_PIN_BASE)));
	} else if (para >= SUNXI_PM_BASE) {
		super_standby_para->gpio_enable_bitmap |=
			(WAKEUP_GPIO_PM((para - SUNXI_PM_BASE)));
	} else if (para >= SUNXI_PL_BASE) {
		super_standby_para->gpio_enable_bitmap |=
			(WAKEUP_GPIO_PL((para - SUNXI_PL_BASE)));
	} else if (para >= SUNXI_PH_BASE) {
		super_standby_para->cpux_gpiog_bitmap |=
			(WAKEUP_GPIO_GROUP('H'));
	} else if (para >= SUNXI_PG_BASE) {
		super_standby_para->cpux_gpiog_bitmap |=
			(WAKEUP_GPIO_GROUP('G'));
	} else if (para >= SUNXI_PF_BASE) {
		super_standby_para->cpux_gpiog_bitmap |=
			(WAKEUP_GPIO_GROUP('F'));
	} else if (para >= SUNXI_PE_BASE) {
		super_standby_para->cpux_gpiog_bitmap |=
			(WAKEUP_GPIO_GROUP('E'));
	} else if (para >= SUNXI_PD_BASE) {
		super_standby_para->cpux_gpiog_bitmap |=
			(WAKEUP_GPIO_GROUP('D'));
	} else if (para >= SUNXI_PC_BASE) {
		super_standby_para->cpux_gpiog_bitmap |=
			(WAKEUP_GPIO_GROUP('C'));
	} else if (para >= SUNXI_PB_BASE) {
		super_standby_para->cpux_gpiog_bitmap |=
			(WAKEUP_GPIO_GROUP('B'));
	} else if (para >= SUNXI_PA_BASE) {
		super_standby_para->cpux_gpiog_bitmap |=
			(WAKEUP_GPIO_GROUP('A'));
	} else {
		pr_info("cpux need care gpio %d. not support it.\n",
			para);
	}
	spin_unlock(&data_lock);

	return 0;
}
EXPORT_SYMBOL_GPL(enable_gpio_wakeup_src);

static int sunxi_suspend_valid(suspend_state_t state)
{
	pr_debug("%s\n", __func__);

	if (!((state > PM_SUSPEND_ON) && (state < PM_SUSPEND_MAX))) {
		pr_err("state (%d) invalid!\n", state);
		return 0;
	}

	return 1;
}

static int __sunxi_suspend_enter(void)
{
	super_standby_para->event |= CPUS_MEM_WAKEUP;

	if (time_to_wakeup_ms > 0) {
		super_standby_para->event |= CPUS_WAKEUP_TIMEOUT;
		super_standby_para->timeout = time_to_wakeup_ms;
	}
	pr_info("enter standby, wakesrc:0x%x, timeout: %d\n",
		super_standby_para->event,
		super_standby_para->timeout);
	__dma_flush_area(super_standby_para, standby_space_size);
	arm_cpuidle_suspend(3);

	return 0;
}

static int sunxi_suspend_enter(suspend_state_t state)
{
	/*
	 * This local variable may be calculated with virt_to_phys
	 * in the function of arisc_query_wakeup_source,so it must
	 * be located in data section.
	 */
	static unsigned int wakeup_events;

	if (__sunxi_suspend_enter()) {
		pr_err("sunxi suspend failed, need to resume");
		return -1;
	}

	arisc_query_wakeup_source(&wakeup_events);
	pr_info("platform wakeup, standby wakesource is:0x%x\n",
				wakeup_events);

	return 0;
}

static const struct platform_suspend_ops sunxi_suspend_ops = {
	.valid        = sunxi_suspend_valid,
	.enter        = sunxi_suspend_enter,
};

static extended_standby_t sun50iw1_superstandby = {
	.id = 0x8,
	.soc_pwr_dm_state.state =
		BIT(VCC_DRAM_BIT) |
		BIT(VDD_CPUS_BIT) |
		BIT(VCC_LPDDR_BIT) |
		BIT(VCC_PL_BIT) |
		BIT(VDD_SYS_BIT) |
		BIT(VCC_PLL_BIT),
	.soc_pwr_dm_state.sys_mask = 0xffffffff,
	.soc_pwr_dm_state.volt[0]      = 0x0,
	.cpux_clk_state.osc_en         = 0x0,
	.cpux_clk_state.init_pll_dis   = BIT(PM_PLL_DRAM),
	.cpux_clk_state.exit_pll_en    = 0x0,
	.cpux_clk_state.pll_change     = 0x0,
	.cpux_clk_state.bus_change     = 0x0,
	.soc_dram_state.selfresh_flag = 0x1,
	.soc_io_state.hold_flag = 0x1,
	.soc_io_state.io_state[0] = {0x01c208b4, 0x00f0f0ff, 0x00707077},
	.soc_io_state.io_state[1] = {0x01c208b4, 0x000f0f00, 0x00070700},
};

static extended_standby_t sun50iw1_usbstandby = {
	.id = 0x2,
	.soc_pwr_dm_state.state =
		BIT(VCC_DRAM_BIT) |
		BIT(VDD_CPUS_BIT) |
		BIT(VCC_LPDDR_BIT) |
		BIT(VCC_PL_BIT) |
		BIT(VDD_SYS_BIT) |
		BIT(VCC_IO_BIT) |
		BIT(VCC_PLL_BIT),
	.soc_pwr_dm_state.sys_mask = 0xffffffff,
	.soc_pwr_dm_state.volt[0]           = 0x0,
	.soc_pwr_dm_state.volt[VDD_SYS_BIT] = 980,
	.soc_pwr_dm_state.volt[VCC_PLL_BIT] = 2500,
	.soc_pwr_dm_state.volt[VCC_IO_BIT]  = 3000,
	.cpux_clk_state.osc_en         = BIT(OSC_LOSC_BIT),
	.cpux_clk_state.init_pll_dis   = BIT(PM_PLL_DRAM),
	.cpux_clk_state.exit_pll_en    = 0x0,
	.cpux_clk_state.pll_change     = 0x0,
	.cpux_clk_state.pll_factor[PM_PLL_PERIPH] = {
		.factor1 = 1,
		.factor2 = 1,
		.factor3 = 0,
	},
	.cpux_clk_state.bus_change =
		BIT(BUS_AHB1) |
		BIT(BUS_AHB2) |
		BIT(BUS_APB1) |
		BIT(BUS_APB2),
	.cpux_clk_state.bus_factor[BUS_AHB1] = {
		.src = CLK_SRC_LOSC,
		.pre_div = 0,
		.div_ratio = 0,
	},
	.cpux_clk_state.bus_factor[BUS_AHB2] = {
		.src = CLK_SRC_AHB1,
		.pre_div = 0,
		.div_ratio = 0,
	},
	.cpux_clk_state.bus_factor[BUS_APB1] = {
		.src = CLK_SRC_AHB1,
		.pre_div = 0,
		.div_ratio = 0,
	},
	.cpux_clk_state.bus_factor[BUS_APB2] = {
		.src = CLK_SRC_LOSC,
		.n = 0,
		.m = 0,
	},
	.soc_dram_state.selfresh_flag = 0x1,
	.soc_io_state.hold_flag = 0x0,
	.soc_io_state.io_state[0] = {0x01c208b4, 0x00f0f0ff, 0x00707077},
	.soc_io_state.io_state[1] = {0x01c208b4, 0x000f0f00, 0x00070700},
};

static extended_standby_t sun50iw3_superstandby = {
	.id = 0x8,
	.soc_pwr_dm_state.state =
		BIT(VCC_DRAM_BIT) |
		BIT(VDD_CPUS_BIT) |
		BIT(VCC_LPDDR_BIT) |
		BIT(VCC_PL_BIT) |
		BIT(VDD_SYS_BIT) |
		BIT(VCC_PLL_BIT),
	.soc_pwr_dm_state.sys_mask = 0xffffffff,
	.soc_pwr_dm_state.volt[0]      = 0x0,
	.cpux_clk_state.osc_en         = 0x0,
	.cpux_clk_state.init_pll_dis   = BIT(PM_PLL_DRAM),
	.cpux_clk_state.exit_pll_en    = 0x0,
	.cpux_clk_state.pll_change     = 0x0,
	.cpux_clk_state.bus_change     = 0x0,
	.soc_dram_state.selfresh_flag = 0x1,
	.soc_io_state.hold_flag = 0x1,
	/* for pf port: set the io to disable state.; */
	.soc_io_state.io_state[0] = {0xd300b0b4, 0x00f0f0ff, 0x00707077},
	.soc_io_state.io_state[1] = {0xd300b0b4, 0x000f0f00, 0x00070700},
};

static extended_standby_t sun50iw3_usbstandby = {
	.id = 0x2,
	/* note: vcc_io for phy; */
	.soc_pwr_dm_state.state =
		BIT(VCC_DRAM_BIT) |
		BIT(VDD_CPUS_BIT) |
		BIT(VCC_LPDDR_BIT) |
		BIT(VCC_PL_BIT) |
		BIT(VDD_SYS_BIT) |
		BIT(VCC_IO_BIT) |
		BIT(VCC_PLL_BIT),
	.soc_pwr_dm_state.sys_mask = 0xffffffff,
	.soc_pwr_dm_state.volt[0] = 0x0,
	.cpux_clk_state.osc_en =
		BIT(OSC_LOSC_BIT) |
		BIT(OSC_HOSC_BIT) |
		BIT(OSC_LDO0_BIT) |
		BIT(OSC_LDO1_BIT),
	.cpux_clk_state.init_pll_dis = BIT(PM_PLL_DRAM),
	/* mean PLL_DRAM is shutdowned & open by dram driver. */
	/* hsic pll can be disabled,
	 * cpus can change cci400 clk from hsic_pll.
	 */
	.cpux_clk_state.exit_pll_en = 0x0,
	.soc_dram_state.selfresh_flag = 0x1,
	.soc_io_state.hold_flag = 0x0,
	/* for pf port: set the io to disable state.; */
	.soc_io_state.io_state[0] = {0xd300b0b4, 0x00f0f0ff, 0x00707077},
	.soc_io_state.io_state[1] = {0xd300b0b4, 0x000f0f00, 0x00070700},
};

static extended_standby_t sun50iw6_superstandby = {
	.id = 0x8,
	.soc_pwr_dm_state.state =
		BIT(VCC_DRAM_BIT) |
		BIT(VDD_CPUS_BIT) |
		BIT(VCC_LPDDR_BIT) |
		BIT(VCC_PL_BIT) |
		BIT(VDD_SYS_BIT) |
		BIT(VCC_PLL_BIT),
	.soc_pwr_dm_state.sys_mask = 0xffffffff,
	.soc_pwr_dm_state.volt[0]      = 0x0,
	.cpux_clk_state.osc_en         = 0x0,
	.cpux_clk_state.init_pll_dis   = BIT(PM_PLL_DRAM),
	.cpux_clk_state.exit_pll_en    = 0x0,
	.cpux_clk_state.pll_change     = 0x0,
	.cpux_clk_state.bus_change     = 0x0,
	.soc_dram_state.selfresh_flag = 0x1,
	.soc_io_state.hold_flag = 0x1,
	/* for pf port: set the io to disable state.; */
	.soc_io_state.io_state[0] = {0x0300b074, 0x0ffff000, 0x07777000},
	.soc_io_state.io_state[1] = {0x0300b074, 0x00000000, 0x00000000},
};

static extended_standby_t sun50iw6_usbstandby = {
	.id = 0x2,
	/* note: vcc_io for phy; */
	.soc_pwr_dm_state.state =
		BIT(VCC_DRAM_BIT) |
		BIT(VDD_CPUS_BIT) |
		BIT(VCC_LPDDR_BIT) |
		BIT(VCC_PL_BIT) |
		BIT(VDD_SYS_BIT) |
		BIT(VCC_IO_BIT) |
		BIT(VCC_PLL_BIT),
	.soc_pwr_dm_state.sys_mask = 0xffffffff,
	.soc_pwr_dm_state.volt[0] = 0x0,
	.cpux_clk_state.osc_en =
		BIT(OSC_LOSC_BIT) |
		BIT(OSC_HOSC_BIT) |
		BIT(OSC_LDO0_BIT) |
		BIT(OSC_LDO1_BIT),
	.cpux_clk_state.init_pll_dis = BIT(PM_PLL_DRAM),
	/* mean PLL_DRAM is shutdowned & open by dram driver. */
	/* hsic pll can be disabled,
	 * cpus can change cci400 clk from hsic_pll.
	 */
	.cpux_clk_state.exit_pll_en = 0x0,
	.soc_dram_state.selfresh_flag = 0x1,
	.soc_io_state.hold_flag = 0x0,
	/* for pf port: set the io to disable state.; */
	.soc_io_state.io_state[0] = {0x0300b074, 0x0ffff000, 0x07777000},
	.soc_io_state.io_state[1] = {0x0300b074, 0x00000000, 0x00000000},
};

static const struct of_device_id pm_of_match[] __initconst = {
	{
		.compatible = "allwinner,sun50iw1-superstandby",
		.data = (extended_standby_t *)&sun50iw1_superstandby,
	},
	{
		.compatible = "allwinner,sun50iw1-usbstandby",
		.data = (extended_standby_t *)&sun50iw1_usbstandby,
	},
	{
		.compatible = "allwinner,sun50iw3-superstandby",
		.data = (extended_standby_t *)&sun50iw3_superstandby,
	},
	{
		.compatible = "allwinner,sun50iw3-usbstandby",
		.data = (extended_standby_t *)&sun50iw3_usbstandby,
	},
	{
		.compatible = "allwinner,sun50iw6-superstandby",
		.data = (extended_standby_t *)&sun50iw6_superstandby,
	},
	{
		.compatible = "allwinner,sun50iw6-usbstandby",
		.data = (extended_standby_t *)&sun50iw6_usbstandby,
	},
	{},
};

static int __init sunxi_suspend_init(void)
{
	struct device_node *np;
	const struct of_device_id *matched_np;
	const extended_standby_t *extended_standby_info;
	extended_standby_t *extend_standby_para;
	int ret = 0;
	unsigned int value[3] = { 0, 0, 0 };

	pr_debug("%s\n", __func__);

	init_sys_pwr_dm();

	np = of_find_matching_node_and_match(NULL, pm_of_match,
				&matched_np);
	if (IS_ERR_OR_NULL(np)) {
		pr_err("get [allwinner,standby_space] device node error\n");
		return -ENODEV;
	}

	extended_standby_info = (extended_standby_t *)matched_np->data;

	ret = of_property_read_u32_array(np, "space1", value,
				ARRAY_SIZE(value));
	if (ret) {
		pr_err("get standby_space1 err.\n");
		return -EINVAL;
	}

	pr_err("%s, 0x%x, 0x%x, 0x%x\n", __FUNCTION__,
			value[0], value[1], value[2]);
	super_standby_para = (super_standby_t *)phys_to_virt(value[0]);
	super_standby_para->pextended_standby = value[0] + 0x400;
	extend_standby_para =
	(extended_standby_t *)((char *)super_standby_para + 0x400);
	*extend_standby_para = *extended_standby_info;
	extend_standby_para->soc_pwr_dm_state.sys_mask = sys_mask;

	standby_space_size = value[2];
	super_standby_para->event = 0;
	suspend_set_ops(&sunxi_suspend_ops);

	return ret;
}
module_init(sunxi_suspend_init);

static void __exit sunxi_suspend_exit(void)
{
	pr_debug("%s\n", __func__);
	suspend_set_ops(NULL);
}
module_exit(sunxi_suspend_exit);

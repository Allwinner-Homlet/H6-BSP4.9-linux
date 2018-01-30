/*
 * drivers/power/axp/BMU1760/BMU1760.c
 * (C) Copyright 2010-2017
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * driver of bmu1760
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/reboot.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/mfd/core.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/power/aw_pm.h>
#include "../axp-core.h"
#include "../axp-charger.h"
#include "bmu1760.h"

static struct axp_dev *bmu1760_pm_power;
struct axp_config_info bmu1760_config;
struct wakeup_source *bmu1760_ws;
static int bmu1760_pmu_num;

static const struct axp_compatible_name_mapping bmu1760_cn_mapping[] = {
	{
		.device_name = "bmu1760",
		.mfd_name = {
			.powerkey_name  = "bmu1760-powerkey",
			.charger_name   = "bmu1760-charger",
		},
	},
};

static struct axp_regmap_irq_chip bmu1760_regmap_irq_chip = {
	.name           = "bmu1760_irq_chip",
	.status_base    = BMU1760_INTSTS1,
	.enable_base    = BMU1760_INTEN1,
	.num_regs       = 5,
};

static struct resource bmu1760_pek_resources[] = {
	{BMU1760_IRQ_PEKRE,  BMU1760_IRQ_PEKRE,  "PEK_DBR",      IORESOURCE_IRQ,},
	{BMU1760_IRQ_PEKFE,  BMU1760_IRQ_PEKFE,  "PEK_DBF",      IORESOURCE_IRQ,},
};

static struct resource bmu1760_charger_resources[] = {
	{BMU1760_IRQ_ACIN,   BMU1760_IRQ_ACIN,   "ac in",        IORESOURCE_IRQ,},
	{BMU1760_IRQ_ACRE,   BMU1760_IRQ_ACRE,   "ac out",       IORESOURCE_IRQ,},
	{BMU1760_IRQ_BATIN,  BMU1760_IRQ_BATIN,  "bat in",       IORESOURCE_IRQ,},
	{BMU1760_IRQ_BATRE,  BMU1760_IRQ_BATRE,  "bat out",      IORESOURCE_IRQ,},
	{BMU1760_IRQ_CHAST,  BMU1760_IRQ_CHAST,  "charging",     IORESOURCE_IRQ,},
	{BMU1760_IRQ_CHAOV,  BMU1760_IRQ_CHAOV,  "charge over",  IORESOURCE_IRQ,},
	{BMU1760_IRQ_LOWN1,  BMU1760_IRQ_LOWN1,  "low warning1", IORESOURCE_IRQ,},
	{BMU1760_IRQ_LOWN2,  BMU1760_IRQ_LOWN2,  "low warning2", IORESOURCE_IRQ,},
};

static struct mfd_cell bmu1760_cells[] = {
	{
		.name          = "bmu1760-powerkey",
		.num_resources = ARRAY_SIZE(bmu1760_pek_resources),
		.resources     = bmu1760_pek_resources,
	},
	{
		.name          = "bmu1760-charger",
		.num_resources = ARRAY_SIZE(bmu1760_charger_resources),
		.resources     = bmu1760_charger_resources,
	},
};

void bmu1760_power_off(void)
{
	pr_info("[%s] send power-off command!\n", axp_name[bmu1760_pmu_num]);

	axp_regmap_set_bits(bmu1760_pm_power->regmap, 0x17, 0x01);
	mdelay(20);
	pr_warn("[%s] warning!!! axp can't power-off,\"\
		\" maybe some error happened!\n", axp_name[bmu1760_pmu_num]);
}

static int bmu1760_init_chip(struct axp_dev *bmu1760)
{
	uint8_t chip_id;
	int err;

	err = axp_regmap_read(bmu1760->regmap, BMU1760_IC_TYPE, &chip_id);
	if (err) {
		pr_err("[%s] try to read chip id failed!\n",
				axp_name[bmu1760_pmu_num]);
		return err;
	}

	/*only support bmu1760*/
	if (((chip_id & 0xc0) == 0x00) &&
		((chip_id & 0x0f) == 0x06)
		) {
		pr_info("[%s] chip id detect 0x%x !\n",
				axp_name[bmu1760_pmu_num], chip_id);
	} else {
		pr_info("[%s] chip id is error 0x%x !\n",
				axp_name[bmu1760_pmu_num], chip_id);
	}

	/*Init IRQ wakeup en*/
	if (bmu1760_config.pmu_irq_wakeup)
		axp_regmap_set_bits(bmu1760->regmap, 0x17, 0x10); /* enable */
	else
		axp_regmap_clr_bits(bmu1760->regmap, 0x17, 0x10); /* disable */

	return 0;
}

static void bmu1760_wakeup_event(void)
{
	__pm_wakeup_event(bmu1760_ws, 0);
}

static s32 bmu1760_usb_det(void)
{
	return 0;
}

static s32 bmu1760_usb_vbus_output(int high)
{
	u8 ret = 0;

	return ret;
}

static int bmu1760_cfg_pmux_para(int num, struct aw_pm_info *api, int *pmu_id)
{
	char name[8];
	struct device_node *np;

	sprintf(name, "pmu%d", num);

	np = of_find_node_by_type(NULL, name);
	if (np == NULL) {
		pr_err("can not find device_type for %s\n", name);
		return -1;
	}

	if (!of_device_is_available(np)) {
		pr_err("can not find node for %s\n", name);
		return -1;
	}
#ifdef CONFIG_AXP_TWI_USED
	if (api != NULL) {
		api->pmu_arg.twi_port = bmu1760_pm_power->regmap->client->adapter->nr;
		api->pmu_arg.dev_addr = bmu1760_pm_power->regmap->client->addr;
	}
#endif
	*pmu_id = bmu1760_config.pmu_id;

	return 0;
}

static const char *bmu1760_get_pmu_name(void)
{
	return axp_name[bmu1760_pmu_num];
}

static struct axp_dev *bmu1760_get_pmu_dev(void)
{
	return bmu1760_pm_power;
}

struct axp_platform_ops bmu1760_platform_ops = {
	.usb_det = bmu1760_usb_det,
	.usb_vbus_output = bmu1760_usb_vbus_output,
	.cfg_pmux_para = bmu1760_cfg_pmux_para,
	.get_pmu_name = bmu1760_get_pmu_name,
	.get_pmu_dev  = bmu1760_get_pmu_dev,
};

static const struct i2c_device_id bmu1760_id_table[] = {
	{ "bmu1760", 0 },
	{}
};

static const struct of_device_id bmu1760_dt_ids[] = {
	{ .compatible = "bmu1760", },
	{},
};
MODULE_DEVICE_TABLE(of, bmu1760_dt_ids);

static int bmu1760_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int ret;
	struct axp_dev *bmu1760;
	struct device_node *node = client->dev.of_node;

	bmu1760_pmu_num = axp_get_pmu_num(bmu1760_cn_mapping,
				ARRAY_SIZE(bmu1760_cn_mapping));
	if (bmu1760_pmu_num < 0) {
		pr_err("%s get pmu num failed\n", __func__);
		return bmu1760_pmu_num;
	}

	if (node) {
		/* get dt and sysconfig */
		if (!of_device_is_available(node)) {
			bmu1760_config.pmu_used = 0;
			pr_err("%s: pmu_used = %u\n", __func__,
					bmu1760_config.pmu_used);
			return -EPERM;
		} else {
			bmu1760_config.pmu_used = 1;
			ret = axp_dt_parse(node, bmu1760_pmu_num,
					&bmu1760_config);
			if (ret) {
				pr_err("%s parse device tree err\n", __func__);
				return -EINVAL;
			}
		}
	} else {
		pr_err("AXP22x device tree err!\n");
		return -EBUSY;
	}

	bmu1760 = devm_kzalloc(&client->dev, sizeof(*bmu1760), GFP_KERNEL);
	if (!bmu1760)
		return -ENOMEM;

	bmu1760->dev = &client->dev;
	bmu1760->nr_cells = ARRAY_SIZE(bmu1760_cells);
	bmu1760->cells = bmu1760_cells;
	bmu1760->pmu_num = bmu1760_pmu_num;

	ret = axp_mfd_cell_name_init(bmu1760_cn_mapping,
				ARRAY_SIZE(bmu1760_cn_mapping), bmu1760->pmu_num,
				bmu1760->nr_cells, bmu1760->cells);
	if (ret)
		return ret;

	bmu1760->regmap = axp_regmap_init_i2c(&client->dev);
	if (IS_ERR(bmu1760->regmap)) {
		ret = PTR_ERR(bmu1760->regmap);
		dev_err(&client->dev, "regmap init failed: %d\n", ret);
		return ret;
	}

	ret = bmu1760_init_chip(bmu1760);
	if (ret)
		return ret;

	ret = axp_mfd_add_devices(bmu1760);
	if (ret) {
		dev_err(bmu1760->dev, "failed to add MFD devices: %d\n", ret);
		return ret;
	}

	bmu1760->irq_data = axp_irq_chip_register(bmu1760->regmap, client->irq,
						IRQF_SHARED
						| IRQF_NO_SUSPEND,
						&bmu1760_regmap_irq_chip,
						bmu1760_wakeup_event);
	if (IS_ERR(bmu1760->irq_data)) {
		ret = PTR_ERR(bmu1760->irq_data);
		dev_err(&client->dev, "axp init irq failed: %d\n", ret);
		return ret;
	}

	bmu1760_pm_power = bmu1760;

	if (!pm_power_off)
		pm_power_off = bmu1760_power_off;

	axp_platform_ops_set(bmu1760->pmu_num, &bmu1760_platform_ops);

	bmu1760_ws = wakeup_source_register("bmu1760_wakeup_source");

	return 0;
}

static int bmu1760_remove(struct i2c_client *client)
{
	struct axp_dev *bmu1760 = i2c_get_clientdata(client);

	if (bmu1760 == bmu1760_pm_power) {
		bmu1760_pm_power = NULL;
		pm_power_off = NULL;
	}

	axp_mfd_remove_devices(bmu1760);
	axp_irq_chip_unregister(client->irq, bmu1760->irq_data);

	return 0;
}

/*
 * enable restart pmic when pwrok drive low
 */
static void bmu1760_shutdown(struct i2c_client *client)
{
	axp_regmap_set_bits(bmu1760_pm_power->regmap, 0x14, 0x40);
}

static struct i2c_driver bmu1760_driver = {
	.driver = {
		.name   = "bmu1760",
		.owner  = THIS_MODULE,
		.of_match_table = bmu1760_dt_ids,
	},
	.probe      = bmu1760_probe,
	.remove     = bmu1760_remove,
	.shutdown   = bmu1760_shutdown,
	.id_table   = bmu1760_id_table,
};

static int __init bmu1760_i2c_init(void)
{
	int ret;

	ret = i2c_add_driver(&bmu1760_driver);
	if (ret != 0)
		pr_err("Failed to register bmu1760 I2C driver: %d\n", ret);
	return ret;
}
subsys_initcall(bmu1760_i2c_init);

static void __exit bmu1760_i2c_exit(void)
{
	i2c_del_driver(&bmu1760_driver);
}
module_exit(bmu1760_i2c_exit);

MODULE_DESCRIPTION("PMIC Driver for bmu1760");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
MODULE_LICENSE("GPL");

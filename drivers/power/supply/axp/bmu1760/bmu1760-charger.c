/*
 * drivers/power/axp/axp22x/axp22x-charger.c
 * (C) Copyright 2010-2017
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * charger driver of axp22x
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/power_supply.h>
#include "../axp-core.h"
#include "../axp-charger.h"
#include "bmu1760-charger.h"

static int bmu1760_get_ac_voltage(struct axp_charger_dev *cdev)
{
	return 0;
}

static int bmu1760_get_ac_current(struct axp_charger_dev *cdev)
{
	return 0;
}

static int bmu1760_set_ac_vhold(struct axp_charger_dev *cdev, int vol)
{
	return 0;
}

static int bmu1760_get_ac_vhold(struct axp_charger_dev *cdev)
{
	return 0;
}

static int bmu1760_set_ac_ihold(struct axp_charger_dev *cdev, int cur)
{
	return 0;
}

static int bmu1760_get_ac_ihold(struct axp_charger_dev *cdev)
{
	return 0;
}

static struct axp_ac_info bmu1760_ac_info = {
	.det_bit         = 1,
	.det_offset      = 0,
	.get_ac_voltage  = bmu1760_get_ac_voltage,
	.get_ac_current  = bmu1760_get_ac_current,
	.set_ac_vhold    = bmu1760_set_ac_vhold,
	.get_ac_vhold    = bmu1760_get_ac_vhold,
	.set_ac_ihold    = bmu1760_set_ac_ihold,
	.get_ac_ihold    = bmu1760_get_ac_ihold,
};

static int bmu1760_get_usb_voltage(struct axp_charger_dev *cdev)
{
	return 0;
}

static int bmu1760_get_usb_current(struct axp_charger_dev *cdev)
{
	return 0;
}

static int bmu1760_set_usb_vhold(struct axp_charger_dev *cdev, int vol)
{
	return 0;
}

static int bmu1760_get_usb_vhold(struct axp_charger_dev *cdev)
{
	return 0;
}

static int bmu1760_set_usb_ihold(struct axp_charger_dev *cdev, int cur)
{
	return 0;
}

static int bmu1760_get_usb_ihold(struct axp_charger_dev *cdev)
{
	return 0;
}

static struct axp_usb_info bmu1760_usb_info = {
	.get_usb_voltage = bmu1760_get_usb_voltage,
	.get_usb_current = bmu1760_get_usb_current,
	.set_usb_vhold   = bmu1760_set_usb_vhold,
	.get_usb_vhold   = bmu1760_get_usb_vhold,
	.set_usb_ihold   = bmu1760_set_usb_ihold,
	.get_usb_ihold   = bmu1760_get_usb_ihold,
};

static int bmu1760_get_rest_cap(struct axp_charger_dev *cdev)
{
	u8 val, temp_val[2], batt_max_cap_val[2];
	int batt_max_cap, coulumb_counter;
	int rest_vol;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_read(map, BMU1760_CAP, &val);
	if (!(val & 0x80))
		return 0;

	rest_vol = (int) (val & 0x7F);

	axp_regmap_reads(map, 0xe2, 2, temp_val);
	coulumb_counter = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
						* 1456 / 1000;

	axp_regmap_reads(map, 0xe0, 2, temp_val);
	batt_max_cap = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
						* 1456 / 1000;

	/* Avoid the power stay in 100% for a long time. */
	if (coulumb_counter > batt_max_cap) {
		batt_max_cap_val[0] = temp_val[0] | (0x1<<7);
		batt_max_cap_val[1] = temp_val[1];
		axp_regmap_writes(map, 0xe2, 2, batt_max_cap_val);
		AXP_DEBUG(AXP_SPLY, cdev->chip->pmu_num,
				"Axp259 coulumb_counter = %d\n", batt_max_cap);
	}

	return rest_vol;
}

static int bmu1760_get_bat_health(struct axp_charger_dev *cdev)
{
	return POWER_SUPPLY_HEALTH_GOOD;
}

static inline int bmu1760_vbat_to_mV(u32 reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 1200 / 1000;
}

static int bmu1760_get_vbat(struct axp_charger_dev *cdev)
{
	u8 tmp[2];
	u32 res;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_reads(map, BMU1760_VBATH_RES, 2, tmp);
	res = (tmp[0] << 8) | tmp[1];

	return bmu1760_vbat_to_mV(res);
}

static inline int bmu1760_ibat_to_mA(u32 reg)
{
	return (int)(((reg >> 8) << 4) | (reg & 0x000F));
}

static inline int bmu1760_icharge_to_mA(u32 reg)
{
	return (int)(((reg >> 8) << 4) | (reg & 0x000F));
}

static int bmu1760_get_ibat(struct axp_charger_dev *cdev)
{
	u8 tmp[2];
	u32 res;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_reads(map, BMU1760_IBATH_REG, 2, tmp);
	res = (tmp[0] << 8) | tmp[1];

	return bmu1760_icharge_to_mA(res);
}

static int bmu1760_get_disibat(struct axp_charger_dev *cdev)
{
	u8 tmp[2];
	u32 dis_res;
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_reads(map, BMU1760_DISIBATH_REG, 2, tmp);
	dis_res = (tmp[0] << 8) | tmp[1];

	return bmu1760_ibat_to_mA(dis_res);
}
#if 0
static int bmu1760_set_chg_cur(struct axp_charger_dev *cdev, int cur)
{
	uint8_t tmp = 0;
	struct axp_regmap *map = cdev->chip->regmap;

	if (cur == 0)
		axp_regmap_clr_bits(map, BMU1760_CHARGE_CONTROL1, 0x80);
	else
		axp_regmap_set_bits(map, BMU1760_CHARGE_CONTROL1, 0x80);

	if (cur >= 400 && cur <= 3400) {
		tmp = (cur - 400) / 200;
		axp_regmap_update(map, BMU1760_CHARGE_CONTROL2, tmp, 0x1F);
	} else if (cur < 400) {
		axp_regmap_clr_bits(map, BMU1760_CHARGE_CONTROL2, 0x1F);
	} else {
		axp_regmap_set_bits(map, BMU1760_CHARGE_CONTROL2, 0x1F);
	}

	return 0;
}

static int bmu1760_set_chg_vol(struct axp_charger_dev *cdev, int vol)
{
	uint8_t tmp = 0;
	struct axp_regmap *map = cdev->chip->regmap;

	if (vol < 4100) {
		axp_regmap_clr_bits(map, BMU1760_CHARGE_CONTROL2, 0xe0);
	} else if (vol >= 4100 && vol <= 4250) {
		tmp = (vol - 4100) / 50;
		tmp <<= 5;
		axp_regmap_update(map, BMU1760_CHARGE_CONTROL2, tmp, 0xe0);
	} else if (vol >= 4350 && vol <= 4500) {
		tmp = (vol - 4350) / 50;
		tmp |= (0x1 << 2);
		tmp <<= 5;
		axp_regmap_update(map, BMU1760_CHARGE_CONTROL2, tmp, 0xe0);
	} else if (vol > 4500) {
		axp_regmap_set_bits(map, BMU1760_CHARGE_CONTROL2, 0xe0);
	} else {
		pr_warn("unsupported voltage: %dmv, use default 4200mv\n", vol);
		axp_regmap_update(map, BMU1760_CHARGE_CONTROL2, 0x2 << 5, 0xe0);
	}

	return 0;
}
#endif
static struct axp_battery_info bmu1760_batt_info = {
	.chgstat_bit          = 2,			//2--4
	.chgstat_offset       = 0,
	.det_bit              = 4,
	.det_offset           = 2,
	.det_valid            = 3,
	.cur_direction_bit    = 0,
	.cur_direction_offset = 2,
	.get_rest_cap         = bmu1760_get_rest_cap,
	.get_bat_health       = bmu1760_get_bat_health,
	.get_vbat             = bmu1760_get_vbat,
	.get_ibat             = bmu1760_get_ibat,
	.get_disibat          = bmu1760_get_disibat,
	//.set_chg_cur          = bmu1760_set_chg_cur,
	//.set_chg_vol          = bmu1760_set_chg_vol,		//this is not available in 1760
};

static struct power_supply_info battery_data = {
	.name = "PTI PL336078",
	.technology = POWER_SUPPLY_TECHNOLOGY_LiFe,
	.voltage_max_design = 4200000,
	.voltage_min_design = 3500000,
	.use_for_apm = 1,
};

static struct axp_supply_info bmu1760_spy_info = {
	.ac   = &bmu1760_ac_info,
	.usb  = &bmu1760_usb_info,
	.batt = &bmu1760_batt_info,
};

static int bmu1760_charger_init(struct axp_dev *axp_dev)
{
	u8 ocv_cap[32];
	u8 val = 0;
	int cur_coulomb_counter, rdc;
	struct axp_regmap *map = axp_dev->regmap;

	if (bmu1760_config.pmu_init_chg_pretime < 40)
		bmu1760_config.pmu_init_chg_pretime = 40;
	val = (bmu1760_config.pmu_init_chg_pretime - 40)/10;
	if (val >= 3)
		val = 3;
	val = 0x80 + (val<<6);
	axp_regmap_update(map, 0x8e, val, 0xe0);

	if (bmu1760_config.pmu_init_chg_csttime == 360*5)
		val = 0;
	else if (bmu1760_config.pmu_init_chg_csttime == 360*8)
		val = 1;
	else if (bmu1760_config.pmu_init_chg_csttime == 360*12)
		val = 2;
	else if (bmu1760_config.pmu_init_chg_csttime == 360*20)
		val = 3;
	else
		val = 0;
	val = (val << 1) + 0x01;
	axp_regmap_update(map, 0x8d, val, 0x07);

	/* adc set */
	val = BMU1760_ADC_BATVOL_ENABLE | BMU1760_ADC_BATCUR_ENABLE;
	if (bmu1760_config.pmu_bat_temp_enable != 0)
		val = val | BMU1760_ADC_TSVOL_ENABLE;
	axp_regmap_update(map, BMU1760_ADC_CONTROL, val,
						BMU1760_ADC_BATVOL_ENABLE
						| BMU1760_ADC_BATCUR_ENABLE
						| BMU1760_ADC_TSVOL_ENABLE);

	axp_regmap_read(map, BMU1760_TS_PIN_CONTROL, &val);
	switch (bmu1760_config.pmu_init_adc_freq / 100) {
	case 1:
		val &= ~(3 << 5);
		break;
	case 2:
		val &= ~(3 << 5);
		val |= 1 << 5;
		break;
	case 4:
		val &= ~(3 << 5);
		val |= 2 << 5;
		break;
	case 8:
		val |= 3 << 5;
		break;
	default:
		break;
	}

	if (bmu1760_config.pmu_bat_temp_enable != 0)
		val &= (~(1 << 7));
	axp_regmap_write(map, BMU1760_TS_PIN_CONTROL, val);

	/* bat para */
	axp_regmap_write(map, BMU1760_WARNING_LEVEL,
		((bmu1760_config.pmu_battery_warning_level1 - 5) << 4)
		+ bmu1760_config.pmu_battery_warning_level2);
#if 0
	/* set target voltage */
	if ((bmu1760_config.pmu_init_chgvol >= 4100)
		&& (bmu1760_config.pmu_init_chgvol <= 4250))
		val = (bmu1760_config.pmu_init_chgvol - 4100) / 50;
	else if ((bmu1760_config.pmu_init_chgvol >= 4350)
		&& (bmu1760_config.pmu_init_chgvol <= 4500))
		val = (bmu1760_config.pmu_init_chgvol - 4150) / 50;
	else
		val = 2;

	val <<= 5;
	axp_regmap_update(map, BMU1760_CHARGE_CONTROL2, val, 0xe0);
#endif

	ocv_cap[0]  = bmu1760_config.pmu_bat_para1;
	ocv_cap[1]  = bmu1760_config.pmu_bat_para2;
	ocv_cap[2]  = bmu1760_config.pmu_bat_para3;
	ocv_cap[3]  = bmu1760_config.pmu_bat_para4;
	ocv_cap[4]  = bmu1760_config.pmu_bat_para5;
	ocv_cap[5]  = bmu1760_config.pmu_bat_para6;
	ocv_cap[6]  = bmu1760_config.pmu_bat_para7;
	ocv_cap[7]  = bmu1760_config.pmu_bat_para8;
	ocv_cap[8]  = bmu1760_config.pmu_bat_para9;
	ocv_cap[9]  = bmu1760_config.pmu_bat_para10;
	ocv_cap[10] = bmu1760_config.pmu_bat_para11;
	ocv_cap[11] = bmu1760_config.pmu_bat_para12;
	ocv_cap[12] = bmu1760_config.pmu_bat_para13;
	ocv_cap[13] = bmu1760_config.pmu_bat_para14;
	ocv_cap[14] = bmu1760_config.pmu_bat_para15;
	ocv_cap[15] = bmu1760_config.pmu_bat_para16;
	ocv_cap[16] = bmu1760_config.pmu_bat_para17;
	ocv_cap[17] = bmu1760_config.pmu_bat_para18;
	ocv_cap[18] = bmu1760_config.pmu_bat_para19;
	ocv_cap[19] = bmu1760_config.pmu_bat_para20;
	ocv_cap[20] = bmu1760_config.pmu_bat_para21;
	ocv_cap[21] = bmu1760_config.pmu_bat_para22;
	ocv_cap[22] = bmu1760_config.pmu_bat_para23;
	ocv_cap[23] = bmu1760_config.pmu_bat_para24;
	ocv_cap[24] = bmu1760_config.pmu_bat_para25;
	ocv_cap[25] = bmu1760_config.pmu_bat_para26;
	ocv_cap[26] = bmu1760_config.pmu_bat_para27;
	ocv_cap[27] = bmu1760_config.pmu_bat_para28;
	ocv_cap[28] = bmu1760_config.pmu_bat_para29;
	ocv_cap[29] = bmu1760_config.pmu_bat_para30;
	ocv_cap[30] = bmu1760_config.pmu_bat_para31;
	ocv_cap[31] = bmu1760_config.pmu_bat_para32;
	axp_regmap_writes(map, 0xC0, 32, ocv_cap);

	/*Init CHGLED function*/
	if (bmu1760_config.pmu_chgled_func)
		axp_regmap_set_bits(map, 0x90, 0x02); /* control by charger */
	else
		axp_regmap_set_bits(map, 0x90, 0x07); /* drive MOTO */

	/*set CHGLED Indication Type*/
	if (bmu1760_config.pmu_chgled_type)
		axp_regmap_set_bits(map, 0x90, 0x01); /* Type B */
	else
		axp_regmap_clr_bits(map, 0x90, 0x07); /* Type A */

	/*Init battery capacity correct function*/
	if (bmu1760_config.pmu_batt_cap_correct)
		axp_regmap_set_bits(map, 0xb8, 0x20); /* enable */
	else
		axp_regmap_clr_bits(map, 0xb8, 0x20); /* disable */

	/*battery detect enable*/
	if (bmu1760_config.pmu_batdeten)
		axp_regmap_set_bits(map, 0x8e, 0x08);
	else
		axp_regmap_clr_bits(map, 0x8e, 0x08);

	/* RDC initial */
	axp_regmap_read(map, BMU1760_RDC0, &val);
	if ((bmu1760_config.pmu_battery_rdc) && (!(val & 0x40))) {
		rdc = (bmu1760_config.pmu_battery_rdc * 10000 + 5371) / 10742;
		axp_regmap_write(map, BMU1760_RDC0, ((rdc >> 8) & 0x1F)|0x80);
		axp_regmap_write(map, BMU1760_RDC1, rdc & 0x00FF);
	}

	axp_regmap_read(map, BMU1760_BATCAP0, &val);
	if ((bmu1760_config.pmu_battery_cap) && (!(val & 0x80))) {
		cur_coulomb_counter = bmu1760_config.pmu_battery_cap
					* 1000 / 1456;
		axp_regmap_write(map, BMU1760_BATCAP0,
					((cur_coulomb_counter >> 8) | 0x80));
		axp_regmap_write(map, BMU1760_BATCAP1,
					cur_coulomb_counter & 0x00FF);
	} else if (!bmu1760_config.pmu_battery_cap) {
		axp_regmap_write(map, BMU1760_BATCAP0, 0x00);
		axp_regmap_write(map, BMU1760_BATCAP1, 0x00);
	}

	if (bmu1760_config.pmu_bat_unused == 1)
		bmu1760_spy_info.batt->det_unused = 1;
	else
		bmu1760_spy_info.batt->det_unused = 0;

	bmu1760_spy_info.usb->det_unused = 1;

	if (bmu1760_config.pmu_bat_temp_enable != 0) {
		axp_regmap_write(map, BMU1760_VLTF_CHARGE,
				bmu1760_config.pmu_bat_charge_ltf * 10 / 128);
		axp_regmap_write(map, BMU1760_VHTF_CHARGE,
				bmu1760_config.pmu_bat_charge_htf * 10 / 128);
		axp_regmap_write(map, BMU1760_VLTF_WORK,
				bmu1760_config.pmu_bat_shutdown_ltf * 10 / 128);
		axp_regmap_write(map, BMU1760_VHTF_WORK,
				bmu1760_config.pmu_bat_shutdown_htf * 10 / 128);
	}

	return 0;
}

static struct axp_interrupts bmu1760_charger_irq[] = {
	{"ac in",         axp_ac_in_isr},
	{"ac out",        axp_ac_out_isr},
	{"bat in",        axp_capchange_isr},
	{"bat out",       axp_capchange_isr},
	{"charging",      axp_change_isr},
	{"charge over",   axp_change_isr},
	{"low warning1",  axp_low_warning1_isr},
	{"low warning2",  axp_low_warning2_isr},
};

static void bmu1760_private_debug(struct axp_charger_dev *cdev)
{
	u8 tmp[2];
	struct axp_regmap *map = cdev->chip->regmap;

	axp_regmap_reads(map, 0x5a, 2, tmp);
	AXP_DEBUG(AXP_SPLY, cdev->chip->pmu_num,
			"acin_vol = %d\n", ((tmp[0] << 4) | (tmp[1] & 0xF))
			* 8);

	axp_regmap_reads(map, 0xbc, 2, tmp);
	AXP_DEBUG(AXP_SPLY, cdev->chip->pmu_num,
			"ocv_vol = %d\n", ((tmp[0] << 4) | (tmp[1] & 0xF))
			* 1200 / 1000);

	axp_regmap_read(map, 0xe4, &tmp[0]);
	if (tmp[0] & 0x80)
		AXP_DEBUG(AXP_SPLY, cdev->chip->pmu_num,
			"ocv_percent = %d\n", tmp[0] & 0x7f);

	axp_regmap_read(map, 0xe5, &tmp[0]);
	if (tmp[0] & 0x80)
		AXP_DEBUG(AXP_SPLY, cdev->chip->pmu_num,
			"coulomb_percent = %d\n", tmp[0] & 0x7f);
}

static int bmu1760_charger_probe(struct platform_device *pdev)
{
	int ret, i, irq;
	struct axp_charger_dev *chg_dev;
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	if (pdev->dev.of_node) {
		/* get dt and sysconfig */
		ret = axp_charger_dt_parse(pdev->dev.of_node, &bmu1760_config);
		if (ret) {
			pr_err("%s parse device tree err\n", __func__);
			return -EINVAL;
		}
	} else {
		pr_err("axp22 charger device tree err!\n");
		return -EBUSY;
	}

	bmu1760_ac_info.ac_vol = bmu1760_config.pmu_ac_vol;
	bmu1760_ac_info.ac_cur = bmu1760_config.pmu_ac_cur;
	bmu1760_batt_info.runtime_chgcur = bmu1760_config.pmu_runtime_chgcur;
	bmu1760_batt_info.suspend_chgcur = bmu1760_config.pmu_suspend_chgcur;
	bmu1760_batt_info.shutdown_chgcur = bmu1760_config.pmu_shutdown_chgcur;
	battery_data.voltage_max_design = bmu1760_config.pmu_init_chgvol
								* 1000;
	battery_data.voltage_min_design = bmu1760_config.pmu_pwroff_vol
								* 1000;
	battery_data.energy_full_design = bmu1760_config.pmu_battery_cap;

	bmu1760_charger_init(axp_dev);

	chg_dev = axp_power_supply_register(&pdev->dev, axp_dev,
					&battery_data, &bmu1760_spy_info);
	if (IS_ERR_OR_NULL(chg_dev))
		goto fail;
	chg_dev->private_debug = bmu1760_private_debug;

	for (i = 0; i < ARRAY_SIZE(bmu1760_charger_irq); i++) {
		irq = platform_get_irq_byname(pdev, bmu1760_charger_irq[i].name);
		if (irq < 0)
			continue;

		ret = axp_request_irq(axp_dev, irq,
				bmu1760_charger_irq[i].isr, chg_dev);
		if (ret != 0) {
			dev_err(&pdev->dev, "failed to request %s IRQ %d: %d\n",
					bmu1760_charger_irq[i].name, irq, ret);
			goto out_irq;
		}

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
			bmu1760_charger_irq[i].name, irq, ret);
	}

	platform_set_drvdata(pdev, chg_dev);

	return 0;

out_irq:
	for (i = i - 1; i >= 0; i--) {
		irq = platform_get_irq_byname(pdev, bmu1760_charger_irq[i].name);
		if (irq < 0)
			continue;
		axp_free_irq(axp_dev, irq);
	}
fail:
	return -1;
}

static int bmu1760_charger_remove(struct platform_device *pdev)
{
	int i, irq;
	struct axp_charger_dev *chg_dev = platform_get_drvdata(pdev);
	struct axp_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);

	for (i = 0; i < ARRAY_SIZE(bmu1760_charger_irq); i++) {
		irq = platform_get_irq_byname(pdev, bmu1760_charger_irq[i].name);
		if (irq < 0)
			continue;
		axp_free_irq(axp_dev, irq);
	}

	axp_power_supply_unregister(chg_dev);

	return 0;
}

static int bmu1760_charger_suspend(struct platform_device *dev,
				pm_message_t state)
{
	struct axp_charger_dev *chg_dev = platform_get_drvdata(dev);

	axp_suspend_flag = AXP_WAS_SUSPEND;
	axp_charger_suspend(chg_dev);

	return 0;
}

static int bmu1760_charger_resume(struct platform_device *dev)
{
	struct axp_charger_dev *chg_dev = platform_get_drvdata(dev);
	struct axp_regmap *map = chg_dev->chip->regmap;
	int pre_rest_vol;

	if (axp_suspend_flag == AXP_SUSPEND_WITH_IRQ) {
		axp_suspend_flag = AXP_NOT_SUSPEND;
#ifdef CONFIG_AXP_NMI_USED
		enable_nmi();
#endif
	} else {
		axp_suspend_flag = AXP_NOT_SUSPEND;
	}

	pre_rest_vol = chg_dev->rest_vol;
	axp_charger_resume(chg_dev);

	if (chg_dev->rest_vol - pre_rest_vol) {
		pr_info("battery vol change: %d->%d\n",
				pre_rest_vol, chg_dev->rest_vol);
		axp_regmap_write(map, 0x05, chg_dev->rest_vol | 0x80);
	}

	return 0;
}

static void bmu1760_charger_shutdown(struct platform_device *dev)
{
	struct axp_charger_dev *chg_dev = platform_get_drvdata(dev);

	axp_charger_shutdown(chg_dev);
}

static const struct of_device_id bmu1760_charger_dt_ids[] = {
	{ .compatible = "bmu1760-charger", },
	{},
};
MODULE_DEVICE_TABLE(of, bmu1760_charger_dt_ids);

static struct platform_driver bmu1760_charger_driver = {
	.driver     = {
		.name   = "bmu1760-charger",
		.of_match_table = bmu1760_charger_dt_ids,
	},
	.probe    = bmu1760_charger_probe,
	.remove   = bmu1760_charger_remove,
	.suspend  = bmu1760_charger_suspend,
	.resume   = bmu1760_charger_resume,
	.shutdown = bmu1760_charger_shutdown,
};

static int __init bmu1760_charger_initcall(void)
{
	int ret;

	ret = platform_driver_register(&bmu1760_charger_driver);
	if (IS_ERR_VALUE(ret)) {
		pr_err("%s: failed, errno %d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}
fs_initcall_sync(bmu1760_charger_initcall);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
MODULE_DESCRIPTION("Charger Driver for bmu1760 PMIC");
MODULE_ALIAS("platform:bmu1760-charger");

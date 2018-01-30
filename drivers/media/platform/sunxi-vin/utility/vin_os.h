/*
 * linux-4.9/drivers/media/platform/sunxi-vin/utility/vin_os.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 ******************************************************************************
 *
 * vin_os.h
 *
 * Hawkview ISP - vin_os.h module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version         Author         Date           Description
 *
 *   3.0        Yang Feng      2015/12/02      ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#ifndef __VIN__OS__H__
#define __VIN__OS__H__

#include <linux/device.h>
#include <linux/clk.h>
#include <linux/clk/sunxi.h>
#include <linux/interrupt.h>
#include "../platform/platform_cfg.h"

#include <linux/dma-mapping.h>

#define IS_FLAG(x, y) (((x)&(y)) == y)

#define VIN_LOG_MD				(1 << 0)	/*0x0 */
#define VIN_LOG_FLASH				(1 << 1)	/*0x2 */
#define VIN_LOG_CCI				(1 << 2)	/*0x4 */
#define VIN_LOG_CSI				(1 << 3)	/*0x8 */
#define VIN_LOG_MIPI				(1 << 4)	/*0x10*/
#define VIN_LOG_ISP				(1 << 5)	/*0x20*/
#define VIN_LOG_STAT				(1 << 6)	/*0x40*/
#define VIN_LOG_SCALER				(1 << 7)	/*0x80*/
#define VIN_LOG_POWER				(1 << 8)	/*0x100*/
#define VIN_LOG_CONFIG				(1 << 9)	/*0x200*/
#define VIN_LOG_VIDEO				(1 << 10)	/*0x400*/
#define VIN_LOG_FMT				(1 << 11)	/*0x800*/

extern unsigned int vin_log_mask;

#define vin_log(flag, arg...) do { \
	if (flag & vin_log_mask) { \
		switch (flag) { \
		case VIN_LOG_MD: \
			pr_debug("[VIN_LOG_MD]" arg); \
			break; \
		case VIN_LOG_FLASH: \
			pr_debug("[VIN_LOG_FLASH]" arg); \
			break; \
		case VIN_LOG_CCI: \
			pr_debug("[VIN_LOG_CCI]" arg); \
			break; \
		case VIN_LOG_CSI: \
			pr_debug("[VIN_LOG_CSI]" arg); \
			break; \
		case VIN_LOG_MIPI: \
			pr_debug("[VIN_LOG_MIPI]" arg); \
			break; \
		case VIN_LOG_ISP: \
			pr_debug("[VIN_LOG_ISP]" arg); \
			break; \
		case VIN_LOG_STAT: \
			pr_debug("[VIN_LOG_STAT]" arg); \
			break; \
		case VIN_LOG_SCALER: \
			pr_debug("[VIN_LOG_SCALER]" arg); \
			break; \
		case VIN_LOG_POWER: \
			pr_debug("[VIN_LOG_POWER]" arg); \
			break; \
		case VIN_LOG_CONFIG: \
			pr_debug("[VIN_LOG_CONFIG]" arg); \
			break; \
		case VIN_LOG_VIDEO: \
			pr_debug("[VIN_LOG_VIDEO]" arg); \
			break; \
		case VIN_LOG_FMT: \
			pr_debug("[VIN_LOG_FMT]" arg); \
			break; \
		default: \
			pr_debug("[VIN_LOG]" arg); \
			break; \
		} \
	} \
} while (0)

#define vin_err(x, arg...) pr_err("[VIN_ERR]"x, ##arg)
#define vin_warn(x, arg...) pr_warn("[VIN_WARN]"x, ##arg)
#define vin_print(x, arg...) pr_info("[VIN]"x, ##arg)

struct vin_mm {
	size_t size;
	void *phy_addr;
	void *vir_addr;
	void *dma_addr;
	struct ion_client *client;
	struct ion_handle *handle;
};

extern int os_gpio_set(struct gpio_config *gpio_list);
extern int os_gpio_write(u32 gpio, __u32 out_value, int force_value_flag);
extern int os_mem_alloc(struct device *dev, struct vin_mm *mem_man);
extern void os_mem_free(struct device *dev, struct vin_mm *mem_man);

#endif	/*__VIN__OS__H__*/

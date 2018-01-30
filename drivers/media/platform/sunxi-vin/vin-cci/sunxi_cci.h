/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-cci/sunxi_cci.h
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
 * sunxi_cci.h
 *
 * Hawkview ISP - sunxi_cci.h module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version         Author         Date         Description
 *
 *   2.0         Yang Feng     2014/06/23     Second Version
 *
 ******************************************************************************
 */

#ifndef _SUNXI_CCI_H_
#define _SUNXI_CCI_H_

#include "../platform/platform_cfg.h"

struct cci_dev {
	struct platform_device *pdev;
	unsigned int id;
	spinlock_t slock;
	int irq;
	int use_cnt;
	wait_queue_head_t wait;

	void __iomem *base;

	struct list_head cci_list;
	struct pinctrl *pctrl;
	struct clk *clock;
};

#endif /*_SUNXI_CCI_H_*/

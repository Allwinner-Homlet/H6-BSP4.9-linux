/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-isp/isp_platform_drv.c
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
 * isp_platform_drv.c
 *
 * Hawkview ISP - isp_platform_drv.c module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http:
 *
 * Version          Author         Date       Description
 *
 *   2.0         Yang Feng      2014/06/20    Second Version
 *
 ******************************************************************************
 */
#include <linux/kernel.h>
#include "isp_platform_drv.h"
#include "sun8iw12p1/sun8iw12p1_isp_reg_cfg.h"
struct isp_platform_drv *isp_platform_curr;

int isp_platform_register(struct isp_platform_drv *isp_drv)
{
	int platform_id;

	platform_id = isp_drv->platform_id;
	isp_platform_curr = isp_drv;
	return 0;
}

int isp_platform_init(unsigned int platform_id)
{
	switch (platform_id) {
	case ISP_PLATFORM_SUN8IW12P1:
		sun8iw12p1_isp_platform_register();
		break;
	default:
		sun8iw12p1_isp_platform_register();
		break;
	}
	return 0;
}

struct isp_platform_drv *isp_get_driver(void)
{
	if (isp_platform_curr == NULL)
		pr_err("[ISP ERR] isp platform curr have not init!\n");
	return isp_platform_curr;
}



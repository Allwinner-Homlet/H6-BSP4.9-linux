/*
 * sound\soc\sunxi\sunxi-i2s.h
 * (C) Copyright 2010-2017
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SUNXI_I2S_H__
#define __SUNXI_I2S_H__
#include "sunxi-pcm.h"
struct sunxi_i2s {
	struct clk *pllclk;
	struct clk *moduleclk;
	struct clk *ac_rst;
	void __iomem  *sunxi_i2s_membase;
	struct sunxi_dma_params play_dma_param;
	struct sunxi_dma_params capture_dma_param;
	struct snd_soc_dai_driver dai;
};
#endif

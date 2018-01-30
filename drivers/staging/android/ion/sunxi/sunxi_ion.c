/*
 * Allwinner SUNXI ION Driver
 *
 * Copyright (c) 2017 Allwinnertech.
 *
 * Author: fanqinghua <fanqinghua@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "Ion: " fmt

#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/mm.h>
#include "../ion_priv.h"
#include "../ion.h"
#include "../ion_of.h"

struct sunxi_ion_dev {
	struct ion_heap	**heaps;
	struct ion_device *idev;
	struct ion_platform_data *data;
};
struct device *g_ion_dev;
struct ion_device *idev;
/* export for IMG GPU(sgx544) */
EXPORT_SYMBOL(idev);

static struct ion_of_heap sunxi_heaps[] = {
	PLATFORM_HEAP("allwinner,sys_user", 0,
		      ION_HEAP_TYPE_SYSTEM, "sys_user"),
	PLATFORM_HEAP("allwinner,sys_contig", 1,
		      ION_HEAP_TYPE_SYSTEM_CONTIG, "sys_contig"),
	PLATFORM_HEAP("allwinner,cma", ION_HEAP_TYPE_DMA, ION_HEAP_TYPE_DMA,
		      "cma"),
	PLATFORM_HEAP("allwinner,secure", ION_HEAP_TYPE_SECURE,
		      ION_HEAP_TYPE_SECURE, "secure"),
	{}
};

struct device *get_ion_dev(void)
{
	return g_ion_dev;
}

static int sunxi_ion_probe(struct platform_device *pdev)
{
	struct sunxi_ion_dev *ipdev;
	int i;

	ipdev = devm_kzalloc(&pdev->dev, sizeof(*ipdev), GFP_KERNEL);
	if (!ipdev)
		return -ENOMEM;

	g_ion_dev = &pdev->dev;
	platform_set_drvdata(pdev, ipdev);

	ipdev->idev = ion_device_create(NULL);
	if (IS_ERR(ipdev->idev))
		return PTR_ERR(ipdev->idev);

	idev = ipdev->idev;

	ipdev->data = ion_parse_dt(pdev, sunxi_heaps);
	if (IS_ERR(ipdev->data)) {
		pr_err("%s: ion_parse_dt error!\n", __func__);
		return PTR_ERR(ipdev->data);
	}

	ipdev->heaps = devm_kzalloc(&pdev->dev,
				sizeof(struct ion_heap) * ipdev->data->nr,
				GFP_KERNEL);
	if (!ipdev->heaps) {
		ion_destroy_platform_data(ipdev->data);
		return -ENOMEM;
	}

	for (i = 0; i < ipdev->data->nr; i++) {
		ipdev->heaps[i] = ion_heap_create(&ipdev->data->heaps[i]);
		if (!ipdev->heaps) {
			ion_destroy_platform_data(ipdev->data);
			return -ENOMEM;
		} else if (ipdev->heaps[i] == ERR_PTR(-EINVAL)) {
			return 0;
		}
		ion_device_add_heap(ipdev->idev, ipdev->heaps[i]);
	}
	return 0;
}

static int sunxi_ion_remove(struct platform_device *pdev)
{
	struct sunxi_ion_dev *ipdev;
	int i;

	ipdev = platform_get_drvdata(pdev);

	for (i = 0; i < ipdev->data->nr; i++)
		ion_heap_destroy(ipdev->heaps[i]);

	ion_destroy_platform_data(ipdev->data);
	ion_device_destroy(ipdev->idev);

	return 0;
}

static const struct of_device_id sunxi_ion_match_table[] = {
	{.compatible = "allwinner,sunxi-ion"},
	{},
};

static struct platform_driver sunxi_ion_driver = {
	.probe = sunxi_ion_probe,
	.remove = sunxi_ion_remove,
	.driver = {
		.name = "ion-sunxi",
		.of_match_table = sunxi_ion_match_table,
	},
};

static int __init sunxi_ion_init(void)
{
	return platform_driver_register(&sunxi_ion_driver);
}
subsys_initcall(sunxi_ion_init);

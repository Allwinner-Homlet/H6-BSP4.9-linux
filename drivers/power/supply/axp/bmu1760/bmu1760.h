/*
 * drivers/power/axp/bmu1760/bmu1760.h
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Pannan <pannan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#ifndef BMU1760_H_
#define BMU1760_H_

/* For BMU1760 */
#define BMU1760_STATUS              (0x00)
#define BMU1760_IC_TYPE             (0x03)
#define BMU1760_GPIO1_CTL           (0x18)
#define BMU1760_GPIO2_CTL           (0x19)
#define BMU1760_GPIO1_SIGNAL        (0x1A)
#define BMU1760_ADC_EN              (0x24)
#define BMU1760_TS_PIN_CONTROL      (0x81)
#define BMU1760_POK_SET             (0x15)
#define BMU1760_OFF_CTL             (0x28)
#define BMU1760_INTEN1              (0x40)
#define BMU1760_INTEN2              (0x41)
#define BMU1760_INTEN3              (0x42)
#define BMU1760_INTEN4              (0x43)
#define BMU1760_INTEN5              (0x44)
#define BMU1760_INTSTS1             (0x48)
#define BMU1760_INTSTS2             (0x49)
#define BMU1760_INTSTS3             (0x4A)
#define BMU1760_INTSTS4             (0x4B)
#define BMU1760_INTSTS5             (0x4C)
#define BMU1760_WARNING_LEVEL       (0xE6)
#define BMU1760_ADDR_EXTENSION      (0xFF)


/* bit definitions for AXP events ,irq event */
/* BMU1760 */
#define BMU1760_IRQ_ICTEMOV       (0)
#define BMU1760_IRQ_PMOSEN        (1)
#define BMU1760_IRQ_BUCKLO        (2)
#define BMU1760_IRQ_BUCKHI        (3)
#define BMU1760_IRQ_ACRE          (4)
#define BMU1760_IRQ_ACIN          (5)
#define BMU1760_IRQ_ACOV          (6)
#define BMU1760_IRQ_VACIN         (7)
#define BMU1760_IRQ_LOWN2         (8)
#define BMU1760_IRQ_LOWN1         (9)
#define BMU1760_IRQ_CHAOV         (10)
#define BMU1760_IRQ_CHAST         (11)
#define BMU1760_IRQ_BATSAFE_QUIT  (12)
#define BMU1760_IRQ_BATSAFE_ENTER (13)
#define BMU1760_IRQ_BATRE         (14)
#define BMU1760_IRQ_BATIN         (15)
#define BMU1760_IRQ_QBWUT         (16)
#define BMU1760_IRQ_BWUT          (17)
#define BMU1760_IRQ_QBWOT         (18)
#define BMU1760_IRQ_BWOT          (19)
#define BMU1760_IRQ_QBCUT         (20)
#define BMU1760_IRQ_BCUT          (21)
#define BMU1760_IRQ_QBCOT         (22)
#define BMU1760_IRQ_BCOT          (23)
#define BMU1760_IRQ_GPIO0         (24)
#define BMU1760_IRQ_BATCHG        (25)
#define BMU1760_IRQ_POKOFF        (26)
#define BMU1760_IRQ_POKLO         (27)
#define BMU1760_IRQ_POKSH         (28)
#define BMU1760_IRQ_PEKFE         (29)
#define BMU1760_IRQ_PEKRE         (30)
#define BMU1760_IRQ_BUCKOV_6V6    (32)

extern int axp_debug;
extern struct axp_config_info bmu1760_config;

#endif /* BMU1760_H_ */

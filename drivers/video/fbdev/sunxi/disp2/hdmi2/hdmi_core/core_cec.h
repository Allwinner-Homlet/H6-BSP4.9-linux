/*
 * linux-3.10/drivers/video/sunxi/disp2/hdmi2/hdmi_core/core_cec.h
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

#ifndef _CORE_CEC_H_
#define _CORE_CEC_H_

#define CEC_BUF_SIZE 32

#define IMAGE_VIEW_ON			0x4

#define	ACTIVE_SOURCE			0x82
#define INACTIVE_SOURCE			0x9d
#define REQUEST_ACTIVE_SOURCE		0x85

#define ROUTING_CHANGE			0x80
#define ROUTING_INFORMATION		0x81
#define SET_STREAM_PATH			0x86


#define STANDBY				0x36

#define ABORT				0xff
#define FEATURE_ABORT			0x0

#define GIVE_PHYSICAL_ADDRESS		0x83
#define REPORT_PHYSICAL_ADDRESS		0x84

#define GIVE_DEVICE_VENDOR_ID		0x8c
#define DEVICE_VENDOR_ID			0x87

#define GIVE_DEVICE_POWER_STATUS	0x8f
#define REPORT_POWER_STATUS		0x90

enum cec_power_status {
	CEC_POWER_ON = 0,
	CEC_STANDBY = 1,
	CEC_STANDBY_TO_ON = 2,
	CEC_ON_TO_STANDBY = 3,
};

enum cec_logic_addr {
	TV_DEV = 0,
	RD_DEV_1,
	RD_DEV_2,
	TN_DEV_1,
	PB_DEV_1 = 4,
	AUDIO_SYS,
	TN_DEV_2,
	TN_DEV_3,
	PB_DEV_2 = 8,
	RD_DEV_3,
	TN_DEV_4,
	PB_DEV_3 = 11,
	CEC_BROADCAST = 15,
};

struct cec_opcode {
	u8 opcode;
	u8 name[30];
};

extern u32 hdmi_enable_mask;
extern u32 hdmi_suspend_mask;

int cec_thread_init(void *param);
void cec_thread_exit(void);
s32 hdmi_cec_enable(int enable);
void hdmi_cec_soft_disable(void);
void cec_set_local_standby(bool enable);
bool cec_get_local_standby(void);
void hdmi_cec_wakup_request(void);
s32 hdmi_cec_standby_request(void);
void hdmi_cec_send_inactive_source(void);
s32 hdmi_cec_send_one_touch_play(void);
ssize_t cec_dump_core(char *buf);

extern void mc_cec_clock_enable(hdmi_tx_dev_t *dev, u8 bit);
#endif

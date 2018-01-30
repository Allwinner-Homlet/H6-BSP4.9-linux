/*
 * linux-3.10/drivers/video/sunxi/disp2/hdmi2/hdmi_core/core_cec.c
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
#include "hdmi_core.h"
#include "api/api.h"
#include "api/cec.h"

#if defined(__LINUX_PLAT__)

#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#define WAKEUP_TV_MSG			(1 << 0)
#define STANDBY_TV_MSG			(1 << 1)

DEFINE_MUTEX(thread_lock);
DEFINE_MUTEX(send_lock);

static int cec_active;
static bool cec_local_standby;
static char cec_msg_rx[16];
static int msg_count_rx;
static char cec_msg_tx[16];
static int msg_count_tx;
static struct task_struct *cec_task;
static char src_logic, dst_logic;
static bool active_src;

static char *logic_addr[] = {
	"TV",
	"Recording_1",
	"Recording_2",
	"Tuner_1",
	"Playback_1",
	"Audio",
	"Tuner_2",
	"Tuner_3",
	"Playback_2",
	"Recording_3",
	"Tuner_4",
	"Playback_3",
	"Reserve",
	"Reserve",
	"Specific Use",
	"All",
};

static struct cec_opcode use_cec_opcode[] = {
	{IMAGE_VIEW_ON, "image view on"},
	{ACTIVE_SOURCE, "active source"},
	{REQUEST_ACTIVE_SOURCE, "request active source"},
	{INACTIVE_SOURCE, "inactive source"},
	{ROUTING_CHANGE, "routing change"},
	{ROUTING_INFORMATION, "routing information"},
	{SET_STREAM_PATH, "set stream path"},
	{STANDBY, "standby"},
	{ABORT, "abort"},
	{FEATURE_ABORT, "feature abort"},
	{GIVE_PHYSICAL_ADDRESS, "give physical address"},
	{REPORT_PHYSICAL_ADDRESS, "report physical address"},
	{GIVE_DEVICE_VENDOR_ID, "give device vendor id"},
	{DEVICE_VENDOR_ID, "vendor id"},
	{GIVE_DEVICE_POWER_STATUS, "give device power status"},
	{REPORT_POWER_STATUS, "report power status"},
};



static int get_cec_opcode_name(char opcode, char *buf)
{
	int i, index = -1;
	bool find = false;
	int count = sizeof(use_cec_opcode) / sizeof(struct cec_opcode);

	for (i = 0; i < count; i++) {
		if (use_cec_opcode[i].opcode == opcode) {
			strcpy(buf, use_cec_opcode[i].name);
			find = true;
			index = i;
		}
	}

	if (!find)
		CEC_INF("Not suopport\n");

	return index;
}

void cec_set_local_standby(bool enable)
{
	cec_local_standby = enable;
}

bool cec_get_local_standby(void)
{
	return cec_local_standby;
}

static u16 get_cec_phyaddr(void)
{
	struct hdmi_tx_core *core = get_platform();

	if (core->mode.sink_cap && core->mode.edid_done)
		return core->mode.sink_cap->edid_mHdmivsdb.mPhysicalAddress;
	return 0;
}

static void hdmi_printf_cec_info(char *buf, int count, int type)
{
	int i, ret;
	char opcode_name[30];
	u32 init_addr = (buf[0] >> 4) & 0x0f;
	u32 follow_addr = buf[0] & 0x0f;

	if (!buf) {
		pr_err("Error:print buf is NULL\n");
		return;
	}

	if (count <= 0) {
		pr_err("Error: No valid print count\n");
		return;
	}

	if ((init_addr < 0) || (init_addr > 15)) {
		pr_err("Error: Invalid init_addr%d\n", init_addr);
		return;
	}

	if ((follow_addr < 0) || (follow_addr > 15)) {
		pr_err("Error: Invalid init_addr%d\n", follow_addr);
		return;
	}

	if (type)
		CEC_INF("%s--->%s\n",
			logic_addr[init_addr],
			logic_addr[follow_addr]);
	else
		CEC_INF("%s<---%s\n",
			logic_addr[follow_addr],
			logic_addr[init_addr]);

	if (count > 1)
		ret = get_cec_opcode_name(buf[1], opcode_name);

	if ((count > 1) && (ret >= 0))
		CEC_INF("%s\n", opcode_name);
	else if (count == 1)
		CEC_INF("CEC Ping\n");

	CEC_INF("CEC data:");
	for (i = 0; i < count; i++)
		CEC_INF("0x%02x ", buf[i]);
	CEC_INF("\n\n");
}

static s32 hdmi_cec_send_with_allocated_srclogic(char *buf, unsigned size,
							 unsigned dst)
{
	unsigned src = src_logic;
	int retval;
	struct hdmi_tx_core *core = get_platform();
	if (!cec_active) {
		pr_err("cec NOT enable now!\n");
		retval = -1;
		goto _cec_send_out;
	}
	mutex_lock(&send_lock);

	retval = cec_ctrlSendFrame(&core->hdmi_tx, buf, size, src, dst);

	msg_count_tx = retval + 1;
	memset(cec_msg_tx, 0, sizeof(cec_msg_tx));
	cec_msg_tx[0] = (src << 4) | dst;
	memcpy(&cec_msg_tx[1], buf, size);
	hdmi_printf_cec_info(cec_msg_tx, msg_count_tx, 1);

	mutex_unlock(&send_lock);

_cec_send_out:
	return retval;
}

static s32 hdmi_cec_get_simple_msg(unsigned char *msg, unsigned size)
{
	struct hdmi_tx_core *core = get_platform();
	return cec_ctrlReceiveFrame(&core->hdmi_tx, msg, size);
}

static void hdmi_cec_image_view_on(void)
{
	char image_view_on[2] = {IMAGE_VIEW_ON, 0};
	hdmi_cec_send_with_allocated_srclogic(image_view_on, 1, TV_DEV);
}

static void hdmi_cec_send_active_source(void)
{
	char active[4];
	u16 addr = get_cec_phyaddr();

	active[0] = ACTIVE_SOURCE;
	active[1] = (addr >> 8) & 0xff;
	active[2] = addr & 0xff;

	hdmi_cec_send_with_allocated_srclogic(active, 3, CEC_BROADCAST);
}

static void hdmi_cec_send_request_active_source(void)
{
	char rq[1];

	rq[0] = REQUEST_ACTIVE_SOURCE;

	hdmi_cec_send_with_allocated_srclogic(rq, 1, CEC_BROADCAST);
}

void hdmi_cec_send_inactive_source(void)
{
	char active[4];
	u16 addr = get_cec_phyaddr();

	if (!active_src)
		return;

	active[0] = INACTIVE_SOURCE;
	active[1] = (addr >> 8) & 0xff;
	active[2] = addr & 0xff;

	hdmi_cec_send_with_allocated_srclogic(active, 3, TV_DEV);
}

/*static void hdmi_cec_send_routing_info(void)
{
	char info[4];
	u16 addr = get_cec_phyaddr();

	info[0] = ROUTING_INFORMATION;
	info[1] = (addr >> 8) & 0xff;
	info[2] = addr & 0xff;

	hdmi_cec_send_with_allocated_srclogic(info, 3, CEC_BROADCAST);
}*/

static int hdmi_cec_standby(void)
{
	int ret;
	char standby[2] = {STANDBY, 0};

	ret = hdmi_cec_send_with_allocated_srclogic(standby, 1, CEC_BROADCAST);
	if (ret < 0)
		return -1;
	else
		return 0;
}

static void hdmi_cec_send_feature_abort(char dst)
{
	char fea_abort[3] = {FEATURE_ABORT, 0xff, 4};
	hdmi_cec_send_with_allocated_srclogic(fea_abort, 3, dst);
}

static int hdmi_cec_send_phyaddr(void)
{
	unsigned char phyaddr[4];
	u16 addr = get_cec_phyaddr();
	int ret;

	phyaddr[0] = REPORT_PHYSICAL_ADDRESS;
	phyaddr[1] = (addr >> 8) & 0xff;
	phyaddr[2] = addr & 0xff;
	phyaddr[3] = 4;

	ret = hdmi_cec_send_with_allocated_srclogic(phyaddr, 4, CEC_BROADCAST);
	if (ret < 0)
		pr_err("send physical address failed\n");

	return ret;
}

static int hdmi_cec_send_vendor_id(void)
{
	unsigned char ven_id[4];

	ven_id[0] = DEVICE_VENDOR_ID;
	ven_id[1] = 0;
	ven_id[2] = 0x0c;
	ven_id[3] = 0x03;

	hdmi_cec_send_with_allocated_srclogic(ven_id, 4, CEC_BROADCAST);
	return 0;
}

static int hdmi_cec_send_power_status(char st, char dst)
{
	unsigned char status[2];
	int ret;

	status[0] = REPORT_POWER_STATUS;
	status[1] = st;

	ret = hdmi_cec_send_with_allocated_srclogic(status, 2, dst);
	if (ret < 0)
		pr_err("%s:failed!\n", __func__);
	return ret;
}

s32 hdmi_cec_send_one_touch_play(void)
{
	int temp = cec_active;

	mutex_lock(&thread_lock);

	if (!cec_active)
		cec_active = 1;

	hdmi_cec_image_view_on();
	msleep(500);
	hdmi_cec_send_active_source();
	active_src = true;

	cec_active = temp;

	mutex_unlock(&thread_lock);
	return 0;
}

/*intend to abtain the follower's and initiator's logical address*/
static int hdmi_cec_send_poll(void)
{
	int i, poll_ret;
	unsigned char src = 0, buf;
	struct hdmi_tx_core *core = get_platform();

	for (i = 0; i < 3; i++) {
		if (i == 0) {
			if (src_logic != 0)
				src = src_logic;
			else
				src = PB_DEV_1;
		} else {
			if (src == PB_DEV_1)
				src = (i == 1) ? PB_DEV_2 : PB_DEV_3;
			else if (src == PB_DEV_2)
				src = (i == 1) ? PB_DEV_1 : PB_DEV_3;
			else if (src == PB_DEV_3)
				src = (i == 1) ? PB_DEV_1 : PB_DEV_2;
			else {
				pr_err("Error: error pre src logical address\n");
				src = (i == 1) ? PB_DEV_1 : PB_DEV_2;
			}
		}

		buf = (src << 4) | src;
		hdmi_printf_cec_info(&buf, 1, 1);
		mutex_lock(&send_lock);
		poll_ret = cec_send_poll(&core->hdmi_tx, src);
		mutex_unlock(&send_lock);
		if (!poll_ret) {
			CEC_INF("Get cec src logical address:%s\n", logic_addr[src]);
			cec_CfgLogicAddr(&core->hdmi_tx, src, 1);
			src_logic = src;
			return 0;
		}
		pr_warn("Warn:The polling logical address:0x%x do not get nack\n",
								src);
	}

	if (i == 3) {
		pr_err("err:sink do not respond polling\n");
		src_logic = PB_DEV_1;
		return -1;
	} else {
		return 0;
	}
}

/*static void hdmi_cec_active_src(void)
{
	int time_out = 20;
	int ret;
	char msg[16];

	mutex_lock(&thread_lock);
	hdmi_cec_send_request_active_source();

	while (time_out--) {
		ret = hdmi_cec_get_simple_msg(msg, sizeof(msg));
		if ((ret > 0) && (msg[1] == ACTIVE_SOURCE)) {
			active_src = false;
			pr_info("CEC WARN:There has been an Active Source\n");
			break;
		}
		msleep(20);
	}

	if (time_out <= 0) {
		hdmi_cec_send_active_source();
		active_src = true;
	}
	mutex_unlock(&thread_lock);
}*/

void hdmi_cec_wakup_request(void)
{
	int time_out = 20;
	int ret;
	char msg[16];

	active_src = true;
	mutex_lock(&thread_lock);
	hdmi_cec_send_request_active_source();
	while (time_out--) {
		ret = hdmi_cec_get_simple_msg(msg, sizeof(msg));
		if ((ret > 0) && (msg[1] == ACTIVE_SOURCE)) {
			hdmi_printf_cec_info(msg, ret, 0);
			active_src = false;
			pr_info("CEC WARN:There has been an Active Source\n");
			break;
		}
		msleep(20);
	}

	if (time_out <= 0) {
		hdmi_cec_image_view_on();
		udelay(200);
		hdmi_cec_send_active_source();
		active_src = true;
	}
	mutex_unlock(&thread_lock);
}

s32 hdmi_cec_standby_request(void)
{
	int ret = 0;

	mutex_lock(&thread_lock);
	if (!cec_get_local_standby()) {
		ret = hdmi_cec_standby();
		CEC_INF("cec standby boardcast send\n");
	}
	mutex_unlock(&thread_lock);

	return ret;
}

static void cec_msg_sent(struct device *dev, char *buf)
{
	char *envp[2];

	envp[0] = buf;
	envp[1] = NULL;
	kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);
}

static void rx_msg_process(struct device *pdev, unsigned char *msg, int count)
{
	int send_up = 1;
	char buf[CEC_BUF_SIZE];

	if (((msg[0] & 0x0f) == src_logic) && (count == 1)) {
		hdmi_cec_send_phyaddr();
		udelay(200);
		hdmi_cec_send_vendor_id();
		send_up = 1;
	} else if (count > 1) {
		switch (msg[1]) {
		case ABORT:
			dst_logic = (msg[0] >> 4) & 0x0f;
			if ((msg[0] & 0x0f) == src_logic)
				hdmi_cec_send_feature_abort(dst_logic);
			send_up = 1;
			break;

		case REQUEST_ACTIVE_SOURCE:
			if (((msg[0] & 0x0f) == 0x0f)
					&& active_src) {
				hdmi_cec_send_active_source();
				active_src = true;
			}
			send_up = 1;
			break;

		case GIVE_PHYSICAL_ADDRESS:
			if ((msg[0] & 0x0f) == src_logic)
				hdmi_cec_send_phyaddr();
			send_up = 1;
			break;
		/*case GIVE_DEVICE_VENDOR_ID:
			hdmi_cec_send_vendor_id();
			send_up = 1;
			break;*/
		case SET_STREAM_PATH:
			if ((((msg[2] << 8) | msg[3]) == get_cec_phyaddr())
						&& (count == 4)) {
				if ((msg[0] & 0x0f) == 0x0f) {
					hdmi_cec_send_active_source();
					active_src = true;
				}
			} else if ((msg[0] & 0x0f) == 0x0f) {
				active_src = false;
			}

			send_up = 1;
			break;
		/*case ROUTING_CHANGE:
			if ((((msg[4] << 8) | msg[5]) == get_cec_phyaddr())
				&& (count >= 6))
				hdmi_cec_send_routing_info();

				send_up = 1;
				break;
		case ROUTING_INFORMATION:
			hdmi_cec_send_routing_info();
			send_up = 1;
			break;*/
		case GIVE_DEVICE_POWER_STATUS:
			if ((!hdmi_suspend_mask) && (!hdmi_enable_mask)) {
				dst_logic = (msg[0] >> 4) & 0x0f;
				if ((msg[0] & 0x0f) == src_logic)
					hdmi_cec_send_power_status(CEC_STANDBY_TO_ON, dst_logic);
			} else if ((!hdmi_suspend_mask) && hdmi_enable_mask) {
				dst_logic = (msg[0] >> 4) & 0x0f;
				if ((msg[0] & 0x0f) == src_logic)
					hdmi_cec_send_power_status(CEC_POWER_ON, dst_logic);
			} else if (hdmi_suspend_mask && hdmi_enable_mask) {
				dst_logic = (msg[0] >> 4) & 0x0f;
				if ((msg[0] & 0x0f) == src_logic)
					hdmi_cec_send_power_status(CEC_ON_TO_STANDBY, dst_logic);
			} else {
				pr_err("Error: unknow current power status\n");
			}

			send_up = 1;
			break;
		case STANDBY:
			if (((msg[0] & 0x0f) != 0xf) && ((msg[0] & 0x0f) != src_logic))
				send_up = 0;
			else
				send_up = 1;
			break;

		case INACTIVE_SOURCE:
			if (((msg[0] & 0x0f) == src_logic)
					&& (count == 4)
					&& (((msg[2] << 8) | msg[3])
					!= get_cec_phyaddr())) {
				hdmi_cec_send_active_source();
				active_src = true;
			}
			send_up = 1;
			break;

		case ACTIVE_SOURCE:
			if ((msg[0] & 0x0f) == 0xf
					&& (count == 4)
					&& (((msg[2] << 8) | msg[3])
					!= get_cec_phyaddr()))
				active_src = false;
			send_up = 1;
			break;

		default:
			break;
		}
	}

	memset(buf, 0, CEC_BUF_SIZE);
	snprintf(buf, sizeof(buf), "CEC_MSG=0x%x", msg[1]);
	if (send_up)
		cec_msg_sent(pdev, buf);
}

static int cec_thread(void *param)
{
	int ret = 0;
	unsigned char msg[16];
	struct device *pdev = (struct device *)param;

	while (1) {
		if (kthread_should_stop()) {
			break;
		}

		mutex_lock(&thread_lock);
		if (cec_active) {
			ret = hdmi_cec_get_simple_msg(msg, sizeof(msg));
			if (ret > 0) {
				dst_logic = 0;

				msg_count_rx = ret;
				memset(cec_msg_rx, 0, sizeof(cec_msg_rx));
				memcpy(cec_msg_rx, msg, sizeof(msg));
				hdmi_printf_cec_info(cec_msg_rx, msg_count_rx, 0);
				rx_msg_process(pdev, msg, ret);
			}
		}

		mutex_unlock(&thread_lock);
		msleep(10);
	}

	return 0;
}

int cec_thread_init(void *param)
{
	cec_task = kthread_create(cec_thread, (void *)param, "cec thread");
	if (IS_ERR(cec_task)) {
		printk("Unable to start kernel thread %s.\n\n", "cec thread");
		cec_task = NULL;
		return PTR_ERR(cec_task);
	}
	wake_up_process(cec_task);
	return 0;
}

void cec_thread_exit(void)
{
	if (cec_task) {
		kthread_stop(cec_task);
		cec_task = NULL;
	}
}

/*Just for CEC Super Standby
* In  CEC Super Standby Mode, CEC can not be disable when system suspend,
* So we create a cec soft disable operation which just disable some value
*/
void hdmi_cec_soft_disable(void)
{
	cec_active = 0;
}

s32 hdmi_cec_enable(int enable)
{
	struct hdmi_tx_core *core = get_platform();

	LOG_TRACE1(enable);

	mutex_lock(&thread_lock);
	if (enable) {
		if (cec_active == 1) {
			mutex_unlock(&thread_lock);
			return 0;
		}

		/*
		 * Enable cec module and set logic address to playback device 1.
		 * TODO: ping other devices and allocate a idle logic address.
		 */
		core->hdmi_tx.snps_hdmi_ctrl.cec_on = true;
		mc_cec_clock_enable(&core->hdmi_tx, 0);
		udelay(200);
		cec_Init(&core->hdmi_tx);
		cec_active = 1;
		hdmi_cec_send_poll();
		/*msleep(50);*/
		udelay(200);
		hdmi_cec_send_phyaddr();
	} else {
		if (cec_active == 0) {
			mutex_unlock(&thread_lock);
			return 0;
		}
		cec_active = 0;
		cec_Disable(&core->hdmi_tx, 0);
		mc_cec_clock_enable(&core->hdmi_tx, 1);
		core->hdmi_tx.snps_hdmi_ctrl.cec_on = false;
	}
	mutex_unlock(&thread_lock);
	return 0;
}

ssize_t cec_dump_core(char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "\n");

	if (cec_task)
		n += sprintf(buf + n, "cec thread is running\n");
	else
		n += sprintf(buf + n, "cec thread has been stopped\n");

	if (cec_active) {
		n += sprintf(buf + n, "cec is active\n");
	} else {
		n += sprintf(buf + n, "cec is NOT active\n\n");
		return n;
	}

	if (cec_local_standby)
		n += sprintf(buf + n, "cec is in local standby model\n");
	else
		n += sprintf(buf + n, "cec is NOT in local standby model\n");

	n += sprintf(buf + n, "Tx Current cec physical address:0x%x\n",
							get_cec_phyaddr());
	n += sprintf(buf + n, "Tx Current logical address:%s\n",
							logic_addr[(unsigned char)src_logic]);
	n += sprintf(buf + n, "Rx Current logical address:%s\n",
							logic_addr[(unsigned char)dst_logic]);

	n += sprintf(buf + n, "\n\n");

	return n;
}

#endif

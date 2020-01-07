/** @file
 *  @brief BAS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "cts.h"

extern u16_t buf_start;
extern u16_t buf_end;

u8_t   cts_notify_enabled = 0;
u8_t   cts_time_synced = 0;
static u32_t time;

static void cts_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	cts_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;

}

static ssize_t read_cts(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	time = k_uptime_get_32();

	if(buf_start == buf_end)
		time = 0;

	int err = bt_gatt_attr_read(conn, attr, buf, len, offset, &time,
				 sizeof(u32_t));

	cts_time_synced = 1;

	return err;
}

/* Battery Service Declaration */
BT_GATT_SERVICE_DEFINE(cts_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CTS),
	BT_GATT_CHARACTERISTIC(BT_UUID_CTS_CURRENT_TIME,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_cts, NULL, &time),
	BT_GATT_CCC(cts_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

void cts_init(void)
{
	//bt_gatt_service_register(&cts_svc);
}

int cts_notify(u32_t _timestamp)
{
	if (!cts_notify_enabled) {
		return 0;
	}

	return bt_gatt_notify(NULL, &cts_svc.attrs[1], &_timestamp, sizeof(u32_t));
}

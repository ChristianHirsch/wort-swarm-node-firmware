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

#include "battery.h"

u8_t   bas_notify_enabled;
static u16_t battery_lvl;

static void blvl_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	bas_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_blvl(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	const int value = battery_read_value();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(int));
}

/* Battery Service Declaration */
BT_GATT_SERVICE_DEFINE(bas_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, read_blvl, NULL, &battery_lvl),
	BT_GATT_CCC(blvl_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

void bas_init(void)
{
	//bt_gatt_service_register(&bas_svc);
}

int bas_notify(u16_t _battery_lvl)
{
	battery_lvl = _battery_lvl;

	if (!bas_notify_enabled) {
		return 0;
	}

	return bt_gatt_notify(NULL, &bas_svc.attrs[1], &battery_lvl, sizeof(u16_t));
}

#include <zephyr.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "cts.h"
#include "node.h"

#include <drivers/counter.h>

static  u8_t cts_exact_time_256[10];
static  u8_t cts_update;

s64_t cts_uptime_synced_ms = 0;
u32_t cts_uptime_synced_s  = 0;
u32_t cts_ref_unix_sec     = 0;

static u8_t counting = 0;

static ssize_t read_cts(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		void *buf, u16_t len, u16_t offset);
static ssize_t write_cts(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		const void *buf, u16_t len, u16_t offset,
		u8_t flags);

/* Battery Service Declaration */
BT_GATT_SERVICE_DEFINE(cts_svc,
		BT_GATT_PRIMARY_SERVICE(BT_UUID_CTS),
		BT_GATT_CHARACTERISTIC(BT_UUID_CTS_CURRENT_TIME,
				BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
				read_cts, write_cts, cts_exact_time_256),
				BT_GATT_CCC(NULL, BT_GATT_PERM_NONE),
);

/* no leap seconds! */
static u32_t exact_time_256_to_unix_sec(u8_t *ct)
{
	u32_t year = (u32_t)sys_le16_to_cpu(*(u16_t *)ct);

	u32_t time_sec = (year - 1) / 4;
	time_sec   = (time_sec - 492) * 24 * 60 * 60;		// leap year days
	time_sec  += (year - 1970) * 365 * 24 * 60 * 60;	//without leap year

	for(u8_t i=1; i<ct[2]; i++)
	{
		switch(i) {
		case 2:
			time_sec += 28 * 24 * 60 * 60;
			if(year % 4 == 0)
			{
				time_sec += 24 * 60 * 60;
			}
			break;
		case 4:
		case 6:
		case 9:
		case 11:
			time_sec += 30 * 24 * 60 * 60;
			break;
		default:
			time_sec += 31 * 24 * 60 * 60;
			break;
		}
	}

	time_sec += (ct[3]-1) * 24 * 60 * 60;	// day of month
	time_sec += (ct[4]) * 60 * 60;			// hours
	time_sec += (ct[5]) * 60;				// minutes
	time_sec += (ct[6]);					// seconds

	return time_sec;
}

static void unix_sec_to_exact_time_256(u32_t unix_sec, u8_t *et256)
{
	u32_t tmp = 0;

	/* seconds */
	et256[6] =  unix_sec % 60;

	/* minutes */
	et256[5] = (unix_sec / 60) % 60;

	/* hours */
	et256[4] = (unix_sec / 3600) % 24;

	/* year */
	tmp  = (unix_sec / 3600 / 24) * 100;
	tmp /= 36525;
	tmp += 1970;
	memcpy(&et256[0], &tmp, sizeof(tmp));

	/* day of month */
	tmp  = (u32_t) *(u16_t *)et256 - 1970;
	tmp *= 36525;
	tmp += 50;
	tmp /= 100;
	tmp  = (unix_sec / 3600 / 24) - tmp + 1;

	for(et256[2] = 1; et256[2] < 13 ; et256[2]++)
	{
		u8_t days;
		switch(et256[2])
		{
		case 2:
			days = 28;
			break;
		case 4:
		case 6:
		case 9:
		case 11:
			days = 30;
			break;
		default:
			days = 31;
			break;
		}
		if (tmp < days)
			break;
		tmp -= days;
	}

	et256[3] = (u8_t) tmp;
	if (et256[2] < 3 && (*(u16_t *)et256 - 1970) % 4 == 2) {
		et256[3]++;
	}
}

static ssize_t read_cts(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		void *buf, u16_t len, u16_t offset)
{
	u8_t et256[10] = { 0 };

	s64_t now  = (k_uptime_get() - cts_uptime_synced_ms) / 1000;

	u32_t now32 = (u32_t) now;
	now32 += cts_ref_unix_sec;

	unix_sec_to_exact_time_256(now32, et256);

	int err = bt_gatt_attr_read(conn, attr, buf, len, offset, et256,
			sizeof(et256));

	return err;
}

static ssize_t write_cts(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		const void *buf, u16_t len, u16_t offset,
		u8_t flags)
{
	s64_t now = k_uptime_get();

	u8_t *value = attr->user_data;

	if (offset + len > sizeof(cts_exact_time_256)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	cts_update = 1U;
	cts_uptime_synced_ms = now;
	now /= 1000;
	cts_uptime_synced_s = (u32_t) now;
	cts_ref_unix_sec = exact_time_256_to_unix_sec(cts_exact_time_256);

	if (counting == 1) {
		// clock stretching?!

	}

	now  = cts_ref_unix_sec % CONFIG_SENSE_INTERVAL;
	now  = 1000 * now + (cts_uptime_synced_ms % 1000);

	node_start_sensing((CONFIG_SENSE_INTERVAL*1000) - now);
	counting = 1;

	return len;
}

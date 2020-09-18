#include "ass.h"

#include "uuid.h"
#include "node.h"

#include <logging/log.h>

#include <bluetooth/gatt.h>
#include <bluetooth/conn.h>

static u8_t *data_buf = NULL;

static u8_t indicate_enabled = 0;
static u16_t mtu_size = 0;

static size_t buffer_size = 0;
static size_t mtu_buffer_size = 0;

struct bt_gatt_indicate_params i_params;

extern struct bt_conn *client_connected;

LOG_MODULE_REGISTER(ASS);

static void sync_ccc_cfg_changed(const struct bt_gatt_attr *attr,
			u16_t value);
static void ass_indicate(void);

/* ASS Declaration */
BT_GATT_SERVICE_DEFINE(ass_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ASS),

	BT_GATT_CHARACTERISTIC(BT_UUID_ASS_GSC,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_READ, NULL, NULL, NULL),

	BT_GATT_CCC(sync_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_DESCRIPTOR(BT_UUID_ASS_TID, BT_GATT_PERM_NONE, NULL, NULL, 0),
	BT_GATT_DESCRIPTOR(BT_UUID_ASS_BVD, BT_GATT_PERM_NONE, NULL, NULL, 0),
	BT_GATT_DESCRIPTOR(BT_UUID_ASS_ATD, BT_GATT_PERM_NONE, NULL, NULL, 0),
	BT_GATT_DESCRIPTOR(BT_UUID_ASS_ACX, BT_GATT_PERM_NONE, NULL, NULL, 0),
	BT_GATT_DESCRIPTOR(BT_UUID_ASS_ACY, BT_GATT_PERM_NONE, NULL, NULL, 0),
	BT_GATT_DESCRIPTOR(BT_UUID_ASS_ACZ, BT_GATT_PERM_NONE, NULL, NULL, 0),
);

static void indicate_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			u8_t err)
{
	LOG_INF("Indication %s\n", err != 0U ? "fail" : "success");
	node_buf_get_finish(mtu_buffer_size);
	ass_indicate();
}

static void sync_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 u16_t value)
{
	indicate_enabled = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;

	LOG_INF("sync_ccc_cfg_changed to %u", value);

	if (indicate_enabled)
	{
		mtu_size = bt_gatt_get_mtu(client_connected);
		LOG_INF("mtu size = %u", mtu_size);

		data_buf = (u8_t *)k_malloc(mtu_size);

		if(data_buf == 0)
		{
			LOG_ERR("error allocating %u bytes of memory", mtu_size);
			return;
		}

		i_params.attr = &ass_svc.attrs[2];
		i_params.data = data_buf;
		i_params.func = indicate_cb;

		buffer_size = node_buf_get_elem_size();
		mtu_buffer_size = (mtu_size - 1) / buffer_size;

		ass_indicate();
	}
	else {
		k_free(data_buf);
		data_buf = 0;
	}
}

static void ass_indicate(void)
{
	if(!indicate_enabled)
		return;

	LOG_INF("send indication");

	data_buf[0] = (u8_t) node_buf_cpy_data((void *)&data_buf[1],
			mtu_buffer_size);

	i_params.len  = sizeof(data_buf[0]) + data_buf[0] * buffer_size;
	if (data_buf[0] == 0) {
		i_params.func = NULL;
	}

	int err = bt_gatt_indicate(NULL, &i_params);
	if (err != 0) {
		LOG_ERR("error sending indication: %i", err);
	}
}

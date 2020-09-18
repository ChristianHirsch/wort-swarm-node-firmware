/* main.c - Application main for soil-swarm-node */

#include <zephyr.h>
#include <zephyr/types.h>
#include <logging/log.h>
#include <settings/settings.h>
#include <soc.h>

#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

//		BT_LE_ADV_OPT_USE_NAME |     /* adv name */            \

#define BT_LE_ADV_CONN_NAME_ID_CODED BT_LE_ADV_PARAM( \
		BT_LE_ADV_OPT_CONNECTABLE |  /* connectable */         \
		BT_LE_ADV_OPT_USE_IDENTITY | /* use identity */        \
		BT_LE_ADV_OPT_EXT_ADV | /* use extended advertising */ \
		BT_LE_ADV_OPT_CODED,    /* advertise coded phy */      \
		BT_GAP_ADV_SLOW_INT_MIN, \
		BT_GAP_ADV_SLOW_INT_MAX, NULL)

#define BT_LE_ADV_CONN_NAME_ID BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | \
		BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_USE_IDENTITY, \
		BT_GAP_ADV_SLOW_INT_MIN, \
		BT_GAP_ADV_SLOW_INT_MAX, NULL)

// BT_GAP_ADV_SLOW_INT_MIN = 0x0640 ^= 1s
// BT_GAP_ADV_SLOW_INT_MAX = 0x0780 ^= 1.2s
#define BT_LE_ADV_NCONN_NAME_ID BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_NAME | \
		BT_LE_ADV_OPT_USE_IDENTITY, \
		BT_GAP_ADV_SLOW_INT_MIN, \
		BT_GAP_ADV_SLOW_INT_MAX, NULL)

LOG_MODULE_REGISTER(main);

K_THREAD_STACK_DEFINE(disconnect_timeout_stack, 32);
struct k_thread disconnect_timeout_data;
k_tid_t disconnect_timeout_tid;

void disconnect_timeout(void *, void *, void *);

extern volatile struct time_sync remote_ts;

static bool is_client_connected  = false;
struct bt_conn *client_connected = 0;
u32_t time_connected = 0;

u16_t num_connected = 0;

struct bt_le_adv_param *adv_params = BT_LE_ADV_CONN_NAME_ID;

static const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
		BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
		BT_DATA_BYTES(BT_DATA_SVC_DATA16,
				0xaa, 0xfe, /* Eddystone UUID */
				0x10, /* Eddystone-URL frame type */
				0x04, /* Calibrated Tx power at 0m */
				0x00, /* URL Scheme Prefix http://www. */
				//'a','f','a','r','c','l','o','u','d','.','e','u','/')
				'a','f','a','r','c','l','o','u','d','.','a','t','/')
};

void start_advertising() {
	int err = 0, err_cnt = 0;
	do {
		err = bt_le_adv_start(adv_params, ad, ARRAY_SIZE(ad), NULL, 0);
		if(err == 0 || err_cnt > 3)
			break;
		err_cnt++;
		k_sleep(K_MSEC(15));
	} while(err);

	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}
	LOG_INF("Advertising successfully started");
}

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		LOG_INF("Connection failed (err %u)\n", err);
		return;
	}

	client_connected = conn;
	is_client_connected = true;

	disconnect_timeout_tid = k_thread_create(&disconnect_timeout_data,
			disconnect_timeout_stack,
			K_THREAD_STACK_SIZEOF(disconnect_timeout_stack),
			disconnect_timeout,
			NULL, NULL, NULL, 1, 0, K_MINUTES(5));
	num_connected++;

	LOG_INF("Connected (%u times)", num_connected);
}

static void bt_ready(int err)
{
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	start_advertising();
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	LOG_INF("Disconnected (reason %u)\n", reason);
	is_client_connected = false;

	if (disconnect_timeout_tid)
	{
		k_wakeup(disconnect_timeout_tid);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void disconnect_timeout(void *v1, void *v2, void *v3)
{
	/* if connection is still up after 15min, (auto-)disconnect */
	if (is_client_connected == true)
	{
		bt_conn_disconnect(client_connected, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}

	disconnect_timeout_tid = 0;
}

void main(void)
{
	int err = 0;

	err = bt_enable(bt_ready);
	if (err != 0) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
	bt_conn_cb_register(&conn_callbacks);

	while (1) {
		k_sleep(K_FOREVER);
	}
}

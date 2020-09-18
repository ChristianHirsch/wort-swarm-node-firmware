#include "node.h"

#include <zephyr.h>
#include <init.h>
#include <logging/log.h>
#include <device.h>

#include <kernel.h>

#include <drivers/sensor.h>

#include "battery.h"

#define SENSE_INTERVAL CONFIG_SENSE_INTERVAL
#define MEASUREMENTS_SIZE CONFIG_MEASUREMENTS_SIZE

struct measurement measurements[MEASUREMENTS_SIZE];

s16_t buf_start = 0;
s16_t buf_end   = 0;
u32_t sense_interval_mult = 1;

extern u32_t cts_uptime_synced_s;
extern u32_t cts_ref_unix_sec;

K_SEM_DEFINE(measurements_sem, 1, 1);
K_SEM_DEFINE(loop_guard, 1, 1);

static void node_loop(struct k_timer *timer);

K_THREAD_STACK_DEFINE(node_loop_worker_stack_area, 1024);
struct k_thread node_loop_worker_data;
k_tid_t node_loop_worker_tid;

static void node_loop_worker(void *, void *, void *);

LOG_MODULE_REGISTER(NODE);

K_TIMER_DEFINE(loop_timer, node_loop, NULL);

static int node_initialize(struct device *arg)
{
#ifdef CONFIG_DEBUG
	k_timer_start(&loop_timer, K_SECONDS(SENSE_INTERVAL), K_SECONDS(SENSE_INTERVAL));
#endif

	return 0;
}

void node_start_sensing(s32_t delay)
{
	k_timer_start(&loop_timer, K_MSEC(delay), K_SECONDS(SENSE_INTERVAL));
}

static void node_loop(struct k_timer *timer)
{
	if (k_sem_take(&loop_guard, K_NO_WAIT) != 0)
		return;

	node_loop_worker_tid = k_thread_create(&node_loop_worker_data, node_loop_worker_stack_area,
			K_THREAD_STACK_SIZEOF(node_loop_worker_stack_area),
			node_loop_worker,
			NULL, NULL, NULL,
			5, 0, K_NO_WAIT);
}

static void node_loop_worker(void *p1, void *p2, void *p3)
{
	static u8_t buf_full = 0;

	LOG_INF("enter loop %x %x", buf_start, buf_end);

	struct device *icm20948;
	struct sensor_value acc_sample[3], gyro_sample[3], mag_sample[3];

	struct device *si705x;
	struct sensor_value temp_sample;

	/* measure battery */
	s16_t batt_mV = battery_sample();
	if (batt_mV < 3700) {
		battery_charge_enable();
	} else if (batt_mV > 4000) {
		battery_charge_disable();
	}

	/* measure temperature */
	si705x = device_get_binding(DT_LABEL(DT_INST(0, silabs_si705x)));
	sensor_sample_fetch_chan(si705x, SENSOR_CHAN_AMBIENT_TEMP);
	sensor_channel_get(si705x, SENSOR_CHAN_AMBIENT_TEMP, &temp_sample);

	/* measure acceleration */
	icm20948 = device_get_binding(DT_LABEL(DT_INST(0, tdk_icm20948)));
	sensor_sample_fetch_chan(icm20948, SENSOR_CHAN_ACCEL_XYZ);
	//sensor_channel_get(icm20948, SENSOR_CHAN_ACCEL_XYZ, acc_sample);
	//sensor_channel_get(icm20948, SENSOR_CHAN_GYRO_XYZ, gyro_sample);

	s64_t ts = k_uptime_get();
	ts /= 1000;

	/* save to buffer */

	if (buf_full) {
		k_sem_take(&measurements_sem, K_FOREVER);
		for (s16_t i = 0; i < MEASUREMENTS_SIZE / 2; i++) {
			s16_t buf_dst = (buf_start + i) % MEASUREMENTS_SIZE;
			s16_t buf_src = (buf_start + i * 2) % MEASUREMENTS_SIZE;
			measurements[buf_dst].timestamp   = measurements[buf_src].timestamp;
			measurements[buf_dst].battery     = measurements[buf_src].battery;
			measurements[buf_dst].temperature = measurements[buf_src].temperature;

			measurements[buf_dst].acc[0] = measurements[buf_src].acc[0];
			measurements[buf_dst].acc[1] = measurements[buf_src].acc[1];
			measurements[buf_dst].acc[2] = measurements[buf_src].acc[2];
		}
		buf_end = (buf_start + MEASUREMENTS_SIZE / 2 + 1) % MEASUREMENTS_SIZE;
		k_sem_give(&measurements_sem);
		sense_interval_mult *= 2;
		buf_full = 0;
	}

	k_sem_take(&measurements_sem, K_FOREVER);
	measurements[buf_end].timestamp		= (u32_t)ts;
	measurements[buf_end].battery		= (u16_t) batt_mV / 10;
	measurements[buf_end].temperature	= (u16_t)(temp_sample.val1 * 100) + (u16_t)(temp_sample.val2 / 10000);
	measurements[buf_end].acc[0]		= (s16_t) 0; //(acc_sample[0].val1 * 100) + (s16_t)(acc_sample[0].val2 / 10000);
	measurements[buf_end].acc[1]		= (s16_t) 1; //(acc_sample[1].val1 * 100) + (s16_t)(acc_sample[1].val2 / 10000);
	measurements[buf_end].acc[2]		= (s16_t) 2; //(acc_sample[2].val1 * 100) + (s16_t)(acc_sample[2].val2 / 10000);

	LOG_INF("Timestamp:   %u", measurements[buf_end].timestamp);
	LOG_INF("Battery:     %3u.%02u V", measurements[buf_end].battery / 100,
			measurements[buf_end].battery % 100);
	LOG_INF("Temperature: %3u.%02u V", measurements[buf_end].temperature / 100,
				measurements[buf_end].temperature % 100);
	LOG_INF("Acceleration X: %3i.%02u Gauss", measurements[buf_end].acc[0] / 100,
			measurements[buf_end].acc[0] % 100);

	buf_end = (buf_end + 1) % MEASUREMENTS_SIZE;
	k_sem_give(&measurements_sem);

	buf_full = (buf_end == buf_start);

	k_sem_give(&loop_guard);
}

size_t node_buf_get_elem_size(void)
{
	struct measurement *meas = measurements;
	return sizeof(meas->timestamp)
			+ sizeof(meas->battery)
			+ sizeof(meas->temperature)
			+ sizeof(meas->acc);
}

size_t node_buf_cpy_data(void *dst, size_t cnt)
{
	size_t i;
	u8_t *bdst = (u8_t *)dst;

	struct measurement *meas;

	k_sem_take(&measurements_sem, K_FOREVER);
	for (i = 0; i < cnt; i++)
	{
		if (((buf_start + i) % MEASUREMENTS_SIZE) == buf_end)
			break;

		meas = &measurements[buf_start + i];

		u32_t timestamp = meas->timestamp
				- cts_uptime_synced_s
				+ cts_ref_unix_sec;

		memcpy(bdst, &timestamp, sizeof(timestamp));
		bdst = bdst + sizeof(meas->timestamp);

		memcpy(bdst, &meas->battery, sizeof(meas->battery));
		bdst = bdst + sizeof(meas->battery);

		memcpy(bdst, &meas->temperature, sizeof(meas->temperature));
		bdst = bdst + sizeof(meas->temperature);

		memcpy(bdst, meas->acc, sizeof(meas->acc));
		bdst = bdst + sizeof(meas->acc);
	}
	k_sem_give(&measurements_sem);

	return i;
}

u8_t node_buf_get_finish(size_t cnt)
{
	for(size_t i = 0; i < cnt; i++)
	{
		buf_start = (buf_start + 1) % MEASUREMENTS_SIZE;
		if (buf_start == buf_end)
			return 0;
	}

	LOG_INF("buf_start = %u", buf_start);

	return 1;
}

SYS_INIT(node_initialize, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

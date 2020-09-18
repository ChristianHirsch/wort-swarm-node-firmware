/*
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_si705x

#include <init.h>
#include <drivers/sensor.h>
#include <sys/byteorder.h>
#include <kernel.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include <device.h>

#include "si705x.h"

#include <drivers/i2c.h>

LOG_MODULE_REGISTER(SI705X, CONFIG_SENSOR_LOG_LEVEL);

static int si705x_channel_get(struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct si705x_data *data = (struct si705x_data *)dev->driver_data;

	int32_t buf = (int32_t)data->buf[0] << 8 | (int32_t)data->buf[1];

	buf *= 17572;
	buf /= 65536;
	buf -= 4685;

	val->val1 = buf / 100;
	val->val2 = (buf % 100) * 10000;

	return 0;
}

static int si705x_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	int err;
	struct si705x_data *data = (struct si705x_data *)dev->driver_data;
	uint8_t cmd;

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_AMBIENT_TEMP:
		cmd = SI705X_CMD_MEASURE_NO_HOLD_MASTER_MODE;

		err = i2c_write(data->i2c, &cmd, sizeof(cmd), DT_INST_REG_ADDR(0));
		if (err) {
			LOG_ERR("Error writing measurement command");
		}

		k_sleep(K_MSEC(500));

		err = i2c_read(data->i2c, data->buf, sizeof(data->buf), DT_INST_REG_ADDR(0));
		if (err) {
			LOG_ERR("Error reading measurement");
		}

	default:
		return -ENOTSUP;
	}
	return 0;
}

int si705x_init(struct device *dev)
{
	struct si705x_data *data = (struct si705x_data *)dev->driver_data;

	data->i2c = device_get_binding(DT_INST_BUS_LABEL(0));
	if (!data->i2c) {
		LOG_DBG("Bus not found: %s", DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

	return 0;
}

struct si705x_data si705x_data;

static const struct sensor_driver_api si705x_driver_api = {
	.sample_fetch = si705x_sample_fetch,
	.channel_get = si705x_channel_get
};

DEVICE_AND_API_INIT(si705x, DT_INST_LABEL(0), si705x_init,
		    &si705x_data, NULL, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &si705x_driver_api);

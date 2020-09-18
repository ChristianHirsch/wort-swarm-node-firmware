/*
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT tdk_icm20948

#include <string.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include "icm20948.h"

//#ifdef DT_TDK_ICM20948_BUS_I2C

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_DECLARE(ICM20948);

static inline int icm20948_change_bank(struct icm20948_data *data,
				       u16_t reg_bank_addr)
{
	u8_t bank = (u8_t)(reg_bank_addr >> 7);

	if (bank != data->bank) {
		k_sleep(K_MSEC(500));
		data->bank = bank;
		return i2c_reg_write_byte(data->bus,
					  DT_INST_REG_ADDR(0),
					  ICM20948_REG_BANK_SEL, (bank << 4));
	}
	return 0;
}

static int icm20948_i2c_read_data(struct icm20948_data *data,
				  u16_t reg_bank_addr, u8_t *value, u8_t len)
{
	u8_t reg_addr = (u8_t)(reg_bank_addr & 0xff);

	if (icm20948_change_bank(data, reg_bank_addr)) {
		return -EIO;
	}
	return i2c_burst_read(data->bus, DT_INST_REG_ADDR(0),
			      reg_addr, value, len);
}

static int icm20948_i2c_write_data(struct icm20948_data *data,
				   u16_t reg_bank_addr, u8_t *value, u8_t len)
{
	u8_t reg_addr = (u8_t)(reg_bank_addr & 0xff);

	if (icm20948_change_bank(data, reg_bank_addr)) {
		return -EIO;
	}
	return i2c_burst_write(data->bus, DT_INST_REG_ADDR(0),
			       reg_addr, value, len);
}

static int icm20948_i2c_read_reg(struct icm20948_data *data,
				 u16_t reg_bank_addr, u8_t *value)
{
	int err;
	u8_t reg_addr = (u8_t)(reg_bank_addr & 0xff);

	err = icm20948_change_bank(data, reg_bank_addr);
	if (err) {
		return -EIO;
	}

	err = i2c_reg_read_byte(data->bus, DT_INST_REG_ADDR(0),
				 reg_addr, value);
	if (err) {
		LOG_ERR("Error reading byte via I2C");
		return -EIO;
	}

	return 0;
}

static int icm20948_i2c_write_reg(struct icm20948_data *data,
				  u16_t reg_bank_addr, u8_t value)
{
	u8_t reg_addr = (u8_t)(reg_bank_addr & 0xff);

	if (icm20948_change_bank(data, reg_bank_addr)) {
		return -EIO;
	}
	return i2c_reg_write_byte(data->bus, DT_INST_REG_ADDR(0),
				  reg_addr, value);
}

static int icm20948_i2c_update_reg(struct icm20948_data *data,
				   u16_t reg_bank_addr, u8_t mask, u8_t value)
{
	u8_t reg_addr = (u8_t)(reg_bank_addr & 0xff);

	if (icm20948_change_bank(data, reg_bank_addr)) {
		return -EIO;
	}
	return i2c_reg_update_byte(data->bus, DT_INST_REG_ADDR(0),
				   reg_addr, mask,
				   value << __builtin_ctz(mask));
}

static const struct icm20948_tf icm20948_i2c_transfer_fn = {
	.read_data  = icm20948_i2c_read_data,
	.write_data = icm20948_i2c_write_data,
	.read_reg   = icm20948_i2c_read_reg,
	.write_reg  = icm20948_i2c_write_reg,
	.update_reg = icm20948_i2c_update_reg
};

int icm20948_i2c_init(struct device *dev)
{
	struct icm20948_data *data = dev->driver_data;

	data->hw_tf = &icm20948_i2c_transfer_fn;
	return 0;
}

//#endif

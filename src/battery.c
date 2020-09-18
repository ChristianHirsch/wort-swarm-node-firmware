#include "battery.h"

#include <stdio.h>
#include <stdlib.h>

#include <zephyr.h>
#include <init.h>
#include <drivers/gpio.h>
#include <drivers/adc.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#define VBATT DT_PATH(vbatt)

#define BATT_CHRG_EN_PORT	DT_GPIO_LABEL(VBATT, power_gpios)
#define BATT_CHRG_EN_PIN	DT_GPIO_PIN(VBATT, power_gpios)

LOG_MODULE_REGISTER(BATTERY, CONFIG_ADC_LOG_LEVEL);

#define VBATT DT_PATH(vbatt)

static struct device *chrg_en_port;
static u8_t chrg_enabled;

/* This board uses a divider that reduces max voltage to
 * reference voltage (600 mV).
 */
#define BATTERY_ADC_GAIN ADC_GAIN_1

struct io_channel_config {
	const char *label;
	u8_t channel;
};

struct gpio_channel_config {
	const char *label;
	u8_t pin;
	u8_t flags;
};

struct divider_config {
	const struct io_channel_config io_channel;
	const struct gpio_channel_config power_gpios;
	const u32_t output_ohm;
	const u32_t full_ohm;
};

/* https://blog.ampow.com/lipo-voltage-chart/
 */
static const struct battery_level_point lipo_37[] = {
	{ 10000, 4200 },
	{  9500, 4150 },
	{  9000, 4110 },
	{  8500, 4080 },
	{  8000, 4020 },
	{  7500, 3980 },
	{  7000, 3950 },
	{  6500, 3910 },
	{  6000, 3870 },
	{  5500, 3850 },
	{  5000, 3840 },
	{  4500, 3820 },
	{  4000, 3800 },
	{  3500, 3790 },
	{  3000, 3770 },
	{  2500, 3750 },
	{  2000, 3730 },
	{  1500, 3710 },
	{  1000, 3690 },
	{   500, 3610 },
	{     0, 3270 },
};

static const struct divider_config divider_config = {
#if DT_NODE_HAS_STATUS(VBATT, okay)
	.io_channel = {
		DT_IO_CHANNELS_LABEL(VBATT),
		DT_IO_CHANNELS_INPUT(VBATT),
	},
#if DT_NODE_HAS_PROP(VBATT, power_gpios)
	.power_gpios = {
		DT_GPIO_LABEL(VBATT, power_gpios),
		DT_GPIO_PIN(VBATT, power_gpios),
		DT_GPIO_FLAGS(VBATT, power_gpios),
	},
#endif
	.output_ohm = DT_PROP(VBATT, output_ohms),
	.full_ohm = DT_PROP(VBATT, full_ohms),
#else /* /vbatt exists */
	.io_channel = {
		DT_LABEL(DT_ALIAS(adc_0)),
	},
#endif /* /vbatt exists */
	/*  * /
	.io_channel = DT_VOLTAGE_DIVIDER_VBATT_IO_CHANNELS,
#ifdef DT_VOLTAGE_DIVIDER_VBATT_POWER_GPIOS
	.power_gpios = DT_VOLTAGE_DIVIDER_VBATT_POWER_GPIOS,
#endif
	.output_ohm = DT_VOLTAGE_DIVIDER_VBATT_OUTPUT_OHMS,
	.full_ohm = DT_VOLTAGE_DIVIDER_VBATT_FULL_OHMS,
	*/
};

struct divider_data {
	struct device *adc;
	struct device *gpio;
	struct adc_channel_cfg adc_cfg;
	struct adc_sequence adc_seq;
	s16_t raw;
};
static struct divider_data divider_data;

static int divider_setup(void)
{
	const struct divider_config *cfg = &divider_config;
	const struct io_channel_config *iocp = &cfg->io_channel;
	const struct gpio_channel_config *gcp = &cfg->power_gpios;
	struct divider_data *ddp = &divider_data;
	struct adc_sequence *asp = &ddp->adc_seq;
	struct adc_channel_cfg *accp = &ddp->adc_cfg;
	int rc;

	ddp->adc = device_get_binding(iocp->label);
	if (ddp->adc == NULL) {
		LOG_ERR("Failed to get ADC %s", iocp->label);
		return -ENOENT;
	}

	if (gcp->label) {
		ddp->gpio = device_get_binding(gcp->label);
		if (ddp->gpio == NULL) {
			LOG_ERR("Failed to get GPIO %s", gcp->label);
			return -ENOENT;
		}
		rc = gpio_pin_configure(ddp->gpio, gcp->pin,
					GPIO_OUTPUT_INACTIVE | gcp->flags);
		if (rc != 0) {
			LOG_ERR("Failed to control feed %s.%u: %d",
				gcp->label, gcp->pin, rc);
			return rc;
		}
	}

	*asp = (struct adc_sequence){
		.channels = BIT(0),
		.buffer = &ddp->raw,
		.buffer_size = sizeof(ddp->raw),
		.oversampling = 4,
		.calibrate = true,
	};

#ifdef CONFIG_ADC_NRFX_SAADC
	*accp = (struct adc_channel_cfg){
		.gain = BATTERY_ADC_GAIN,
		.reference = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
		.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0 + iocp->channel,
	};

	asp->resolution = 14;
#else /* CONFIG_ADC_var */
#error Unsupported ADC
#endif /* CONFIG_ADC_var */

	rc = adc_channel_setup(ddp->adc, accp);
	LOG_INF("Setup AIN%u got %d", iocp->channel, rc);

	return rc;
}

static bool battery_ok;

static int battery_setup(struct device *arg)
{
	int rc = divider_setup();

	battery_ok = (rc == 0);
	LOG_INF("Battery setup: %d %d", rc, battery_ok);

	/* LDO out */
	chrg_en_port = device_get_binding(BATT_CHRG_EN_PORT);
	if (!chrg_en_port) {
		LOG_ERR("Failed to initialize battery charge enable port");
		return -ENODEV;
	}

	battery_charge_enable();

	return rc;
}

SYS_INIT(battery_setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

int battery_measure_enable(bool enable)
{
	int rc = -ENOENT;

	if (battery_ok) {
		const struct divider_data *ddp = &divider_data;
		const struct gpio_channel_config *gcp = &divider_config.power_gpios;

		rc = 0;
		if (ddp->gpio) {
			rc = gpio_pin_set(ddp->gpio, gcp->pin, enable);
		}
	}
	return rc;
}

s16_t battery_sample(void)
{
	battery_measure_enable(true);

	int rc = -ENOENT;

	if (battery_ok) {
		struct divider_data *ddp = &divider_data;
		const struct divider_config *dcp = &divider_config;
		struct adc_sequence *sp = &ddp->adc_seq;

		rc = adc_read(ddp->adc, sp);
		sp->calibrate = false;
		if (rc == 0) {
			s32_t val = ddp->raw;

			adc_raw_to_millivolts(adc_ref_internal(ddp->adc),
					      ddp->adc_cfg.gain,
					      sp->resolution,
					      &val);
			rc = val * (u64_t)dcp->full_ohm / dcp->output_ohm;
			LOG_DBG("raw %u ~ %u mV => %d mV\n",
				ddp->raw, val, rc);
		}
	}

	battery_measure_enable(false);

	return rc;
}

u16_t battery_level_pptt(unsigned int batt_mV)
{
	const struct battery_level_point *pb = lipo_37;

	if (batt_mV >= pb->lvl_mV) {
		/* Measured voltage above highest point, cap at maximum. */
		return pb->lvl_pptt;
	}
	/* Go down to the last point at or below the measured voltage. */
	while ((pb->lvl_pptt > 0)
	       && (batt_mV < pb->lvl_mV)) {
		++pb;
	}
	if (batt_mV < pb->lvl_mV) {
		/* Below lowest point, cap at minimum */
		return pb->lvl_pptt;
	}

	/* Linear interpolation between below and above points. */
	const struct battery_level_point *pa = pb - 1;

	return pb->lvl_pptt
	       + ((pa->lvl_pptt - pb->lvl_pptt)
		  * (batt_mV - pb->lvl_mV)
		  / (pa->lvl_mV - pb->lvl_mV));
}

void battery_charge_enable(void)
{
	gpio_pin_configure(chrg_en_port, BATT_CHRG_EN_PIN, GPIO_INPUT);
	chrg_enabled = 1;

}

void battery_charge_disable(void)
{
	gpio_pin_configure(chrg_en_port, BATT_CHRG_EN_PIN, GPIO_OUTPUT);
	gpio_pin_set(chrg_en_port, BATT_CHRG_EN_PIN, 0);
	chrg_enabled = 0;
}

u8_t battery_charge_is_enabled(void)
{
	return chrg_enabled;
}

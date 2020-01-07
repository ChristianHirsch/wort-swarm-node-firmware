#include "battery.h"

#include "settings.h"
#include <device.h>
#include <gpio.h>

#include <adc.h>
//#include <hal/nrf_adc.h>

struct device *battery_en;

#define BUFFER_SIZE 1
static int16_t m_sample_buffer[BUFFER_SIZE];

struct device *adc_dev;

static const struct adc_channel_cfg adc_cfg = {
    .gain             = ADC_GAIN,
    .reference        = ADC_REFERENCE,
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
//    .channel_id       = ADC_1ST_CHANNEL_ID,
//    .channel_id       = BATTERY_SENSE_PIN,
    .input_positive   = BATTERY_SENSE_PIN,
};

struct adc_sequence adc_sequence = {
//    .channels    = BIT(ADC_1ST_CHANNEL_ID),
    .buffer      = m_sample_buffer,
    .buffer_size = sizeof(m_sample_buffer),
    .resolution  = ADC_RESOLUTION,
};

int battery_init(struct device *a_dev) {
    battery_en = device_get_binding(BATTERY_EN_PORT);

    gpio_pin_configure(battery_en, BATTERY_EN_PIN, GPIO_DIR_OUT);
    gpio_pin_write(battery_en, BATTERY_EN_PIN, 0);
    
    if(a_dev == 0)
        adc_dev = device_get_binding(ADC_DEVICE_NAME);
    else
        adc_dev = a_dev;

    if(adc_dev == 0)
        return -1;

    return adc_channel_setup(adc_dev, &adc_cfg);
}

int battery_read_value() {
    int value = 0;
	device_set_power_state(battery_en, DEVICE_PM_ACTIVE_STATE, NULL, NULL);
	device_set_power_state(adc_dev,    DEVICE_PM_ACTIVE_STATE, NULL, NULL);

    if(adc_channel_setup(adc_dev, &adc_cfg) != 0)
        return -1;
    adc_sequence.channels = BIT(adc_cfg.channel_id);

    gpio_pin_write(battery_en, BATTERY_EN_PIN, 1);
    value = adc_read(adc_dev, &adc_sequence);
    gpio_pin_write(battery_en, BATTERY_EN_PIN, 0);

	device_set_power_state(battery_en, DEVICE_PM_SUSPEND_STATE, NULL, NULL);
	device_set_power_state(adc_dev,    DEVICE_PM_SUSPEND_STATE, NULL, NULL);

    if(value < 0)
        return -1;

    return m_sample_buffer[0];
}

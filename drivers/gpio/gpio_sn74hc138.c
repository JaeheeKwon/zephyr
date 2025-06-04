/*
 * Copyright (c) 2022 Matthias Freese
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_sn74hc138

/**
 * @file Driver for 74 HC shift register
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ti_sn74hc138, CONFIG_GPIO_LOG_LEVEL);

struct ti_sn74hc138_gpio_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config config;

	const struct gpio_dt_spec input_a_gpio;
	const struct gpio_dt_spec input_b_gpio;
	const struct gpio_dt_spec input_c_gpio;
};

struct ti_sn74hc138_gpio_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data data;
};

static int ti_sn74hc138_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(flags);
	return 0;
}

static int ti_sn74hc138_port_get_raw(const struct device *dev, uint32_t *value)
{
	return -ENOTSUP;
}

static int ti_sn74hc138_port_set_masked_raw(const struct device *dev, uint32_t mask,
					      uint32_t value)
{
	uint8_t idx = 0;
	const struct ti_sn74hc138_gpio_config *config = dev->config;

	/* exactly one output can be selected at a time */
	if (POPCOUNT(value) != 1) {
		return -EINVAL;
	}

	idx = u32_count_trailing_zeros(value);

	gpio_pin_set_dt(&config->input_a_gpio, idx & 1);
	gpio_pin_set_dt(&config->input_b_gpio, (idx >> 1) & 1);
	gpio_pin_set_dt(&config->input_c_gpio, (idx >> 2) & 1);

	return 0;
}

static int ti_sn74hc138_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	return -ENOTSUP;
}

static int ti_sn74hc138_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	return -ENOTSUP;
}

static int ti_sn74hc138_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	return -ENOTSUP;
}

static DEVICE_API(gpio, ti_sn74hc138_drv_api_funcs) = {
	.pin_configure = ti_sn74hc138_config,
	.port_get_raw = ti_sn74hc138_port_get_raw,
	.port_set_masked_raw = ti_sn74hc138_port_set_masked_raw,
	.port_set_bits_raw = ti_sn74hc138_port_set_bits_raw,
	.port_clear_bits_raw = ti_sn74hc138_port_clear_bits_raw,
	.port_toggle_bits = ti_sn74hc138_port_toggle_bits,
};

/**
 * @brief Initialization function of sn74hc138
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int ti_sn74hc138_init(const struct device *dev)
{
	int ret;

	const struct ti_sn74hc138_gpio_config *config = dev->config;

	gpio_pin_configure_dt(&config->input_a_gpio, GPIO_OUTPUT_HIGH);
	gpio_pin_configure_dt(&config->input_b_gpio, GPIO_OUTPUT_HIGH);
	gpio_pin_configure_dt(&config->input_c_gpio, GPIO_OUTPUT_HIGH);

	return 0;
}
#define TI_SN74HC138_GPIO_INIT(n)                                              \
	static const struct ti_sn74hc138_gpio_config ti_sn74hc138_gpio_config_##n = {\
		.config =                                                                  \
			{                                                                        \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),                   \
			},                                                                       \
		.input_a_gpio = GPIO_DT_SPEC_INST_GET(0, input_a_gpios),                   \
		.input_b_gpio = GPIO_DT_SPEC_INST_GET(0, input_b_gpios),                   \
		.input_c_gpio = GPIO_DT_SPEC_INST_GET(0, input_c_gpios)};                  \
                                                                               \
	static struct ti_sn74hc138_gpio_data ti_sn74hc138_gpio_data_##n;             \
                                                                               \
	DEVICE_DT_INST_DEFINE(n, ti_sn74hc138_init, NULL, &ti_sn74hc138_gpio_data_##n,\
			      &ti_sn74hc138_gpio_config_##n, POST_KERNEL,                         \
			      CONFIG_GPIO_INIT_PRIORITY, &ti_sn74hc138_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(TI_SN74HC138_GPIO_INIT)

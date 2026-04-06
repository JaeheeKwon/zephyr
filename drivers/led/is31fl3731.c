/*
 * Conceptual Driver Adaptation for IS31FL3731
 * Based on the structure of the IS31FL3733 Zephyr driver.
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(is31fl3731, CONFIG_LED_LOG_LEVEL);

/* IS31FL3731 register definitions */
#define DT_DRV_COMPAT issi_is31fl3731 // Hypothetical DT compatibility

// IS31FL3731 Matrix Layout: 9 rows (CA1-CA9) x 16 columns (C1-C16) [4]
#define IS31FL3731_ROW_COUNT 9
#define IS31FL3731_COL_COUNT 16
#define IS31FL3731_MAX_LED   (IS31FL3731_ROW_COUNT * IS31FL3731_COL_COUNT) // 144 LEDs [4]

/* Command Register and Page Selection (FDh) [2] */
#define CMD_SEL_REG    0xFD
#define CMD_SEL_FRAME1 0x00 /* Points to Page One (Frame 1 Register) [2] */
#define CMD_SEL_FUNC   0x0B /* Points to Page Nine (Function Register) [6] */

/* Frame 1 Register Addresses (assuming we operate primarily on Frame 1) [3] */
#define LED_CONTROL_START_REG  0x00 /* 00h ~ 11h controls On/Off state [3, 18] */
#define PWM_START_REG          0x24 /* 24h ~ B3h controls PWM [3, 16] */
#define LED_CONTROL_BYTE_COUNT (IS31FL3731_MAX_LED / 8) // 144 / 8 = 18 bytes

/* Function Register (Page 9) Definitions [12, 13] */
#define CONFIG_REG            0x00 /* Used for MODE/FS settings [19] (not SSD) */
#define SHUTDOWN_REG          0x0A /* Used for Software Shutdown Control (SSD) [13] */
#define SHUTDOWN_REG_SSD_MASK 0x1  /* D0 of 0Ah: 1 = Normal Operation, 0 = Shutdown [13] */

struct is31fl3731_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec sdb;
};

struct is31fl3731_data {
	uint32_t selected_page;
	uint8_t scratch_buf[IS31FL3731_MAX_LED + 1]; // Used for bulk operations
	uint8_t current_ssd_state;                   // Store the state of the SSD bit in 0Ah
};

/*
 * Selects target register page for IS31FL3731 by writing to FDh.
 * Note: IS31FL3731 does NOT require the FEh Command Lock Unlock step.
 */
static int is31fl3731_select_page(const struct device *dev, uint8_t page)
{
	const struct is31fl3731_config *config = dev->config;
	struct is31fl3731_data *data = dev->data;
	int ret;

	if (data->selected_page == page) {
		return 0;
	}

	// IS31FL3731: Directly write page value to Command Register (FDh) [2]
	ret = i2c_reg_write_byte_dt(&config->bus, CMD_SEL_REG, page);

	if (ret < 0) {
		// LOG_ERR("Could not select active page"); // Conceptual logging
		return ret;
	}

	data->selected_page = page;

	return ret;
}

static int is31fl3731_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct is31fl3731_config *config = dev->config;
	int ret = 0;
	uint8_t led_brightness =
		(uint8_t)(((uint32_t)value * 255) / LED_BRIGHTNESS_MAX); // Scale brightness

	if (led >= IS31FL3731_MAX_LED) {
		return -EINVAL;
	}

	/* Step 1: Select Frame 1 (Page 1) */
	ret = is31fl3731_select_page(dev, CMD_SEL_FRAME1);
	if (ret < 0) {
		return ret;
	}

	/* Step 2: Calculate target register address. */
	struct is31fl3731_data *data = dev->data;
	uint8_t *pwm_start_buf;

	if ((start_channel + num_channels) > IS31FL3731_MAX_LED) {
		return -EINVAL;
	}

	/* Step 1: Select Frame 1 (Page 1) */
    ret = is31fl3731_select_page(dev, CMD_SEL_FRAME data following the address byte
    memcpy((data->scratch_buf + 1), buf, num_channels);

    // Total length to write: 1 byte address + num_channels data
    return i2c_write_dt(&config->bus, data->scratch_buf, num_channels + 1);
}

/**
 * @brief Blanks IS31FL3731 LED display using Software Shutdown (SSD) register.
 */
int is31fl3731_blank(const struct device *dev, bool blank_en)
{
	const struct is31fl3731_config *config = dev->config;
	struct is31fl3731_data *data = dev->data;
	int ret;

	ret = is31fl3731_select_page(dev, CMD_SEL_FUNC);
	if (ret < 0) {
		return ret;
	}

	if (blank_en) {
		// Shutdown Mode: SSD bit D0 = 0 [13]
		data->current_ssd_state &= ~SHUTDOWN_REG_SSD_MASK;
	} else {
		// Normal Operation: SSD bit D0 = 1 [13]
		data->current_ssd_state |= SHUTDOWN_REG_SSD_MASK;
	}

	return i2c_reg_write_byte_dt(&config->bus, SHUTDOWN_REG, data->current_ssd_state);
}

// --- Initialization Function ---

static int is31fl3731_init(const struct device *dev)
{
	const struct is31fl3731_config *config = dev->config;
	struct is31fl3731_data *data = dev->data;
	int ret = 0U;

	// Check I2C/SDB readiness (similar to 3733 init) [21]
	// ... I2C/GPIO readiness checks (omitted) ...

	if (config->sdb.port != NULL) {
		// Set SDB pin high to exit hardware shutdown [21, 22]
		// ... configure SDB pin to high output (omitted) ...
	}

	/* 1. Reset/Initialize Frame Registers */

	// The 3731 datasheet notes that Frame Register data is not assured when powered on and
	// requires initialization [23]. Since there is no explicit hardware reset register like the
	// 3733's 0x11 (PG3), we must initialize control registers.

	/* 2. Exit Software Shutdown (SSD) */

	ret = is31fl3731_select_page(dev, CMD_SEL_FUNC); // Page 9 [6]
	if (ret < 0) {
		return ret;
	}

	// Write 1 to SSD bit (D0) in Shutdown Register (0Ah) for Normal Operation [13]
	data->current_ssd_state = SHUTDOWN_REG_SSD_MASK;
	ret = i2c_reg_write_byte_dt(&config->bus, SHUTDOWN_REG, data->current_ssd_state);
	if (ret < 0) {
		return ret;
	}

	/* 3. Enable All LEDs in Frame 1 */

	ret = is31fl3731_select_page(dev, CMD_SEL_FRAME1); // Page 1 [2]
	if (ret < 0) {
		return ret;
	}

	// Set LED Control Registers (00h-11h) to 0xFF (LED on) [3, 18]
	data->scratch_buf = LED_CONTROL_START_REG; // Start address is 0x00
	// Fill 18 bytes (144/8) with 0xFF (LED ON)
	memset(data->scratch_buf + 1, 0xFF, LED_CONTROL_BYTE_COUNT);

	// Use I2C auto increment write starting at 0x00 [24]
	return i2c_write_dt(&config->bus, data->scratch_buf, LED_CONTROL_BYTE_COUNT + 1);
}

// --- Device API Definition ---

static DEVICE_API(led, is31fl3731_api) = {
	.set_brightness = is31fl3731_led_set_brightness,
	.write_channels = is31fl3731_led_write_channels,
};

#define IS31FL3731_DEVICE(n)                                                                       \
	static const struct is31fl3731_config is31fl3731_config_##n = {                            \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.sdb = GPIO_DT_SPEC_INST_GET_OR(n, sdb_gpios, {}),                                 \
	};                                                                                         \
                                                                                                   \
	static struct is31fl3731_data is31fl3731_data_##n = {                                      \
		.selected_page = CMD_SEL_REG,                                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &is31fl3731_init, NULL, &is31fl3731_data_##n,                     \
			      &is31fl3731_config_##n, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,       \
			      &is31fl3731_api);

DT_INST_FOREACH_STATUS_OKAY(IS31FL3731_DEVICE)


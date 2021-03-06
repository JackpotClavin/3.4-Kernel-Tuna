/* Power support for Samsung Tuna Board.
 *
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/max17040_battery.h>
#include <linux/pda_power.h>
#include <linux/platform_device.h>

#include "board-tuna.h"
#include "mux.h"
#include "pm.h"

/* These will be different on pre-lunchbox, lunchbox, and final */
#define GPIO_CHARGING_N		83
#define GPIO_TA_NCONNECTED	142
#define GPIO_CHARGE_N		13
#define CHG_CUR_ADJ		102

#define TPS62361_GPIO   7

static struct gpio charger_gpios[] = {
	{ .gpio = GPIO_CHARGING_N, .flags = GPIOF_IN, .label = "charging_n" },
	{ .gpio = GPIO_TA_NCONNECTED, .flags = GPIOF_IN, .label = "charger_n" },
	{ .gpio = GPIO_CHARGE_N, .flags = GPIOF_OUT_INIT_HIGH, .label = "charge_n" },
	{ .gpio = CHG_CUR_ADJ, .flags = GPIOF_OUT_INIT_LOW, .label = "charge_cur_adj" },
};

static int charger_init(struct device *dev)
{
	return gpio_request_array(charger_gpios, ARRAY_SIZE(charger_gpios));
}

static void charger_exit(struct device *dev)
{
	gpio_free_array(charger_gpios, ARRAY_SIZE(charger_gpios));
}

static void charger_set_charge(int state)
{
	gpio_set_value(GPIO_CHARGE_N, !state);
}

static int charger_is_ac_online(void)
{
	return !gpio_get_value(GPIO_TA_NCONNECTED);
}

static int charger_is_charging(void)
{
	return !gpio_get_value(GPIO_CHARGING_N);
}

static __initdata struct resource charger_resources[] = {
	[0] = {
		.name = "ac",
		.flags = IORESOURCE_IRQ |
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
	},
	[1] = {
		.name = "usb",
		.flags = IORESOURCE_IRQ |
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
	}
};

static __initdata struct pda_power_pdata charger_pdata = {
	.init = charger_init,
	.exit = charger_exit,
	.is_ac_online = charger_is_ac_online,
	.is_usb_online = charger_is_ac_online,
	.set_charge = charger_set_charge,
	.wait_for_status = 500,
	.wait_for_charger = 500,
};

static struct max17040_platform_data max17043_pdata = {
	.charger_online = charger_is_ac_online,
	.charger_enable = charger_is_charging,
};

static __initdata struct i2c_board_info max17043_i2c[] = {
	{
		I2C_BOARD_INFO("max17040", (0x6C >> 1)),
		.platform_data = &max17043_pdata,
	}
};

void __init omap4_tuna_power_init(void)
{
	struct platform_device *pdev;
	int status;
#if 0
	if (omap4_tuna_final_gpios()) {
		/* Vsel0 = gpio, vsel1 = gnd */
		status = omap_tps6236x_board_setup(true, TPS62361_GPIO, -1,
					OMAP_PIN_OFF_OUTPUT_HIGH, -1);
		if (status)
			pr_err("TPS62361 initialization failed: %d\n", status);
	}
#endif

	if (omap4_tuna_get_revision() == TUNA_REV_PRE_LUNCHBOX) {
		charger_gpios[0].gpio = 11;
		charger_gpios[1].gpio = 12;
	} else if (!omap4_tuna_final_gpios()) {
		charger_gpios[0].gpio = 159;
		charger_gpios[1].gpio = 160;
	}

	omap_mux_init_gpio(charger_gpios[0].gpio, OMAP_PIN_INPUT);
	omap_mux_init_gpio(charger_gpios[1].gpio, OMAP_PIN_INPUT);
	omap_mux_init_gpio(charger_gpios[2].gpio, OMAP_PIN_OUTPUT);

	pdev = platform_device_register_resndata(NULL, "pda-power", -1,
		charger_resources, ARRAY_SIZE(charger_resources),
		&charger_pdata, sizeof(charger_pdata));

	charger_resources[0].start = gpio_to_irq(GPIO_TA_NCONNECTED);
	charger_resources[0].end = gpio_to_irq(GPIO_TA_NCONNECTED);

	charger_resources[1].start = gpio_to_irq(GPIO_TA_NCONNECTED);
	charger_resources[1].end = gpio_to_irq(GPIO_TA_NCONNECTED);

	i2c_register_board_info(4, max17043_i2c, ARRAY_SIZE(max17043_i2c));
}

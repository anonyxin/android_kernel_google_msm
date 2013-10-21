/*
  * Copyright (C) 2011,2012 LGE, Inc.
  *
  * Author: Sungwoo Cho <sungwoo.cho@lge.com>
  *
  * This software is licensed under the terms of the GNU General
  * License version 2, as published by the Free Software Foundation,
  * may be copied, distributed, and modified under those terms.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
  * GNU General Public License for more details.
  */

#include <linux/types.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/i2c/isa1200.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <mach/gpiomux.h>
#include <mach/msm_iomap.h>
#include <mach/board_xiaomi.h>
#include <mach/msm_xo.h>

#include "devices.h"
#include "board-aries.h"

#ifdef CONFIG_HAPTIC_ISA1200
#define ISA1200_HAP_EN_GPIO		PM8921_GPIO_PM_TO_SYS(33)
#define ISA1200_HAP_LEN_GPIO	PM8921_GPIO_PM_TO_SYS(20)

static struct isa1200_regulator isa1200_reg_data[] = {
	{
		.name = "vddp",
		.min_uV = ISA_I2C_VTG_MIN_UV,
		.max_uV = ISA_I2C_VTG_MAX_UV,
		.load_uA = ISA_I2C_CURR_UA,
	},
};

static struct isa1200_platform_data isa1200_1_pdata = {
	.name = "vibrator",
	.pwm_ch_id = 0,
	.max_timeout = 15000,
	.hap_en_gpio = ISA1200_HAP_EN_GPIO,
	.hap_len_gpio = ISA1200_HAP_LEN_GPIO,
	.mode_ctrl = PWM_INPUT_MODE,
	.pwm_fd = {
		.pwm_freq = 44800,
	},
	.smart_en = false,
	.is_erm = true,
	.ext_clk_en = false,
	.chip_en = 1,
	.duty = 90,
	.regulator_info = isa1200_reg_data,
	.num_regulators = ARRAY_SIZE(isa1200_reg_data),
};
#endif

static struct i2c_board_info i2c_misc_info[] __initdata = {
#ifdef CONFIG_HAPTIC_ISA1200
	{
		I2C_BOARD_INFO("isa1200_1", 0x90>>1),
		.platform_data = &isa1200_1_pdata,
	},
#endif
};

static struct i2c_registry misc_devices __initdata = {
	I2C_FFA,
	APQ_8064_GSBI1_QUP_I2C_BUS_ID,
	i2c_misc_info,
	ARRAY_SIZE(i2c_misc_info),
};

void __init apq8064_init_misc(void)
{
	i2c_register_board_info(misc_devices.bus,
		misc_devices.info,
		misc_devices.len);
}

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
#include <linux/android_vibrator.h>
#include <linux/platform_device.h>
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

/* gpio and clock control for vibrator */
#define REG_WRITEL(value, reg)  writel(value, (MSM_CLK_CTL_BASE+reg))
#define REG_READL(reg)          readl((MSM_CLK_CTL_BASE+reg))

#define GPn_MD_REG(n)           (0x2D00+32*(n))
#define GPn_NS_REG(n)           (0x2D24+32*(n))

/* When use SM100 with GP_CLK
  170Hz motor : 22.4KHz - M=1, N=214 ,
  230Hz motor : 29.4KHZ - M=1, N=163 ,
  */

/* Vibrator GPIOs */
#ifdef CONFIG_ANDROID_VIBRATOR
#define GPIO_MOTOR_EN           PM8921_GPIO_PM_TO_SYS(33)
#define GPIO_MOTOR_PWM          3

#define GP_CLK_ID               0 /* gp clk 0 */
#define GP_CLK_M_DEFAULT        1
#define GP_CLK_N_DEFAULT        166
#define GP_CLK_D_MAX            GP_CLK_N_DEFAULT
#define GP_CLK_D_HALF           (GP_CLK_N_DEFAULT >> 1)

#define MOTOR_AMP               88

static struct gpiomux_setting vibrator_suspend_cfg_gpio3 = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting vibrator_active_cfg = {
	.func = GPIOMUX_FUNC_2, /*gp_mn:2 */
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config gpio3_vibrator_configs[] = {
	{
		.gpio = GPIO_MOTOR_PWM,
		.settings = {
			[GPIOMUX_ACTIVE]    = &vibrator_active_cfg,
			[GPIOMUX_SUSPENDED] = &vibrator_suspend_cfg_gpio3,
		},
	},
};

static struct msm_xo_voter *vib_clock;
static int gpio_vibrator_en = GPIO_MOTOR_EN;
static int gp_clk_id = 0;
static DEFINE_MUTEX(vib_lock);

static void vibrator_clock_init(void)
{
	/* Vote for XO clock */
	vib_clock = msm_xo_get(MSM_XO_TCXO_D0, "vib_clock");
	if (IS_ERR(vib_clock)) {
		pr_warn("%s: Couldn't get TCXO_D0 vote for vibrator\n",
				__func__);
	}
}

static inline void vibrator_clock_on(void)
{
	if (msm_xo_mode_vote(vib_clock, MSM_XO_MODE_ON) < 0)
		pr_warn("%s: Failed to vote for TCX0_D0_ON\n", __func__);
}

static inline void vibrator_clock_off(void)
{
	if (msm_xo_mode_vote(vib_clock, MSM_XO_MODE_OFF) < 0)
		pr_warn("%s: Failed to vote for TCX0_D0_OFF\n", __func__);
}

static struct regulator *vreg_l16 = NULL;
static int vibrator_enabled = 0;

static int vibrator_power_set(int enable)
{
	int rc;

	if (enable == vibrator_enabled)
		return 0;

	mutex_lock(&vib_lock);

	rc = regulator_set_voltage(vreg_l16, 2800000, 2800000);
	if (rc < 0)
		pr_err("%s: regulator_set_voltage failed\n", __func__);

	vibrator_enabled = enable;
	if (enable) {
		rc = regulator_enable(vreg_l16);
		if (rc < 0)
			pr_err("%s: regulator_enable failed\n", __func__);
		else {
			rc = gpio_request(GPIO_MOTOR_PWM, "motor_pwm");
			if (rc < 0)
				pr_warn("%s: gpio_request failed\n", __func__);
			vibrator_clock_on();
		}
	} else {
		if (regulator_is_enabled(vreg_l16) > 0 ) {
			vibrator_clock_off();
			gpio_free(GPIO_MOTOR_PWM);
			rc = regulator_disable(vreg_l16);
			if (rc < 0)
				pr_err("%s: regulator_disable failed\n", __func__);
		}
	}

	mutex_unlock(&vib_lock);

	return rc;
}

static inline int vibrator_adjust_amp(int amp)
{
	int level = 0;
	bool minus = false;

	if (amp < 0) {
		minus = true;
		amp = -amp;
	}

	level = (2 * amp * (GP_CLK_D_HALF-2) + 100) / (2 * 100);
	if (!level && amp)
		level = 1;

	if (minus && level)
		level = -level;

	return level;
}

static int vibrator_pwm_set(int enable, int amp, int n_value)
{
	uint M_VAL = GP_CLK_M_DEFAULT;
	uint D_VAL = 0;
	uint D_INV = 0;
	uint clk_id = gp_clk_id;

	pr_debug("%s: amp=%d, n_value=%d\n", __func__, amp, n_value);

	if (enable) {
		if (amp)
			D_VAL = vibrator_adjust_amp(amp) + GP_CLK_D_HALF;
		if (D_VAL > GP_CLK_D_HALF) {
			D_VAL = GP_CLK_D_MAX - D_VAL;
			D_INV = 1;
		}

		REG_WRITEL(
			(((M_VAL & 0xffU) << 16U) + /* M_VAL[23:16] */
			((~(D_VAL << 1)) & 0xffU)),  /* D_VAL[7:0] */
			GPn_MD_REG(clk_id));

		REG_WRITEL(
			((((~(n_value-M_VAL)) & 0xffU) << 16U) + /* N_VAL[23:16] */
			(1U << 11U) +  /* CLK_ROOT_ENA[11]  : Enable(1) */
			((D_INV & 0x01U) << 10U) +  /* CLK_INV[10]       : Disable(0) */
			(1U << 9U) +   /* CLK_BRANCH_ENA[9] : Enable(1) */
			(1U << 8U) +   /* NMCNTR_EN[8]      : Enable(1) */
			(0U << 7U) +   /* MNCNTR_RST[7]     : Not Active(0) */
			(2U << 5U) +   /* MNCNTR_MODE[6:5]  : Dual-edge mode(2) */
			(3U << 3U) +   /* PRE_DIV_SEL[4:3]  : Div-4 (3) */
			(5U << 0U)),   /* SRC_SEL[2:0]      : CXO (5)  */
			GPn_NS_REG(clk_id));
		pr_debug("%s: PWM is enable with M=%d N=%d D=%d\n",
				__func__,
				M_VAL, n_value, D_VAL);
	} else {
		REG_WRITEL(
			((((~(n_value-M_VAL)) & 0xffU) << 16U) + /* N_VAL[23:16] */
			(0U << 11U) +  /* CLK_ROOT_ENA[11]  : Disable(0) */
			(0U << 10U) +  /* CLK_INV[10]	    : Disable(0) */
			(0U << 9U) +	 /* CLK_BRANCH_ENA[9] : Disable(0) */
			(0U << 8U) +   /* NMCNTR_EN[8]      : Disable(0) */
			(0U << 7U) +   /* MNCNTR_RST[7]     : Not Active(0) */
			(2U << 5U) +   /* MNCNTR_MODE[6:5]  : Dual-edge mode(2) */
			(3U << 3U) +   /* PRE_DIV_SEL[4:3]  : Div-4 (3) */
			(5U << 0U)),   /* SRC_SEL[2:0]      : CXO (5)  */
			GPn_NS_REG(clk_id));
		pr_debug("%s: PWM is disable\n", __func__);
	}

	return 0;
}

static int vibrator_ic_enable_set(int enable)
{
	pr_debug("%s: enable=%d\n", __func__, enable);

	if (enable)
		gpio_set_value(gpio_vibrator_en, 1);
	else
		gpio_set_value(gpio_vibrator_en, 0);

	return 0;
}

static int vibrator_init(void)
{
	int rc;

	/* GPIO function setting */
	msm_gpiomux_install(gpio3_vibrator_configs,
			ARRAY_SIZE(gpio3_vibrator_configs));

	rc = gpio_request_one(gpio_vibrator_en, GPIOF_OUT_INIT_LOW, "motor_en");
	if (rc < 0) {
		pr_err("%s: MOTOR_EN %d request failed\n",
				__func__, gpio_vibrator_en);
		return rc;
	}

	vreg_l16 = regulator_get(NULL, "vibrator");   //2.6 ~ 3V
	if (IS_ERR(vreg_l16)) {
		rc = PTR_ERR(vreg_l16);
		pr_err("%s: regulator get of vibrator failed\n",
				__func__);
		goto err_regulator_get;
	}

	vibrator_clock_init();
	vibrator_ic_enable_set(0);
	vibrator_pwm_set(0, 0, GP_CLK_N_DEFAULT);
	vibrator_power_set(0);

	return 0;

err_regulator_get:
	gpio_free(gpio_vibrator_en);
	return rc;
}

static struct android_vibrator_platform_data vibrator_data = {
	.enable_status = 0,
	.amp = MOTOR_AMP,
	.vibe_n_value = GP_CLK_N_DEFAULT,
	.vibe_warmup_delay = 15,
	.power_set = vibrator_power_set,
	.pwm_set = vibrator_pwm_set,
	.ic_enable_set = vibrator_ic_enable_set,
	.vibrator_init = vibrator_init,
};

static struct platform_device android_vibrator_device = {
	.name   = "android-vibrator",
	.id = -1,
	.dev = {
		.platform_data = &vibrator_data,
	},
};
#endif /* CONFIG_ANDROID_VIBRATOR */

static struct platform_device *misc_devices[] __initdata = {
#ifdef CONFIG_ANDROID_VIBRATOR
	&android_vibrator_device,
#endif
};

void __init apq8064_init_misc(void)
{
	platform_add_devices(misc_devices, ARRAY_SIZE(misc_devices));
}

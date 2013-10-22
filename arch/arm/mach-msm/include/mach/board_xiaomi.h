/* arch/arm/mach-msm/include/mach/board_xiaomi.h
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2012, The Linux Foundation. All rights reserved.
 * Copyright (c) 2012, XIAOMI Inc.
 * Author: Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ASM_ARCH_MSM_BOARD_XIAOMI_H
#define __ASM_ARCH_MSM_BOARD_XIAOMI_H

#ifdef CONFIG_ANDROID_PERSISTENT_RAM
#define XIAOMI_PERSISTENT_RAM_SIZE	(SZ_1M)
#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define XIAOMI_RAM_CONSOLE_SIZE	(124*SZ_1K * 2)
#endif

#ifdef CONFIG_XIAOMI_CRASH_HANDLER
#define XIAOMI_CRASH_LOG_SIZE	(4*SZ_1K + SZ_1K)
#endif

typedef enum {
	HW_REV_EVB1 = 0,
	HW_REV_EVB2,
	HW_REV_A,
	HW_REV_B,
	HW_REV_C,
	HW_REV_D,
	HW_REV_E,
	HW_REV_F,
	HW_REV_G,
	HW_REV_H,
	HW_REV_1_0,
	HW_REV_1_1,
	HW_REV_1_2,
	HW_REV_MAX
} hw_rev_type;

hw_rev_type xiaomi_get_board_revno(void);

#ifdef CONFIG_LCD_KCAL
struct kcal_data {
	int red;
	int green;
	int blue;
};

struct kcal_platform_data {
	int (*set_values) (int r, int g, int b);
	int (*get_values) (int *r, int *g, int *b);
	int (*refresh_display) (void);
};
#endif

int __init xiaomi_get_uart_mode(void);

/* from androidboot.mode */
enum xiaomi_boot_mode_type {
	XIAOMI_BOOT_MODE_NORMAL = 0,
	XIAOMI_BOOT_MODE_CHARGER,
	XIAOMI_BOOT_MODE_CHARGERLOGO,
	XIAOMI_BOOT_MODE_FACTORY,
	XIAOMI_BOOT_MODE_FACTORY2,
	XIAOMI_BOOT_MODE_PIFBOOT,
	XIAOMI_BOOT_MODE_PIFBOOT2,
};

void __init xiaomi_reserve(void);

#ifdef CONFIG_ANDROID_PERSISTENT_RAM
void __init xiaomi_add_persistent_ram(void);
#else
static inline void __init xiaomi_add_persistent_ram(void)
{
	/* empty */
}
#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
void __init xiaomi_add_ramconsole_devices(void);
#else
static inline void __init xiaomi_add_ramconsole_devices(void)
{
	/* empty */
}
#endif

#ifdef CONFIG_XIAOMI_CRASH_HANDLER
void __init xiaomi_add_panic_handler_devices(void);
int get_ssr_magic_number(void);
void set_ssr_magic_number(const char *subsys_name);
#else
static inline void __init xiaomi_add_panic_handler_devices(void)
{
	/* empty */
}
#endif

#ifdef CONFIG_XIAOMI_QFPROM_INTERFACE
void __init xiaomi_add_qfprom_devices(void);
#endif

#ifdef CONFIG_XIAOMI_PRIMACONFIG_INTERFACE
void __init xiaomi_add_primaconfig_devices(void);
#endif

enum xiaomi_boot_mode_type xiaomi_get_boot_mode(void);
int xiaomi_get_factory_boot(void);

#endif // __ASM_ARCH_MSM_BOARD_XIAOMI_H

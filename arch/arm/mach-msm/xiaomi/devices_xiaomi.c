/* Copyright (c) 2012, XIAOMI Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/memory.h>
#include <linux/persistent_ram.h>

#include <asm/setup.h>
#include <asm/sizes.h>
#include <asm/system_info.h>
#include <asm/memory.h>

#include <mach/board_xiaomi.h>

#include <ram_console.h>

/* setting whether uart console is enalbed or disabled */
static int uart_console_mode = 0;

int __init xiaomi_get_uart_mode(void)
{
	return uart_console_mode;
}

static int __init xiaomi_uart_mode(char *uart_mode)
{
	if (!strncmp("enable", uart_mode, 5)) {
		printk(KERN_INFO"UART CONSOLE : enable\n");
		uart_console_mode = 1;
	}
	else
		printk(KERN_INFO"UART CONSOLE : disable\n");

	return 1;
}
__setup("uart_console=", xiaomi_uart_mode);

/* for board revision */
static hw_rev_type xiaomi_bd_rev = HW_REV_EVB1;

static int __init board_revno_setup(char *rev_info)
{
	/* CAUTION: These strings are come from LK. */
	char *rev_str[] = {"evb1", "evb2", "rev_a", "rev_b", "rev_c", "rev_d",
		"rev_e", "rev_f", "rev_g", "rev_h", "rev_10", "rev_11", "rev_12",
		"reserved"};
	int i;

	printk(KERN_INFO "BOARD : XIAOMI input %s \n", rev_info);
	for (i=0; i< HW_REV_MAX; i++) {
		if( !strncmp(rev_info, rev_str[i], 6)) {
			xiaomi_bd_rev = (hw_rev_type) i;
			system_rev = xiaomi_bd_rev;
			break;
		}
	}

	printk(KERN_INFO "BOARD : XIAOMI matched %s \n", rev_str[xiaomi_bd_rev]);
	return 1;
}
__setup("xiaomi.rev=", board_revno_setup);

hw_rev_type xiaomi_get_board_revno(void)
{
    return xiaomi_bd_rev;
}

#ifdef CONFIG_LCD_KCAL
extern int kcal_set_values(int kcal_r, int kcal_g, int kcal_b);
static int __init display_kcal_setup(char *kcal)
{
	char vaild_k = 0;
	int kcal_r = 0;
	int kcal_g = 0;
	int kcal_b = 0;

	sscanf(kcal, "%d|%d|%d|%c", &kcal_r, &kcal_g, &kcal_b, &vaild_k );
	pr_info("kcal is %d|%d|%d|%c\n", kcal_r, kcal_g, kcal_b, vaild_k);

	if (vaild_k != 'K') {
		pr_info("kcal not calibrated yet : %d\n", vaild_k);
		kcal_r = kcal_g = kcal_b = 255;
		pr_info("set to default : %d\n", kcal_r);
	}

	kcal_set_values(kcal_r, kcal_g, kcal_b);
	return 1;
}
__setup("xiaomi.kcal=", display_kcal_setup);
#endif

/* get boot mode information from cmdline.
 * If any boot mode is not specified,
 * boot mode is normal type.
 */
static enum xiaomi_boot_mode_type xiaomi_boot_mode = XIAOMI_BOOT_MODE_NORMAL;
int __init xiaomi_boot_mode_init(char *s)
{
	if (!strcmp(s, "charger"))
		xiaomi_boot_mode = XIAOMI_BOOT_MODE_CHARGER;
	else if (!strcmp(s, "chargerlogo"))
		xiaomi_boot_mode = XIAOMI_BOOT_MODE_CHARGERLOGO;
	else if (!strcmp(s, "factory"))
		xiaomi_boot_mode = XIAOMI_BOOT_MODE_FACTORY;
	else if (!strcmp(s, "factory2"))
		xiaomi_boot_mode = XIAOMI_BOOT_MODE_FACTORY2;
	else if (!strcmp(s, "pifboot"))
		xiaomi_boot_mode = XIAOMI_BOOT_MODE_PIFBOOT;
	else if (!strcmp(s, "pifboot2"))
		xiaomi_boot_mode = XIAOMI_BOOT_MODE_PIFBOOT2;

	return 1;
}
__setup("androidboot.mode=", xiaomi_boot_mode_init);

enum xiaomi_boot_mode_type xiaomi_get_boot_mode(void)
{
	return xiaomi_boot_mode;
}

int xiaomi_get_factory_boot(void)
{
	int res;

	/* if boot mode is factory,
	 * cable must be factory cable.
	 */
	switch (xiaomi_boot_mode) {
		case XIAOMI_BOOT_MODE_FACTORY:
		case XIAOMI_BOOT_MODE_FACTORY2:
		case XIAOMI_BOOT_MODE_PIFBOOT:
		case XIAOMI_BOOT_MODE_PIFBOOT2:
			res = 1;
			break;
		default:
			res = 0;
			break;
	}

	return res;
}

#ifdef CONFIG_ANDROID_PERSISTENT_RAM
static struct persistent_ram_descriptor pram_descs[] = {
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	{
		.name = "ram_console",
		.size = XIAOMI_RAM_CONSOLE_SIZE,
	},
#endif
#ifdef CONFIG_XIAOMI_CRASH_HANDLER
	{
		.name = "panic-handler",
		.size = XIAOMI_CRASH_LOG_SIZE,
	},
#endif
};

static struct persistent_ram xiaomi_persistent_ram = {
	.size = XIAOMI_PERSISTENT_RAM_SIZE,
	.num_descs = ARRAY_SIZE(pram_descs),
	.descs = pram_descs,
};

void __init xiaomi_add_persistent_ram(void)
{
	struct persistent_ram *pram = &xiaomi_persistent_ram;
	struct membank* bank = &meminfo.bank[0];

	pram->start = bank->start + bank->size - XIAOMI_PERSISTENT_RAM_SIZE;

	persistent_ram_early_init(pram);
}
#endif

void __init xiaomi_reserve(void)
{
	xiaomi_add_persistent_ram();
}

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static char bootreason[128] = {0,};
int __init xiaomi_boot_reason(char *s)
{
	int n;

	if (*s == '=')
		s++;
	n = snprintf(bootreason, sizeof(bootreason),
		 "Boot info:\n"
		 "Last boot reason: %s\n", s);
	bootreason[n] = '\0';
	return 1;
}
__setup("bootreason", xiaomi_boot_reason);

struct ram_console_platform_data ram_console_pdata = {
	.bootinfo = bootreason,
};

static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1,
	.dev = {
		.platform_data = &ram_console_pdata,
	}
};

void __init xiaomi_add_ramconsole_devices(void)
{
	platform_device_register(&ram_console_device);
}
#endif /* CONFIG_ANDROID_RAM_CONSOLE */

#ifdef CONFIG_XIAOMI_CRASH_HANDLER
static struct platform_device panic_handler_device = {
	.name = "panic-handler",
	.id = -1,
};

void __init xiaomi_add_panic_handler_devices(void)
{
	platform_device_register(&panic_handler_device);
}
#endif /* CONFIG_XIAOMI_CRASH_HANDLER */

#ifdef CONFIG_XIAOMI_QFPROM_INTERFACE
static struct platform_device qfprom_device = {
	.name = "xiaomi-apq8064-qfprom",
	.id = -1,
};
void __init xiaomi_add_qfprom_devices(void)
{
	platform_device_register(&qfprom_device);
}
#endif

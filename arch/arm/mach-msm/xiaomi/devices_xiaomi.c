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

/* setting whether uart console is enabled or disabled */
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

#ifdef CONFIG_ANDROID_PERSISTENT_RAM
static struct persistent_ram_descriptor pram_descs[] = {
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	{
		.name = "ram_console",
		.size = XIAOMI_RAM_CONSOLE_SIZE,
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

#ifdef CONFIG_XIAOMI_PRIMACONFIG_INTERFACE
static struct platform_device primaconfig_device = {
	.name = "primaconfig",
	.id = -1,
};
void __init xiaomi_add_primaconfig_devices(void)
{
	platform_device_register(&primaconfig_device);
}
#endif /* CONFIG_XIAOMI_PRIMACONFIG_INTERFACE */

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 Igor Belwon <igor.belwon@mentallysanemainliners.org>
 */

#include <config.h>
#include <env.h>
#include <init.h>
#include <asm/global_data.h>
#include <asm/system.h>
#include <fastboot.h>
#include <string.h>

DECLARE_GLOBAL_DATA_PTR;

void __weak fastboot_oem_board(char *cmd_parameter, void *data, u32 size, char *response)
{
	if(strncmp(cmd_parameter, "poweroff", 10) == 0)
	{
		fastboot_okay("goodbye :(", response);
		psci_system_off();
	}
	else
	{
		fastboot_fail("unknown oem_board command", response);
	}
}

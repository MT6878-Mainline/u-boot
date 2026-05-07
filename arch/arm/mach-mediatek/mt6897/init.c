// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 Igor Belwon <igor.belwon@mentallysanemainliners.org>
 */

#include <fdtdec.h>
#include <init.h>
#include <asm/armv8/mmu.h>
#include <asm/system.h>
#include <asm/global_data.h>
#include <linux/sizes.h>

DECLARE_GLOBAL_DATA_PTR;

int print_cpuinfo(void)
{
	return 0;
}

int dram_init(void)
{
	int ret;

	ret = fdtdec_setup_mem_size_base();
	if (ret)
		return ret;

	// Fix ram_size to 2GB, above that things start to break
	gd->ram_size = get_ram_size((void *)gd->ram_base, SZ_2G);

	return 0;
}

void reset_cpu(void)
{
	psci_system_reset();
}

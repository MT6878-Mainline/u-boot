// SPDX-License-Identifier: GPL-2.0+
/*
 * To be filled... :)
 */

#include <command.h>
#include <display_options.h>
#include <linux/compiler.h>

extern void bl31_entry(void);

int do_run_bl31(struct cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct entry_point_info bl33_info;

	bl33_info.h.attr = EP_NON_SECURE | EP_ST_ENABLE;
	bl33_info.pc = 0x41F00000; // dummy BL33
	bl33_info.args.arg0 = 0;

	flush_dcache_range(&bl33_info, sizeof(bl33_info));

	el3_call((uintptr_t)bl31_entry, (uintptr_t)&bl33_info, 0, 0);

	return 0;
}

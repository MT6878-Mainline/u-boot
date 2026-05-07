// SPDX-License-Identifier: GPL-2.0
/*
 * MediaTek generic mobile board source
 * Mostly stolen from board/samsung/exynos-mobile/exynos-mobile.c
 *
 * Copyright (c) 2025 Kaustabh Chakraborty <kauschluss@disroot.org>
 * Copyright (c) 2026 Igor Belwon <igor.belwon@mentallysanemainliners.org>
 */

#include <asm/armv8/mmu.h>
#include <blk.h>
#include <bootflow.h>
#include <ctype.h>
#include <dm/ofnode.h>
#include <efi.h>
#include <efi_loader.h>
#include <env.h>
#include <errno.h>
#include <init.h>
#include <linux/sizes.h>
#include <lmb.h>
#include <part.h>
#include <stdbool.h>
#include <string.h>

#include <scsi.h>

DECLARE_GLOBAL_DATA_PTR;

#define lmb_alloc(size, addr) \
	lmb_alloc_mem(LMB_MEM_ALLOC_ANY, SZ_2M, addr, size, LMB_NONE)

struct efi_fw_image fw_images[] = {
	{
		.fw_name = u"UBOOT_BOOT_PARTITION",
		.image_index = 1,
	},
};

struct efi_capsule_update_info update_info = {
	.dfu_string = NULL,
	.images = fw_images,
	.num_images = ARRAY_SIZE(fw_images),
};

static int mediatek_fastboot_setup(void)
{
	struct blk_desc *blk_dev;
	struct disk_partition info = {0};
	phys_addr_t addr;
	int i;

	/* Allocate and define buffer address for fastboot interface. */
	if (lmb_alloc(CONFIG_FASTBOOT_BUF_SIZE, &addr)) {
		log_err("%s: failed to allocate fastboot buffer\n", __func__);
		return -ENOMEM;
	}
	env_set_hex("fastboot_addr_r", addr);

	scsi_scan(true);

	blk_dev = blk_get_dev("scsi", CONFIG_FASTBOOT_FLASH_BLOCK_DEVICE_ID);
	if (!blk_dev) {
		log_err("%s: required scsi device not available\n", __func__);
		return -ENODEV;
	}

	for (i = 1; i < CONFIG_EFI_PARTITION_ENTRIES_NUMBERS; i++) {
		if (part_get_info(blk_dev, i, &info))
			continue;

		// works printk("Partition: %s\n", info.name);
	}

	return 0;
}

int misc_init_r(void)
{
	return mediatek_fastboot_setup();
}

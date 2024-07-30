// SPDX-License-Identifier: GPL-2.0
/*
 * TODO
 */

#include <asm/armv8/mmu.h>

int board_debug_uart_init(void)
{
	return 0;	
}

int board_init(void)
{
	return 0;
}

static struct mm_region tetris_mem_map[] = {
    {
        /* Peripheral region */
        .virt = 0x00000000UL,
        .phys = 0x00000000UL,
        .size = 0x1B000000UL,
        .attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
                PTE_BLOCK_NON_SHARE |
                PTE_BLOCK_PXN | PTE_BLOCK_UXN,
    },
    {
        /* DRAM region */
        .virt = 0x40000000UL,
        .phys = 0x40000000UL,
        .size = 0x10000000UL,
        .attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) | PTE_BLOCK_OUTER_SHARE
    },
    {
        /* TEE Reserved */
        .virt = 0x70000000UL,
        .phys = 0x70000000UL,
        .size = 0x06c00000UL,
         .attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
            PTE_BLOCK_NON_SHARE |
            PTE_BLOCK_PXN | PTE_BLOCK_UXN,
    },
    {
        /* MCUPM Reserved */
        .virt = 0x7fa80000UL,
        .phys = 0x7fa80000UL,
        .size = 0x00100000UL,
        .attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
            PTE_BLOCK_NON_SHARE |
            PTE_BLOCK_PXN | PTE_BLOCK_UXN,
    },
    {
        /* SSPM Reserved */
        .virt = 0x7fb80000UL,
        .phys = 0x7fb80000UL,
        .size = 0x00280000UL,
        .attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
            PTE_BLOCK_NON_SHARE |
            PTE_BLOCK_PXN | PTE_BLOCK_UXN,
    },
    {
        /* Framebuffer Reserved */
        .virt = 0xfe0d0000UL,
        .phys = 0xfe0d0000UL,
        .size = 0x01f2f000UL,
        .attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
            PTE_BLOCK_NON_SHARE |
            PTE_BLOCK_PXN | PTE_BLOCK_UXN,
    },
    {
        /* SCP Reserved */
        .virt = 0xb8000000UL,
        .phys = 0xb8000000UL,
        .size = 0x02300000UL,
        .attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
            PTE_BLOCK_NON_SHARE |
            PTE_BLOCK_PXN | PTE_BLOCK_UXN,
    },
    {
        /* ATF Reserved */
        .virt = 0xbfe00000UL,
        .phys = 0xbfe00000UL,
        .size = 0x00200000UL,
        .attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
            PTE_BLOCK_NON_SHARE |
            PTE_BLOCK_PXN | PTE_BLOCK_UXN,
    },
    {
        0,
    }
};

struct mm_region *mem_map = tetris_mem_map;

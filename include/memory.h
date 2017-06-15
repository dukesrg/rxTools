/*
 * Copyright (C) 2015, 2017 The PASTA Team, dukesrg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MEMORY_H
#define MEMORY_H

#define ARM9_VRAM_ADDR (0x18000000)
#define ARM11_VRAM_PADDR ARM9_VRAM_ADDR

#define MEM_ARM9_ITCM		0x00000000
#define MEM_ARM11_BOOTROM	0x00000000
#define MEM_ARM11_BOOTROM_MIRROR	0x00010000
#define MEM_ARM9_ITCM_KERNEL	0x01FF8000
#define MEM_ARM9_ITCM_BOOTROM	0x07FF8000
#define MEM_ARM9_RAM		0x08000000
#define MEM_ARM9_RAM_KTR	0x08100000
#define MEM_IO			0x10000000
#define MEM_ARM11_MPCORE	0x17E00000
#define MEM_ARM11_L2C		0x17E10000
#define MEM_VRAM		0x18000000
#define MEM_ARM11_RAM_KTR	0x1F000000
#define MEM_DSP			0x1FF00000
#define MEM_AXI_WRAM		0x1FF80000
#define MEM_FCRAM		0x20000000
#define MEM_FCRAM_KTR		0x28000000
#define MEM_ARM9_DTCM		0xFFF00000
#define MEM_BOOTROM		0xFFFF0000

#endif

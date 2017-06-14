/*
 * Copyright (C) 2017 dukesrg
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
 * You should have received a copy of the GNU General Public Licenselan
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ARM_CP15_H
#define ARM_CP15_H

// Control Register
#define CR_M	(1 << 0)	// MMU enable
#define CR_A	(1 << 1)	// Alignment abort enable
#define CR_C	(1 << 2)	// Dcache enable
#define CR_W	(1 << 3)	// Write buffer enable
#define CR_P	(1 << 4)	// 32-bit exception handler
#define CR_D	(1 << 5)	// 32-bit data address range
#define CR_L	(1 << 6)	// Implementation defined
#define CR_B	(1 << 7)	// Big endian
#define CR_S	(1 << 8)	// System MMU protection
#define CR_R	(1 << 9)	// ROM MMU protection
#define CR_F	(1 << 10)	// Implementation defined
#define CR_Z	(1 << 11)	// Implementation defined
#define CR_I	(1 << 12)	// Icache enable
#define CR_V	(1 << 13)	// Vectors relocated to 0xffff0000
#define CR_RR	(1 << 14)	// Round Robin cache replacement
#define CR_L4	(1 << 15)	// LDR pc can set T bit
#define CR_DT	(1 << 16)
#ifdef CONFIG_MMU
#define CR_HA	(1 << 17)	// Hardware management of Access Flag
#else
#define CR_BR	(1 << 17)	// MPU Background region enable (PMSA)
#endif
#define CR_IT	(1 << 18)
#define CR_ST	(1 << 19)
#define CR_FI	(1 << 21)	// Fast interrupt (lower latency mode)
#define CR_U	(1 << 22)	// Unaligned access operation
#define CR_XP	(1 << 23)	// Extended page tables
#define CR_VE	(1 << 24)	// Vectored interrupts
#define CR_EE	(1 << 25)	// Exception (Big) Endian
#define CR_TRE	(1 << 28)	// TEX remap enable
#define CR_AFE	(1 << 29)	// Access flag enable
#define CR_TE	(1 << 30)	// Thumb exception enable

// Protection Region Base and Size Registers
#define PR_EN		1
#define PR_SZ_4K	(0x0B << 1)
#define PR_SZ_8K	(0x0C << 1)
#define PR_SZ_16K	(0x0D << 1)
#define PR_SZ_32K	(0x0E << 1)
#define PR_SZ_64K	(0x0F << 1)
#define PR_SZ_128K	(0x10 << 1)
#define PR_SZ_256K	(0x11 << 1)
#define PR_SZ_512K	(0x12 << 1)
#define PR_SZ_1M	(0x13 << 1)
#define PR_SZ_2M	(0x14 << 1)
#define PR_SZ_4M	(0x15 << 1)
#define PR_SZ_8M	(0x16 << 1)
#define PR_SZ_16M	(0x17 << 1)
#define PR_SZ_32M	(0x18 << 1)
#define PR_SZ_64M	(0x19 << 1)
#define PR_SZ_128M	(0x1A << 1)
#define PR_SZ_256M	(0x1B << 1)
#define PR_SZ_512M	(0x1C << 1)
#define PR_SZ_1G	(0x1D << 1)
#define PR_SZ_2G	(0x1E << 1)
#define PR_SZ_4G	(0x1F << 1)

#endif
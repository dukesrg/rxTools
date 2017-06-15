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

#ifndef ARM_H
#define ARM_H

// ARM946E-S registers

// Program Status Registers
#define PSR_MODE_USER	0x10
#define PSR_MODE_FIQ	0x11
#define PSR_MODE_IRQ	0x12
#define PSR_MODE_SUPER	0x13
#define PSR_MODE_ABORT	0x17
#define PSR_MODE_UNDEFINED	0x1B
#define PSR_MODE_SYSTEM	0x1F
#define PSR_THUMB_STATE	(1 << 5)
#define PSR_FIQ_DISABLE	(1 << 6)
#define PSR_IRQ_DISABLE	(1 << 7)

// Control Register
#define CR_M	(1 << 0)	// Protection unit enable
#define CR_C	(1 << 2)	// Data cache enable
#define CR_B	(1 << 7)	// Big endian
#define CR_I	(1 << 12)	// Instruction cache enable
#define CR_V	(1 << 13)	// Alternate vector select
#define CR_RR	(1 << 14)	// Round Robin cache replacement
#define CR_L4	(1 << 15)	// Disable loading TBIT
#define CR_DT	(1 << 16)	// Data TCM enable
#define CR_HA	(1 << 17)	// Data TCM load mode
#define CR_IT	(1 << 18)	// Instruction TCM enable
#define CR_ST	(1 << 19)	// Instruction TCM load mode

// Cache Configuration/Write Buffer Control Registers
#define CB_0	(1 << 0)
#define CB_1	(1 << 1)
#define CB_2	(1 << 2)
#define CB_3	(1 << 3)
#define CB_4	(1 << 4)
#define CB_5	(1 << 5)
#define CB_6	(1 << 6)
#define CB_7	(1 << 7)

// Access Permission Registers
#define AP_NA_NA	0
#define AP_RW_NA	1
#define AP_RW_RO	2
#define AP_RW_RW	3
#define AP_EX_RO_NA	5
#define AP_EX_RO_RO	6
#define AP_STD(AP0, AP1, AP2, AP3, AP4, AP5, AP6, AP7)	(AP0 | (AP1 << 2) | (AP2 << 4) | (AP3 << 6) | (AP4 << 8) | (AP5 << 10) | (AP6 << 12) | (AP7 << 14))
#define AP_EXT(AP0, AP1, AP2, AP3, AP4, AP5, AP6, AP7)	(AP0 | (AP1 << 4) | (AP2 << 8) | (AP3 << 12) | (AP4 << 16) | (AP5 << 20) | (AP6 << 24) | (AP7 << 28))
#define AP_ALL_ACCESS	AP_EXT(AP_RW_RW, AP_RW_RW, AP_RW_RW, AP_RW_RW, AP_RW_RW, AP_RW_RW, AP_RW_RW, AP_RW_RW)

// Protection Region Base and Size Registers
#define PR_EN		1
#define PR_SZ_4K	0x16
#define PR_SZ_8K	0x18
#define PR_SZ_16K	0x1A
#define PR_SZ_32K	0x1C
#define PR_SZ_64K	0x1E
#define PR_SZ_128K	0x20
#define PR_SZ_256K	0x22
#define PR_SZ_512K	0x24
#define PR_SZ_1M	0x26
#define PR_SZ_2M	0x28
#define PR_SZ_4M	0x2A
#define PR_SZ_8M	0x2C
#define PR_SZ_16M	0x2E
#define PR_SZ_32M	0x30
#define PR_SZ_64M	0x32
#define PR_SZ_128M	0x34
#define PR_SZ_256M	0x36
#define PR_SZ_512M	0x38
#define PR_SZ_1G	0x3A
#define PR_SZ_2G	0x3C
#define PR_SZ_4G	0x3E

//Tightly-coupled Memory Region Registers
#define TCM_SZ_4K	0x06
#define TCM_SZ_8K	0x08
#define TCM_SZ_16K	0x0A
#define TCM_SZ_32K	0x0C
#define TCM_SZ_64K	0x0E
#define TCM_SZ_128K	0x10
#define TCM_SZ_256K	0x12
#define TCM_SZ_512K	0x14
#define TCM_SZ_1M	0x16
#define TCM_SZ_2M	0x18
#define TCM_SZ_4M	0x1A
#define TCM_SZ_8M	0x1C
#define TCM_SZ_16M	0x1E
#define TCM_SZ_32M	0x20
#define TCM_SZ_64M	0x22
#define TCM_SZ_128M	0x24
#define TCM_SZ_256M	0x26
#define TCM_SZ_512M	0x28
#define TCM_SZ_1G	0x2A
#define TCM_SZ_2G	0x2C
#define TCM_SZ_4G	0x2E

#endif

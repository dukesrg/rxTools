/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2014-2015, Normmatt, 173210
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2, as described below:
 *
 * This file is free software: you may copy, redistribute and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#ifndef TMIO_H
#define TMIO_H

#include <stdint.h>

enum tmio_dev_id {
	TMIO_DEV_SDMC = 0,
	TMIO_DEV_NAND = 1,

	TMIO_DEV_NUM
};

#define CSD_STRUCTURE_SD1	0
#define CSD_STRUCTURE_SD2	1
#define CSD_STRUCTURE_EMMC4	2

#define SPEC_VERS_EMMC4		4

typedef union {
	struct {
		uint16_t MDT_Y:4, MDT_M:8, :4;
		uint32_t PSN;
		uint8_t PSRV_L:4, PSRV_H:4;
		char PNM[5];
		char OID[2];
		uint8_t MID;
		uint8_t	unused;
	} __attribute__((packed)) sd;
	struct {
		uint8_t MDT_Y:4, MDT_M:4;
		uint32_t PSN;
		uint8_t PRV_L:4, PRV_H:4;
		char PNM[6];
		uint8_t OID;
		uint8_t CBX:2, :6;
		uint8_t MID;
		uint8_t	unused;
	} __attribute__((packed)) emmc;
} card_cid;

typedef union {
	struct {
		uint8_t :2, FILE_FORMAT:2, TMP_WRITE_PROTECT:1, PERM_WRITE_PROTECT:1, COPY:1, FILE_FORMAT_GRP:1;
		uint16_t :5, WRITE_BL_PARTIAL:1, WRITE_BL_LEN:4, R2W_FACTOR:3, :2, WP_GRP_ENABLE:1;
		uint32_t WP_GRP_SIZE:7, SECTOR_SIZE:7, ERASE_BLK_EN:1, C_SIZE_MULT:3, VDD_W_CURR_MAX:3, VDD_W_CURR_MIN:3, VDD_R_CURR_MAX:3, VDD_R_CURR_MIN:3, C_SIZE_L:2;
		uint16_t C_SIZE_H:10, :2, DSR_IMP:1, READ_BLK_MISALIGN:1, WRITE_BLK_MISALIGN:1, READ_BL_PARTIAL:1;
		uint16_t READ_BL_LEN:4, CCC:12;
		uint8_t TRAN_SPEED;
		uint8_t NSAC;
		uint8_t TAAC;
		uint8_t :6, CSD_STRUCTURE:2;
		uint8_t	unused;
	} __attribute__((packed)) sd1;
	struct {
		uint8_t ECC:2, FILE_FORMAT:2, TMP_WRITE_PROTECT:1, PERM_WRITE_PROTECT:1, COPY:1, FILE_FORMAT_GRP:1;
		uint16_t CONTENT_PROT_APP:1, :4, WRITE_BL_PARTIAL:1, WRITE_BL_LEN:4, R2W_FACTOR:3, DEFAULT_ECC:2, WP_GRP_ENABLE:1;
		uint32_t WP_GRP_SIZE:5, ERASE_GRP_MULT:5, ERASE_GRP_SIZE:5, C_SIZE_MULT:3, VDD_W_CURR_MAX:3, VDD_W_CURR_MIN:3, VDD_R_CURR_MAX:3, VDD_R_CURR_MIN:3, C_SIZE_L:2;
		uint16_t C_SIZE_H:10, :2, DSR_IMP:1, READ_BLK_MISALIGN:1, WRITE_BLK_MISALIGN:1, READ_BL_PARTIAL:1;
		uint16_t READ_BL_LEN:4, CCC:12;
		uint8_t TRAN_SPEED;
		uint8_t NSAC;
		uint8_t TAAC;
		uint8_t :2, SPEC_VERS:4, CSD_STRUCTURE:2;
		uint8_t	unused;
	} __attribute__((packed)) emmc;
	struct {
		uint8_t	:1, CRC:7;
		uint8_t :2, FILE_FORMAT:2, TMP_WRITE_PROTECT:1, PERM_WRITE_PROTECT:1, COPY:1, FILE_FORMAT_GRP:1;
		uint16_t :5, WRITE_BL_PARTIAL:1, WRITE_BL_LEN:4, R2W_FACTOR:3, :2, WP_GRP_ENABLE:1;
		uint16_t WP_GRP_SIZE:7, SECTOR_SIZE:7, ERASE_BLK_EN:1, :1;
		uint32_t C_SIZE:22, :6, DSR_IMP:1, READ_BLK_MISALIGN:1, WRITE_BLK_MISALIGN:1, READ_BL_PARTIAL:1;
		uint16_t READ_BL_LEN:4, CCC:12;
		uint8_t TRAN_SPEED;
		uint8_t NSAC;
		uint8_t TAAC;
		uint8_t :6, CSD_STRUCTURE:2;
		uint8_t	unused;
	} __attribute__((packed)) sd2;
} card_csd;

typedef union {
	card_cid CID;
	card_csd CSD;
} tmio_response;

typedef struct {
	uint32_t RCA;
	uint_fast16_t clk;
	uint_fast8_t addr_mul;
	uint_fast8_t OPT;
	card_cid CID;
	card_csd CSD;
} tmio_device;

#define IO_REG_BASE		0x10000000
#define IO_REG(x)		(IO_REG_BASE + x)
#define IO_REG_8(x)		*(volatile uint8_t*)IO_REG(x)
#define IO_REG_16(x)		*(volatile uint16_t*)IO_REG(x)
#define IO_REG_32(x)		*(volatile uint32_t*)IO_REG(x)

#define REG_MMC_BASE		0x6000
#define REG_MMC_16(x)		IO_REG_16(REG_MMC_BASE + x)
#define REG_MMC_32(x)		IO_REG_32(REG_MMC_BASE + x)
#define REG_MMC_128(x)		(*(volatile tmio_response*)IO_REG(REG_MMC_BASE + x))

#define REG_MMC_CMD		REG_MMC_16(0x00)
#define REG_MMC_PORTSEL		REG_MMC_16(0x02)
#define REG_MMC_CMDARG		REG_MMC_32(0x04)
#define REG_MMC_STOP		REG_MMC_16(0x08)
#define REG_MMC_BLKCOUNT	REG_MMC_16(0x0A)
#define REG_MMC_RESP0		REG_MMC_32(0x0C)
#define REG_MMC_RESP		REG_MMC_128(0x0C)
#define REG_MMC_IRQ_STATUS	REG_MMC_32(0x1C)
#define REG_MMC_IRQ_MASK	REG_MMC_32(0x20)
#define REG_MMC_CLKCTL		REG_MMC_16(0x24)
#define REG_MMC_BLKLEN		REG_MMC_16(0x26)
#define REG_MMC_OPT		REG_MMC_16(0x28)
#define REG_MMC_FIFO		REG_MMC_16(0x30)
#define REG_MMC_DATACTL 	REG_MMC_16(0xD8)
#define REG_MMC_RESET		REG_MMC_16(0xE0)
//#define REG_MMC_PROTECTED	REG_MMC_16(0xF6) //bit 0 determines if sd is protected or not?
#define REG_MMC_FC		REG_MMC_16(0xFC)
#define REG_MMC_FE		REG_MMC_16(0xFE)
#define REG_MMC_DATACTL32 	REG_MMC_16(0x100)
#define REG_MMC_BLKLEN32	REG_MMC_16(0x104)
#define REG_MMC_BLKCOUNT32	REG_MMC_16(0x108)
#define REG_MMC_FIFO32		REG_MMC_32(0x10C)
//#define REG_MMC_CLK_AND_WAIT_CTL	REG_MMC_32(0x138)
//#define REG_MMC_RESET_SDIO	REG_MMC_32(0x1E0)

void tmio_init(void);
uint32_t tmio_init_dev(enum tmio_dev_id target);
card_cid *tmio_get_cid(enum tmio_dev_id target);
uint32_t tmio_get_size(enum tmio_dev_id target);
uint32_t tmio_readsectors(enum tmio_dev_id target, uint32_t sector_no, uint_fast16_t numsectors, uint8_t *out);
uint32_t tmio_writesectors(enum tmio_dev_id target, uint32_t sector_no, uint_fast16_t numsectors, uint8_t *in);

#define TMIO_BBS 512

#endif

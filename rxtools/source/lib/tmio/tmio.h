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
	card_cid cid;
	card_csd csd;
	uint8_t as8[16];
	uint32_t as32[4];
} tmio_response;

typedef struct {
	uint32_t initarg;
	uint32_t isSDHC;
	uint32_t clk;
	uint32_t SDOPT;
	card_cid cid;
	card_csd csd;
} tmio_device;

#define REG_TMIO_RESP	((volatile sd_cid*)0x1000600C)

void tmio_init(void);
uint32_t tmio_init_sdmc(void);
uint32_t tmio_init_nand(void);

card_cid *tmio_get_cid(enum tmio_dev_id target);
uint32_t tmio_get_size(enum tmio_dev_id target);
card_csd *tmio_get_csd(enum tmio_dev_id target);

uint32_t tmio_readsectors(enum tmio_dev_id target,
	uint32_t sector_no, uint32_t numsectors, uint8_t *out);

uint32_t tmio_writesectors(enum tmio_dev_id target,
	uint32_t sector_no, uint32_t numsectors, uint8_t *in);

#define TMIO_BBS 512

#endif

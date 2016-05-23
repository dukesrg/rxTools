/*
 * Copyright (C) 2015-2016 The PASTA Team, dukesrg
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

#ifndef NAND_H
#define NAND_H

#include <stdint.h>
#include "aes.h"
#include "strings.h"

#define AGB_SAVE_MAGIC 'VAS.'
#define NAND_SECTOR_SIZE 0x200

typedef enum {
// NCSD partitions
	NAND_PARTITION_TWL = 0,
	NAND_PARTITION_AGB_SAVE = 1,
	NAND_PARTITION_FIRM0 = 2,
	NAND_PARTITION_FIRM1 = 3,
	NAND_PARTITION_CTR = 4,
	NAND_PARTITION_5 = 5,
	NAND_PARTITION_6 = 6,
	NAND_PARTITION_7 = 7,
// TWL partition logical partitions 0-3
	NAND_PARTITION_TWLN = 8,
	NAND_PARTITION_TWLP = 9,
	NAND_PARTITION_TWL2 = 10,
	NAND_PARTITION_TWL3 = 11,
// CTR partition logical partitions 0-3
	NAND_PARTITION_CTRNAND = 12,
	NAND_PARTITION_CTR1 = 13,
	NAND_PARTITION_CTR2 = 14,
	NAND_PARTITION_CTR3 = 15,
	NAND_PARTITION_COUNT
} nand_partition_index;

typedef enum {
	SYSNAND = 0,
	EMUNAND0 = 1,
	EMUNAND = EMUNAND0,
	EMUNAND1 = 2,
	EMUNAND2 = 3,
	NAND_COUNT
} nand_type;

typedef enum {
	NAND_FORMAT_PLAIN = 0,
	NAND_FORMAT_RED = NAND_FORMAT_PLAIN,
	NAND_FORMAT_GW = 1
} nand_format;

typedef struct {
	uint32_t first_sector;
	uint32_t sectors_count;
	uint_fast8_t keyslot;
} nand_partition_entry;

typedef struct {
	nand_partition_entry partition[NAND_PARTITION_COUNT];
	uint32_t first_sector;
	uint32_t sectors_count;
	nand_format format;
} nand_metrics;

static const char *const *const nand_names[] = {
	&S_SYSNAND,
	&S_EMUNAND,
	&S_EMUNAND1,
	&S_EMUNAND2
};
 
static const char *const *const nand_partition_names[] = {
	&S_TWL,
	&S_AGB_SAVE,
	&S_FIRM0,
	&S_FIRM1,
	&S_CTR,
	&S_NCSD5,
	&S_NCSD6,
	&S_NCSD7,
	&S_TWLN,
	&S_TWLP,
	&S_TWL2,
	&S_TWL3,
	&S_CTRNAND,
	&S_CTR1,
	&S_CTR2,
	&S_CTR3
};

extern int sysver;

uint32_t checkNAND(nand_type index);
uint32_t checkEmuNAND();
void GetNANDCTR(aes_ctr *ctr);
uint_fast8_t nandInit();
nand_partition_entry *GetNANDPartition(nand_type type, nand_partition_index partition);
nand_metrics *GetNANDMetrics(nand_type type);
void nand_readsectors(uint32_t sector_no, uint32_t numsectors, uint8_t *out, nand_type nidx, nand_partition_index pidx);
void nand_writesectors(uint32_t sector_no, uint32_t numsectors, uint8_t *out, nand_type nidx, nand_partition_index pidx);

#endif

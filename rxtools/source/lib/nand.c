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

#include <string.h>
#include "mpcore.h"
#include "CTRDecryptor.h"
#include "tmio/tmio.h"
#include "fatfs/diskio.h"
#include "bootsector.h"
#include "nand.h"
#include "ncsd.h"
#include "aes.h"

static nand_metrics nand[NAND_COUNT];
static aes_ctr NANDCTR;
int sysver;

#define FS_COUNTER_DISPLACEMENT32 0x0C
#define FS_COUNTER_MAGIC 0x0005C980
#define FS_COUNTER_MAGIC2 0x800005C9
#define FS_COUNTER_SCAN_START (uint32_t*)0x08000000
#define FS_COUNTER_SCAN_END (uint32_t*)0x080D8FFC

// Counter offsets for old 3DS
static const uint32_t *fsCountersCtr[] = {
	(uint32_t*)0x080D7CAC, //4.x
	(uint32_t*)0x080D858C, //5.x
	(uint32_t*)0x080D748C, //6.x
	(uint32_t*)0x080D740C, //7.x
	(uint32_t*)0x080D74CC, //8.x
	(uint32_t*)0x080D794C, //9.x
};

// The counter offset of 9.x on new 3DS
static const uint32_t *fsCounter9Ktr = (uint32_t*)0x080D8B8C;

static uint_fast8_t getLogicalPartitions(nand_type nidx, nand_partition_index partition) {
	mbr mbr_in;

	nand_readsectors(0, 1, (uint8_t*)&mbr_in, nidx, partition);
	if (mbr_in.partition_table.marker != END_OF_SECTOR_MARKER) {
		for (size_t i = PARTITION_MAX_COUNT; i--; nand[nidx].partition[NCSD_PARTITION_COUNT + partition + i] = (nand_partition_entry){0});
		return 0;
	} else
		for (size_t i = PARTITION_MAX_COUNT; i--;) {
			nand[nidx].partition[NCSD_PARTITION_COUNT + partition + i].first_sector = nand[nidx].partition[partition].first_sector + mbr_in.partition_table.partition[i].relative_sectors;
			nand[nidx].partition[NCSD_PARTITION_COUNT + partition + i].sectors_count = mbr_in.partition_table.partition[i].total_sectors;
			nand[nidx].partition[NCSD_PARTITION_COUNT + partition + i].keyslot = nand[nidx].partition[partition].keyslot;
		}
	return 1;
}

static uint_fast8_t getPartitions(nand_type nidx, uint32_t first_sector, uint32_t sectors_count) {
	uint_fast8_t id = nidx == SYSNAND ? TMIO_DEV_NAND : TMIO_DEV_SDMC;
	ncsd_header ncsd;
	
	nand[nidx].sectors_count = 0;
	if (!tmio_readsectors(id, first_sector, 1, (uint8_t*)&ncsd) && ncsd.magic == NCSD_MAGIC) {
		nand[nidx].format = NAND_FORMAT_PLAIN;
		nand[nidx].first_sector = first_sector;
	} else if (!tmio_readsectors(id, first_sector + sectors_count - 1, 1, (uint8_t*)&ncsd) && ncsd.magic == NCSD_MAGIC) {
		nand[nidx].format = NAND_FORMAT_GW;
		nand[nidx].first_sector = first_sector - 1;
		nand[nidx].sectors_count = sectors_count;
	} else {
		for (size_t i = NCSD_PARTITION_COUNT; i--; nand[nidx].partition[i] = (nand_partition_entry){0});
		return 0;
	}
	for (size_t i = NCSD_PARTITION_COUNT; i--;) {
		nand[nidx].partition[i].first_sector = ncsd.partition[i].offset;
		nand[nidx].partition[i].sectors_count = ncsd.partition[i].size;
		nand[nidx].partition[i].keyslot = ncsd.partition_fs_type[i] + ncsd.partition_crypt_type[i] + 1;
		if (ncsd.partition[i].offset >= nand[nidx].sectors_count)
			nand[nidx].sectors_count = ncsd.partition[i].offset + ncsd.partition[i].size;
	}
	getLogicalPartitions(nidx, NAND_PARTITION_TWL);
	getLogicalPartitions(nidx, NAND_PARTITION_CTR);
	return 1;
}

uint_fast8_t nandInit() {
	mbr sd_mbr;
	aes_ctr_data *ctr = NULL;

	if (getMpInfo() == MPINFO_CTR) {
		for (size_t i = 0; !ctr && i < sizeof(fsCountersCtr) / sizeof(fsCountersCtr[0]); i++)
			if (*fsCountersCtr[i] == FS_COUNTER_MAGIC) {
				sysver = i + 3;
				ctr = (aes_ctr_data *)(fsCountersCtr[i] + FS_COUNTER_DISPLACEMENT32);
			}
	} else if (*fsCounter9Ktr == FS_COUNTER_MAGIC) {
		sysver = 9;
		ctr = (aes_ctr_data *)(fsCounter9Ktr + FS_COUNTER_DISPLACEMENT32);
	}
	for (uint32_t *ctrptr = FS_COUNTER_SCAN_END; !ctr && ctrptr > FS_COUNTER_SCAN_START; ctrptr--)
		if (*ctrptr == FS_COUNTER_MAGIC && *(ctrptr + 1) == FS_COUNTER_MAGIC2)
			ctr = (aes_ctr_data *)(ctrptr + FS_COUNTER_DISPLACEMENT32);

	if (!ctr)
		return 0;
	
	NANDCTR = (aes_ctr){*ctr, AES_CNT_INPUT_LE_REVERSE}; //Reverse LE is optimal for AES CTR mode

	tmio_init();
	tmio_init_nand();
	tmio_init_sdmc();

	if (!getPartitions(SYSNAND, 0, 0))
		return 0;

	tmio_readsectors(TMIO_DEV_SDMC, 0, 1, (uint8_t*)&sd_mbr);
	if (sd_mbr.partition_table.marker == END_OF_SECTOR_MARKER)
		for (size_t i = PARTITION_MAX_COUNT; --i;)
			if (sd_mbr.partition_table.partition[i].system_id == PARTITION_TYPE_3DS_NAND)
				getPartitions(i, sd_mbr.partition_table.partition[i].relative_sectors, sd_mbr.partition_table.partition[i].total_sectors);

	if (!nand[EMUNAND].sectors_count &&
		(getPartitions(EMUNAND, 1, nand[SYSNAND].sectors_count) || getPartitions(EMUNAND, 1, tmio_dev[TMIO_DEV_NAND].total_size)) &&
		sd_mbr.partition_table.marker == END_OF_SECTOR_MARKER &&
		(sd_mbr.partition_table.partition[1].system_id == PARTITION_TYPE_NONE || sd_mbr.partition_table.partition[1].system_id == PARTITION_TYPE_3DS_NAND)
	) {
		sd_mbr.partition_table.partition[1].system_id = PARTITION_TYPE_3DS_NAND;
		sd_mbr.partition_table.partition[1].relative_sectors = 1;
		sd_mbr.partition_table.partition[1].total_sectors = nand[EMUNAND].sectors_count;
		tmio_writesectors(TMIO_DEV_SDMC, 0, 1, (uint8_t*)&sd_mbr);
	}

	return 1;
}

uint32_t checkNAND(nand_type type) {
	return type < NAND_COUNT ? nand[type].sectors_count * NAND_SECTOR_SIZE : 0;
}

uint32_t checkEmuNAND() { //for compatibility
	return checkNAND(EMUNAND);
}

void GetNANDCTR(aes_ctr *ctr) {
	*ctr = NANDCTR;
}

nand_metrics *GetNANDMetrics(nand_type type) {
	return type < NAND_COUNT ? &nand[type] : NULL;
}

nand_partition_entry *GetNANDPartition(nand_type type, nand_partition_index partition) {
	return type < NAND_COUNT && partition < NAND_PARTITION_COUNT ? &nand[type].partition[partition] : NULL;
}

void nand_readsectors(uint32_t sector_no, uint32_t numsectors, uint8_t *out, nand_type nidx, nand_partition_index pidx) {
	aes_ctr ctr = NANDCTR;
	uint_fast8_t id = nidx == SYSNAND ? TMIO_DEV_NAND : TMIO_DEV_SDMC;

	sector_no += nand[nidx].partition[pidx].first_sector;
	aes_add_ctr(&ctr, sector_no * NAND_SECTOR_SIZE / AES_BLOCK_SIZE);
	aes_set_key(&(aes_key){NULL, 0, nand[nidx].partition[pidx].keyslot, 0});
	if (sector_no || nand[nidx].format != NAND_FORMAT_GW)
		tmio_readsectors(id, nand[nidx].first_sector + sector_no, numsectors, out);
	else {
		tmio_readsectors(id, nand[nidx].first_sector + nand[nidx].sectors_count, 1, out);
		if (numsectors > 1)
			tmio_readsectors(id, nand[nidx].first_sector + 1, numsectors - 1, out + NAND_SECTOR_SIZE);
	}
	if (nand[nidx].partition[pidx].keyslot > 3)
		aes(out, out, numsectors * NAND_SECTOR_SIZE, &ctr, AES_CTR_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);
	else
		aes(out, out, numsectors * NAND_SECTOR_SIZE, &ctr, AES_CTR_DECRYPT_MODE | AES_CNT_INPUT_LE_REVERSE | AES_CNT_OUTPUT_LE_REVERSE);
}

void nand_writesectors(uint32_t sector_no, uint32_t numsectors, uint8_t *out, nand_type nidx, nand_partition_index pidx) {
	aes_ctr ctr = NANDCTR;
	uint_fast8_t id = nidx == SYSNAND ? TMIO_DEV_NAND : TMIO_DEV_SDMC;

	sector_no += nand[nidx].partition[pidx].first_sector;
	aes_add_ctr(&ctr, sector_no * NAND_SECTOR_SIZE / AES_BLOCK_SIZE);
	aes_set_key(&(aes_key){NULL, 0, nand[nidx].partition[pidx].keyslot, 0});
	aes(out, out, numsectors * NAND_SECTOR_SIZE, &ctr, AES_CTR_ENCRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);
	if (sector_no || nand[nidx].format != NAND_FORMAT_GW)
		tmio_writesectors(id, nand[nidx].first_sector + sector_no, numsectors, out);
	else {
		tmio_writesectors(id, nand[nidx].first_sector + nand[nidx].sectors_count, 1, out);
		if (numsectors > 1)
			tmio_writesectors(id, nand[nidx].first_sector + 1, numsectors - 1, out + NAND_SECTOR_SIZE);
	}
}

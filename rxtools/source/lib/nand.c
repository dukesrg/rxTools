/*
 * Copyright (C) 2015 The PASTA Team
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
#include "mbr.h"
#include "nand.h"
#include "ncsd.h"
#include "aes.h"

typedef struct {
	nand_partition partition[NAND_PARTITION_COUNT];
	uint32_t first_sector;
	uint32_t sectors_count;
	nand_format format;
} nand_metrics;

static nand_metrics nand[NAND_COUNT];

static aes_ctr NANDCTR;
int sysver;

// Counter offsets for old 3DS
static const uintptr_t fsCountersCtr[] = {
	0x080D7CAC, //4.x
	0x080D858C, //5.x
	0x080D748C, //6.x
	0x080D740C, //7.x
	0x080D74CC, //8.x
	0x080D794C, //9.x
};

// The counter offset of 9.x on new 3DS
static const uintptr_t fsCounter9Ktr = 0x080D8B8C;

static inline uint_fast8_t getKeyslot(ncsd_header *ncsd, uint_fast8_t index) {
	return ncsd->partition_fs_type[index] + ncsd->partition_crypt_type[index] + 1;
}

static void getLogicalPartitions(nand_metrics *n, nand_partition_index partition, mbr *mbr_in) {
	aes_ctr ctr = NANDCTR;
	aes_add_ctr(&ctr, n->partition[partition].first_sector * NAND_SECTOR_SIZE / AES_BLOCK_SIZE);
	aes_set_key(&(aes_key){NULL, 0, n->partition[partition].keyslot, 0});
	aes(mbr_in, mbr_in, sizeof(mbr), &ctr, AES_CTR_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);
	if (mbr_in->partition_table.magic != MBR_BOOT_MAGIC)
		for (size_t i = MBR_PARTITION_COUNT; i--; n->partition[NCSD_PARTITION_COUNT + partition + i] = (nand_partition){0});
	else
		for (size_t i = MBR_PARTITION_COUNT; i--;) {
			n->partition[NCSD_PARTITION_COUNT + partition + i].first_sector = n->partition[partition].first_sector + mbr_in->partition_table.partition[i].lba_first_sector;
			n->partition[NCSD_PARTITION_COUNT + partition + i].sectors_count = mbr_in->partition_table.partition[i].lba_sectors_count;
			n->partition[NCSD_PARTITION_COUNT + partition + i].keyslot = n->partition[partition].keyslot;
		}
}

static void getPartitions(nand_metrics *n, ncsd_header *ncsd, mbr *ctr_mbr) {
	n->sectors_count = 0;
	for (size_t i = NCSD_PARTITION_COUNT; i--;) {
		n->partition[i].first_sector = ncsd->partition[i].offset;
		n->partition[i].sectors_count = ncsd->partition[i].size;
		n->partition[i].keyslot = ncsd->partition_fs_type[i] + ncsd->partition_crypt_type[i] + 1;
		if (ncsd->partition[i].offset >= n->sectors_count)
			n->sectors_count = ncsd->partition[i].offset + ncsd->partition[i].size;
	}
	getLogicalPartitions(n, NAND_PARTITION_TWL, (mbr*)ncsd);
	getLogicalPartitions(n, NAND_PARTITION_CTR, ctr_mbr);
}
	
static void detectNAND(nand_metrics *n, uint32_t first_sector, uint32_t sectors_count) {
	ncsd_header ncsd;
	mbr ctr_mbr;
	tmio_readsectors(TMIO_DEV_SDMC, first_sector, 1, (uint8_t*)&ncsd);
	if (ncsd.magic != NCSD_MAGIC) {
		tmio_readsectors(TMIO_DEV_SDMC, first_sector + sectors_count - 1, 1,(uint8_t*)&ncsd);
		if (ncsd.magic == NCSD_MAGIC) {
			n->format = NAND_FORMAT_GW;
			n->first_sector = first_sector - 1;
		}
	} else {
		n->format = NAND_FORMAT_RED;
		n->first_sector = first_sector;
	}
	if (ncsd.magic == NCSD_MAGIC) {
		tmio_readsectors(TMIO_DEV_SDMC, n->first_sector + ncsd.partition[NAND_PARTITION_CTR].offset, 1, (uint8_t*)&ctr_mbr);
		getPartitions(n, &ncsd, &ctr_mbr);
	} else
		n->sectors_count = 0;
}

uint_fast8_t nandInit() {
	ncsd_header ncsd;
	mbr ctr_mbr;

	disk_initialize(DRV_SDMC);
	disk_initialize(DRV_NAND);

	nand[SYSNAND].format = NAND_FORMAT_PLAIN;
	nand[SYSNAND].first_sector = 0;
	tmio_readsectors(TMIO_DEV_NAND, nand[SYSNAND].first_sector, 1, (uint8_t*)&ncsd);
	if (ncsd.magic != 'DSCN')
		return 0;

	tmio_readsectors(TMIO_DEV_NAND, nand[SYSNAND].first_sector + ncsd.partition[NAND_PARTITION_CTR].offset, 1, (uint8_t*)&ctr_mbr);
	getPartitions(&nand[SYSNAND], &ncsd, &ctr_mbr);

	mbr sd_mbr;
	tmio_readsectors(TMIO_DEV_SDMC, 0, 1, (uint8_t*)&sd_mbr);
	if (sd_mbr.partition_table.magic == MBR_BOOT_MAGIC) {
		for (size_t i = MBR_PARTITION_COUNT; --i;) {
			nand[i].sectors_count = 0;
			if (sd_mbr.partition_table.partition[i].type == MBR_PARTITION_TYPE_3DS_NAND)
				detectNAND(&nand[i], sd_mbr.partition_table.partition[i].lba_first_sector, sd_mbr.partition_table.partition[i].lba_sectors_count);
		}
	}

	if (!nand[EMUNAND].sectors_count) {
		detectNAND(&nand[EMUNAND], 1, nand[SYSNAND].sectors_count);
		if (nand[EMUNAND].sectors_count && sd_mbr.partition_table.partition[1].type == MBR_PARTITION_TYPE_NONE && sd_mbr.partition_table.magic == MBR_BOOT_MAGIC) {
			sd_mbr.partition_table.partition[1].type = MBR_PARTITION_TYPE_3DS_NAND;
			sd_mbr.partition_table.partition[1].lba_first_sector = 1;
			sd_mbr.partition_table.partition[1].lba_sectors_count = nand[EMUNAND].sectors_count;
			tmio_writesectors(TMIO_DEV_SDMC, 0, 1, (uint8_t*)&sd_mbr);
		}
	}

	return 1;
}

static void *findCounter(void)
{
	uint32_t i;

	if (getMpInfo() == MPINFO_KTR) {
		if (*(uint32_t *)fsCounter9Ktr == 0x5C980) {
			sysver = 9;
			return (void *)(fsCounter9Ktr + 0x30);
		}
	} else {
		for (i = 0; i < sizeof(fsCountersCtr) / sizeof(uintptr_t); i++)
			if (*(uint32_t *)fsCountersCtr[i] == 0x5C980) {
				sysver = i + 3;
				return (void *)(fsCountersCtr[i] + 0x30);
			}
	}

	// If value not in previous list start memory scanning (test range)
	for (i = 0x080D8FFC; i > 0x08000000; i -= sizeof(uint32_t))
		if (((uint32_t *)i)[0] == 0x5C980 && ((uint32_t *)i)[1] == 0x800005C9)
			return (void *)(i + 0x30);

	return NULL;
}

/**Copy NAND Cypto to our region.*/
void FSNandInitCrypto(void) {
	aes_ctr_data *ctr = findCounter();
	if (ctr)
		NANDCTR = (aes_ctr){*ctr, AES_CNT_INPUT_LE_REVERSE}; //Reverse LE is optimal for AES CTR mode
}

uint32_t checkNAND(nand_type index) {
	return nand[index].sectors_count * NAND_SECTOR_SIZE;
}

uint32_t checkEmuNAND() { //for compatibility
	return checkNAND(EMUNAND);
}

void GetNANDCTR(aes_ctr *ctr) {
	*ctr = NANDCTR;
}

nand_partition *GetNANDPartition(nand_type type, nand_partition_index partition) {
	return &nand[type].partition[partition];
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
		tmio_readsectors(id, nand[nidx].first_sector + sector_no + 1, numsectors - 1, out + NAND_SECTOR_SIZE);
	}
	aes(out, out, numsectors * NAND_SECTOR_SIZE, &ctr, AES_CTR_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);
}

void nand_writesectors(uint32_t sector_no, uint32_t numsectors, uint8_t *out, nand_type nidx, nand_partition_index pidx) {
	aes_ctr ctr = NANDCTR;
	uint_fast8_t id = nidx == SYSNAND ? TMIO_DEV_NAND : TMIO_DEV_SDMC;

	sector_no += nand[nidx].partition[pidx].first_sector;
	aes_add_ctr(&ctr, sector_no * NAND_SECTOR_SIZE / AES_BLOCK_SIZE);
	aes_set_key(&(aes_key){NULL, 0, nand[nidx].partition[pidx].keyslot, 0});
	aes(out, out, numsectors * NAND_SECTOR_SIZE, &ctr, AES_CTR_ENCRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);
/* NAND write disabled for debug
	if (sector_no || nand[nidx].format != NAND_FORMAT_GW)
		tmio_writesectors(id, nand[nidx].first_sector + sector_no, numsectors, out);
	else {
		tmio_writesectors(id, nand[nidx].first_sector + nand[nidx].sectors_count, 1, out);
		tmio_writesectors(id, nand[nidx].first_sector + sector_no + 1, numsectors - 1, out + NAND_SECTOR_SIZE);
	}
*/
}
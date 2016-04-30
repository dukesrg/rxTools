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
#include "nand.h"
#include "aes.h"

#define NAND_SECTOR_SIZE 0x200

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
//	uint32_t *ctr32 = (uint32_t*)findCounter();
//	if (ctr32)
//		NANDCTR.mode = AES_CNT_INPUT_BE_NORMAL;
//		for (size_t i = 4; i--; NANDCTR.ctr.data32[i] = __builtin_bswap32(*ctr32++));
//		NANDCTR.mode = AES_CNT_INPUT_BE_REVERSE;
//		for (size_t i = 4; i--; NANDCTR.data32[3-i] = __builtin_bswap32(*ctr32++));
//		NANDCTR.mode = AES_CNT_INPUT_LE_NORMAL;
//		for (size_t i = 4; i--; NANDCTR.data32[i] = *ctr32++);

}

unsigned int checkEmuNAND() {
	uint32_t addr[] = {0x3AF00000, 0x4D800000, 0x3BA00000, 0x76000000}; //CTR,KTR,CTR,KTR
	uint8_t *check = (uint8_t *)0x26000000;
	for(int i = getMpInfo() == MPINFO_KTR; i < sizeof(addr)/sizeof(addr[0]); i += 2) {
		tmio_readsectors(TMIO_DEV_SDMC, addr[i] >> 9, 1, check);
		if (*(uint32_t*)(check + 0x100) == 'DSCN') //NCSD
			return addr[i];
	}
	return 0;
}

void GetNANDCTR(aes_ctr *ctr) {
	*ctr = NANDCTR;
}

void nand_readsectors(uint32_t sector_no, uint32_t numsectors, uint8_t *out, nand_partition partition) {
	aes_ctr ctr = NANDCTR;
	aes_key key = {0};
	switch (partition) {
		case TWLN: case TWLP:
			key.slot = 0x3;
			break;
		case AGB_SAVE:
			key.slot = 0x7;
			break;
		case FIRM0: case FIRM1:
			key.slot = 0x6;
			break;
		case CTRNAND:
			if (getMpInfo() == MPINFO_CTR) {
				key.slot = 0x4;
				break;
			}
			partition = KTR_CTRNAND;
		case KTR_CTRNAND:
			key.slot = 0x5;
			break;
	}
	tmio_readsectors(TMIO_DEV_NAND, sector_no + partition / NAND_SECTOR_SIZE, numsectors, out);
	aes_set_key(&key);
	aes_add_ctr(&ctr, (sector_no * NAND_SECTOR_SIZE + partition) / AES_BLOCK_SIZE);
	aes(out, out, numsectors * NAND_SECTOR_SIZE, &ctr, AES_CTR_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);
}

void nand_writesectors(uint32_t sector_no, uint32_t numsectors, uint8_t *out, nand_partition partition) {
	aes_ctr ctr = NANDCTR;
	aes_key key = {0};
	switch (partition) {
		case TWLN: case TWLP:
			key.slot = 0x3;
			break;
		case AGB_SAVE:
			key.slot = 0x7;
			break;
		case FIRM0: case FIRM1:
			key.slot = 0x6;
			break;
		case CTRNAND:
			if (getMpInfo() == MPINFO_CTR) {
				key.slot = 0x4;
				break;
			}
			partition = KTR_CTRNAND;
		case KTR_CTRNAND:
			key.slot = 0x5;
			break;
	}
	aes_set_key(&key);
	aes_add_ctr(&ctr, (sector_no * NAND_SECTOR_SIZE + partition) / AES_BLOCK_SIZE);
	aes(out, out, numsectors * NAND_SECTOR_SIZE, &ctr, AES_CTR_ENCRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);
//	tmio_writesectors(TMIO_DEV_NAND, sector_no + partition / NAND_SECTOR_SIZE, numsectors, out);	//Stubbed, i don't wanna risk
}

void emunand_readsectors(uint32_t sector_no, uint32_t numsectors, uint8_t *out, nand_partition partition) {
	aes_ctr ctr = NANDCTR;
	aes_key key = {0};
	switch (partition) {
		case TWLN: case TWLP:
			key.slot = 0x3;
			break;
		case AGB_SAVE:
			key.slot = 0x7;
			break;
		case FIRM0: case FIRM1:
			key.slot = 0x6;
			break;
		case CTRNAND:
			if (getMpInfo() == MPINFO_CTR) {
				key.slot = 0x4;
				break;
			}
			partition = KTR_CTRNAND;
		case KTR_CTRNAND:
			key.slot = 0x5;
			break;
	}
	tmio_readsectors(TMIO_DEV_SDMC, sector_no + partition / NAND_SECTOR_SIZE, numsectors, out);
	aes_set_key(&key);
	aes_add_ctr(&ctr, (sector_no * NAND_SECTOR_SIZE + partition) / AES_BLOCK_SIZE);
	aes(out, out, numsectors * NAND_SECTOR_SIZE, &ctr, AES_CTR_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);
}

void emunand_writesectors(uint32_t sector_no, uint32_t numsectors, uint8_t *out, nand_partition partition) {
	aes_ctr ctr = NANDCTR;
	aes_key key = {0};
	switch (partition) {
		case TWLN: case TWLP:
			key.slot = 0x3;
			break;
		case AGB_SAVE:
			key.slot = 0x7;
			break;
		case FIRM0: case FIRM1:
			key.slot = 0x6;
			break;
		case CTRNAND:
			if (getMpInfo() == MPINFO_CTR) {
				key.slot = 0x4;
				break;
			}
			partition = KTR_CTRNAND;
		case KTR_CTRNAND:
			key.slot = 0x5;
			break;
	}
	aes_set_key(&key);
	aes_add_ctr(&ctr, (sector_no * NAND_SECTOR_SIZE + partition) / AES_BLOCK_SIZE);
	aes(out, out, numsectors * NAND_SECTOR_SIZE, &ctr, AES_CTR_ENCRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);
//	tmio_writesectors(TMIO_DEV_SDMC, sector_no + partition / NAND_SECTOR_SIZE, numsectors, out);	//Stubbed, i don't wanna risk
}
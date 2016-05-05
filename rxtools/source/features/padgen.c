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

#include <stdlib.h>
#include "fs.h"
#include "hid.h"
#include "lang.h"
#include "padgen.h"
#include "aes.h"
#include "progress.h"
#include "strings.h"
#include "CTRDecryptor.h"

#define MOVABLE_SEED_SIZE 0x120
#include "draw.h"

static uint_fast8_t NcchPadgen(NcchInfo *info) {
	uint32_t size = 0;
	aes_ctr ctr = {(aes_ctr_data){{0}}, AES_CNT_INPUT_BE_NORMAL};
	aes_key key = {NULL, AES_CNT_INPUT_BE_NORMAL, 0, KEYY};
	size_t i;
	
	if (!info->n_entries ||
		info->n_entries > MAX_PAD_ENTRIES ||
		info->ncch_info_version != 0xF0000003
	) return 0;
	
	for (i = info->n_entries; i--; size += info->entries[i].size_mb);
	statusInit(size, lang(SF_GENERATING), lang(S_NCCH_XORPAD));
	for (i = info->n_entries; i--;) {
		if (info->entries[i].uses7xCrypto >> 8 == 0xDEC0DE) // magic value to manually specify keyslot
			key.slot = info->entries[i].uses7xCrypto & 0x3F;
		else if (info->entries[i].uses7xCrypto == 0xA) // won't work on an Old 3DS
			key.slot = 0x18;
		else if (info->entries[i].uses7xCrypto)
			key.slot = 0x25;
		else
			key.slot = 0x2C;
		key.data = (aes_key_data*)&info->entries[i].keyY;
		ctr.data = *(aes_ctr_data*)&info->entries[i].CTR;
		if (!CreatePad(&ctr, &key, info->entries[i].size_mb, info->entries[i].filename, i))
			return 0;
	}

	return 1;
}

static uint_fast8_t SdPadgen(SdInfo *info) {
	File pf;
	const wchar_t *filename;
	const wchar_t *filenames[] = {
		L"0:movable.sed",
		L"2:private/movable.sed",
		L"1:private/movable.sed",
		0
	};
	const wchar_t **pfilename;
	union {
		uint8_t data[MOVABLE_SEED_SIZE];
		uint32_t magic;
	} movable_seed = {0};
	uint32_t size = 0;
	size_t i;
	aes_key Key = {(aes_key_data*)&movable_seed.data[0x110], AES_CNT_INPUT_BE_NORMAL, 0x34, KEYY};

	if (!info->n_entries ||
		info->n_entries > MAX_PAD_ENTRIES
	) return 0;

	for (pfilename = filenames; *pfilename; pfilename++) {
		filename = *pfilename;
		// Load console 0x34 keyY from movable.sed if present on SD card
		if (FileOpen(&pf, filename, 0)) {
			if ((FileRead2(&pf, &movable_seed, MOVABLE_SEED_SIZE) != MOVABLE_SEED_SIZE ||
				movable_seed.magic != 'DEES') &&
				(FileClose(&pf) || 1)
			) return 0;
			FileClose(&pf);
			aes_set_key(&Key);
			break;
		}
	}
	Key.data = NULL;
	for (i = info->n_entries; i--; size += info->entries[i].size_mb);
	statusInit(size, lang(SF_GENERATING), lang(S_SD_XORPAD));
	for (i = info->n_entries; i--;)
		if (!CreatePad(&(aes_ctr){*(aes_ctr_data*)&info->entries[i].CTR, AES_CNT_INPUT_BE_NORMAL}, &Key, info->entries[i].size_mb, info->entries[i].filename, i))
			return 0;

	return 1;
}

uint_fast8_t PadGen(wchar_t *filename) {
	File pf;
	union {
		SdInfo sd;
		NcchInfo ncch;
	} info;
	
	if (!filename || !FileOpen(&pf, filename, 0) || (
		FileRead2(&pf, &info, pf.fsize) != pf.fsize &&
		(FileClose(&pf) || 1)
	)) return 0;

	FileClose(&pf);
	return info.ncch.padding == 0xFFFFFFFF ? NcchPadgen(&info.ncch) : SdPadgen(&info.sd);
}

uint_fast8_t CreatePad(aes_ctr *ctr, aes_key *key, uint32_t size_mb, const char *filename, int index) {
	File pf;
	size_t size = size_mb << 20;
	wchar_t fname[sizeof(((SdInfoEntry*)0)->filename)];

	if (mbstowcs(fname, filename, sizeof(((SdInfoEntry*)0)->filename)) == (size_t)-1 ||
		!FileOpen(&pf, wcsncmp(fname, L"sdmc:/", 6) == 0 ? fname + 6 : fname, 1)
	) return 0;

	aes_set_key(key);

	if (!decryptFile(&pf, NULL, size, 0, ctr, key, AES_CTR_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL)) {
		FileClose(&pf);
		return 0;
	}

	FileClose(&pf);
	return 1;
}
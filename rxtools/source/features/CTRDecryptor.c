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
#include <stdlib.h>
#include <stdio.h>
#include "CTRDecryptor.h"
#include "screenshot.h"
#include "fs.h"
#include "console.h"
#include "draw.h"
#include "hid.h"
#include "ncch.h"
#include "ncsd.h"
#include "aes.h"
#include "lang.h"
#include "menu.h"
#include "progress.h"
#include "strings.h"

#define BUF_SIZE 0x100000

typedef struct {
	char name[8];
	uint32_t offset;
	uint32_t size;
} __attribute__((packed)) exefs_file_header;

typedef struct {
	exefs_file_header file[10];
	uint8_t reserved1[0x20];
	uint8_t hash[10][0x20];
} __attribute__((packed)) exefs_header;

uint_fast8_t decryptFile(File *outfile, File *infile, size_t size, size_t offset, aes_ctr *ctr, aes_key *key, uint32_t mode) {
	uint8_t outbuf[BUF_SIZE];
	uint8_t *inbuf = infile ? outbuf : NULL;
	size_t size_todo = size;

	aes_set_key(key);
	while (size) {
		size_t blocksize = size < BUF_SIZE ? size : BUF_SIZE;
		if (infile)
			FileRead(infile, inbuf, blocksize, offset);
		aes(outbuf, inbuf, blocksize, ctr, mode);
		FileWrite(outfile, outbuf, blocksize, offset);
		size -= blocksize;
		offset += blocksize;
		if (!progressSetPos(size_todo - size))
			return 0;
	}
	progressPinOffset();
	return 1;
}

uint_fast8_t ProcessCTR(wchar_t *path){
	File myFile;
	if(FileOpen(&myFile, path, 0)){
		ConsoleInit();
		ConsoleSetTitle(strings[STR_DECRYPT], strings[STR_CTR]);
		uint32_t ncch_base;
		ctr_ncchheader NCCH;
		FileRead(&myFile, &NCCH, sizeof(NCCH), ncch_base = 0);
		if (NCCH.magic == NCSD_MAGIC)
			FileRead(&myFile, &NCCH, sizeof(NCCH), ncch_base = 0x4000);
		if (NCCH.magic != NCCH_MAGIC) {
			FileClose(&myFile);
			return 0;
		}
		if (!NCCH.cryptomethod && NCCH.flags7 & NCCHFLAG_NOCRYPTO) {
			FileClose(&myFile);
			return 0;
		}

		statusInit((NCCH.extendedheadersize + (NCCH.exefssize + NCCH.romfssize) * 0x200) >> 20, 0, lang(SF_DECRYPTING), NCCH.productcode);
		aes_ctr ctr;
		aes_key key = {(aes_key_data*)NCCH.signature, AES_CNT_INPUT_BE_NORMAL, 0x2C, KEYY};
		if (NCCH.extendedheadersize) {
			ncch_get_counter(&NCCH, &ctr, NCCHTYPE_EXHEADER);
			if (!decryptFile(&myFile, &myFile, sizeof(ctr_ncchexheader), ncch_base + sizeof(ctr_ncchheader), &ctr, &key, AES_CTR_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL))
				return 0;
		}
		if (NCCH.exefssize) {
			ncch_get_counter(&NCCH, &ctr, NCCHTYPE_EXEFS);
			size_t offset = 0;
			if (NCCH.cryptomethod) {
				exefs_header exefshdr;
				FileRead(&myFile, &exefshdr, sizeof(exefshdr), ncch_base + NCCH.exefsoffset * NCCH_MEDIA_UNIT_SIZE);
				aes(&exefshdr, &exefshdr, sizeof(exefshdr), &ctr, AES_CTR_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);
				FileWrite(&myFile, &exefshdr, sizeof(exefshdr), ncch_base + NCCH.exefsoffset * NCCH_MEDIA_UNIT_SIZE);
				offset += sizeof(exefshdr);
				if (!memcmp(exefshdr.file[0].name, ".code", 5)) {
					key.slot = 0x25;
					if (!decryptFile(&myFile, &myFile, exefshdr.file[0].size, ncch_base + NCCH.exefsoffset * NCCH_MEDIA_UNIT_SIZE + sizeof(exefshdr) + exefshdr.file[0].offset, &ctr, &key, AES_CTR_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL))
						return 0;
					aes_add_ctr(&ctr, (exefshdr.file[1].offset - exefshdr.file[0].offset - exefshdr.file[0].size) / AES_BLOCK_SIZE);
					key.slot = 0x2C;
					offset += exefshdr.file[1].offset;
				}
			}
			if (!decryptFile(&myFile, &myFile, NCCH.exefssize * NCCH_MEDIA_UNIT_SIZE - offset, ncch_base + NCCH.exefsoffset * NCCH_MEDIA_UNIT_SIZE + offset, &ctr, &key, AES_CTR_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL))
				return 0;
		}
		if (NCCH.romfssize) {
			ncch_get_counter(&NCCH, &ctr, NCCHTYPE_ROMFS);
			if (NCCH.cryptomethod)
				key.slot = 0x25;
			if (!decryptFile(&myFile, &myFile, NCCH.romfssize * NCCH_MEDIA_UNIT_SIZE, ncch_base + NCCH.romfsoffset * NCCH_MEDIA_UNIT_SIZE, &ctr, &key, AES_CTR_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL))
				return 0;
		}
		NCCH.flags7 |= NCCHFLAG_NOCRYPTO; //Disable encryption
		NCCH.cryptomethod = 0;  //Disable 7.XKey usage
		if (ncch_base == 0x4000) //Only for NCSD
			FileWrite(&myFile, ((uint8_t*)&NCCH) + sizeof(NCCH.signature), sizeof(NCCH) - sizeof(NCCH.signature), 0x1100);
		FileWrite(&myFile, &NCCH, sizeof(NCCH), ncch_base);
		FileClose(&myFile);
		return 1;
	}
	return 0;
}

int ExploreFolders(wchar_t* folder){
	int nfiles = 0;
	DIR myDir;
	FILINFO curInfo;
	memset(&myDir, 0, sizeof(DIR));
	memset(&curInfo, 0, sizeof(FILINFO));
	FILINFO *myInfo = &curInfo;

	myInfo->fname[0] = 'A';
	while(f_opendir(&myDir, folder) != FR_OK);
	for(int i = 0; myInfo->fname[0] != 0; i++){
		if( f_readdir(&myDir, myInfo)) break;
		if(myInfo->fname[0] == '.' || !wcscmp(myInfo->fname, L"NINTEN~1")) continue;

		wchar_t path[_MAX_LFN];
		swprintf(path, _MAX_LFN, L"%ls/%ls", folder, myInfo->fname);
		if(path[wcslen(path) - 1] == '/') break;

		if (myInfo->fattrib & AM_DIR)
			nfiles += ExploreFolders(path);
		else if(ProcessCTR(path))
			nfiles++;
	}
	f_closedir(&myDir);
	return nfiles;
}

void CTRDecryptor(){
	ConsoleInit();
	ConsoleSetTitle(strings[STR_DECRYPT], strings[STR_CTR]);
	ConsoleShow();

	int nfiles = ExploreFolders(L"");

	ConsoleInit();
	print(strings[STR_DECRYPTED], nfiles, strings[STR_FILES]);
	print(strings[STR_PRESS_BUTTON_ACTION], strings[STR_BUTTON_A], strings[STR_CONTINUE]);

	ConsoleShow();
	WaitForButton(keys[KEY_A].mask);
}

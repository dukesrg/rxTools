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
#include "crypto.h"
#include "lang.h"
#include "menu.h"

#define BUFFER_ADDR ((uint8_t*)0x21000000)
#define BLOCK_SIZE  (8*1024*1024)

char str[100];

uint32_t DecryptPartition(PartitionInfo* info){
	if(info->keyY != NULL)
		setup_aeskey(info->keyslot, AES_BIG_INPUT|AES_NORMAL_INPUT, info->keyY);
	use_aeskey(info->keyslot);

	aes_ctr ctr __attribute__((aligned(32))) = *info->ctr;

	uint32_t size_bytes = info->size;
	for (uint32_t i = 0; i < size_bytes; i += BLOCK_SIZE) {
		for (uint32_t j = 0; (j < BLOCK_SIZE) && (i+j < size_bytes); j+= 16) {
			set_ctr(AES_BIG_INPUT|AES_NORMAL_INPUT, &ctr);
			aes_decrypt((void*)info->buffer+j, (void*)info->buffer+j, 1, AES_CTR_MODE);
			add_ctr(&ctr, 1);
			TryScreenShot(); //Putting it here allows us to take screenshots at any decryption point, since everyting loops in this
		}
	}
	return 0;
}

void ProcessExeFS(PartitionInfo *info){ //We expect Exefs to take just a block. Why? No exefs right now reached 8MB.
	if(info->keyslot == 0x2C){
		DecryptPartition(info);
	}else if(info->keyslot == 0x25){  //The new keyX is a bit tricky, 'couse only .code is encrypted with it
		PartitionInfo myInfo = *info;
		aes_ctr OriginalCTR = *myInfo.ctr;
		myInfo.keyslot = 0x2C;
		myInfo.size = 0x200;
		DecryptPartition(&myInfo);
		add_ctr(myInfo.ctr, 0x200 / 16);
		if(myInfo.buffer[0] == '.' && myInfo.buffer[1] == 'c' && myInfo.buffer[2] == 'o' && myInfo.buffer[3] == 'd' && myInfo.buffer[4] == 'e'){
			//The 7.xKey encrypted .code partition
			uint32_t codeSize = *((unsigned int*)(myInfo.buffer + 0x0C));
			uint32_t nextSection = *((unsigned int*)(myInfo.buffer + 0x18)) + 0x200;
			myInfo.buffer += 0x200;
			myInfo.size = codeSize;
			myInfo.keyslot = 0x25;
			DecryptPartition(&myInfo);
			//The rest is normally encrypted
			myInfo = *info;
			myInfo.buffer += nextSection;
			myInfo.size -= nextSection;
			myInfo.keyslot = 0x2C;
			*myInfo.ctr = OriginalCTR;
			add_ctr(myInfo.ctr, nextSection/16);
			DecryptPartition(&myInfo);
		}else{
			myInfo.size = info->size-0x200;
			myInfo.buffer += 0x200;
			DecryptPartition(&myInfo);
		}
	}
}

int ProcessCTR(wchar_t *path){
	PartitionInfo myInfo;
	File myFile;
	if(FileOpen(&myFile, path, false)){
		ConsoleInit();
		ConsoleSetTitle(strings[STR_DECRYPT], strings[STR_CTR]);
		uint32_t ncch_base;
		ctr_ncchheader NCCH;
		FileRead(&myFile, &NCCH, sizeof(NCCH), ncch_base = 0);
		if (NCCH.magic == 'DSCN') //NCSD
			FileRead(&myFile, &NCCH, sizeof(NCCH), ncch_base = 0x4000);
		if (NCCH.magic != 'HCCN') { //NCCH
			FileClose(&myFile);
			return 2;
		}

		print(L"%s\n", NCCH.productcode);
		if(NCCH.cryptomethod){
			print(strings[STR_CRYPTO_TYPE], strings[STR_KEY7]);
		}else if(!(NCCH.flags7 | NCCHFLAG_NOCRYPTO)){
			print(strings[STR_CRYPTO_TYPE], strings[STR_SECURE]);
		}else{
			print(strings[STR_CRYPTO_TYPE], strings[STR_NONE]);
			print(strings[STR_COMPLETED]);
			FileClose(&myFile);
			ConsoleShow();
			return 3;
		}

		aes_ctr CTR;
		if(NCCH.extendedheadersize){
			print(strings[STR_DECRYPTING], strings[STR_EXHEADER]);
			ConsoleShow();
			ncch_get_counter(&NCCH, &CTR, NCCHTYPE_EXHEADER);
			FileRead(&myFile, BUFFER_ADDR, sizeof(ctr_ncchexheader), ncch_base + sizeof(ctr_ncchheader));
			myInfo = (PartitionInfo){BUFFER_ADDR, NCCH.signature, &CTR, sizeof(ctr_ncchexheader), 0x2C};
			DecryptPartition(&myInfo);
			FileWrite(&myFile, BUFFER_ADDR, sizeof(ctr_ncchexheader), sizeof(ctr_ncchheader));
		}
		if(NCCH.exefssize){
			print(strings[STR_DECRYPTING], strings[STR_EXEFS]);
			ConsoleShow();
			ncch_get_counter(&NCCH, &CTR, NCCHTYPE_EXEFS);
			myInfo = (PartitionInfo){BUFFER_ADDR, NCCH.signature, &CTR, NCCH.exefssize * NCCH_MEDIA_UNIT_SIZE, NCCH.cryptomethod ? 0x25 : 0x2C};
			FileRead(&myFile, BUFFER_ADDR, myInfo.size, ncch_base + NCCH.exefsoffset * NCCH_MEDIA_UNIT_SIZE);
			ProcessExeFS(&myInfo); //Explanation at function definition
			FileWrite(&myFile, BUFFER_ADDR, NCCH.exefssize * NCCH_MEDIA_UNIT_SIZE, ncch_base + NCCH.exefsoffset * NCCH_MEDIA_UNIT_SIZE);
		}
		if(NCCH.romfssize){
			print(strings[STR_DECRYPTING], strings[STR_ROMFS]);
			ConsoleShow();
			ncch_get_counter(&NCCH, &CTR, NCCHTYPE_ROMFS);
			myInfo = (PartitionInfo){BUFFER_ADDR, NCCH.signature, &CTR, 0, NCCH.cryptomethod ? 0x25 : 0x2C};
			for(int i = 0; i < (NCCH.romfssize * NCCH_MEDIA_UNIT_SIZE + BLOCK_SIZE - 1) / BLOCK_SIZE; i++){
				print(L"%3d%%\b\b\b\b",
					(int)((i*BLOCK_SIZE)/(NCCH.romfssize * NCCH_MEDIA_UNIT_SIZE / 100)));
				myInfo.size = FileRead(&myFile, BUFFER_ADDR, i*BLOCK_SIZE <= (NCCH.romfssize * NCCH_MEDIA_UNIT_SIZE) ? BLOCK_SIZE : (NCCH.romfssize * NCCH_MEDIA_UNIT_SIZE) % BLOCK_SIZE, ncch_base + NCCH.romfsoffset * NCCH_MEDIA_UNIT_SIZE + i*BLOCK_SIZE);
				DecryptPartition(&myInfo);
				add_ctr(myInfo.ctr, myInfo.size/16);
				FileWrite(&myFile, BUFFER_ADDR, myInfo.size, ncch_base + NCCH.romfsoffset * NCCH_MEDIA_UNIT_SIZE + i*BLOCK_SIZE);
			}
			print(L"\n");
		}
		NCCH.flags7 |= NCCHFLAG_NOCRYPTO; //Disable encryption
		NCCH.cryptomethod = 0;  //Disable 7.XKey usage
		if (ncch_base == 0x4000) //Only for NCSD
			FileWrite(&myFile, ((uint8_t*)&NCCH) + sizeof(NCCH.signature), sizeof(NCCH) - sizeof(NCCH.signature), 0x1100);
		FileWrite(&myFile, &NCCH, sizeof(NCCH), ncch_base);
		FileClose(&myFile);
		print(strings[STR_COMPLETED]);
		ConsoleShow();
		return 0;
	}
	return 1;
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

		if(myInfo->fattrib & AM_DIR){
			nfiles += ExploreFolders(path);
		}else if(true){
			if(ProcessCTR(path) == 0){
				nfiles++;
			}
		}

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

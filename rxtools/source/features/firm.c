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
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <elf.h>
#include <reboot.h>
#include "firm.h"
#include "mpcore.h"
#include "hid.h"
#include "lang.h"
#include "console.h"
#include "fs.h"
#include "nand.h"
#include "ncch.h"
#include "draw.h"
#include "menu.h"
#include "fileexplorer.h"
#include "TitleKeyDecrypt.h"
#include "configuration.h"
#include "lang.h"
#include "aes.h"
#include "progress.h"

const wchar_t *firmPathFmt= L"" FIRM_PATH_FMT;
const wchar_t *firmPatchPathFmt = L"" FIRM_PATCH_PATH_FMT;

unsigned int emuNandMounted = 0;
_Noreturn void (* const _softreset)() = (void *)0x080F0000;

_Noreturn void execReboot(uint32_t, void *, uintptr_t, const Elf32_Shdr *);

static FRESULT loadExecReboot()
{
	FIL fd;
	FRESULT r;
	UINT br;

	r = f_open(&fd, L"" SYS_PATH "/reboot.bin", FA_READ);
	if (r != FR_OK)
		return r;

	r = f_read(&fd, (void*)0x080F0000, 0x8000, &br);
	if (r != FR_OK)
		return r;

	f_close(&fd);
	_softreset();
}

static int loadFirm(wchar_t *path, UINT *fsz)
{
	FIL fd;
	FRESULT r;

	r = f_open(&fd, path, FA_READ);
	if (r != FR_OK)
		return r;

	r = f_read(&fd, (void *)FIRM_ADDR, f_size(&fd), fsz);
	if (r != FR_OK)
		return r;

	f_close(&fd);

	return ((FirmHdr *)FIRM_ADDR)->magic == FIRM_MAGIC ? 0 : -1;
}

static int decryptFirmKtrArm9(void *p) {
//	uint8_t key[AES_BLOCK_SIZE];
	aes_key *key;
//	PartitionInfo info;
	Arm9Hdr *hdr;
	FirmSeg *seg, *btm;

	seg = ((FirmHdr *)p)->segs;
	for (btm = seg + FIRM_SEG_NUM; seg->isArm11; seg++)
		if (seg == btm)
			return 0;

	hdr = (void *)(p + seg->offset);

//	info.ctr = &hdr->ctr;
//	info.buffer = (uint8_t *)hdr + 0x800;
//	info.keyY = hdr->keyY;
//	info.size = atoi(hdr->size);

//	use_aeskey(0x11);
	aes_set_key(&(aes_key){NULL, 0, 0x11, 0});
	if (hdr->ext.pad[0] == 0xFFFFFFFF) {
//		info.keyslot = 0x15;
//		aes_decrypt(hdr->keyX, key, 1, AES_ECB_DECRYPT_MODE);
//		setup_aeskeyX(info.keyslot, key);
		key = &(aes_key){&(aes_key_data){{0}}, AES_CNT_INPUT_BE_NORMAL, 0x15, KEYX};
		aes((void*)key->data, (void*)&hdr->keyX, sizeof(aes_key_data), NULL, AES_ECB_DECRYPT_MODE);
		aes_set_key(key);
		key->data = &hdr->keyY;
		key->type = KEYY;
	} else {
//		info.keyslot = 0x16;
//		aes_decrypt(hdr->ext.s.keyX_0x16, key, 1, AES_ECB_DECRYPT_MODE);
		key = &(aes_key){&hdr->keyY, AES_CNT_INPUT_BE_NORMAL, 0x16, KEYY};
//		aes(key->data, hdr->ext.s.keyX_0x16, sizeof(aes_key_data), NULL, AES_ECB_DECRYPT_MODE);
	}

	aes_set_key(key);
//	return DecryptPartition(&info);
	aes_ctr ctr = {hdr->ctr, AES_CNT_INPUT_BE_NORMAL};
	aes((uint8_t *)hdr + 0x800, (uint8_t *)hdr + 0x800, strtoul(hdr->size, NULL, 10), &ctr, AES_CBC_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);

	return 1;
}

uint8_t *decryptFirmTitleNcch(uint8_t* title, size_t *size) {
	ctr_ncchheader *NCCH = ((ctr_ncchheader*)title);
	if (NCCH->magic != NCCH_MAGIC) return NULL;
	aes_ctr ctr;
	ncch_get_counter(NCCH, &ctr, NCCHTYPE_EXEFS);

	aes_set_key(&(aes_key){(aes_key_data*)NCCH->signature, AES_CNT_INPUT_BE_NORMAL, 0x2C, KEYY});
	aes(title + NCCH->exefsoffset * NCCH_MEDIA_UNIT_SIZE, title + NCCH->exefsoffset * NCCH_MEDIA_UNIT_SIZE, NCCH->exefssize * NCCH_MEDIA_UNIT_SIZE, &ctr, AES_CTR_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);

	if (size != NULL)
		*size = NCCH->exefssize * NCCH_MEDIA_UNIT_SIZE - sizeof(ctr_ncchheader);

	uint8_t *firm = (uint8_t*)(title + NCCH->exefsoffset * NCCH_MEDIA_UNIT_SIZE + sizeof(FirmHdr));

	if ((REG_CFG11_SOCINFO & CFG11_SOCINFO_KTR) && !decryptFirmKtrArm9(firm))
		return NULL;
	return firm;
}

static void setAgbBios()
{
	File agb_firm;
	wchar_t path[_MAX_LFN];
	unsigned char svc = (cfgs[CFG_AGB_BIOS].val.i ? 0x26 : 0x01);

	getFirmPath(path, TID_CTR_AGB_FIRM);
	if (FileOpen(&agb_firm, path, 0))
	{
		FileWrite(&agb_firm, &svc, 1, 0xD7A12);
		FileClose(&agb_firm);
	}
}

int rxMode(int_fast8_t drive)
{
	wchar_t path[64];
	const char *shstrtab;
	const wchar_t *msg;
	uint8_t keyx[16];
	uint32_t tid;
	int r, sector;
	Elf32_Ehdr *ehdr;
	Elf32_Shdr *shdr, *btm;
	void *keyxArg;
	FIL fd;
	UINT br, fsz;

	if (drive > 0) {
		sector = checkNAND(drive);
		if (sector == 0) {
			ConsoleInit();
			ConsoleSetTitle(L"EMUNAND NOT FOUND!");
			print(L"The emunand was not found on\n");
			print(L"your SDCard. \n");
			print(L"\n");
			print(L"Press A to boot SYSNAND\n");
			ConsoleShow();

			WaitForButton(keys[KEY_A].mask);

//			swprintf(path, _MAX_LFN, L"/rxTools/Theme/%u/boot.bin",
//				cfgs[CFG_THEME].val.i);
//			DrawSplash(&bottomScreen, path);
		}
	} else
		sector = 0;

	tid = (REG_CFG11_SOCINFO & CFG11_SOCINFO_KTR) ? TID_KTR_NATIVE_FIRM : TID_CTR_NATIVE_FIRM;

	setAgbBios();

	if (sysver < 7 && f_open(&fd, L"slot0x25KeyX.bin", FA_READ) == FR_OK) {
		f_read(&fd, keyx, sizeof(keyx), &br);
		f_close(&fd);
		keyxArg = keyx;
	} else
		keyxArg = NULL;

	getFirmPath(path, tid);
	r = loadFirm(path, &fsz);
	if (r) {
		msg = L"Failed to load NATIVE_FIRM: %d\n"
			L"Reboot rxTools and try again.\n";
		goto fail;
	}

	((FirmHdr *)FIRM_ADDR)->arm9Entry = 0x0801B01C;

	getFirmPatchPath(path, tid);
	r = f_open(&fd, path, FA_READ);
	if (r != FR_OK)
		goto patchFail;

	r = f_read(&fd, (void *)PATCH_ADDR, PATCH_SIZE, &br);
	if (r != FR_OK)
		goto patchFail;

	f_close(&fd);

	ehdr = (void *)PATCH_ADDR;
	shdr = (void *)(PATCH_ADDR + ehdr->e_shoff);
	shstrtab = (char *)PATCH_ADDR + shdr[ehdr->e_shstrndx].sh_offset;
	for (btm = shdr + ehdr->e_shnum; shdr != btm; shdr++) {
		if (!strcmp(shstrtab + shdr->sh_name, ".patch.p9.reboot.body")) {
			execReboot(sector, keyxArg, ehdr->e_entry, shdr);
			__builtin_unreachable();
		}
	}

	msg = L".patch.p9.reboot.body not found\n"
		L"Please check your installation.\n";
fail:
	ConsoleInit();
	ConsoleSetTitle(L"rxMode");
	print(msg, r);
	print(L"\n");
	print(strings[STR_PRESS_BUTTON_ACTION],
		strings[STR_BUTTON_A], strings[STR_CONTINUE]);
	ConsoleShow();
	WaitForButton(keys[KEY_A].mask);

	return r;

patchFail:
	msg = L"Failed to load the patch: %d\n"
		L"Check your installation.\n";
	goto fail;
}

//Just patches signatures check, loads in sysnand
#define PASTA_FIRM_SEEK_SIZE 0xF0000

int PastaMode() {
	/*PastaMode is ready for n3ds BUT there's an unresolved bug which affects nand reading functions, like nand_readsectors(0, 0xF0000 / 0x200, firm, FIRM0);*/

	uint8_t *firm = (void*)FIRM_ADDR;

	nand_readsectors(0, PASTA_FIRM_SEEK_SIZE / NAND_SECTOR_SIZE, firm, SYSNAND, NAND_PARTITION_FIRM0);
	if (*(uint32_t*)firm != FIRM_MAGIC)
		nand_readsectors(0, PASTA_FIRM_SEEK_SIZE / NAND_SECTOR_SIZE, firm, SYSNAND, NAND_PARTITION_FIRM1);

	if (REG_CFG11_SOCINFO & CFG11_SOCINFO_KTR) {
		//new 3ds patches
		decryptFirmKtrArm9((void *)FIRM_ADDR);
		uint8_t patch0[] = { 0x00, 0x20, 0x3B, 0xE0 };
		uint8_t patch1[] = { 0x00, 0x20, 0x08, 0xE0 };
		memcpy((uint32_t*)(FIRM_ADDR + 0xB39D8), patch0, sizeof(patch0));
		memcpy((uint32_t*)(FIRM_ADDR + 0xB9204), patch1, sizeof(patch1));
	} else {
		//o3ds patches
		uint8_t sign1[] = { 0xC1, 0x17, 0x49, 0x1C, 0x31, 0xD0, 0x68, 0x46, 0x01, 0x78, 0x40, 0x1C, 0x00, 0x29, 0x10, 0xD1 };
		uint8_t sign2[] = { 0xC0, 0x1C, 0x76, 0xE7, 0x20, 0x00, 0x74, 0xE7, 0x22, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F };
		uint8_t patch1[] = { 0x00, 0x20, 0x4E, 0xB0, 0x70, 0xBD };
		uint8_t patch2[] = { 0x00, 0x20 };

		for (size_t i = 0; i < PASTA_FIRM_SEEK_SIZE; i++) {
			if (!memcmp(firm + i, sign1, sizeof(sign1)))
				memcpy(firm + i, patch1, sizeof(patch1));
			if (!memcmp(firm + i, sign2, sizeof(sign2)))
				memcpy(firm + i, patch2, sizeof(patch2));
		}
	}

	return loadExecReboot();
}

void FirmLoader(wchar_t *firm_path){

	UINT fsz;
	if (loadFirm(firm_path, &fsz))
	{
		ConsoleInit();
		ConsoleSetTitle(strings[STR_LOAD], strings[STR_FIRMWARE_FILE]);
		print(strings[STR_WRONG], L"", strings[STR_FIRMWARE_FILE]);
		print(strings[STR_PRESS_BUTTON_ACTION], strings[STR_BUTTON_A], strings[STR_CONTINUE]);
		ConsoleShow();
		WaitForButton(keys[KEY_A].mask);
		return;
	}
		if (loadExecReboot())
	{
		ConsoleInit();
		ConsoleSetTitle(strings[STR_LOAD], strings[STR_FIRMWARE_FILE]);
		print(strings[STR_ERROR_LAUNCHING], strings[STR_FIRMWARE_FILE]);
		print(strings[STR_PRESS_BUTTON_ACTION], strings[STR_BUTTON_A], strings[STR_CONTINUE]);
		ConsoleShow();
		WaitForButton(keys[KEY_A].mask);
		return;
	}
}

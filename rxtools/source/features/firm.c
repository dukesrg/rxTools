/*
 * Copyright (C) 2015-2017 The PASTA Team, dukesrg
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
#include "tmd.h"
#include "ticket.h"
#include "signature.h"
#include "native_firm.h"

//FIRM processing additional job flags
typedef enum {
	FIRM_PATCH = 1<<0, //apply patches
	FIRM_COPY = 1<<1, //copy section to target address
	FIRM_SAVE = 1<<2 //save to file
} firm_operation;

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

static uint_fast8_t firmPatch(void *data) { //path FIRM sections
//	firm_section_header *section = ((firm_header*)data).sections;
//	for (size_t i = sizeof(firm_header->sections)/sizeof(firm_section_header); i--; section++) {
//todo: static firmware patching
//	}
	return 1;
}

static uint_fast8_t firmLoad(wchar_t *path) { //load FIRM file sections directly to target addresses
	File f;
	firm_header firm;
	firm_section_header *section = firm.sections;
	
	if (!(FileOpen(&f, path, 0) && (
		(FileRead2(&f, &firm, sizeof(firm)) == sizeof(firm) &&
			firm.magic == FIRM_MAGIC
		) || (FileClose(&f) && 0)
	))) return 0;

	for (size_t i = sizeof(firm_header->sections)/sizeof(firm_section_header); i--; section++)
		if (!((FileSeek(&f, section->offset) &&
				FileRead2(&f, section->load_address, section->size) == section->size
			) || (FileClose(&f) && 0)
		)) return 0;

	FileClose(&f);
	return 1;
}	

static uint_fast8_t firmCopy(void *data) { //copy FIRM sections to target addresses
	firm_section_header *section = ((firm_header*)data).sections;
	for (size_t i = sizeof(firm_header->sections)/sizeof(firm_section_header); i--; section++)
		memcpy(section->load_address, data + section->offset, section->size);
	return 1;
}

static uint_fast8_t firmSave(wchar_t *path, void *data, size_t size) { //decrypt FIRM title and save NCCH to file
	File f;
	return FileOpen(&f, path, 1) &&
		(FileWrite2(&f, data, size) == size || (FileClose(&f) && 0)) &&
		(FileClose(&f) || 1);
}

static uint_fast8_t processFirmFile(uint32_t title_id_lo, firm_operation operation) {
	static const wchar_t pathFmt[] = L"rxTools/firm/00040138%08lx%ls.bin";
	const uint64_t title_id = (uint64_t)__builtin_bswap32(title_id_lo) << 32 | 0x38010400;
	wchar_t path[_MAX_LFN + 1];
	void *data;
	size_t size;
	File f;

	if (swprintf(path, _MAX_LFN + 1, pathFmt, title_id_lo, L"") > 0 &&
		FileOpen(&f, path, 0) && (
			((size = FileSize(path)) &&
				(data = __builtin_alloca(size)) &&
				FileRead2(&f, data, size) == size
			) || (FileClose(&f) && 0)
		)
	) {
		FileClose(&f);
		aes_key Key = {&(aes_key_data){{0}}};
	
		swprintf(path, sizeof(path), pathFmt, title_id_lo, L"_cetk");
		uint_fast8_t drive = 1;
		uint_fast8_t maxdrive = 2; //todo get max NAND drive number
		if (!ticketGetKeyCetk(&Key, title_id, path)) //try with cetk
			for (uint_fast8_t drive = 0; drive <= 2 && !ticketGetKey2(&Key, title_id, drive); drive++); //try with title.db from all NAND drives
		if (drive <= maxdrive) {
			aes_set_key(&Key);
			aes(data, data, size, &(aes_ctr){{{0}}, AES_CNT_INPUT_BE_NORMAL}, AES_CBC_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);
			return (data = decryptFirmTitleNcch(data, &size))
				(!(operation & FIRM_PATCH) || firmPatch(data)) &&
				(!(operation & FIRM_COPY) || firmCopy(data)) &&
				(!(operation & FIRM_SAVE) || (getFirmPath(path, title_id_lo) && firmSave(path, data, size)));
		}
	}

	return 0;
}

static uint_fast8_t processFirmInstalled(uint32_t title_id_lo, firm_operation operation) {
	void *data;
	wchar_t path[_MAX_LFN + 1], apppath[_MAX_LFN + 1];
	File f;
	size_t size;
	tmd_data tmd;
	tmd_content_chunk content_chunk;

	for (size_t drive = 1; drive <= 2; drive++) //todo: max NAND drive
		if (swprintf(path, _MAX_LFN + 1, L"%u:title/00040138/%08x/content", drive, title_id_lo) > 0 &&
			tmdPreloadRecent(&tmd, path) != 0xFFFFFFFF &&
			FileOpen(&f, path, 0) && (
				(FileSeek(&f, signatureAdvance(tmd.sig_type) + sizeof(tmd.header) + sizeof(tmd.content_info)) &&
					FileRead2(&f, &content_chunk, sizeof(content_chunk)) == sizeof(content_chunk) //FIRM title have only one file, so just first chunk is needed
					) || (FileClose(&f) && 0) //close on fail
				) && (FileClose(&f) || 1) && //close on success
				wcscpy(wcsrchr(path, L'/'), L"/%08lx.app") && //make APP path
				swprintf(apppath, _MAX_LFN + 1, path, __builtin_bswap32(content_chunk.content_id)) > 0 &&
				FileOpen(&f, apppath, 0) && (
					((size = FileGetSize(&f)) &&
						(data = __builtin_alloca(size)) &&
						FileRead2(&f, data, size) == size
					) || (FileClose(&f) && 0)
				) && (FileClose(&f) || 1) &&
				(data = decryptFirmTitleNcch(data, &size)) &&
				(!(operation & FIRM_PATCH) || firmPatch(data)) &&
				(!(operation & FIRM_COPY) || firmCopy(data)) &&
				(!(operation & FIRM_SAVE) || (getFirmPath(path, title_id_lo) && firmSave(path, data, size)));
		) return 1;

	return 0;
}

static uint_fast8_t processFirm(uint32_t title_id_lo, firm_operation operation) {
	return processFirmFile(title_id_lo, operation) ||
		processFirmInstalled(title_id_lo, operation); 
}

int rxMode(int_fast8_t drive)
{
	wchar_t path[_MAX_LFN + 1];
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

	if (sysver < 7 && f_open(&fd, L"slot0x25KeyX.bin", FA_READ) == FR_OK) {
		f_read(&fd, keyx, sizeof(keyx), &br);
		f_close(&fd);
		keyxArg = keyx;
	} else
		keyxArg = NULL;

	if (REG_CFG11_SOCINFO & CFG11_SOCINFO_KTR) {
		tid = TID_KTR_NATIVE_FIRM;
	} else {
		getFirmPath(path, TID_CTR_TWL_FIRM);
		if (!FileExists(path)) {
			statusInit(1, 0, L"Decrypting TWL_FIRM");
			if (!processFirm(TID_CTR_TWL_FIRM, FIRM_PATCH | FIRM_SAVE)) {
				DrawInfo(NULL, lang(S_CONTINUE), lang("Error decrypting TWL_FIRM"));
				return 0;
			}
			progressSetPos(1);
		}
	
		getFirmPath(path, TID_CTR_AGB_FIRM);
		if (!FileExists(path)) {
			statusInit(1, 0, L"Decrypting AGB_FIRM");
			if (!processFirm(TID_CTR_AGB_FIRM, FIRM_PATCH | FIRM_SAVE)) {
				DrawInfo(NULL, lang(S_CONTINUE), lang("Error decrypting AGB_FIRM"));
				return 0;
			}
			progressSetPos(1);
			setAgbBios();
		}

		tid = TID_CTR_NATIVE_FIRM;
	}
	getFirmPath(path, tid);
	if (!firmLoad(path)) {
		statusInit(1, 0, L"Decrypting NATIVE_FIRM");
		if (!processFirm(tid, FIRM_PATCH | FIRM_SAVE | FIRM_COPY)) {
			DrawInfo(NULL, lang(S_CONTINUE), lang("Error decrypting NATIVE_FIRM"));
			return 0;
		}
		progressSetPos(1);
	}

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

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
 * You should have received a copy of the GNU General Public Licenselan
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include "menu.h"
#include "crypto.h"
#include "fs.h"
#include "console.h"
#include "draw.h"
#include "hid.h"
#include "firm.h"
#include "configuration.h"
#include "log.h"
#include "AdvancedFileManager.h"
#include "lang.h"
#include "theme.h"
#include "mpcore.h"
#include "strings.h"
#include "cfnt.h"

#include "ncch.h"
#include "romfs.h"
#include "progress.h"

#define BUF_SIZE 0x10000

static const wchar_t *const keyX16path = L"0:key_0x16.bin";
static const wchar_t *const keyX1Bpath = L"0:key_0x1B.bin";
static const wchar_t *const keyX25path = L"0:slot0x25KeyX.bin";
static const wchar_t *const rxRootPath = L"0:rxTools/%ls";
static const wchar_t *const menuPath = L"sys/gui.json";
static const wchar_t *const fontPath = L"data/cbf_std.bcfnt";

static const wchar_t *const jsonPattern = L"*.json";

static void setConsole() {
	ConsoleSetXY(15, 20);
	ConsoleSetWH(bottomScreen.w - 30, bottomScreen.h - 80);
	ConsoleSetBorderColor(BLUE);
	ConsoleSetTextColor(RGB(0, 141, 197));
	ConsoleSetBackgroundColor(TRANSPARENT);
	ConsoleSetSpecialColor(BLUE);
	ConsoleSetSpacing(2);
	ConsoleSetBorderWidth(3);
}

static inline size_t lz11Dec(uint8_t **dst, uint8_t *src) {
	uint8_t *origsrc = src;
	uint8_t flags = *src++;
	for (size_t i = 8; i--; flags <<= 1) {
		if (flags & 0x80) {
			uint32_t len = *src >> 4;
			switch (len) {
				case 1:
					len = (*src++ << 12 & 0xF000) + 0x100;
				case 0:
					len += *src++ << 4;
					len += (*src >> 4) + 0x10;
				default:
					len++;
			}
			uint32_t disp = *src++ << 8 & 0xF00;
			disp |= *src++;
			memcpy(*dst, *dst + ~disp, len);
			*dst += len;
		} else {
			*(*dst)++ = *src++;
		}
	}
	return src - origsrc;
}

static uint_fast8_t extractFont(wchar_t *dst, wchar_t *src) {
	File fd;
	ctr_ncchheader NCCH;
	uint32_t size;
	if (!(FileOpen(&fd, src, 0) && (
		(FileRead2(&fd, &NCCH, sizeof(NCCH)) == sizeof(NCCH) &&
		NCCH.magic == 'HCCN' &&
		FileSeek(&fd, NCCH.romfsoffset * NCCH_MEDIA_UNIT_SIZE) &&
		(size = NCCH.romfssize * NCCH_MEDIA_UNIT_SIZE)) ||
		(FileClose(&fd) && 0)
	))) return 0;

	uint8_t romfs[size];
	statusInit(size * 2, lang("Extracting font"));	
	
	uint32_t bytesread;
	uint8_t *buf = romfs;
	if (NCCH.cryptomethod || !(NCCH.flags7 & NCCHFLAG_NOCRYPTO)) {
		aes_ctr CTR;
		uint_fast8_t keyslot = NCCH.cryptomethod ? 0x25 : 0x2C;
		ncch_get_counter(&NCCH, &CTR, NCCHTYPE_ROMFS);
		while ((bytesread = FileRead2(&fd, buf, BUF_SIZE))) {
			setup_aeskey(keyslot, AES_BIG_INPUT | AES_NORMAL_INPUT, NCCH.signature);
			use_aeskey(keyslot);
			for (size_t i = 0; i < bytesread; i += AES_BLOCK_SIZE) {
				set_ctr(AES_BIG_INPUT | AES_NORMAL_INPUT, &CTR);
				aes_decrypt(buf + i, buf + i, 1, AES_CTR_MODE);
				add_ctr(&CTR, 1);
			}
			progressSetPos((buf += bytesread) - romfs);
		}
	} else {
		while ((bytesread = FileRead2(&fd, buf, BUF_SIZE)))
			progressSetPos((buf += bytesread) - romfs);
	}
	FileClose(&fd);

	ivfc_header *IVFC = (ivfc_header*)romfs;
	if (IVFC->magic != 'CFVI')
		return 0;
	
	romfs_header *header = (romfs_header*)((void*)IVFC + IVFC->l2_offset_lo);
	file_metadata *fmeta = (file_metadata*)((void*)header + header->file_metadata_offset);

	uint32_t progress = size;
	progressPinOffset();
	wchar_t path[_MAX_LFN + 1];
	wchar_t *lzext;
	wcscpy(path, dst);
	dst = path + wcslen(path);
	uint8_t *fontdata = (void*)header + header->file_data_offset + fmeta->file_data_offset;
	size_t writebytes;
	for (size_t i = 0; i <= fmeta->name_length; dst[i] = fmeta->name[i], i++);
	size = fmeta->file_data_length_lo;
	if ((lzext = wcsstr(path, L".lz"))) {
		*lzext = 0;		
		if (!FileOpen(&fd, path, 1))
			return 0;
		struct {
			uint32_t size;
			uint8_t data[0];
		} *lz11 = (void*)fontdata;
		uint8_t *savedbuf;
		savedbuf = buf = fontdata = __builtin_alloca((lz11->size >> 8) + 7);
		size -= sizeof(lz11->size);
		size_t decoded = 0;
		progressSetMax(progress + size);
		do {
			while ((decoded += lz11Dec(&buf, lz11->data + decoded)) < size && buf - savedbuf < BUF_SIZE);
			if (decoded < size)
				writebytes = buf - savedbuf - (buf - savedbuf) % BUF_SIZE;
			else 
				writebytes = (lz11->size >> 8) + fontdata - savedbuf;
			savedbuf += FileWrite2(&fd, savedbuf, writebytes);
			progressSetPos(decoded);
		} while (decoded < size);
	} else { //in case font file in RomFS is not packed
		if (!FileOpen(&fd, path, 1))
			return 0;
		buf = fontdata;
		progressSetMax(progress + size);
		while (size) {
			writebytes = FileWrite2(&fd, buf, size > BUF_SIZE ? BUF_SIZE : size);
			size -= writebytes;
			buf += writebytes;
			progressSetPos(buf - fontdata);
		}
	}
	
	FileClose(&fd);
	return 1;
}

uint_fast8_t initKeyX(uint_fast8_t slot, const wchar_t *path) {
	uint8_t buf[AES_BLOCK_SIZE];
	FIL f;

	if (!FileOpen(&f, path, 0) ||
		(FileRead2(&f, buf, AES_BLOCK_SIZE) != AES_BLOCK_SIZE &&
		(FileClose(&f) || 1)
	)) return 0;
	FileClose(&f);
	setup_aeskeyX(slot, buf);
	return 1;
}

static _Noreturn void mainLoop() {
	uint32_t pad;

	while (1) {
		pad = InputWait();
		if (pad & keys[KEY_DDOWN].mask)
			MenuNextSelection();

		if (pad & keys[KEY_DUP].mask)
			MenuPrevSelection();

		if (pad & keys[KEY_A].mask) {
			MenuSelect();
		}

		if (pad & keys[KEY_B].mask) {
			MenuClose();
		}

		MenuShow();
	}
}

__attribute__((section(".text.start"), noreturn)) void _start()
{

	wchar_t langDir[_MAX_LFN + 1];
	wchar_t themeDir[_MAX_LFN + 1];
	wchar_t path[_MAX_LFN + 1];
	size_t size;
	uint32_t pad;
	uint_fast8_t themeFailed = 0;

	// Enable TMIO IRQ
	*(volatile uint32_t *)0x10001000 = 0x00010000;

	preloadStringsA();

	// init screen buffer addresses from const vectors
	bottomScreen.addr = (uint8_t*)*(uint32_t*)bottomScreen.addr;
	top1Screen.addr = (uint8_t*)*(uint32_t*)top1Screen.addr;
	top2Screen.addr = (uint8_t*)*(uint32_t*)top2Screen.addr;

	if (!FSInit()) {
		DrawInfo(NULL, lang(S_REBOOT), lang(SF_FAILED_TO), lang(S_MOUNT), lang(S_FILE_SYSTEM));
		Shutdown(1);		
	}

	if (!(finf || (swprintf(path, _MAX_LFN + 1, rxRootPath, fontPath) > 0 &&
		(size = cfntPreload(path)) &&
		cfntLoad(__builtin_alloca(size), path, size)
	))) {
		*(wcsrchr(path, L'/') + 1) = 0;
		if (extractFont(path, L"1:title/0004009b/00014002/content/00000000.app") &&
			swprintf(path, _MAX_LFN + 1, rxRootPath, fontPath) > 0 &&
			(size = cfntPreload(path)) &&
			cfntLoad(__builtin_alloca(size), path, size)
		);
	}

	setConsole();

	readCfg();

	if (!(swprintf(langDir, _MAX_LFN + 1, rxRootPath, L"lang") > 0 &&
		(size = FileMaxSize(langDir, jsonPattern)) &&
		langInit(&(Json){__builtin_alloca(size), size, __builtin_alloca((size >> 2) * sizeof(jsmntok_t)), size >> 2}, langDir, jsonPattern) &&
		langLoad(cfgs[CFG_LANGUAGE].val.s, LANG_SET) &&
		swprintf(path, _MAX_LFN + 1, L"%ls/%s%ls", langDir, cfgs[CFG_LANGUAGE].val.s, wcsrchr(jsonPattern, L'.'))
	)) DrawInfo(NULL, lang(S_CONTINUE), lang(SF_FAILED_TO), lang(S_LOAD), path);
	
	swprintf(path, _MAX_LFN + 1, rxRootPath, menuPath);
	if (!(size = FileSize(path)) ||
		!menuLoad(&(Json){__builtin_alloca(size), size, __builtin_alloca((size >> 2) * sizeof(jsmntok_t)), size >> 2}, path)
	) DrawInfo(NULL, lang(S_CONTINUE), lang(SF_FAILED_TO), lang(S_LOAD), path);

	if (getMpInfo() != MPINFO_KTR || (
		(initKeyX(0x16, keyX16path) || (DrawInfo(lang(S_NO_KTR_KEYS), lang(S_CONTINUE), lang(SF_FAILED_TO), lang(S_LOAD), keyX16path) && 0)) &&
		(initKeyX(0x1B, keyX1Bpath) || (DrawInfo(lang(S_NO_KTR_KEYS), lang(S_CONTINUE), lang(SF_FAILED_TO), lang(S_LOAD), keyX1Bpath) && 0))
	)) {
		swprintf(path, _MAX_LFN + 1, rxRootPath, L"");
		f_mkdir(path);
		wcscat(path, L"nand");
		f_mkdir(path);
		InstallConfigData();
	}

	if (!(swprintf(themeDir, _MAX_LFN + 1, rxRootPath, L"theme") > 0 &&
		(size = FileMaxSize(themeDir, jsonPattern)) &&
		themeInit(&(Json){__builtin_alloca(size), size, __builtin_alloca((size >> 2) * sizeof(jsmntok_t)), size >> 2}, themeDir, jsonPattern) &&
		themeLoad(cfgs[CFG_THEME].val.s, THEME_SET) &&
		swprintf(path, _MAX_LFN + 1, L"%ls/%s%ls", themeDir, cfgs[CFG_THEME].val.s, wcsrchr(jsonPattern, L'.')) > 0) &&
		(themeFailed = 1)
	) DrawInfo(NULL, lang(S_CONTINUE), lang(SF_FAILED_TO), lang(S_LOAD), path);

	pad = GetInput();

	if (~pad & keys[cfgs[CFG_GUI_FORCE].val.i].mask) {
		if (pad & keys[cfgs[CFG_EMUNAND_FORCE].val.i].mask)
			rxMode(1);
		else if (pad & keys[cfgs[CFG_SYSNAND_FORCE].val.i].mask)
			rxMode(0);
		else if (pad & keys[cfgs[CFG_PASTA_FORCE].val.i].mask)
			PastaMode();
		else switch (cfgs[CFG_BOOT_DEFAULT].val.i) {
			case BOOT_EMUNAND:
				rxMode(1);
			case BOOT_SYSNAND:
				rxMode(0);
			case BOOT_PASTA:
				PastaMode();
		}
	}

	if (themeFailed) { //Fallback to Emu- or SysNAND if UI should be loaded but theme is not available
 		DrawInfo(NULL, lang(S_CONTINUE), lang(SF_FAILED_TO), lang(S_LOAD), path);
		rxMode(checkEmuNAND() ? 1 : 0);
	}

	if (sysver < 7 && !initKeyX(0x25, keyX25path))
		DrawInfo(lang(S_NO_KEYX25), lang(S_CONTINUE), lang(SF_FAILED_TO), lang(S_LOAD), keyX25path);

	MenuInit();
	MenuShow();
	mainLoop();
}
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
#include "screenshot.h"
#include "firm.h"
#include "configuration.h"
#include "log.h"
#include "AdvancedFileManager.h"
#include "lang.h"
#include "theme.h"
#include "mpcore.h"
#include "strings.h"

static const wchar_t *const keyX16path = L"0:key_0x16.bin";
static const wchar_t *const keyX1Bpath = L"0:key_0x1B.bin";
static const wchar_t *const keyX25path = L"0:slot0x25KeyX.bin";
static const wchar_t *const rxRootPath = L"0:rxTools";
static const wchar_t *const fontPath = L"%ls/sys/font.bin";
static const wchar_t *const langPath = L"%ls/lang/%s.json";

static int warned = 0;

static void setConsole() {
	ConsoleSetXY(15, 20);
	ConsoleSetWH(BOT_SCREEN_WIDTH - 30, SCREEN_HEIGHT - 80);
	ConsoleSetBorderColor(BLUE);
	ConsoleSetTextColor(RGB(0, 141, 197));
	ConsoleSetBackgroundColor(TRANSPARENT);
	ConsoleSetSpecialColor(BLUE);
	ConsoleSetSpacing(2);
	ConsoleSetBorderWidth(3);
}

static void install() {
	f_mkdir(L"rxTools");
	f_mkdir(L"rxTools/nand");
	InstallConfigData();
}

bool initKeyX(uint_fast8_t slot, const wchar_t *path) {
	uint8_t buf[AES_BLOCK_SIZE];
	FIL f;

	if (!FileOpen(&f, path, false) ||
		(FileRead2(&f, buf, AES_BLOCK_SIZE) != AES_BLOCK_SIZE &&
		(FileClose(&f) || true)
	)) return false;
	FileClose(&f);
	setup_aeskeyX(slot, buf);
	return true;
}

static _Noreturn void mainLoop() {
	uint32_t pad;

	while (true) {
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

static void warn(const wchar_t *format, ...)
{
	va_list va;

	if (!warned) {
		ConsoleInit();
		ConsoleSetTitle(strings[STR_WARNING]);
		warned = 1;
	}

	va_start(va, format);
	vprint(format, va);
	va_end(va);

	ConsoleShow();
}

__attribute__((section(".text.start"), noreturn)) void _start()
{
	wchar_t path[_MAX_LFN + 1];
	void *fontBuf;
	int r;
	FIL f;
	uint32_t pad;

	// Enable TMIO IRQ
	*(volatile uint32_t *)0x10001000 = 0x00010000;

	preloadStringsA();

	// init screen buffer addresses from const vectors
	bottomScreen.addr = (uint8_t*)*(uint32_t*)bottomScreen.addr;
	top1Screen.addr = (uint8_t*)*(uint32_t*)top1Screen.addr;
	top2Screen.addr = (uint8_t*)*(uint32_t*)top2Screen.addr;

	if (!FSInit()) {
		DrawInfo(NULL, lang(S_REBOOT, -1), lang(SF_FAILED_TO, -1), lang(S_MOUNT, -1), lang(S_FILE_SYSTEM, -1));
		Shutdown(true);		
	}

	setConsole();

	readCfg();

	swprintf(path, _MAX_LFN + 1, fontPath, rxRootPath);
	if (!FileOpen(&f, path, false) ||
		((fontBuf = __builtin_alloca(f.fsize)) &&
		FileRead2(&f, fontBuf, f.fsize) != f.fsize &&
		(FileClose(&f) || true)
	)) {
 		DrawInfo(NULL, lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), path);
		fontIsLoaded = 0;
	} else {
		FileClose(&f);
		font16.addr = fontBuf;
		font24 = (FontMetrics){12, 24, 24, font16.dwstart, font16.addr + font16.dw * font16.h * 0x10000 / (8 * sizeof(font24.addr[0]))};
		fontIsLoaded = 1;
		preloadStringsU();
	}
	
	r = loadStrings();
/*	
	swprintf(path, _MAX_LFN + 1, langPath, rxRootPath, cfgs[CFG_LANGUAGE].val.s);
	if (!FileOpen(&f, path, false) ||
		((fontBuf = __builtin_alloca(f.fsize)) &&
		FileRead2(&f, fontBuf, f.fsize) != f.fsize &&
		(FileClose(&f) || true)
	)) {
 		DrawInfo(NULL, lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), path);
*/		
	if (fontIsLoaded)
		r = langLoad(cfgs[CFG_LANGUAGE].val.s, LANG_SET);

	if (r < 0)
		warn(L"Failed to load strings: %d\n", r);
//DrawInfo(NULL, lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), keyX16)

	if ((r = menuLoad()) <= 0)
		warn(L"Failed to load gui: %d\n", r);

	if (getMpInfo() != MPINFO_KTR || (
		(initKeyX(0x16, keyX16path) || (DrawInfo(lang(S_NO_KTR_KEYS, -1), lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), keyX16path) && false)) &&
		(initKeyX(0x1B, keyX1Bpath) || (DrawInfo(lang(S_NO_KTR_KEYS, -1), lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), keyX1Bpath) && false))
	)) install();

	r = themeLoad(cfgs[CFG_THEME].val.s, THEME_SET);

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
	if (r < 0) { //Fallback to Emu- or SysNAND if UI should be loaded but theme is not available
 		DrawInfo(NULL, lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), path);
		rxMode(checkEmuNAND() ? 1 : 0);
	}

	if (sysver < 7 && !initKeyX(0x25, keyX25path))
		DrawInfo(lang(S_NO_KEYX25, -1), lang(S_CONTINUE, -1), lang(SF_FAILED_TO, -1), lang(S_LOAD, -1), keyX25path);

	MenuInit();
	MenuShow();
	mainLoop();
}

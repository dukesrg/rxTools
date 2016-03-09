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
#include "MainMenu.h"
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
#include "json.h"
#include "theme.h"
#include "mpcore.h"

#define FONT_NAME "font.bin"

static int warned = 0;

static void setConsole()
{
	ConsoleSetXY(15, 20);
	ConsoleSetWH(BOT_SCREEN_WIDTH - 30, SCREEN_HEIGHT - 80);
	ConsoleSetBorderColor(BLUE);
	ConsoleSetTextColor(RGB(0, 141, 197));
	ConsoleSetBackgroundColor(TRANSPARENT);
	ConsoleSetSpecialColor(BLUE);
	ConsoleSetSpacing(2);
	ConsoleSetBorderWidth(3);
}

static void install()
{
	f_mkdir(L"rxTools");
	f_mkdir(L"rxTools/nand");
	InstallConfigData();
}

static void drawTop()
{
/*	wchar_t str[_MAX_LFN];

	if (cfgs[CFG_3D].val.i) {
		swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/TOPL.bin", cfgs[CFG_THEME].val.i);
		DrawSplash(&top1Screen, str);
		swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/TOPR.bin", cfgs[CFG_THEME].val.i);
		DrawSplash(&top2Screen, str);
	} else {
		Screen tmpScreen = top1Screen;
		tmpScreen.addr = (uint8_t*)0x27000000;
		swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/TOP.bin", cfgs[CFG_THEME].val.i);
		DrawSplash(&tmpScreen, str);
		memcpy(top1Screen.addr, tmpScreen.addr, top1Screen.size);
		memcpy(top2Screen.addr, tmpScreen.addr, top2Screen.size);
	}
*/
}

static FRESULT initKeyX()
{
	uint8_t buff[AES_BLOCK_SIZE];
	UINT br;
	FRESULT r;
	FIL f;

	r = f_open(&f, L"slot0x25KeyX.bin", FA_READ);
	if (r != FR_OK)
		return r;

	r = f_read(&f, buff, sizeof(buff), &br);
	if (br < sizeof(buff))
		return r == FR_OK ? EOF : r;

	f_close(&f);
	setup_aeskeyX(0x25, buff);
	return 0;
}

static FRESULT initN3DSKeys()
{
    uint8_t buff[AES_BLOCK_SIZE];
	UINT br;
	FRESULT r;
	FIL f;

	r = f_open(&f, _T("key_0x16.bin"), FA_READ);
	if (r != FR_OK)
		return r;

	r = f_read(&f, buff, sizeof(buff), &br);
	if (br < sizeof(buff))
		return r == FR_OK ? EOF : r;

	f_close(&f);
	setup_aeskeyX(0x16, buff);

	r = f_open(&f, _T("key_0x1B.bin"), FA_READ);
	if (r != FR_OK)
		return r;

	r = f_read(&f, buff, sizeof(buff), &br);
	if (br < sizeof(buff))
		return r == FR_OK ? EOF : r;

	f_close(&f);
	setup_aeskeyX(0x1B, buff);
	return 0;
}

static _Noreturn void mainLoop()
{
	uint32_t pad;

	while (true) {
		pad = InputWait();
		if (pad & (BUTTON_DOWN | BUTTON_RIGHT | BUTTON_R1))
			MenuNextSelection();

		if (pad & (BUTTON_UP | BUTTON_LEFT | BUTTON_L1))
			MenuPrevSelection();

		if (pad & BUTTON_A) {
			OpenAnimation();
			MenuSelect();
		}

		if (pad & BUTTON_B) {
			MenuClose();
		}

		if (pad & BUTTON_SELECT) {
			fadeOut();
			ShutDown(1); //shutdown
		}
		if (pad & BUTTON_START) {
			fadeOut();
			ShutDown(0); //reboot
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
	static const wchar_t *fontPath= L"" SYS_PATH "/" FONT_NAME;
	void *fontBuf;
	UINT btr, br;
	int r;
	FIL f;

	// Enable TMIO IRQ
	*(volatile uint32_t *)0x10001000 = 0x00010000;

	preloadStringsA();

	// init screen buffer addresses from const vectors
	bottomScreen.addr = (uint8_t*)*(uint32_t*)bottomScreen.addr;
	top1Screen.addr = (uint8_t*)*(uint32_t*)top1Screen.addr;
	top2Screen.addr = (uint8_t*)*(uint32_t*)top2Screen.addr;

	if (!FSInit()) {
		DrawString(&bottomScreen, strings[STR_FAILED],
			BOT_SCREEN_WIDTH / 2, SCREEN_HEIGHT - font16.h, RED, BLACK);
		while (1);
	}

	/*
	set_loglevel(ll_info);
	log(ll_info, "Initializing rxTools...");
	*/

	setConsole();

	fontIsLoaded = 0;
	r = f_open(&f, fontPath, FA_READ);
	if (r == FR_OK) {
		btr = f_size(&f);
		fontBuf = __builtin_alloca(btr);
		r = f_read(&f, fontBuf, btr, &br);
		if (r == FR_OK)
			fontIsLoaded = 1;

		f_close(&f);
		font16.addr = fontBuf;
		font24.sw = 12;
		font24.h = 24;
		font24.dw = 24;
		font24.addr = font16.addr + font16.dw * font16.h * 0x10000 / (8 * sizeof(font24.addr[0]));
	}

	if (fontIsLoaded)
		preloadStringsU();
	else
		warn(L"Failed to load " FONT_NAME ": %d\n", r);

    if (getMpInfo() == MPINFO_KTR)
    {
        r = initN3DSKeys();
        if (r != FR_OK) {
            warn(L"Failed to load keys for N3DS\n"
            "  Code: %d\n"
            "  RxMode will not boot. Please\n"
            "  include key_0x16.bin and\n"
            "  key_0x1B.bin at the root of your\n"
            "  SD card.\n", r);
            InputWait();
            goto postinstall;
        }
    }

	install();
	postinstall:
	readCfg();

//	wchar_t path[_MAX_LFN];

	r = loadStrings();
	if (fontIsLoaded)
		r = langLoad(cfgs[CFG_LANG].val.s, LANG_SET);

	if (r < 0)
		warn(L"Failed to load strings: %d\n", r);

	if ((r = jsonLoad(&menuJson, menuPath)) <= 0)
		warn(L"Failed to load gui: %d\n", r);

	drawTop();

	r = themeLoad(cfgs[CFG_THEME].val.s, THEME_SET);

	if (cfgs[CFG_BOOT_DEFAULT].val.i != BOOT_UI && HID_STATE & keys[cfgs[CFG_FORCE_UI].val.i].mask) {
		if (cfgs[CFG_BOOT_DEFAULT].val.i == BOOT_EMUNAND || ~HID_STATE & keys[cfgs[CFG_FORCE_EMUNAND].val.i].mask)
			rxMode(1);
		else if (cfgs[CFG_BOOT_DEFAULT].val.i == BOOT_SYSNAND || ~HID_STATE & keys[cfgs[CFG_FORCE_SYSNAND].val.i].mask)
			rxMode(0);
		else if (cfgs[CFG_BOOT_DEFAULT].val.i == BOOT_PASTA || ~HID_STATE & keys[cfgs[CFG_FORCE_PASTA].val.i].mask)
			PastaMode();
	}
	if (r < 0) { //Fallback to Emu- or SysNAND if UI should be loaded but theme is not available
		warn(L"Failed to load theme: %d\n", r);
		rxMode(checkEmuNAND() ? 1 : 0);
	}

	if (sysver < 7) {
		r = initKeyX();
		if (r != FR_OK)
			warn(L"Failed to load key X for slot 0x25\n"
				"  Code: %d\n"
				"  If your firmware version is less\n"
				"  than 7.X, some titles decryption\n"
				"  will fail, and some EmuNANDs\n"
				"  will not boot.\n", r);
	}

	if (warned) {
		warn(strings[STR_PRESS_BUTTON_ACTION],
			strings[STR_BUTTON_A], strings[STR_CONTINUE]);
		WaitForButton(BUTTON_A);
	}

	OpenAnimation();
	MenuInit(&MainMenu);
	MenuShow();
	mainLoop();
}
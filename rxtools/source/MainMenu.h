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

#ifndef MY_MENU
#define MY_MENU

#include "hid.h"
#include "draw.h"
#include "console.h"
#include "screenshot.h"
#include "menu.h"
#include "fs.h"
#include "CTRDecryptor.h"
#include "NandDumper.h"
#include "TitleKeyDecrypt.h"
#include "padgen.h"
#include "nandtools.h"
#include "downgradeapp.h"
#include "firm.h"
#include "i2c.h"
#include "configuration.h"
#include "lang.h"
#include "AdvancedFileManager.h"
#include "json.h"

static void ShutDown(int arg){
	i2cWriteRegister(I2C_DEV_MCU, 0x20, (arg) ? (uint8_t)(1<<0):(uint8_t)(1<<2));
	while(1);
}

static Menu DecryptMenu = {
	"Decryption Options",
	.Option = (MenuEntry[6]){
		{ " Decrypt CTR Titles", &CTRDecryptor, L"dec0.bin" },
		{ " Decrypt Title Keys", &DecryptTitleKeys, L"dec1.bin" },
		{ " Decrypt encTitleKeys.bin", &DecryptTitleKeyFile, L"dec2.bin" },
		{ " Generate Xorpads", &PadGen, L"dec3.bin" },
		{ " Decrypt partitions", &DumpNandPartitions, L"dec4.bin" },
		{ " Generate fat16 Xorpad", &GenerateNandXorpads, L"dec5.bin" },
	},
	6,
	0,
	0
};

static Menu DumpMenu = {
	"Dumping Options",
	.Option = (MenuEntry[3]){
		{ " Create NAND dump", &NandDumper, L"dmp0.bin" },
		{ " Dump System Titles", &DumpNANDSystemTitles, L"dmp1.bin" },
		{ " Dump NAND Files", &dumpCoolFiles, L"dmp2.bin" },
	},
	3,
	0,
	0
};

static Menu InjectMenu = {
	"Injection Options",
	.Option = (MenuEntry[2]){
		{ " Inject EmuNAND partitions", &RebuildNand, L"inj0.bin" },
		{ " Inject NAND Files", &restoreCoolFiles, L"inj1.bin" },
	},
	2,
	0,
	0
};

static Menu AdvancedMenu = {
	"Other Options",
	.Option = (MenuEntry[5]){
		{ " Downgrade MSET on SysNAND", &downgradeMSET, L"adv0.bin" },
		{ " Install FBI over H&S App", &installFBI, L"adv1.bin" },
		{ " Restore original H&S App", &restoreHS, L"adv2.bin" },
		{ " Launch PastaMode", (void(*)())&PastaMode, L"adv3.bin" },
		{ " Advanced File Manager", &AdvFileManagerMain, L"adv4.bin" },
	},
	5,
	0,
	0
};

static Menu SettingsMenu = {
	"           SETTINGS",
	.Option = (MenuEntry[9]){
		{ "Force UI boot               ", NULL, L"app.bin" },
		{ "Selected theme:             ", NULL, L"app.bin" },
		{ "Random theme:               ", NULL, L"app.bin" },
		{ "Show AGB_FIRM BIOS:         ", NULL, L"app.bin" },
		{ "Enable 3D UI:               ", NULL, L"app.bin" },
		{ "Autoboot into sysNAND:      ", NULL, L"app.bin" },
		{ "Console language:           ", NULL, L"app.bin" },
		{ "Reboot                      ", NULL, L"app.bin" },
		{ "Shutdown                    ", NULL, L"app.bin" },
	},
	9,
	0,
	0
};

void DecryptMenuInit(){
	MenuInit(&DecryptMenu);
	MenuShow();
	while (true) {
		uint32_t pad_state = InputWait();
		if(pad_state & BUTTON_DOWN) MenuNextSelection();
		if(pad_state & BUTTON_UP)   MenuPrevSelection();
		if(pad_state & BUTTON_A)    MenuSelect();
		if(pad_state & BUTTON_B) 	{ MenuClose(); break; }
		TryScreenShot();
		MenuShow();
	}
}

void DumpMenuInit(){
	MenuInit(&DumpMenu);
	MenuShow();
	while (true) {
		uint32_t pad_state = InputWait();
		if(pad_state & BUTTON_DOWN) MenuNextSelection();
		if(pad_state & BUTTON_UP)   MenuPrevSelection();
		if(pad_state & BUTTON_A)    MenuSelect();
		if(pad_state & BUTTON_B) 	{ MenuClose(); break; }
		TryScreenShot();
		MenuShow();
	}
}

void InjectMenuInit(){
	MenuInit(&InjectMenu);
	MenuShow();
	while (true) {
		uint32_t pad_state = InputWait();
		if(pad_state & BUTTON_DOWN) MenuNextSelection();
		if(pad_state & BUTTON_UP)   MenuPrevSelection();
		if(pad_state & BUTTON_A)    MenuSelect();
		if(pad_state & BUTTON_B) 	{ MenuClose(); break; }
		TryScreenShot();
		MenuShow();
	}
}

void AdvancedMenuInit(){
	MenuInit(&AdvancedMenu);
	MenuShow();
	while (true) {
		uint32_t pad_state = InputWait();
		if(pad_state & BUTTON_DOWN) MenuNextSelection();
		if(pad_state & BUTTON_UP)   MenuPrevSelection();
		if(pad_state & BUTTON_A)    MenuSelect();
		if(pad_state & BUTTON_B) 	{ MenuClose(); break; }
		TryScreenShot();
		MenuShow();
	}
}

void SettingsMenuInit(){
	MenuInit(&SettingsMenu);

//	DIR d;
//	FILINFO fno;
	wchar_t str[_MAX_LFN];
//	const unsigned int maxLangNum = 16;
//	wchar_t langs[maxLangNum][CFG_STR_MAX_LEN];
	unsigned char theme_num = 0;
//	unsigned int curLang, langNum;
/*
	curLang = 0;
	langNum = 0;
	if (!f_opendir(&d, langPath)) {
		mbstowcs(str, cfgs[CFG_LANG].val.s, _MAX_LFN);

		while (langNum < maxLangNum) {
			fno.lfname = langs[langNum];
			fno.lfsize = CFG_STR_MAX_LEN;

			if (f_readdir(&d, &fno))
				break;

			if (fno.lfname[0] == 0)
				break;

			if (!wcscmp(fno.lfname, str))
				curLang = langNum;

			langNum++;
		}

		f_closedir(&d);
	}
*/
	while (true) {
		//UPDATE SETTINGS GUI
/*		swprintf(MyMenu->Name, CONSOLE_MAX_TITLE_LENGTH+1, L"%ls    Build: %s", lang("SETTINGS"), VERSION);
		swprintf(MyMenu->Option[0].Str, CONSOLE_MAX_LINE_LENGTH+1, lang("FORCE_UI_BOOT"), lang(cfgs[CFG_GUI].val.i ? " +" : " -"));
		swprintf(MyMenu->Option[1].Str, CONSOLE_MAX_LINE_LENGTH+1, lang("SELECTED_THEME"), cfgs[CFG_THEME].val.i + '0');
		swprintf(MyMenu->Option[2].Str, CONSOLE_MAX_LINE_LENGTH+1, lang("RANDOM_THEME"), lang(cfgs[CFG_RANDOM].val.i ? " +" : " -"));
		swprintf(MyMenu->Option[3].Str, CONSOLE_MAX_LINE_LENGTH+1, lang("SHOW_AGB"), lang(cfgs[CFG_AGB].val.i ? " +" : " -"));
		swprintf(MyMenu->Option[4].Str, CONSOLE_MAX_LINE_LENGTH+1, lang("ENABLE_3D_UI"), lang(cfgs[CFG_3D].val.i ? " +" : " -"));
		swprintf(MyMenu->Option[5].Str, CONSOLE_MAX_LINE_LENGTH+1, lang("ABSYSN"), lang(cfgs[CFG_ABSYSN].val.i ? " +" : " -"));
		swprintf(MyMenu->Option[6].Str, CONSOLE_MAX_LINE_LENGTH+1, lang("MENU_LANGUAGE"), lang("LANG_NAME"));
		swprintf(MyMenu->Option[7].Str, CONSOLE_MAX_LINE_LENGTH+1, lang("Shutdown"));
		swprintf(MyMenu->Option[8].Str, CONSOLE_MAX_LINE_LENGTH+1, lang("Reboot"));
*/		MenuRefresh();

		uint32_t pad_state = InputWait();
		if (pad_state & BUTTON_DOWN)
		{
			if(MyMenu->Current == 7) MenuNextSelection();
			MenuNextSelection();
		}
		if (pad_state & BUTTON_UP)
		{
			if(MyMenu->Current == 0) MenuPrevSelection();
			MenuPrevSelection();
		}
		if (pad_state & BUTTON_LEFT || pad_state & BUTTON_RIGHT)
		{
			if (MyMenu->Current == 0)
			{
//				cfgs[CFG_GUI].val.i ^= 1;
			}
			else if (MyMenu->Current == 1) //theme selection
			{
				/* Jump to the next available theme */
				File AppBin;
				bool found = false;
				unsigned char i;

				if (pad_state & BUTTON_LEFT && theme_num > 0)
				{
					for (i = theme_num - 1; i > 0; i--)
					{
						swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/app.bin", i);
						if (FileOpen(&AppBin, str, 0))
						{
							FileClose(&AppBin);
							found = true;
							break;
						}
					}

					if (i == 0) found = true;
				} else
				if (pad_state & BUTTON_RIGHT && theme_num < 9)
				{
					for (i = theme_num + 1; i <= 9; i++)
					{
						swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/app.bin", i);
						if (FileOpen(&AppBin, str, 0))
						{
							FileClose(&AppBin);
							found = true;
							break;
						}
					}
				}

				if (found)
				{
					theme_num = i;
/*					if (cfgs[CFG_3D].val.i) {
						swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/TOPL.bin", theme_num);
						DrawSplash(&top1Screen, str);
						swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/TOPR.bin", theme_num);
						DrawSplash(&top2Screen, str);
					} else {
						Screen tmpScreen = top1Screen;
						tmpScreen.addr = (uint8_t*)0x27000000;
						swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/TOP.bin", theme_num);
						DrawSplash(&tmpScreen, str);
						memcpy(top1Screen.addr, tmpScreen.addr, top1Screen.size);
						memcpy(top2Screen.addr, tmpScreen.addr, top2Screen.size);
					}
*/					cfgs[CFG_THEME].val.i = theme_num;
//					trySetLangFromTheme(1);
				}
			}
//			else if (MyMenu->Current == 2) cfgs[CFG_RANDOM].val.i ^= 1;
			else if (MyMenu->Current == 3) cfgs[CFG_AGB].val.i ^= 1;
			else if (MyMenu->Current == 4)
			{
/*				cfgs[CFG_3D].val.i ^= 1;
				if (cfgs[CFG_3D].val.i) {
					swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/TOPL.bin", theme_num);
					DrawSplash(&top1Screen, str);
					swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/TOPR.bin", theme_num);
					DrawSplash(&top2Screen, str);
				} else {
					Screen tmpScreen = top1Screen;
					tmpScreen.addr = (uint8_t*)0x27000000;
					swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/TOP.bin", theme_num);
					DrawSplash(&tmpScreen, str);
					memcpy(top1Screen.addr, tmpScreen.addr, top1Screen.size);
					memcpy(top2Screen.addr, tmpScreen.addr, top2Screen.size);
				}
*/			}
			else if (MyMenu->Current == 5)
			{
//				cfgs[CFG_ABSYSN].val.i ^= 1;
			}
			else if (MyMenu->Current == 6)
			{
				langLoad(cfgs[CFG_LANG].val.s, LANG_NEXT);
			}
			else if (MyMenu->Current == 7)
			{
				MenuNextSelection();
			}
			else if (MyMenu->Current == 8)
			{
				MenuPrevSelection();
			}
		}
		else if (pad_state & BUTTON_A)
		{
			if (MyMenu->Current == 7)
			{
				fadeOut();
				ShutDown(1);
			}
			else if (MyMenu->Current == 8)
			{
				fadeOut();
				ShutDown(0);
			}
		}
		if (pad_state & BUTTON_B)
		{
			//Code to save settings
			writeCfg();
			MenuClose();
			break;
		}

		TryScreenShot();

	}
}

void BootMenuInit(){
	wchar_t str[_MAX_LFN];

	//SHOW ONLY SYSYNAND IF EMUNAND IS NOT FOUND
	swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/boot%c.bin",
		cfgs[CFG_THEME].val.i, checkEmuNAND() ? L'0' : L'S');
	DrawSplash(&bottomScreen, str);
	while (true) {
		uint32_t pad_state = InputWait();
		if ((pad_state & BUTTON_Y) && checkEmuNAND()) {
			rxModeWithSplash(1);      //Boot emunand (only if found)
			DrawSplash(&bottomScreen, str);
		} else if (pad_state & BUTTON_X) {
			rxModeWithSplash(0); //Boot sysnand
			DrawSplash(&bottomScreen, str);
		} else if (pad_state & BUTTON_B)
			break;
	}

	MenuClose();
}

void CreditsMenuInit(){
	wchar_t str[_MAX_LFN];
	swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/credits.bin",
		cfgs[CFG_THEME].val.i);
	DrawSplash(&bottomScreen, str);
	WaitForButton(BUTTON_B);
	OpenAnimation();
}

static Menu MainMenu = {
		"rxTools - Roxas75 [v3.0]",
		.Option = (MenuEntry[7]){
			{ " Launch rxMode", &BootMenuInit, L"menu0.bin" },
			{ " Decryption Options", &DecryptMenuInit, L"menu1.bin" },
			{ " Dumping Options", &DumpMenuInit, L"menu2.bin" },
			{ " Injection Options", &InjectMenuInit, L"menu3.bin" },
			{ " Advanced Options", &AdvancedMenuInit, L"menu4.bin" },
			{ " Settings", &SettingsMenuInit, L"menu5.bin" },
			{ " Credits", &CreditsMenuInit, L"menu6.bin" },
		},
		7,
		0,
		0
	};

#endif

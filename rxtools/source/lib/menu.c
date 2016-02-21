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
C:\rxTools\rxTools-theme\rxtools\source\lib\menu.c * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "configuration.h"
#include "lang.h"
#include "menu.h"
#include "draw.h"
#include "hid.h"
#include "jsmn/jsmn.h"
#include "fs.h"
#include "json.h"
#include "firm.h"
#include "i2c.h"
#include "CTRDecryptor.h"
#include "NandDumper.h"
#include "TitleKeyDecrypt.h"
#include "padgen.h"
#include "nandtools.h"
#include "downgradeapp.h"
#include "AdvancedFileManager.h"

Menu* MyMenu;
Menu *MenuChain[100];
int openedMenus = 0;
int saved_index = 0;

char jsm[LANG_JSON_SIZE];
jsmntok_t tokm[LANG_JSON_TOKENS];
Json menuJson = {jsm, MENU_JSON_SIZE, tokm, MENU_JSON_TOKENS};
const TCHAR *menuPath = _T("") SYS_PATH "/gui.json";

#define MENU_MAX_LEVELS 4
#define MENU_LEVEL_BIT_WIDTH 8 * sizeof(int) / MENU_MAX_LEVELS

typedef enum {
	OBJ_NONE,
	OBJ_MENU,
} objtype;

//menu items hierarchy tags
typedef enum {
	APPLY_NONE,
	APPLY_ANCESTOR,
	APPLY_SIBLING,
	APPLY_TARGET,
	APPLY_DESCENDANT
} menuapply;

typedef enum {
	NAV_UP,
	NAV_DOWN,
	NAV_PREV,
	NAV_NEXT
} menunav;

static void Reboot() {
	fadeOut();
	i2cWriteRegister(I2C_DEV_MCU, 0x20, 4);
	while(1);
}

static void Shutdown() {
	fadeOut();
	i2cWriteRegister(I2C_DEV_MCU, 0x20, 1);
	while(1);
}

static void rxE() {
	rxMode(1);
}

static void rxS() {
	rxMode(0);
}

#define OPTION_MAX_SIZE 32
typedef struct {
	wchar_t name[OPTION_MAX_SIZE];
	wchar_t value[_MAX_LFN];
} Option;

typedef struct {
	int count;
	Option *item;
} Options;

#define LANG_NUM_MAX 16
Option langitem[LANG_NUM_MAX];
Options langitems = {0, langitem};

Options *getLangs(){
	DIR d;
	FILINFO fno;
	wchar_t str[_MAX_LFN];
	unsigned int langNum = 0;

	if (!f_opendir(&d, langPath)) {
		mbstowcs(str, cfgs[CFG_LANG].val.s, _MAX_LFN);

		while (langNum < LANG_NUM_MAX) {
//			fno.lfname = langs[langNum];
//			fno.lfsize = CFG_STR_MAX_LEN;

			if (f_readdir(&d, &fno))
				break;

			if (fno.lfname[0] == 0)
				break;

			wcscpy(langitems.item[langNum].name, fno.lfname);
			wcscpy(langitems.item[langNum].value, fno.lfname);

			langNum++;
		}
		f_closedir(&d);
	}
	langitems.count = langNum;
	return &langitems;
}

#define FUNC_KEY_MAX_SIZE 32
struct {
	char key[FUNC_KEY_MAX_SIZE];
	void( *func)();
} Func[] = { 
	{"FUNC_RXE", &rxE},
	{"FUNC_RXS", &rxS},
	{"FUNC_PASTA", (void(*)())&PastaMode},
	{"FUNC_REBOOT", &Reboot},
	{"FUNC_SHUTDOWN", &Shutdown},
	{"FUNC_DEC_CTR", &CTRDecryptor},
	{"FUNC_DEC_TK", &DecryptTitleKeys},
	{"FUNC_DEC_TKF", &DecryptTitleKeyFile},
	{"FUNC_GEN_T_PAD", &PadGen},
	{"FUNC_DEC_PAR", &DumpNandPartitions},
	{"FUNC_GEN_PAT_PAD", &GenerateNandXorpads},
	{"FUNC_DMP_N", &NandDumper},
	{"FUNC_DMP_NT", &DumpNANDSystemTitles},
	{"FUNC_DMP_NF", &dumpCoolFiles},
	{"FUNC_DMP_RAM", NULL},
	{"FUNC_INJ_N", &RebuildNand},
	{"FUNC_INJ_NF", &restoreCoolFiles},
	{"FUNC_DG_MSET", &downgradeMSET},
	{"FUNC_INS_FBI", &installFBI},
	{"FUNC_INS_HS", &restoreHS},
	{"FUNC_AFM", &AdvFileManagerMain},
	{"FUNC_CHK_E", (void(*)())&checkEmuNAND},
	{"FUNC_OPT_LANG", (void(*)())&getLangs}
};

int menuPosition = 0;
//int icaption, idescr, ihead;
int ifunc, ienabled;

int menuNavigate(int pos, menunav nav) {
	int bitlevel = sizeof(int) * 8 - menuLevel(pos) * MENU_LEVEL_BIT_WIDTH;
	int newpos = pos;
	switch (nav) {
		case NAV_UP:
			if ((bitlevel += MENU_LEVEL_BIT_WIDTH) < sizeof(int) * 8)
				newpos &= -1 << bitlevel;
			break;
		case NAV_PREV:
			newpos -= 1 << bitlevel;
			break;
		case NAV_DOWN:
			if ((bitlevel -= MENU_LEVEL_BIT_WIDTH) > 0)
		case NAV_NEXT:
			newpos += 1 << bitlevel;
			break;
	}
	if ((newpos & ((1 << MENU_LEVEL_BIT_WIDTH) - 1) << bitlevel) == 0) // over/underflow
		newpos = pos;
	return menuTry(newpos, pos);
}

void MenuInit(Menu* menu){
	MyMenu = menu;
	ConsoleInit();
	if (openedMenus == 0) MyMenu->Current = saved_index; //if we're entering the main menu, load the index
	else  MyMenu->Current = 0;
    MyMenu->Showed = 0;
	ConsoleSetTitle(lang(MyMenu->Name, -1));
	for(int i = 0; i < MyMenu->nEntryes; i++){
		ConsoleAddText(lang(MyMenu->Option[i].Str, -1));
	}
	if (menuPosition == 0) //select first upper menu on start;
		menuPosition = menuNavigate(menuPosition, NAV_DOWN);
}

TextColors itemColor = {WHITE, TRANSPARENT},
	selectedColor = {BLACK, WHITE},
	disabledColor = {GREY, TRANSPARENT},
	selecteddisabledColor = {BLACK, GREY},
	descriptionColor = {YELLOW, TRANSPARENT},
	valueColor = {BLUE, TRANSPARENT};

void MenuShow(){
//	wchar_t str[_MAX_LFN];

	//OLD TEXT MENU:
	/*int x = 0, y = 0;
	ConsoleGetXY(&x, &y);
	if (!MyMenu->Showed){
		sprintf(str, "/rxTools/Theme/%c/app.bin", Theme);
		DrawBottomSplash(str);
		ConsoleShow();
		MyMenu->Showed = 1;
	}
	for (int i = 0; i < MyMenu->nEntryes; i++){
		DrawString(BOT_SCREEN, i == MyMenu->Current ? L">" : L" ", x + CHAR_WIDTH*(ConsoleGetSpacing() - 1), (i)* CHAR_WIDTH + y + CHAR_WIDTH*(ConsoleGetSpacing() + 1), ConsoleGetSpecialColor(), ConsoleGetBackgroundColor());
	}*/

	//NEW GUI:
	/*
	swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/%ls",
		cfgs[CFG_THEME].val.i, MyMenu->Option[MyMenu->Current].gfx_splash);
	DrawBottomSplash(str);
	*/
	//NEW TEXT GUI :)
/*	swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/%ls",
		cfgs[CFG_THEME].val.i, MyMenu->Option[MyMenu->Current].gfx_splash);
	DrawSplash(&bottomTmpScreen, str);
*/
//	int level = menuLevel(menuPosition);

//	DrawStringRect(&bottomTmpScreen, lang(MyMenu->Name, -1), 0, font24.h, 0, 0, &selectedColor, &font24);
/*	DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[ihead].start, menuJson.tok[ihead].end - menuJson.tok[ihead].start), 0, font24.h, 0, font24.h, &itemColor, &font24);
	uint32_t y = font24.h + font16.h;
	y += DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[icaption].start, menuJson.tok[icaption].end - menuJson.tok[icaption].start), 0, y, bottomTmpScreen.w / 2, 0, &selectedColor, &font16);
	for (int i = 0; i < MyMenu->nEntryes; i++)
		y += DrawStringRect(&bottomTmpScreen, lang(MyMenu->Option[i].Str, -1), 0, y, bottomTmpScreen.w / 2, 0, i == MyMenu->Current ? &selectedColor : &itemColor, &font16);
	DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[idescr].start, menuJson.tok[idescr].end - menuJson.tok[idescr].start), bottomTmpScreen.w / 2, font24.h + font16.h, 0, 0, &descriptionColor, &font16);
*/
	memcpy(bottomScreen.addr, bottomTmpScreen.addr, bottomScreen.size);
}

void MenuNextSelection(){
	menuPosition = menuNavigate(menuPosition, NAV_NEXT);
	if(MyMenu->Current + 1 < MyMenu->nEntryes){
		MyMenu->Current++;
	}else{
//		MyMenu->Current = 0;
	}

}

void MenuPrevSelection(){
	menuPosition = menuNavigate(menuPosition, NAV_PREV);
	if(MyMenu->Current > 0){
		MyMenu->Current--;
	}else{
//		MyMenu->Current = MyMenu->nEntryes - 1;
	}
}

void (*getFunc(int idx))() {
	char *key = menuJson.js + menuJson.tok[idx].start;
	int keylen = menuJson.tok[idx].end - menuJson.tok[idx].start;
	for(int i = 0; i < sizeof(Func) / sizeof(Func[0]); i++)
		if (memcmp(Func[i].key, key, keylen) == 0)
			return Func[i].func;
	return NULL;
}

void MenuSelect(){
	if (ifunc > 0)
	{
		void(*func)();
		if (ienabled == 0 || ((func = getFunc(ienabled)) != NULL && ((int(*)())func)()))
			if (((func = getFunc(ifunc)) != NULL))
				func();
		MenuShow();
	}
	else
		menuPosition = menuNavigate(menuPosition, NAV_DOWN);
/*
	if(MyMenu->Option[MyMenu->Current].Func != NULL){
		if (openedMenus == 0)saved_index = MyMenu->Current; //if leaving the main menu, save the index
		MenuChain[openedMenus++] = MyMenu;
		MyMenu->Option[MyMenu->Current].Func();
		MenuInit(MenuChain[--openedMenus]);
		MenuShow();
	}
*/
}

void MenuClose(){
	menuPosition = menuNavigate(menuPosition, NAV_UP);
	if (openedMenus > 0){
		OpenAnimation();
	}
}

void MenuRefresh(){
	ConsoleInit();
	MyMenu->Showed = 0;
	ConsoleSetTitle(lang(MyMenu->Name, -1));
	for (int i = 0; i < MyMenu->nEntryes; i++){
		print(L"%ls %ls\n", i == MyMenu->Current ? strings[STR_CURSOR] : strings[STR_NO_CURSOR], lang(MyMenu->Option[i].Str, -1));
	}
	int x = 0, y = 0;
	ConsoleGetXY(&x, &y);
	if (!MyMenu->Showed){
		ConsoleShow();
		MyMenu->Showed = 1;
	}
}

//New menu system

int menuParse(int s, objtype type, int menulevel, int menuposition, int targetposition, int *foundposition, uint32_t *itemy) {
	if (s == menuJson.count)
		return 0;
	if (menuJson.tok[s].type == JSMN_PRIMITIVE || menuJson.tok[s].type == JSMN_STRING)
		return 1;
	else if (menuJson.tok[s].type == JSMN_OBJECT) {
		int mask, i, j = 0, k;
		int idescr = 0, icaption = 0, ien = 0, iselect = 0, ivalue = 0;
		uint32_t submenuy = font24.h + font16.h;
		int targetlevel = menuLevel(targetposition);
		menuapply apply = APPLY_NONE;
		if (type == OBJ_MENU)
			menulevel++;
		for (i = 0; i < menuJson.tok[s].size; i++) {
			j++;
			mask = (0xffffffff << (8 * sizeof(unsigned int) - targetlevel * MENU_LEVEL_BIT_WIDTH));
			if (menuJson.tok[s+j+1].type == JSMN_OBJECT && menulevel > 0){
				menuposition += 1 << ((MENU_MAX_LEVELS - menulevel) * MENU_LEVEL_BIT_WIDTH);
				mask <<= MENU_LEVEL_BIT_WIDTH;
/*				if((menuposition & mask) == (targetposition & mask)){
					apply = APPLY_ANCESTOR;
					if (menuposition == targetposition) {
						apply = APPLY_TARGET;
						*foundposition = menuposition; //at least, we found parent menu position
					}
				}
*/			}else{
				if (menuposition == targetposition){
					apply = APPLY_TARGET;
					*foundposition = targetposition;
				} else if ((menuposition & mask) == (targetposition & mask))
					apply = APPLY_DESCENDANT;
				else {
					mask = (0xffffffff << (8 * sizeof(unsigned int) - (menulevel-1) * MENU_LEVEL_BIT_WIDTH));
					if ((menuposition & mask) == (targetposition & mask))
						apply = APPLY_ANCESTOR;
					else if ((mask = ~mask | (mask << MENU_LEVEL_BIT_WIDTH)) && (menuposition & mask) == (targetposition & mask))
						apply = APPLY_SIBLING;

				}
			}

			switch (menuJson.js[menuJson.tok[s+j].start]){
				case 'c': //"caption"
					j++;
//					if (menulevel == 2 && (apply == APPLY_TARGET || apply == APPLY_ANCESTOR))
//						ihead = s + j;
//					if (apply == APPLY_TARGET)
						icaption = s + j;
					break;
				case 'd': //"description"
					j++;
					if (apply == APPLY_TARGET)
						idescr = s + j;
					break;
				case 'e': //"enabled"
					j++;
					ien = s + j;
					if (apply == APPLY_TARGET)
						ienabled = s + j;
					break;
				case 'f': //"function"
					j++;
					if (apply == APPLY_TARGET)
						ifunc = s + j;
					break;
				case 'm': //"menu"
					k = menuParse(s+j+1, OBJ_MENU, menulevel, menuposition, targetposition, foundposition, &submenuy);
					if (k == 0)
						return 0;
					j += k;
					break;
				case 'o': //"options"
					j++;
					if (apply == APPLY_TARGET)
						iselect = s + j;
					break;
				case 'v': //"value"
					j++;
					if (apply == APPLY_TARGET || apply == APPLY_SIBLING)
						ivalue = s + j;
					break;
				default:
					k = menuParse(s+j+1, type, menulevel, menuposition, targetposition, foundposition, &submenuy);
					if (k == 0)
						return 0;
					j += k;
			}
		}

		TextColors c,s;
		void(*func)();
		if (ien == 0 || ((func = getFunc(ien)) != NULL && ((int(*)())func)())) {
			c = itemColor;
			s = selectedColor;
		}
		else
		{
		 	c = disabledColor;
			s = selecteddisabledColor;
		}

		if (idescr > 0)
			DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[idescr].start, menuJson.tok[idescr].end - menuJson.tok[idescr].start), bottomTmpScreen.w / 2, font24.h + font16.h, 0, 0, &descriptionColor, &font16);

		if (ivalue > 0) {
			for (int i = 0; i < CFG_NUM; i++) {
				if (strncmp(cfgs[i].key, menuJson.js + menuJson.tok[ivalue].start, menuJson.tok[ivalue].end - menuJson.tok[ivalue].start) == 0) {
					wchar_t str[10];
					switch (cfgs[i].type) {
						case CFG_TYPE_STRING:
							DrawStringRect(&bottomTmpScreen, lang(cfgs[i].val.s, -1), bottomTmpScreen.w / 2, *itemy, bottomTmpScreen.w / 2, 0, &valueColor, &font16);
							break;
						case CFG_TYPE_INT:
							swprintf(str, sizeof(str)/sizeof(str[0]), L"%d", cfgs[i].val.i);
							DrawStringRect(&bottomTmpScreen, str, bottomTmpScreen.w / 2, *itemy, bottomTmpScreen.w / 2, 0, &valueColor, &font16);
							break;
						case CFG_TYPE_BOOLEAN:
							DrawStringRect(&bottomTmpScreen, lang(cfgs[i].val.b ? "Enabled" : "Disabled", -1), bottomTmpScreen.w / 2, *itemy, bottomTmpScreen.w / 2, 0, &valueColor, &font16);
							break;
					}
					break;
				}
			}
		}
/*
		if (iselect > 0 && ((func = getFunc(iselect)) != NULL)) {
			Options *opt = ((Options*(*)())func)();
			uint32_t y = font24.h + font16.h;
			for (int i = 0; i < opt->count; i++)
				y += DrawStringRect(&bottomTmpScreen, opt->item[i].name, bottomTmpScreen.w / 2, y, 0, 0, &c, &font16);
		}
*/
		if (icaption > 0) switch (targetlevel) {
			case 1:
				if (apply == APPLY_TARGET)
					DrawSubString(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[icaption].start, menuJson.tok[icaption].end - menuJson.tok[icaption].start), -1, 0, font24.h, &s, &font24);
				else if (apply == APPLY_DESCENDANT)
					*itemy += DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[icaption].start, menuJson.tok[icaption].end - menuJson.tok[icaption].start), 0, *itemy, bottomTmpScreen.w / 2, 0, &c, &font16);
				break;
			case 2:
				if (apply == APPLY_ANCESTOR)
					DrawSubString(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[icaption].start, menuJson.tok[icaption].end - menuJson.tok[icaption].start), -1, 0, font24.h, &s, &font24);
				else if (apply == APPLY_SIBLING)
					*itemy += DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[icaption].start, menuJson.tok[icaption].end - menuJson.tok[icaption].start), 0, *itemy, bottomTmpScreen.w / 2, 0, &c, &font16);
				else if (apply == APPLY_TARGET)
					*itemy += DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[icaption].start, menuJson.tok[icaption].end - menuJson.tok[icaption].start), 0, *itemy, bottomTmpScreen.w / 2, 0, &s, &font16);
				else if (apply == APPLY_DESCENDANT)
					*itemy += DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[icaption].start, menuJson.tok[icaption].end - menuJson.tok[icaption].start), bottomTmpScreen.w / 2, *itemy, bottomTmpScreen.w / 2, 0, &c, &font16);
				break;
			case 3:
				if (apply == APPLY_ANCESTOR)
					DrawSubString(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[icaption].start, menuJson.tok[icaption].end - menuJson.tok[icaption].start), -1, 0, font24.h, &s, &font24);
				else if (apply == APPLY_SIBLING)
					*itemy += DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[icaption].start, menuJson.tok[icaption].end - menuJson.tok[icaption].start), bottomTmpScreen.w / 2, *itemy, bottomTmpScreen.w / 2, 0, &c, &font16);
				else if (apply == APPLY_TARGET)
					*itemy += DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[icaption].start, menuJson.tok[icaption].end - menuJson.tok[icaption].start), bottomTmpScreen.w / 2, *itemy, bottomTmpScreen.w / 2, 0, &s, &font16);
				break;
		}
		return j + 1;
	} else if (menuJson.tok[s].type == JSMN_ARRAY) {
		int i, j = 0;
		for (i = 0; i < menuJson.tok[s].size; i++)
			j += menuParse(s+j+1, type, menulevel, menuposition, targetposition, foundposition, NULL);
		return j + 1;
	}
	return 0;
}

int menuTry(int targetposition, int currentposition) {
	ClearScreen(&bottomTmpScreen, BLACK);
	ifunc = 0;
	ienabled = 0;
	int foundposition = 0;
	menuParse(0, OBJ_NONE, 0, 0, targetposition, &foundposition, NULL);
	if (foundposition == 0)
		menuParse(0, OBJ_NONE, 0, 0, currentposition, &foundposition, NULL);
	return foundposition;
}

int menuLevel(int pos) {
	int i;
	for (i = 0; pos != 0; pos <<= MENU_LEVEL_BIT_WIDTH, i++);
	return i;
}

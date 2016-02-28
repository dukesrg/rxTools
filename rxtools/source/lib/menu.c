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
#include "fs.h"
#include "firm.h"
#include "i2c.h"
#include "CTRDecryptor.h"
#include "NandDumper.h"
#include "TitleKeyDecrypt.h"
#include "padgen.h"
#include "nandtools.h"
#include "downgradeapp.h"
#include "AdvancedFileManager.h"
#include "theme.h"

Menu* MyMenu;
Menu *MenuChain[100];
int openedMenus = 0;
int saved_index = 0;

char jsm[MENU_JSON_SIZE];
jsmntok_t tokm[MENU_JSON_TOKENS];
Json menuJson = {jsm, MENU_JSON_SIZE, tokm, MENU_JSON_TOKENS};
const wchar_t *menuPath = L"" SYS_PATH "/gui.json";

int menuPosition = 0;

#define MENU_MAX_LEVELS 4
#define MENU_LEVEL_BIT_WIDTH 8 * sizeof(menuPosition) / MENU_MAX_LEVELS

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

int ancestors[MENU_MAX_LEVELS - 1]; //item ancestors to fraw menu path

typedef struct { //target item properties
	int index;
	int func;
	int hint;
} Target;
Target target;

typedef struct { //siblings properties
	int caption;
	int enabled;
	int parami;
	int params;
	int value;
} Sibling;
Sibling siblings[1 << MENU_MAX_LEVELS];

int siblingcount; //number of siblings in current menu

/*
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
*/

static void Shutdown(int arg) {
	fadeOut();
	i2cWriteRegister(I2C_DEV_MCU, 0x20, (arg) ? (uint8_t)(1<<0):(uint8_t)(1<<2));
	while(1);
}

bool EmuNANDExists(int foo) {
	return checkEmuNAND();
}

int getConfig(int idx) {
	int i;
	for(i = 0; i < CFG_NUM && strncmp(cfgs[i].key, menuJson.js + menuJson.tok[idx].start, menuJson.tok[idx].end - menuJson.tok[idx].start) != 0; i++);
	return i < CFG_NUM ? i : -1;
}

void ConfigToggle(int idx) {
	if ((idx = getConfig(idx)) >= 0) {
		switch (cfgs[idx].type) {
			case CFG_TYPE_STRING:
				break;
			case CFG_TYPE_INT:
				break;
			case CFG_TYPE_BOOLEAN:
				cfgs[idx].val.b = !cfgs[idx].val.b;
				break;
		}
		writeCfg();
		MenuRefresh();
	}
}

bool ConfigCheck(int idx) {
	if ((idx = getConfig(idx)) >= 0) {
		switch (cfgs[idx].type) {
			case CFG_TYPE_STRING:
//				break;
			case CFG_TYPE_INT:
//				break;
			case CFG_TYPE_BOOLEAN:
				return true;
				break;
		}
	}
	return false;
}

#define FUNC_KEY_MAX_SIZE 32
struct {
	char key[FUNC_KEY_MAX_SIZE];
	void *func;
} Func[] = { 
	{"FUNC_RXMODE", &rxMode},
	{"FUNC_PASTA", &PastaMode},
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
	{"FUNC_CHK_E", &EmuNANDExists},
	{"FUNC_CHK_F", &FileExists},
	{"FUNC_CHK_CFG", &ConfigCheck},
	{"FUNC_CFG_TOGGLE", &ConfigToggle},
	{"FUNC_OPT_LANG", NULL}
};

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

void MenuShow(){
	memcpy(bottomScreen.addr, bottomTmpScreen.addr, bottomScreen.size);
//	memcpy(top1Screen.addr, top1TmpScreen.addr, top1Screen.size);
//	memcpy(top2Screen.addr, top2TmpScreen.addr, top2Screen.size);
}

void MenuNextSelection(){
	menuPosition = menuNavigate(menuPosition, NAV_NEXT);
}

void MenuPrevSelection(){
	menuPosition = menuNavigate(menuPosition, NAV_PREV);
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
	if (siblings[target.index].value != 0) { //target item have a settings key mapped
		bool(*check)(int);
		void(*func)(int);
		if (siblings[target.index].enabled == 0 || ((check = (bool(*)(int))getFunc(siblings[target.index].enabled)) != NULL && check(siblings[target.index].value))) {
			if(target.func != 0 && (func = (void(*)(int))getFunc(target.func)) != NULL)
				func(siblings[target.index].value);
		}
	} else if (siblings[target.index].parami != 0) { //target item have an integer parameter for callback
		bool(*check)(int);
		void(*func)(int);
		int param = strtol(menuJson.js + menuJson.tok[siblings[target.index].parami].start, NULL, 0);
		if (siblings[target.index].enabled == 0 || ((check = (bool(*)(int))getFunc(siblings[target.index].enabled)) != NULL && check(param))) {
			if(target.func != 0 && (func = (void(*)(int))getFunc(target.func)) != NULL)
				func(param);
			else
				menuPosition = menuNavigate(menuPosition, NAV_DOWN);
		}
	} else if (siblings[target.index].params != 0) { //target item have a string parameter for callback function
		wchar_t str[_MAX_LFN];
		bool(*check)(wchar_t*);
		void(*func)(wchar_t*);
		swprintf(str, menuJson.tok[siblings[target.index].params].end - menuJson.tok[siblings[target.index].params].start + 1, L"%s", menuJson.js + menuJson.tok[siblings[target.index].params].start);
		if (siblings[target.index].enabled == 0 || ((check = (bool(*)(wchar_t *))getFunc(siblings[target.index].enabled)) != NULL && check(str))) {
			if (target.func != 0 && (func = (void(*)(wchar_t*))getFunc(target.func)) != NULL)
				func(str);
			else
				menuPosition = menuNavigate(menuPosition, NAV_DOWN);
		}
	} else { //target item have no parameter for callback function
		bool(*check)();
		void(*func)();
		if (siblings[target.index].enabled == 0 || ((check = (bool(*)())getFunc(siblings[target.index].enabled)) != NULL && check())) {
			if (target.func != 0 && (func = (void(*)())getFunc(target.func)) != NULL)
				func();
			else
				menuPosition = menuNavigate(menuPosition, NAV_DOWN);
		}
	}
	MenuShow();
}

void MenuClose(){
	menuPosition = menuNavigate(menuPosition, NAV_UP);
/*	if (openedMenus > 0){
		OpenAnimation();
	}
*/
}

void MenuRefresh(){
/*	ConsoleInit();
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
*/
	menuPosition = menuTry(menuPosition, menuPosition);
}

//New menu system

int menuParse(int s, objtype type, int menulevel, int menuposition, int targetposition, int *foundposition) {
	if (s == 0) { //parse from the begining - initialize data structures;
		memset(ancestors, 0, sizeof(ancestors));
		target = (Target){0};
		for (int i = 0; i < sizeof(siblings)/sizeof(siblings[0]); siblings[i++] = (Sibling){0});
//		siblings[siblingcount = 0] = (Sibling){0};
		siblingcount = 0;
	}
	if (s == menuJson.count)
		return 0;
	if (menuJson.tok[s].type == JSMN_PRIMITIVE || menuJson.tok[s].type == JSMN_STRING)
		return 1;
	else if (menuJson.tok[s].type == JSMN_OBJECT) {
		int mask, i, j = 0, k;
		int targetlevel = menuLevel(targetposition);
		menuapply apply = APPLY_NONE;
		if (type == OBJ_MENU)
			menulevel++;

//		if (menulevel >= 2)
//			ancestors[menulevel - 2] = 0;
		for (i = 0; i < menuJson.tok[s].size; i++) {
			j++;
			mask = (0xffffffff << (8 * sizeof(unsigned int) - targetlevel * MENU_LEVEL_BIT_WIDTH));
			if (menuJson.tok[s+j+1].type == JSMN_OBJECT && menulevel > 0){
				menuposition += 1 << ((MENU_MAX_LEVELS - menulevel) * MENU_LEVEL_BIT_WIDTH);
				if (menuposition == (targetposition & mask << MENU_LEVEL_BIT_WIDTH)) //set parent menu style
					themeSet(cfgs[CFG_THEME].val.i, menuJson.js + menuJson.tok[s+j].start);
//				mask << MENU_LEVEL_BIT_WIDTH;
//				if((menuposition & mask) == (targetposition & mask)){
//					apply = APPLY_ANCESTOR;

//					if (menuposition == targetposition) {
//						istyle = s+j;
//						themeSet(cfgs[CFG_THEME].val.i, menuJson.js + menuJson.tok[s+j].start);
//						apply = APPLY_TARGET;
//						*foundposition = menuposition; //at least, we found parent menu position
//					}
//				}
			}else{
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
					switch(apply) {
						case APPLY_ANCESTOR:
							ancestors[menulevel - 2] = s + ++j;
							break;
						case APPLY_TARGET:
							target.index = siblingcount;
						case APPLY_SIBLING:
							siblings[siblingcount].caption = s + ++j;
							break;
						case APPLY_DESCENDANT:
						default:
							j++;
					}
					break;
				case 'e': //"enabled"
					j++;
					if (apply == APPLY_TARGET || apply == APPLY_SIBLING)
						siblings[siblingcount].enabled = s + j;
					break;
				case 'h': //"hint"
					j++;
					if (apply == APPLY_TARGET)
						target.hint = s + j;
					break;
				case 'f': //"function"
					j++;
					if (apply == APPLY_TARGET)
						target.func = s + j;
					break;
				case 'm': //"menu"
					if (targetlevel == 1) //top menu level - set 'menu' style
						themeSet(cfgs[CFG_THEME].val.i, menuJson.js + menuJson.tok[s+j].start);
					k = menuParse(s+j+1, OBJ_MENU, menulevel, menuposition, targetposition, foundposition);
					if (k == 0)
						return 0;
					j += k;
					break;
/*				case 'o': //"options"
					j++;
//					if (apply == APPLY_TARGET)
//						iselect = s + j;
					break;
*/				case 'p': //"parameter"
					switch(menuJson.tok[s + ++j].type) {
						case JSMN_PRIMITIVE:							
							if (apply == APPLY_TARGET)
								siblings[siblingcount].parami = s + j;
							break;
						case JSMN_STRING:							
							if (apply == APPLY_TARGET)
								siblings[siblingcount].params = s + j;
							break;
						default:
							break;
					}
					break;
				case 'v': //"value"
					j++;
					if (apply == APPLY_TARGET || apply == APPLY_SIBLING)
						siblings[siblingcount].value = s + j;
					break;
				default:
					k = menuParse(s+j+1, type, menulevel, menuposition, targetposition, foundposition);
					if (k == 0)
						return 0;
					j += k;
			}
		}

		if (apply == APPLY_TARGET || apply == APPLY_SIBLING)
			siblingcount++;
//			siblings[siblingcount++] = (Sibling){0};

		return j + 1;
	} else if (menuJson.tok[s].type == JSMN_ARRAY) {
		int i, j = 0;
		for (i = 0; i < menuJson.tok[s].size; i++)
			j += menuParse(s+j+1, type, menulevel, menuposition, targetposition, foundposition);
		return j + 1;
	}
	return 0;
}

int menuTry(int targetposition, int currentposition) {
	int i, j, foundposition = 0;
	uint32_t x = 0, y = font24.h;
	bool enabled;
	wchar_t str[_MAX_LFN];
	bool(*check)();
	bool(*checki)(int);
	bool(*checks)(wchar_t*);

	menuParse(0, OBJ_NONE, 0, 0, targetposition, &foundposition);
	if (foundposition == 0) //fallback in case requested menu not exists
		menuParse(0, OBJ_NONE, 0, 0, currentposition, &foundposition);

/*	if (wcslen(style.top1img) > 0) {
		DrawSplash(&top1TmpScreen, style.top1img);
		if (wcslen(style.top2img) > 0)
			DrawSplash(&top2TmpScreen, style.top2img);
		else
			memcpy(top2TmpScreen.addr, top1TmpScreen.addr, top1Screen.size);
	} else {
		ClearScreen(&top1TmpScreen, BLACK);
		ClearScreen(&top2TmpScreen, BLACK);
	}
*/
	if (wcslen(style.bottomimg) > 0)
		DrawSplash(&bottomTmpScreen, style.bottomimg);
	else
		ClearScreen(&bottomTmpScreen, BLACK);

	for (i = 0; i < sizeof(ancestors)/sizeof(ancestors[0]) && ancestors[i] != 0; i++)
		x += font24.dw + DrawSubString(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[ancestors[i]].start, menuJson.tok[ancestors[i]].end - menuJson.tok[ancestors[i]].start), -1, x, 0, &style.color, &font24);

	for (i = 0; i < sizeof(siblings)/sizeof(siblings[0]) && siblings[i].caption != 0; i++) {
		if ((siblings[i].value != 0) && (j = getConfig(siblings[i].value)) >= 0) {
			switch (cfgs[j].type) {
				case CFG_TYPE_STRING:
					DrawStringRect(&bottomTmpScreen, lang(cfgs[j].val.s, -1), bottomTmpScreen.w / 2, y, bottomTmpScreen.w / 2, 0, &style.value, &font16);
					break;
				case CFG_TYPE_INT:
					swprintf(str, sizeof(str)/sizeof(str[0]), L"%d", cfgs[j].val.i);
					DrawStringRect(&bottomTmpScreen, str, bottomTmpScreen.w / 2, y, bottomTmpScreen.w / 2, 0, &style.value, &font16);
					break;
				case CFG_TYPE_BOOLEAN:
					DrawStringRect(&bottomTmpScreen, lang(cfgs[j].val.b ? "Enabled" : "Disabled", -1), bottomTmpScreen.w / 2, y, bottomTmpScreen.w / 2, 0, &style.value, &font16);
					break;
			}
		}
		if (siblings[i].enabled == 0) {
			enabled = true;
		} else if (siblings[i].value != 0) { //item have a settings key mapped
			enabled = (check = (bool(*)(int))getFunc(siblings[i].enabled)) != NULL && check(siblings[i].value);
		} else if (siblings[i].parami != 0) { //item have an integer parameter for enabled check callback function
			enabled = (checki = (bool(*)(int))getFunc(siblings[i].enabled)) != NULL && checki(strtol(menuJson.js + menuJson.tok[siblings[i].parami].start, NULL, 0));
		} else if (siblings[i].params != 0) { //item have a string parameter for enabled check callback function
			swprintf(str, menuJson.tok[siblings[i].params].end - menuJson.tok[siblings[i].params].start + 1, L"%s", menuJson.js + menuJson.tok[siblings[i].params].start);
			enabled = (checks = (bool(*)(wchar_t*))getFunc(siblings[i].enabled)) != NULL && checks(str);
		} else { //item have no parameter for enabled check callback function
			enabled = (check = (bool(*)())getFunc(siblings[i].enabled)) != NULL && check();
		}
		if (i == target.index) {
			if (target.hint != 0)
				DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[target.hint].start, menuJson.tok[target.hint].end - menuJson.tok[target.hint].start), bottomTmpScreen.w / 2, font24.h, 0, 0, &style.hint, &font16);
			y += DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[siblings[i].caption].start, menuJson.tok[siblings[i].caption].end - menuJson.tok[siblings[i].caption].start), 0, y, bottomTmpScreen.w / 2, 0, enabled ? &style.selected : &style.unselected, &font16);
		} else
			y += DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[siblings[i].caption].start, menuJson.tok[siblings[i].caption].end - menuJson.tok[siblings[i].caption].start), 0, y, bottomTmpScreen.w / 2, 0, enabled ? &style.color : &style.disabled, &font16);
	}
	
	return foundposition;
}

int menuLevel(int pos) {
	int i;
	for (i = 0; pos != 0; pos <<= MENU_LEVEL_BIT_WIDTH, i++);
	return i;
}
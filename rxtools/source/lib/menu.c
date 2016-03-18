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
#include "json.h"
#include "theme.h"
#include "tmd.h"

#define MENU_JSON_SIZE		0x4000
#define MENU_JSON_TOKENS	0x400

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
	int description;
} Target;
Target target;

typedef struct { //siblings properties
	int caption;
	int enabled;
	int parami;
	int params;
	int parama;
	int value;
} Sibling;
Sibling siblings[1 << MENU_MAX_LEVELS];

int siblingcount; //number of siblings in current menu

const char *bootoptions[BOOT_COUNT] = {
	"rxTools UI",
	"rxMode EmuNAND",
	"rxMode SysNAND",
	"Pasta mode"
};

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
	for(i = 0; i < CFG_NUM && memcmp(cfgs[i].key, menuJson.js + menuJson.tok[idx].start, menuJson.tok[idx].end - menuJson.tok[idx].start) != 0; i++);
	return i < CFG_NUM ? i : -1;
}

int nextKey(int idx) {
	do {
		if (++idx >= KEY_COUNT)
			idx = 0;
	} while (keys[idx].mask == 0);
	return idx;
}

int prevKey(int idx) {
	do {
		if (--idx < 0)
			idx = KEY_COUNT - 1;
	} while (keys[idx].mask == 0);
	return idx;
}

int nextBoot(int idx) {
	if (++idx >= BOOT_COUNT)
		idx = 0;
	return idx;
}

int prevBoot(int idx) {
	if (--idx < 0)
		idx = BOOT_COUNT - 1;
	return idx;
}

void ConfigToggle(int idx) {
	if ((idx = getConfig(idx)) >= 0) {
		switch (cfgs[idx].type) {
			case CFG_TYPE_STRING:
				if (idx == CFG_LANG)
					langLoad(cfgs[idx].val.s, LANG_NEXT);
				else if (idx == CFG_THEME)
					themeLoad(cfgs[idx].val.s, THEME_NEXT);
				break;
			case CFG_TYPE_INT:
				if (idx == CFG_FORCE_UI || idx == CFG_FORCE_EMUNAND || idx == CFG_FORCE_SYSNAND || idx == CFG_FORCE_PASTA)
					cfgs[idx].val.i = nextKey(cfgs[idx].val.i);
				else if (idx == CFG_BOOT_DEFAULT)
					cfgs[idx].val.i = nextBoot(cfgs[idx].val.i);
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

static uint32_t cksum(File *fp)
{
	uint32_t tbl[256];
	uint8_t *buf = (uint8_t*)0x21000000;
	size_t size;
	uint32_t i, j, crc;

	size = FileRead(fp, buf, 0x00400000, 0);
	for (i = 0; i < 256; i++)
	{
		crc = i << 24;
		for (j = 8; j > 0; j--)
		{
			if (crc & 0x80000000)
				crc = (crc << 1) ^ 0x04c11db7;
			else
				crc = (crc << 1);
			tbl[i] = crc;
		}
	}
	crc = 0;
	for (i = 0; i < size; i++)
		crc = (crc << 8) ^ tbl[((crc >> 24) ^ *buf++) & 0xFF];
	for (; size; size >>= 8)
		crc = (crc << 8) ^ tbl[((crc >> 24) ^ size) & 0xFF];
	return ~crc;
}

bool MsetCheck(wchar_t *path) {
	static uint32_t mset_hash[10] = { 0x96AEC379, 0xED315608, 0x3387F2CD, 0xEDAC05D7, 0xACC1BE62, 0xF0FF9F08, 0x565BCF20, 0xA04654C6, 0x2164C3C0, 0xD40B12F4 }; //JPN, USA, EUR, CHN, KOR, TWN
	File fp;
	uint32_t i, crc;
	bool res = false;
	if (FileOpen(&fp, path, 0)) {
		crc = cksum(&fp);
		for (i = 0; !res && i < sizeof(mset_hash)/sizeof(mset_hash[0]); res = crc == mset_hash[i++]);
		FileClose(&fp);
	}
	return res;
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
	{"FUNC_CHK_MSET", &MsetCheck},
	{"FUNC_CFG_TOGGLE", &ConfigToggle},
	{"FUNC_OPT_LANG", NULL}
};

static int getRegion(int drive) {
	File fp;
	int region = 0;
	wchar_t str[_MAX_LFN + 1];
	
	swprintf(str, _MAX_LFN + 1, L"%d:rw/sys/SecureInfo_A", drive);
	if (!FileOpen(&fp, str, 0)) {
		str[wcslen(str) - 1] = L'B';
		if (!FileOpen(&fp, str, 0))
			return -1;
	}
	FileRead(&fp, &region, 1, 0x100);
	FileClose(&fp);
	if (region > 6) //unknown region
		return -1;
	else if (region == 3) //AUS = EUR
		region--;
	else if (region > 3)
		region += 2; //make region compatible with TitleId low, octet 2, low nibble
	return region;
}

static bool checkOption(int func, int params) {
	if (func <= 0)
		return false;
	wchar_t str[_MAX_LFN + 1];
	if (!memcmp("FUNC_CHK_E", menuJson.js + menuJson.tok[func].start, menuJson.tok[func].end - menuJson.tok[func].start)) {
		return checkEmuNAND();
	} else if (!memcmp("FUNC_CHK_F", menuJson.js + menuJson.tok[func].start, menuJson.tok[func].end - menuJson.tok[func].start)) {
		if (params > 0 && menuJson.tok[params].type == JSMN_STRING) { //path
			swprintf(str, _MAX_LFN + 1, L"%.*s", menuJson.tok[params].end - menuJson.tok[params].start, menuJson.js + menuJson.tok[params].start);
			return FileExists(str);
		}
	} else if (!memcmp("FUNC_CHK_CFG", menuJson.js + menuJson.tok[func].start, menuJson.tok[func].end - menuJson.tok[func].start)) {
		if ((params = getConfig(params)) >= 0) {
			switch (cfgs[params].type) {
				case CFG_TYPE_STRING:
//					break;
				case CFG_TYPE_INT:
//					break;
				case CFG_TYPE_BOOLEAN:
					return true;
			}
		}
	} else if (!memcmp("FUNC_CHK_MSET", menuJson.js + menuJson.tok[func].start, menuJson.tok[func].end - menuJson.tok[func].start)) {
		if (params > 0 && menuJson.tok[params].type == JSMN_ARRAY && menuJson.tok[params].size == 6) { //path,nand number,TitleIdHi,TitleIdLo,Version,CRC
			params++;
			swprintf(str, _MAX_LFN + 1, L"%.*s", menuJson.tok[params].end - menuJson.tok[params].start, menuJson.js + menuJson.tok[params].start);
			File fp;
			int i;
			wchar_t apppath[_MAX_LFN + 1];
			if (FileOpen(&fp, str, 0)) {
				uint32_t p[5];
				tmd_data tmd;
				params++;
				for (i = 0; i < 5; i++)
					p[i] = strtoul(menuJson.js + menuJson.tok[params + i].start, NULL, menuJson.tok[params + i].type == JSMN_STRING ? 16 : 10);
				tmd.header.title_id_hi = __builtin_bswap32(p[1]);
				tmd.header.title_id_lo = __builtin_bswap32(p[2]);
				if (p[1] != 0 && p[2] != 0 && (// skip titleID check if zero
					(p[2] >> 12 & 0x0F) != getRegion(p[0]+1) || // region check failed
					!tmdLoad(apppath, &tmd, p[0]+1) || // tmd/app failed
					__builtin_bswap16(tmd.header.title_version) == p[4] // version already installed
				)) {
					FileClose(&fp);
					return false; 
				}
				if (p[4] != 0 && cksum(&fp) != p[4]) { // skip CRC check if zero
					FileClose(&fp);
					return false;
				}
				FileClose(&fp);
				return true;
			}
		}
	}
	return false;
}

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

void MenuInit(){
	ConsoleInit();
	if (menuPosition == 0) //select first upper menu on start;
		menuPosition = menuNavigate(menuPosition, NAV_DOWN);
}

void MenuShow(){
	memcpy(bottomScreen.addr, bottomTmpScreen.addr, bottomScreen.size);
	memcpy(top1Screen.addr, top1TmpScreen.addr, top1Screen.size);
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
	} else if (siblings[target.index].parama != 0) {

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
		wchar_t str[_MAX_LFN + 1];
		bool(*check)(wchar_t*);
		void(*func)(wchar_t*);
		swprintf(str, _MAX_LFN + 1, L"%.*s", menuJson.tok[siblings[target.index].params].end - menuJson.tok[siblings[target.index].params].start, menuJson.js + menuJson.tok[siblings[target.index].params].start);
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
}

void MenuRefresh(){
	menuPosition = menuTry(menuPosition, menuPosition);
}

//New menu system

int menuParse(int s, objtype type, int menulevel, int menuposition, int targetposition, int *foundposition) {
	if (s == 0) { //parse from the begining - initialize data structures;
		memset(ancestors, 0, sizeof(ancestors));
		target = (Target){0};
		for (int i = 0; i < sizeof(siblings)/sizeof(siblings[0]); siblings[i++] = (Sibling){0});
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

		for (i = 0; i < menuJson.tok[s].size; i++) {
			j++;
			mask = (0xffffffff << (8 * sizeof(unsigned int) - targetlevel * MENU_LEVEL_BIT_WIDTH));
			if (menuJson.tok[s+j+1].type == JSMN_OBJECT && menulevel > 0){
				menuposition += 1 << ((MENU_MAX_LEVELS - menulevel) * MENU_LEVEL_BIT_WIDTH);
				if (menuposition == (targetposition & mask << MENU_LEVEL_BIT_WIDTH)) //set parent menu style
//					themeSet(cfgs[CFG_THEME].val.i, menuJson.js + menuJson.tok[s+j].start);
					themeStyleSet(menuJson.js + menuJson.tok[s+j].start);

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
				case 'd': //"description"
					j++;
					if (apply == APPLY_TARGET)
						target.description = s + j;
					break;
				case 'e': //"enabled"
					j++;
					if (apply == APPLY_TARGET || apply == APPLY_SIBLING)
						siblings[siblingcount].enabled = s + j;
					break;
/*				case 'h': //"hint"
					j++;
					if (apply == APPLY_TARGET)
						target.hint = s + j;
					break;
*/				case 'f': //"function"
					j++;
					if (apply == APPLY_TARGET)
						target.func = s + j;
					break;
				case 'm': //"menu"
					if (targetlevel == 1) //top menu level - set 'menu' style
//						themeSet(cfgs[CFG_THEME].val.i, menuJson.js + menuJson.tok[s+j].start);
						themeStyleSet(menuJson.js + menuJson.tok[s+j].start);
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
					switch(menuJson.tok[s + j + 1].type) {
						case JSMN_PRIMITIVE:
							if (apply == APPLY_TARGET || apply == APPLY_SIBLING)
								siblings[siblingcount].parami = s + j + 1;
							break;
						case JSMN_STRING:
							if (apply == APPLY_TARGET || apply == APPLY_SIBLING)
								siblings[siblingcount].params = s + j + 1;
							break;
						case JSMN_ARRAY:
							if (apply == APPLY_TARGET || apply == APPLY_SIBLING)
								siblings[siblingcount].parama = s + j + 1;
							break;
						default:
							break;
					}
					j += menuParse(s+j+1, type, menulevel, menuposition, targetposition, foundposition);
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

		return j + 1;
	} else if (menuJson.tok[s].type == JSMN_ARRAY) {
		int i, j = 0;
		for (i = 0; i < menuJson.tok[s].size; i++)
			j += menuParse(s+j+1, type, menulevel, menuposition, targetposition, foundposition);
		return j + 1;
	}
	return 0;
}

int menuLoad() {
	return jsonLoad(&menuJson, menuPath);
}

int menuTry(int targetposition, int currentposition) {
	int i, j, foundposition = 0;
	uint32_t x, y;
	bool enabled;
	wchar_t str[_MAX_LFN + 1];
	bool(*check)();
	bool(*checki)(int);
	bool(*checks)(wchar_t*);

	menuParse(0, OBJ_NONE, 0, 0, targetposition, &foundposition);
	if (foundposition == 0) //fallback in case requested menu not exists
		menuParse(0, OBJ_NONE, 0, 0, currentposition, &foundposition);

	x = style.captionRect.x;
	y = style.itemsRect.y;

	if (wcslen(style.top1img) > 0) {
		DrawSplash(&top1TmpScreen, style.top1img);
//		if (wcslen(style.top2img) > 0)
//			DrawSplash(&top2TmpScreen, style.top2img);
//		else
//			memcpy(top2TmpScreen.addr, top1TmpScreen.addr, top1Screen.size);
	} else {
		ClearScreen(&top1TmpScreen, BLACK);
//		ClearScreen(&top2TmpScreen, BLACK);
	}

	if (wcslen(style.bottomimg) > 0)
		DrawSplash(&bottomTmpScreen, style.bottomimg);
	else
		ClearScreen(&bottomTmpScreen, BLACK);

	for (i = 0; i < sizeof(ancestors)/sizeof(ancestors[0]) && ancestors[i] != 0; i++)
		x += font24.dw + DrawSubString(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[ancestors[i]].start, menuJson.tok[ancestors[i]].end - menuJson.tok[ancestors[i]].start), -1, x, style.captionRect.y, &style.captionColor, &font24);

	for (i = 0; i < sizeof(siblings)/sizeof(siblings[0]) && siblings[i].caption != 0; i++) {
		if ((siblings[i].value != 0) && (j = getConfig(siblings[i].value)) >= 0) {
			switch (cfgs[j].type) {
				case CFG_TYPE_STRING:
					DrawStringRect(&bottomTmpScreen, lang(cfgs[j].val.s, -1), style.valueRect.x, y, style.valueRect.w, style.valueRect.h, &style.valueColor, &font16);
					break;
				case CFG_TYPE_INT:
					if (j == CFG_FORCE_UI || j == CFG_FORCE_EMUNAND || j == CFG_FORCE_SYSNAND || j == CFG_FORCE_PASTA)
						DrawStringRect(&bottomTmpScreen, lang(keys[cfgs[j].val.i].name, -1), style.valueRect.x, y, style.valueRect.w, style.valueRect.h, &style.valueColor, &font16);
					else if (j == CFG_BOOT_DEFAULT)
						DrawStringRect(&bottomTmpScreen, lang(bootoptions[cfgs[j].val.i], -1), style.valueRect.x, y, style.valueRect.w, style.valueRect.h, &style.valueColor, &font16);
					else {
						swprintf(str, sizeof(str)/sizeof(str[0]), L"%d", cfgs[j].val.i);
						DrawStringRect(&bottomTmpScreen, str, style.valueRect.x, y, style.valueRect.w, style.valueRect.h, &style.valueColor, &font16);
					}
					break;
				case CFG_TYPE_BOOLEAN:
					DrawStringRect(&bottomTmpScreen, lang(cfgs[j].val.b ? "Enabled" : "Disabled", -1), style.valueRect.x, y, style.valueRect.w, style.valueRect.h, &style.valueColor, &font16);
					break;
			}
		}
		enabled = false;
		if (siblings[i].enabled == 0) {
			enabled = true;
		} else if (siblings[i].value != 0) { //item have a settings key mapped
			enabled = (check = (bool(*)(int))getFunc(siblings[i].enabled)) != NULL && check(siblings[i].value);
		} else if (siblings[i].parama != 0) {
			enabled = checkOption(siblings[i].enabled, siblings[i].parama);
		} else if (siblings[i].parami != 0) { //item have an integer parameter for enabled check callback function
			enabled = (checki = (bool(*)(int))getFunc(siblings[i].enabled)) != NULL && checki(strtol(menuJson.js + menuJson.tok[siblings[i].parami].start, NULL, 0));
		} else if (siblings[i].params != 0) { //item have a string parameter for enabled check callback function
			swprintf(str, _MAX_LFN + 1, L"%.*s", menuJson.tok[siblings[i].params].end - menuJson.tok[siblings[i].params].start, menuJson.js + menuJson.tok[siblings[i].params].start);
			enabled = (checks = (bool(*)(wchar_t*))getFunc(siblings[i].enabled)) != NULL && checks(str);
		} else { //item have no parameter for enabled check callback function
			enabled = (check = (bool(*)())getFunc(siblings[i].enabled)) != NULL && check();
		}
		if (i == target.index) {
			if (target.description != 0)
				DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[target.description].start, menuJson.tok[target.description].end - menuJson.tok[target.description].start), style.descriptionRect.x, style.descriptionRect.y, style.descriptionRect.w, style.descriptionRect.h, &style.descriptionColor, &font16);
			y += DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[siblings[i].caption].start, menuJson.tok[siblings[i].caption].end - menuJson.tok[siblings[i].caption].start), style.itemsRect.x, y, style.itemsRect.w, style.itemsRect.h, enabled ? &style.itemsSelected : &style.itemsUnselected, &font16);
		} else {
			y += DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[siblings[i].caption].start, menuJson.tok[siblings[i].caption].end - menuJson.tok[siblings[i].caption].start), style.itemsRect.x, y, style.itemsRect.w, style.itemsRect.h, enabled ? &style.itemsColor : &style.itemsDisabled, &font16);
		}
	}
	
	return foundposition;
}

int menuLevel(int pos) {
	int i;
	for (i = 0; pos != 0; pos <<= MENU_LEVEL_BIT_WIDTH, i++);
	return i;
}

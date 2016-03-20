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
#include "mbedtls/md5.h"

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
	int params;
} Sibling;
Sibling siblings[1 << MENU_MAX_LEVELS];

int siblingcount; //number of siblings in current menu

const char *bootoptions[BOOT_COUNT] = {
	"rxTools GUI",
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
		mbstowcs(str, cfgs[CFG_LANGUAGE].val.s, _MAX_LFN);

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

static bool getStrVal(wchar_t *s, int i) {
	return i > 0 && swprintf(s, _MAX_LFN + 1, L"%.*s", menuJson.tok[i].end - menuJson.tok[i].start, menuJson.js + menuJson.tok[i].start) > 0;
}

static uint32_t getIntVal(int i) {
	return strtoul(menuJson.js + menuJson.tok[i].start, NULL, menuJson.tok[i].type == JSMN_STRING ? 16 : 10);
}
/*
	{"FUNC_DEC_CTR", &CTRDecryptor},
	{"FUNC_DEC_TK", &DecryptTitleKeys},
	{"FUNC_DEC_TKF", &DecryptTitleKeyFile},
	{"FUNC_GEN_T_PAD", &PadGen},
	{"FUNC_DEC_PAR", &DumpNandPartitions},
	{"FUNC_GEN_PAT_PAD", &GenerateNandXorpads},
	{"FUNC_DMP_N", &NandDumper},
	{"FUNC_DMP_NT", &DumpNANDSystemTitles},
	{"FUNC_DMP_NF", &dumpCoolFiles},
	{"FUNC_INJ_N", &RebuildNand},
	{"FUNC_INJ_NF", &restoreCoolFiles},
	{"FUNC_DG_MSET", &downgradeMSET},
	{"FUNC_INS_FBI", &installFBI},
	{"FUNC_INS_HS", &restoreHS},
	{"FUNC_AFM", &AdvFileManagerMain},
*/

static bool runFunc(int func, int params) {
	FIL fp;
	FILINFO fno;
	UINT size;
	int i, funcsize;
	uint32_t hash[4];
	uint8_t *checkhash = (uint8_t*)&hash[0], filehash[16], *buf = (uint8_t*)0x21000000;
	wchar_t str[_MAX_LFN + 1];
	char *funckey;

	if (func == 0)
		return true;
	funckey = menuJson.js + menuJson.tok[func].start + 4;
	funcsize = menuJson.tok[func].end - menuJson.tok[func].start - 4;
	if (!memcmp(funckey - 4, "RUN_", 4)) {
		if (!memcmp(funckey, "RXMODE", funcsize))
			rxMode(getIntVal(params));
		else if (!memcmp(funckey, "PASTA", funcsize))
			PastaMode();
		else if (!memcmp(funckey, "SHUTDOWN", funcsize)) {
			fadeOut();
			i2cWriteRegister(I2C_DEV_MCU, 0x20, (getIntVal(params)) ? (uint8_t)(1<<0):(uint8_t)(1<<2));
			while(1);
		}
	} else if (!memcmp(funckey - 4, "CHK_", 4)) {
		if (!memcmp(funckey, "EMUNAND", funcsize))
			return checkEmuNAND();
		else if (!memcmp(funckey, "FILE", funcsize)) {
			if (params > 0) {
				switch (menuJson.tok[params].type) {
					case JSMN_STRING:
						return getStrVal(str, params) && f_stat(str, NULL) == FR_OK;
					case JSMN_ARRAY:
						fno.lfname = 0;
						if (menuJson.tok[params].size == 0 || !getStrVal(str, params + 1) || f_stat(str, &fno) != FR_OK)
							return false;
						if (menuJson.tok[params].size == 1)
							return true;
						if (fno.fsize != getIntVal(params + 2))
							return false;
						if (menuJson.tok[params].size == 2)
							return true;
						if (f_open(&fp, str, FA_READ | FA_OPEN_EXISTING) != FR_OK)
							return false;
						if (f_read(&fp, buf, fno.fsize, &size) != FR_OK || size != fno.fsize) {
							f_close(&fp);
							return false;
						}
						f_close(&fp);
						char tmp[9];
						for (i = 0; i < 4; i++) {
							strncpy(tmp, menuJson.js + menuJson.tok[params + 3].start + 8 * i, 8);
							hash[i] = __builtin_bswap32(strtoul(tmp, NULL, 16));
//							sscanf(menuJson.js + menuJson.tok[params + 3].start + 8 * i, "%08lx", &hash[i]); //lib code increases binary too much
						}
						mbedtls_md5(buf, size, filehash);
						return !memcmp(checkhash, filehash, 16);
					default:
						break;
				}
			}
		} else if (!memcmp(funckey, "TITLE", funcsize)) {
			if (params > 0 && menuJson.tok[params].type == JSMN_ARRAY && menuJson.tok[params].size == 6) { //path,nand number,TitleIdHi,TitleIdLo,Version,CRC
				params++;
				getStrVal(str, params);
				wchar_t apppath[_MAX_LFN + 1];
				if (FileOpen(&fp, str, 0)) {
					uint32_t p[5];
					tmd_data tmd;
					params++;
					for (i = 0; i < 5; i++)
						p[i] = getIntVal(params + i);
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
	} else if (!memcmp(funckey - 4, "VAL_", 4)) {
		if ((params = getConfig(params)) >= 0) {
			if (!memcmp(funckey, "CHECK", funcsize)) {
				switch (cfgs[params].type) {
					case CFG_TYPE_STRING:
//						break;
					case CFG_TYPE_INT:
//						break;
					case CFG_TYPE_BOOLEAN:
					return true;
				}
			} else if (!memcmp(funckey, "TOGGLE", funcsize)) {
				switch (params) {
					case CFG_BOOT_DEFAULT:
						cfgs[params].val.i = nextBoot(cfgs[params].val.i);
						break;
					case CFG_GUI_FORCE:
					case CFG_EMUNAND_FORCE:
					case CFG_SYSNAND_FORCE:
					case CFG_PASTA_FORCE:
						cfgs[params].val.i = nextKey(cfgs[params].val.i);
						break;
					case CFG_THEME:
						themeLoad(cfgs[params].val.s, THEME_NEXT);
						break;
					case CFG_AGB_BIOS:
						cfgs[params].val.b = !cfgs[params].val.b;
						break;
					case CFG_LANGUAGE:
						langLoad(cfgs[params].val.s, LANG_NEXT);
						break;
					default:
						return false;
				}
				writeCfg();
				MenuRefresh();
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

void MenuSelect() {
	if (runFunc(siblings[target.index].enabled, siblings[target.index].params)) {
		if (target.func)
			runFunc(target.func, siblings[target.index].params);
		else 
			menuPosition = menuNavigate(menuPosition, NAV_DOWN);
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
*/				case 'p': //"parameters"
/*					switch(menuJson.tok[s + j + 1].type) {
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
*/
					if (apply == APPLY_TARGET || apply == APPLY_SIBLING)
						siblings[siblingcount].params = s + j + 1;
//						siblings[siblingcount].value = s + j;
					j += menuParse(s+j+1, type, menulevel, menuposition, targetposition, foundposition);
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

	wcscpy(str, L"");
	for (i = 0; i < sizeof(ancestors)/sizeof(ancestors[0]) && ancestors[i] != 0; i++) {
		if (i > 0)
			wcscat(str, lang("|", 1));
		wcscat(str, lang(menuJson.js + menuJson.tok[ancestors[i]].start, menuJson.tok[ancestors[i]].end - menuJson.tok[ancestors[i]].start));
	}
	DrawSubString(&bottomTmpScreen, str, -1, x, style.captionRect.y, &style.captionColor, &font24);

	for (i = 0; i < sizeof(siblings)/sizeof(siblings[0]) && siblings[i].caption != 0; i++) {
		switch (j = getConfig(siblings[i].params)) {
			case CFG_BOOT_DEFAULT:
				DrawStringRect(&bottomTmpScreen, lang(bootoptions[cfgs[j].val.i], -1), style.valueRect.x, y, style.valueRect.w, style.valueRect.h, &style.valueColor, &font16);
				break;
			case CFG_GUI_FORCE:
			case CFG_EMUNAND_FORCE:
			case CFG_SYSNAND_FORCE:
			case CFG_PASTA_FORCE:
				DrawStringRect(&bottomTmpScreen, lang(keys[cfgs[j].val.i].name, -1), style.valueRect.x, y, style.valueRect.w, style.valueRect.h, &style.valueColor, &font16);
				break;
			case CFG_AGB_BIOS:
				DrawStringRect(&bottomTmpScreen, lang(cfgs[j].val.b ? "Enabled" : "Disabled", -1), style.valueRect.x, y, style.valueRect.w, style.valueRect.h, &style.valueColor, &font16);
				break;
			case CFG_THEME:
			case CFG_LANGUAGE:
				DrawStringRect(&bottomTmpScreen, lang(cfgs[j].val.s, -1), style.valueRect.x, y, style.valueRect.w, style.valueRect.h, &style.valueColor, &font16);
				break;
			default:
//				swprintf(str, sizeof(str)/sizeof(str[0]), L"%d", cfgs[j].val.i);
//				DrawStringRect(&bottomTmpScreen, str, style.valueRect.x, y, style.valueRect.w, style.valueRect.h, &style.valueColor, &font16);
				break;
		}
		enabled = runFunc(siblings[i].enabled, siblings[i].params);

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

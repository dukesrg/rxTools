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
#include "strings.h"
#include "progress.h"
#include "aes.h"
#include "sha.h"
#include "nand.h"
#include "firm.h"
#include "ncsd.h"

static Json menuJson;
static int menuPosition = 0;

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
	int activity;
	int gauge;
} Target;
Target target;

typedef struct { //siblings properties
	int caption;
	int enabled;
	int resolve;
	int params;
} Sibling;
Sibling siblings[1 << MENU_LEVEL_BIT_WIDTH];

int enabledsiblings[1 << MENU_LEVEL_BIT_WIDTH]; //sibling options enabled status cache

int siblingcount; //number of siblings in current menu

static const char *const *const bootoptions[BOOT_COUNT] = {
	&S_RXTOOLS_GUI,
	&S_RXTOOLS_SYSNAND,
	&S_RXTOOLS_EMUNAND,
	&S_PASTA_MODE
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

typedef struct {
	uint32_t title_id_lo;
	const char *const *const name;
} system_region;

#define DRIVE_COUNT 3
#define REGION_COUNT (sizeof(regions)/sizeof(regions)[0])

static const system_region *const getRegion(uint_fast8_t drive) {
	static system_region regions[] = {
		{0x00000000, &S_JAPAN},
		{0x00001000, &S_NORTH_AMERICA},
		{0x00002000, &S_EUROPE_AUSTRALIA},
		{0x00002000, &S_EUROPE_AUSTRALIA},
		{0x00006000, &S_CHINA},
		{0x00007000, &S_SOUTH_KOREA},
		{0x00008000, &S_TAIWAN},
		{0xFFFFFFFF, &S_UNKNOWN}
	};
	File fp;
	uint_fast8_t regionid = REGION_COUNT - 1;
	wchar_t str[_MAX_LFN + 1];

	if (drive < DRIVE_COUNT) {
		swprintf(str, _MAX_LFN + 1, L"%u:rw/sys/SecureInfo_A", drive);
		if (FileOpen(&fp, str, 0) || (str[wcslen(str) - 1] = L'B' && FileOpen(&fp, str, 0))) {
			if (!FileRead(&fp, &regionid, 1, 0x100) || regionid >= REGION_COUNT)
				regionid = REGION_COUNT - 1;
			FileClose(&fp);
		}
	}
	return &regions[regionid];
}

static uint_fast8_t getStrVal(wchar_t *s, int i) {
	return i > 0 && swprintf(s, _MAX_LFN + 1, L"%.*s", menuJson.tok[i].end - menuJson.tok[i].start, menuJson.js + menuJson.tok[i].start) > 0;
}

static uint32_t getIntVal(int i) {
	return strtoul(menuJson.js + menuJson.tok[i].start, NULL, menuJson.tok[i].type == JSMN_STRING ? 16 : 10);
}

static const char *const runResolve(int key, int params) {
	if (key == 0) return NULL;
	wchar_t str[_MAX_LFN + 1];
	char *keyname = menuJson.js + menuJson.tok[key].start + 4;
	int keysize = menuJson.tok[key].end - menuJson.tok[key].start - 4;
	if (!memcmp(keyname - 4, "VAL_", 4)) {
		if (!memcmp(keyname, "CFG", keysize)) {
			switch (params = getConfig(params)) {
				case CFG_BOOT_DEFAULT:
					return *bootoptions[cfgs[params].val.i];
				case CFG_GUI_FORCE:
				case CFG_EMUNAND_FORCE:
				case CFG_SYSNAND_FORCE:
				case CFG_PASTA_FORCE:
					return keys[cfgs[params].val.i].name;
				case CFG_AGB_BIOS:
					return cfgs[params].val.b ? S_ENABLED : S_DISABLED;
				case CFG_THEME:
				case CFG_LANGUAGE:
					return cfgs[params].val.s;
			}
		} else if (!memcmp(keyname, "RXTOOLS_BUILD", keysize)) {
			
		}
		else if (!memcmp(keyname, "REGION", keysize)) return *(getRegion(getIntVal(params))->name);
		else if (!memcmp(keyname, "TITLE_VERSION", keysize)) {
			tmd_data data;
			if (getStrVal(str, params) && tmdLoadRecent(&data, str) != 0xFFFFFFFF) {
				static char tmpstr[6];
				sprintf(tmpstr, "%u", __builtin_bswap16(data.header.title_version));
				return tmpstr;
			}
		}
	}
	return NULL;
}

/*
	{"FUNC_DEC_CTR", &CTRDecryptor},
	{"FUNC_DEC_TK", &DecryptTitleKeys},
	{"FUNC_DEC_TKF", &DecryptTitleKeyFile},
	{"FUNC_DMP_NT", &DumpNANDSystemTitles},
	{"FUNC_DMP_NF", &dumpCoolFiles},
	{"FUNC_INJ_NF", &restoreCoolFiles},
	{"FUNC_DG_MSET", &downgradeMSET},
	{"FUNC_INS_FBI", &installFBI},
	{"FUNC_INS_HS", &restoreHS},
	{"FUNC_AFM", &AdvFileManagerMain},
*/

#define BUF_SIZE 0x10000

static uint_fast8_t runFunc(int func, int params, int activity, int gauge) {
	File fp;
	FILINFO fno;
	UINT size;
	int i, funcsize;
	wchar_t str[_MAX_LFN + 1];
	size_t hashsize;
	uint32_t *hash;
	uint8_t *buf, *filehash, *checkhash;
	char *funckey;

	if (activity)
		statusInit(getIntVal(gauge), langn(menuJson.js + menuJson.tok[activity].start, menuJson.tok[activity].end - menuJson.tok[activity].start));
	if (!func)
		return 1;
	funckey = menuJson.js + menuJson.tok[func].start + 4;
	funcsize = menuJson.tok[func].end - menuJson.tok[func].start - 4;
	if (!memcmp(funckey - 4, "RUN_", 4)) {
		if (!memcmp(funckey, "RXMODE", funcsize)) rxMode(getIntVal(params));
		else if (!memcmp(funckey, "PASTA", funcsize)) PastaMode();
		else if (!memcmp(funckey, "SHUTDOWN", funcsize)) Shutdown(getIntVal(params));
		else if (!memcmp(funckey, "CFG_NEXT", funcsize)) {
			if ((params = getConfig(params)) >= 0) {
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
				}
				writeCfg();
				MenuRefresh();
				return 1;
			}
		}
	} else if (!memcmp(funckey - 4, "CHK_", 4)) {
		if (!memcmp(funckey, "NAND", funcsize)) {
			if (params <= 0) return 0;
			ncsd_header ncsd;
			switch (menuJson.tok[params].type) {
				case JSMN_PRIMITIVE:
					return checkNAND(getIntVal(params) - 1);
				case JSMN_ARRAY:
					if (menuJson.tok[params].size == 0 || !checkNAND(getIntVal(params + 1) - 1)) return 0;
					if (menuJson.tok[params].size == 1) return 1;
					if (!getStrVal(str, params + 2) || !FileOpen(&fp, str, 0) || (
						(FileGetSize(&fp) != GetNANDMetrics(getIntVal(params + 1) - 1)->sectors_count * NAND_SECTOR_SIZE ||
						FileRead2(&fp, &ncsd, NAND_SECTOR_SIZE) != NAND_SECTOR_SIZE) &&
						(FileClose(&fp) || 1)) ||
						ncsd.magic != NCSD_MAGIC
					) return 0;
					FileClose(&fp);
					return 1;
				default:
					break;
			}
		} else if (!memcmp(funckey, "PARTITION", funcsize)) {
			if (params <= 0 || menuJson.tok[params].type != JSMN_ARRAY || menuJson.tok[params].size < 2) return 0;
			nand_partition_entry *partition;
			void *buf;
			uint_fast8_t partition_type;
			if (!(partition = GetNANDPartition(getIntVal(params + 1) - 1, (partition_type = getIntVal(params + 2)))))
				return 0;
			if (menuJson.tok[params].size == 2) return 1;
			buf = __builtin_alloca(NAND_SECTOR_SIZE);
			if (!getStrVal(str, params + 3) || !FileOpen(&fp, str, 0) || (
				(FileGetSize(&fp) != partition->sectors_count * NAND_SECTOR_SIZE ||
				FileRead2(&fp, buf, NAND_SECTOR_SIZE) != NAND_SECTOR_SIZE) &&
				(FileClose(&fp) || 1))
			) return 0;
			FileClose(&fp);
			switch (partition_type) {
				case NAND_PARTITION_AGB_SAVE:
					return *(uint32_t*)buf == AGB_SAVE_MAGIC;
				case NAND_PARTITION_FIRM0:
				case NAND_PARTITION_FIRM1:
					return *(uint32_t*)buf == FIRM_MAGIC;
				case NAND_PARTITION_TWLN:
					if (memcmp(buf + FAT_VOLUME_LABEL_OFFSET, S_TWL, strlen(S_TWL))) return 0;
					goto check_boot_magic;
				case NAND_PARTITION_TWLP:
					if (memcmp(buf + FAT_VOLUME_LABEL_OFFSET, S_TWL, strlen(S_TWL))) return 0;
					goto check_boot_magic;
				case NAND_PARTITION_CTRNAND:
					if (memcmp(buf + FAT_VOLUME_LABEL_OFFSET, S_CTR, strlen(S_CTR))) return 0;
				case NAND_PARTITION_TWL:
				case NAND_PARTITION_CTR:
				check_boot_magic:
					return ((mbr*)buf)->partition_table.magic == MBR_BOOT_MAGIC;
			}
		} else if (!memcmp(funckey, "FILE", funcsize)) {
			if (params <= 0) return 0;
			switch (menuJson.tok[params].type) {
				case JSMN_STRING:
					return getStrVal(str, params) && f_stat(str, NULL) == FR_OK;
				case JSMN_ARRAY:
					fno.lfname = 0;
					if (menuJson.tok[params].size == 0 || !getStrVal(str, params + 1) || f_stat(str, &fno) != FR_OK) return 0;
					if (menuJson.tok[params].size == 1) return 1;
					if (fno.fsize != getIntVal(params + 2)) return 0;
					if (menuJson.tok[params].size == 2) return 1;
					if (!FileOpen(&fp, str, 0) || ((FileGetSize(&fp)) != fno.fsize && (FileClose(&fp) || 1)) ||
						(hashsize = (menuJson.tok[params + 3].end - menuJson.tok[params + 3].start) / 2) < SHA_1_SIZE
					) return 0;
					sha_start(hashsize == SHA_256_SIZE ? SHA_256_MODE : hashsize == SHA_224_SIZE ? SHA_224_MODE : SHA_1_MODE, NULL);
					hash = (uint32_t*)__builtin_alloca(hashsize);
					buf = (uint8_t*)__builtin_alloca(hashsize);
					filehash = (uint8_t*)__builtin_alloca(hashsize);
					checkhash = (uint8_t*)hash;
					while ((size = FileRead2(&fp, buf, BUF_SIZE))) sha_update(buf, size);
					FileClose(&fp);
					sha_finish(filehash);
					char tmp[9];
					for (i = 0; i < hashsize/sizeof(uint32_t); i++) {
						strncpy(tmp, menuJson.js + menuJson.tok[params + 3].start + 8 * i, 8);
						hash[i] = __builtin_bswap32(strtoul(tmp, NULL, 16));
					}
					return !memcmp(checkhash, filehash, hashsize);
				default:
					break;
			}
		} else if (!memcmp(funckey, "TITLE", funcsize)) {
			if (params <= 0 || menuJson.tok[params].type != JSMN_ARRAY || menuJson.tok[params].size < 2) return 0;
			tmd_data tmd;
			uint_fast8_t drive;
			params++;
			getStrVal(str, params);
			drive = getIntVal(params + 1);
			if (tmdPreloadHeader(&tmd, str) &&
				(__builtin_bswap32(tmd.header.title_id_lo) & 0x0000F000) == getRegion(drive)->title_id_lo &&
				tmdValidateChunk(&tmd, str, CONTENT_INDEX_MAIN, drive)
			) return 1;
		} else if (!memcmp(funckey, "CFG", funcsize)) {
			if ((params = getConfig(params)) >= 0) {
				switch (cfgs[params].type) {
					case CFG_TYPE_STRING:
					case CFG_TYPE_INT:
					case CFG_TYPE_BOOLEAN:
					return 1;
				}
			}
/*		} else if (!memcmp(funckey, "MSET", funcsize)) {
			if (params > 0 && menuJson.tok[params].type == JSMN_ARRAY && menuJson.tok[params].size == 6) { //path,nand number,TitleIdHi,TitleIdLo,Version,CRC
				params++;
				getStrVal(str, params);
				wchar_t apppath[_MAX_LFN + 1];
				if (FileOpen(&fp, str, false)) {
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
*/		}
	} else if (!memcmp(funckey - 4, "DMP_", 4)) {
		if (!memcmp(funckey, "NAND", funcsize))
			return params > 0 && menuJson.tok[params].type == JSMN_ARRAY && menuJson.tok[params].size >= 2 &&
				getStrVal(str, params + 2) && DumpNand(getIntVal(params + 1) - 1, str);
		else if (!memcmp(funckey, "PARTITION", funcsize))
			return params > 0 && menuJson.tok[params].type == JSMN_ARRAY && menuJson.tok[params].size >= 3 &&
				getStrVal(str, params + 3) && DumpPartition(getIntVal(params + 1) - 1, getIntVal(params + 2), str);
	} else if (!memcmp(funckey - 4, "INJ_", 4)) {
		if (!memcmp(funckey, "NAND", funcsize))
			return params > 0 && menuJson.tok[params].type == JSMN_ARRAY && menuJson.tok[params].size >= 2 &&
				getStrVal(str, params + 2) && InjectNand(getIntVal(params + 1) - 1, str);
		else if (!memcmp(funckey, "PARTITION", funcsize))
			return params > 0 && menuJson.tok[params].type == JSMN_ARRAY && menuJson.tok[params].size >= 3 &&
				getStrVal(str, params + 3) && InjectPartition(getIntVal(params + 1) - 1, getIntVal(params + 2), str);
	} else if (!memcmp(funckey - 4, "GEN_", 4)) {
		if (!memcmp(funckey, "XORPAD", funcsize)) {
			if (params <= 0) return 0;
			switch (menuJson.tok[params].type) {
				case JSMN_STRING:
					return getStrVal(str, params) && PadGen(str);
				case JSMN_ARRAY:
					if (menuJson.tok[params].size >= 2 && getStrVal(str, params + 2))
						return GenerateNandXorpad(getIntVal(params + 1), str);
				default:
					break;
			}			
		}
	}
	return 0;
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
			if (bitlevel && ((bitlevel -= MENU_LEVEL_BIT_WIDTH) || 1))
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
	DisplayScreen(&bottomScreen);
	DisplayScreen(&top1Screen);
//	DisplayScreen(&top2Screen);
//	memcpy(bottomScreen.addr, bottomTmpScreen.addr, bottomScreen.size);
//	memcpy(top1Screen.addr, top1TmpScreen.addr, top1Screen.size);
//	memcpy(top2Screen.addr, top2TmpScreen.addr, top2Screen.size);
}

void MenuNextSelection(){
	menuPosition = menuNavigate(menuPosition, NAV_NEXT);
}

void MenuPrevSelection(){
	menuPosition = menuNavigate(menuPosition, NAV_PREV);
}

void MenuSelect() {
	if (enabledsiblings[target.index] > 0) {
		if (target.func)
			runFunc(target.func, siblings[target.index].params, target.activity, target.gauge);
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
				case 'a': //"activity"
					j++;
					if (apply == APPLY_TARGET)
						target.activity = s + j;
					break;
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
				case 'g': //"gauge"
					j++;
					if (apply == APPLY_TARGET)
						target.gauge = s + j;
					break;
				case 'm': //"menu"
					if (targetlevel == 1) //top menu level - set 'menu' style
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
					if (apply == APPLY_TARGET || apply == APPLY_SIBLING)
						siblings[siblingcount].params = s + j + 1;
					j += menuParse(s+j+1, type, menulevel, menuposition, targetposition, foundposition);
					break;
				case 'r': //"resolve"
					j++;
					if (apply == APPLY_TARGET || apply == APPLY_SIBLING)
						siblings[siblingcount].resolve = s + j;
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

int menuLevel(int pos) {
	int i;
	for (i = 0; pos != 0; pos <<= MENU_LEVEL_BIT_WIDTH, i++);
	return i;
}

int menuLoad(Json *json, wchar_t *path) {
	menuJson = *json;
	return jsonLoad(&menuJson, path);
}

int menuTry(int targetposition, int currentposition) {
	int i, foundposition = 0;
	uint_fast16_t y;
	uint_fast8_t enabled;
	wchar_t str[_MAX_LFN + 1];

	menuParse(0, OBJ_NONE, 0, 0, targetposition, &foundposition);
	if (foundposition == 0) //fallback in case requested menu not exists
		menuParse(0, OBJ_NONE, 0, 0, currentposition, &foundposition);
	else if (menuLevel(currentposition) != menuLevel(foundposition)) //clear options enabled status cache on menu level change
		memset(enabledsiblings, 0, sizeof(enabledsiblings));

	y = style.itemsRect.y;

	if (*style.top1img) {
		DrawSplash(&top1Screen, style.top1img);
//		if (wcslen(style.top2img) > 0)
//			DrawSplash(&top2Screen, style.top2img);
//		else
//			memcpy(top2Screen.buf2, top1Screen.buf2, top1Screen.size);
	} else {
		ClearScreen(&top1Screen, BLACK);
//		ClearScreen(&top2Screen, BLACK);
	}

	if (*style.bottomimg)
		DrawSplash(&bottomScreen, style.bottomimg);
	else
		ClearScreen(&bottomScreen, BLACK);

	wcscpy(str, L"");
	for (i = 0; i < sizeof(ancestors)/sizeof(ancestors[0]) && ancestors[i] != 0; i++) {
		if (i > 0)
			wcscat(str, lang(S_PARENT_SEPARATOR));
		wcscat(str, langn(menuJson.js + menuJson.tok[ancestors[i]].start, menuJson.tok[ancestors[i]].end - menuJson.tok[ancestors[i]].start));
	}
	DrawStringRect(&bottomScreen, str, &style.captionRect, style.captionColor, style.captionAlign, 30);

	for (i = 0; i < sizeof(siblings)/sizeof(siblings[0]) && siblings[i].caption != 0; i++) {
		if (siblings[i].resolve)
			DrawStringRect(&bottomScreen, lang(runResolve(siblings[i].resolve, siblings[i].params)), &(Rect){style.valueRect.x, y, style.valueRect.w, style.valueRect.h}, style.valueColor, style.valueAlign, 16);
		if (!enabledsiblings[i])
			enabledsiblings[i] = !siblings[i].enabled || runFunc(siblings[i].enabled, siblings[i].params, 0, 0) ? 1 : -1;
		enabled = enabledsiblings[i] > 0;

		if (i == target.index) {
			if (target.description != 0)
				DrawStringRect(&bottomScreen, langn(menuJson.js + menuJson.tok[target.description].start, menuJson.tok[target.description].end - menuJson.tok[target.description].start), &style.descriptionRect, style.descriptionColor, style.descriptionAlign, 16);
			y += DrawStringRect(&bottomScreen, langn(menuJson.js + menuJson.tok[siblings[i].caption].start, menuJson.tok[siblings[i].caption].end - menuJson.tok[siblings[i].caption].start), &(Rect){style.itemsRect.x, y, style.itemsRect.w, style.itemsRect.h}, enabled ? style.itemsSelected : style.itemsUnselected, style.itemsAlign, 16);
		} else {
			y += DrawStringRect(&bottomScreen, langn(menuJson.js + menuJson.tok[siblings[i].caption].start, menuJson.tok[siblings[i].caption].end - menuJson.tok[siblings[i].caption].start), &(Rect){style.itemsRect.x, y, style.itemsRect.w, style.itemsRect.h}, enabled ? style.itemsColor : style.itemsDisabled, style.itemsAlign, 16);
		}
	}
	
	return foundposition;
}

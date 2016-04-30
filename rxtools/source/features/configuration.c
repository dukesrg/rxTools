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

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <reboot.h>
#include "TitleKeyDecrypt.h"
#include "configuration.h"
#include "lang.h"
#include "screenshot.h"
#include "fs.h"
#include "console.h"
#include "draw.h"
#include "hid.h"
#include "mpcore.h"
#include "ncch.h"
#include "crypto.h"
#include "firm.h"
#include "downgradeapp.h"
#include "menu.h"
#include "jsmn/jsmn.h"
#include "progress.h"
#include "strings.h"

#define DATA_PATH	L"rxtools/data"
#define KEYFILENAME	"slot0x25KeyX.bin"

static char cfgLang[CFG_STR_MAX_LEN] = "en";
static char cfgTheme[CFG_STR_MAX_LEN] = "";

Cfg cfgs[] = {
	[CFG_BOOT_DEFAULT] = { "CFG_BOOT_DEFAULT", CFG_TYPE_INT, { .i = BOOT_UI } },
	[CFG_GUI_FORCE] = { "CFG_GUI_FORCE", CFG_TYPE_INT, { .i = KEY_L } },
	[CFG_EMUNAND_FORCE] = { "CFG_EMUNAND_FORCE", CFG_TYPE_INT, { .i = KEY_Y } },
	[CFG_SYSNAND_FORCE] = { "CFG_SYSNAND_FORCE", CFG_TYPE_INT, { .i = KEY_X } },
	[CFG_PASTA_FORCE] = { "CFG_PASTA_FORCE", CFG_TYPE_INT, { .i = KEY_B } },
	[CFG_THEME] = { "CFG_THEME", CFG_TYPE_STRING, { .s = cfgTheme } },
	[CFG_AGB_BIOS] = { "CFG_AGB_BIOS", CFG_TYPE_BOOLEAN, { .i = 0 } },
	[CFG_LANGUAGE] = { "CFG_LANGUAGE", CFG_TYPE_STRING, { .s = cfgLang } }
};

static const wchar_t *jsonPath= L"/rxTools/data/system.json";

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start && strncmp(json + tok->start, s, tok->end - tok->start) == 0)
		return 0;
	return -1;
}

int writeCfg()
{
	File fd;
	char buf[0x400];
	const char *p;
	char *jsonCur;
	unsigned int i;
	size_t len;
	int left, res;

	left = sizeof(buf);
	jsonCur = buf;

	*jsonCur = '{';

	left--;
	jsonCur++;

	i = 0;
	for (i = 0; i < CFG_NUM; i++) {
		if (i > 0) {
			if (left < 1)
				return 1;

			*jsonCur = ',';
			left--;
			jsonCur++;
		}

		res = snprintf(jsonCur, left, "\n\t\"%s\": ", cfgs[i].key);
		if (res < 0 || res >= left)
			return 1;

		left -= res;
		jsonCur += res;

		switch (cfgs[i].type) {
			case CFG_TYPE_INT:
				res = snprintf(jsonCur, left, "%d", cfgs[i].val.i);
				if (res < 0 || res >= left)
					return 1;

				len = res;
				break;

			case CFG_TYPE_BOOLEAN:
				if (cfgs[i].val.i) {
					len = sizeof("true");
					p = "true";
				} else {
					len = sizeof("false");
					p = "false";
				}

				if (len >= left)
					return -1;

				strcpy(jsonCur, p);
				len--;
				break;

			case CFG_TYPE_STRING:
				res = snprintf(jsonCur, left, "\"%s\"", cfgs[i].val.s);
				if (res < 0 || res >= left)
					return 1;

				len = res;
				break;

			default:
				return -1;
		}

		left -= len;
		jsonCur += len;
	}

	left -= 3;
	if (left < 0)
		return 1;

	*jsonCur = '\n';
	jsonCur++;
	*jsonCur = '}';
	jsonCur++;
	*jsonCur = '\n';
	jsonCur++;

	if (!FileOpen(&fd, jsonPath, 1))
		return 1;

	FileWrite(&fd, buf, (uintptr_t)jsonCur - (uintptr_t)buf, 0);
	FileClose(&fd);

	return 0;
}

int readCfg()
{
	const size_t tokenNum = 1 + CFG_NUM * 2;
	jsmntok_t t[tokenNum];
	char buf[256];
	jsmn_parser parser;
	File fd;
	unsigned int i, j;
	int r;
	size_t len;

	if (!FileOpen(&fd, jsonPath, 0))
		return 1;

	len = FileGetSize(&fd);
	if (len > sizeof(buf))
		return 1;

	FileRead(&fd, buf, len, 0);
	FileClose(&fd);

	jsmn_init(&parser);
	r = jsmn_parse(&parser, buf, len, t, tokenNum);
	if (r < 0)
		return r;

	if (r < 1)
		return 1;

	/* Loop over all keys of the root object */
	for (i = 1; i < r; i++) {
		for (j = 0; jsoneq(buf, &t[i], cfgs[j].key) != 0; j++)
			if (j >= CFG_NUM)
				return 1;

		i++;
		switch (cfgs[j].type) {
			case CFG_TYPE_INT:
				cfgs[j].val.i = strtoul(buf + t[i].start, NULL, 0);
				break;

			case CFG_TYPE_BOOLEAN:
				len = t[i].end - t[i].start;
				cfgs[j].val.i = buf[t[i].start] == 't';

				break;

			case CFG_TYPE_STRING:
				len = t[i].end - t[i].start;

				if (len + 1 > CFG_STR_MAX_LEN)
					break;

#ifdef DEBUG
				if (cfgs[j].val.s == NULL)
					break;
#endif

				
				memcpy(cfgs[j].val.s, buf + t[i].start, len);
				cfgs[j].val.s[len] = 0;
		}
	}

	return 0;
}

static FRESULT saveFirm(uint32_t id, const void *p, DWORD n)
{
	wchar_t path[_MAX_LFN + 1];
	UINT bw;
	FRESULT r;
	FIL f;

	getFirmPath(path, id);
	r = f_open(&f, path, FA_WRITE | FA_CREATE_ALWAYS);
	if (r != FR_OK)
		return r;

	r = f_write(&f, p, n, &bw);
	f_close(&f);

	return r;
}

static int processFirmFile(uint32_t lo)
{
	static const wchar_t pathFmt[] = L"rxTools/firm/00040138%08" PRIX32 "%ls.bin";
	const uint32_t hi = 0x00040138;
	uint8_t key[AES_BLOCK_SIZE];
	wchar_t path[_MAX_LFN + 1];
	void *buff, *firm;
	UINT size;
	FRESULT r;
	FIL f;

	swprintf(path, _MAX_LFN + 1, pathFmt, lo, L"");
	r = f_open(&f, path, FA_READ);
	if (r != FR_OK)
		return r;

	size = f_size(&f);
	buff = __builtin_alloca(size + sizeof(uint32_t));
	r = f_read(&f, buff, size, &size);
	f_close(&f);
	if (r != FR_OK)
		return r;

	swprintf(path, _MAX_LFN + 1, pathFmt, lo, L"_cetk");
	if (!getTitleKeyWithCetk(key, path)) {
		firm = decryptFirmTitle(buff, size, &size, key);
		if (firm != NULL)
			return saveFirm(lo, firm, size);
	}

	if (!getTitleKey(key, hi, lo, 1)) {
		firm = decryptFirmTitle(buff, size, &size, key);
		if (firm != NULL)
			return saveFirm(lo, firm, size);
	}

	if (!getTitleKey(key, hi, lo, 2)) {
		firm = decryptFirmTitle(buff, size, &size, key);
		if (firm != NULL)
			return saveFirm(lo, firm, size);
	}

	return -1;
}

static int processFirmInstalled(uint32_t lo)
{
	void *buff, *firm;
	AppInfo appInfo;
	UINT size;
	FRESULT r;
	FIL f;

	appInfo.drive = 1;
	appInfo.tidLo = lo;
	appInfo.tidHi = TID_HI_FIRM;
	FindApp(&appInfo);
	if (f_open(&f, appInfo.content, FA_READ) != FR_OK) {
		appInfo.drive = 2;
		FindApp(&appInfo);
		r = f_open(&f, appInfo.content, FA_READ);
		if (r != FR_OK)
			return r;
	}

	size = f_size(&f);
	buff = __builtin_alloca(size);

	r = f_read(&f, buff, size, &size);
	f_close(&f);
	if (r != FR_OK)
		return r;

	firm = decryptFirmTitleNcch(buff, &size);
	return firm == NULL ? -1 : saveFirm(lo, firm, size);
}

static int processFirm(uint32_t lo)
{
	int r;

	r = processFirmFile(lo);
	if (r && processFirmInstalled(lo))
		return r;

	return 0;
}

static int InstallData() {
	int r;
	int p = 0;

	statusInit(getMpInfo() == MPINFO_CTR ? 3 : 1, L"Decrypting firmware");

	f_mkdir(DATA_PATH);

	r = processFirm(getMpInfo() == MPINFO_CTR ?
		TID_CTR_NATIVE_FIRM : TID_KTR_NATIVE_FIRM);
	if (r)
		return r;

	progressSetPos(++p);

	if (getMpInfo() == MPINFO_CTR) {
		r = processFirm(TID_CTR_AGB_FIRM);
		if (r)
			return r;

		progressSetPos(++p);

		r = processFirm(TID_CTR_TWL_FIRM);
		if (r != FR_OK)
			return r;

		progressSetPos(++p);
	}

	return 0;
}

int CheckInstallationData(){
	File file;
	wchar_t str[_MAX_LFN + 1];

	switch (getMpInfo()) {
		case MPINFO_CTR:
			getFirmPath(str, TID_CTR_NATIVE_FIRM);
			if(!FileOpen(&file, str, 0)) return -1;
			FileClose(&file);

			getFirmPath(str, TID_CTR_TWL_FIRM);
			if(!FileOpen(&file, str, 0)) return -2;
			FileClose(&file);

			getFirmPath(str, TID_CTR_AGB_FIRM);
			if(!FileOpen(&file, str, 0)) return -3;
			FileClose(&file);

			break;

		case MPINFO_KTR:
			getFirmPath(str, TID_KTR_NATIVE_FIRM);
			if(!FileOpen(&file, str, 0)) return -1;
			FileClose(&file);

			break;

		default:
			return 0;
	}

	return 0;
}

void InstallConfigData(){
//	wchar_t path[_MAX_LFN + 1];

	if(CheckInstallationData() == 0)
		return;

//	trySetLangFromTheme(0);
	writeCfg();

/*	swprintf(path, _MAX_LFN + 1, L"/rxTools/Theme/%u/cfg0TOP.bin",
		cfgs[CFG_THEME].val.i);
	DrawSplash(&top1Screen, path);
	DrawSplash(&top2Screen, path);
	swprintf(path, _MAX_LFN + 1, L"/rxTools/Theme/%u/cfg0.bin",
		cfgs[CFG_THEME].val.i);
	DrawSplash(&bottomScreen, path);
*/
	/*int res = */
	InstallData();
/*	swprintf(path, _MAX_LFN + 1, L"/rxTools/Theme/%u/cfg1%c.bin",
		cfgs[CFG_THEME].val.i, res == 0 ? 'O' : 'E');
	DrawSplash(&bottomScreen, path);
	swprintf(path, _MAX_LFN + 1, L"/rxTools/Theme/%u/TOP.bin",
		cfgs[CFG_THEME].val.i);
	DrawSplash(&top1Screen, path);
	swprintf(path, _MAX_LFN + 1, L"/rxTools/Theme/%u/TOPL.bin",
		cfgs[CFG_THEME].val.i);
	DrawSplash(&top1Screen, path);
	swprintf(path, _MAX_LFN + 1, L"/rxTools/Theme/%u/TOPR.bin",
		cfgs[CFG_THEME].val.i);
	DrawSplash(&top2Screen, path);
*/
	InputWait();
}
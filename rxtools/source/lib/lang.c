/*
 * Copyright (C) 2015-2016 The PASTA Team, dukesrg
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "lang.h"
#include "fs.h"

//wchar_t strings[STR_NUM][STR_MAX_LEN] = {};
wchar_t strings[STR_NUM][STR_MAX_LEN] = {};


void preloadStringsA()
{
//Strings used in initial loading and diagnostics
	wcscpy(strings[STR_INITIALIZING], L"Initializing...");
	wcscpy(strings[STR_LOADING], L"Loading...");
	wcscpy(strings[STR_FAILED], L"Failed!");
	wcscpy(strings[STR_ERROR_OPENING], L"Error opening %ls!");
	wcscpy(strings[STR_AUTOBOOT], L"Autoboot");
	wcscpy(strings[STR_OPEN_MENU], L"open menu");
	wcscpy(strings[STR_WARNING], L"Warning");
	wcscpy(strings[STR_PRESS_BUTTON_ACTION], L"Press %ls to %ls\n");
	wcscpy(strings[STR_HOLD_BUTTON_ACTION], L"Hold %ls to %ls\n");
	wcscpy(strings[STR_CONTINUE], L"continue");
//Strings with button names
	wcscpy(strings[STR_BUTTON_A], L"[A]");
	wcscpy(strings[STR_BUTTON_B], L"[B]");
	wcscpy(strings[STR_BUTTON_X], L"[X]");
	wcscpy(strings[STR_BUTTON_Y], L"[Y]");
	wcscpy(strings[STR_BUTTON_L], L"[L]");
	wcscpy(strings[STR_BUTTON_R], L"[R]");
	wcscpy(strings[STR_BUTTON_ZL], L"[ZL]");
	wcscpy(strings[STR_BUTTON_ZR], L"[ZR]");
	wcscpy(strings[STR_BUTTON_SELECT], L"[SELECT]");
	wcscpy(strings[STR_BUTTON_HOME], L"[HOME]");
	wcscpy(strings[STR_BUTTON_START], L"[START]");
//Special strings for interface elements
	wcscpy(strings[STR_CURSOR], L"=>");
	wcscpy(strings[STR_NO_CURSOR], L"  ");
	wcscpy(strings[STR_ENABLED], L" +");
	wcscpy(strings[STR_DISABLED], L" -");
	wcscpy(strings[STR_PROGRESS], L"--");
	wcscpy(strings[STR_PROGRESS_OK], L"++");
	wcscpy(strings[STR_PROGRESS_FAIL], L"**");
//Acronyms that most probably won't be translated
	wcscpy(strings[STR_MENU_LANGUAGE], L"Language  %16ls");
	wcscpy(strings[STR_REBOOT], L"Reboot");
	wcscpy(strings[STR_SHUTDOWN], L"Shutdown");
	wcscpy(strings[STR_BLANK_BUTTON_ACTION], L"%ls %ls\n");
	wcscpy(strings[STR_NAND], L"NAND");
	wcscpy(strings[STR_SYSNAND], L"sysNAND");
	wcscpy(strings[STR_EMUNAND], L"emuNAND");
	wcscpy(strings[STR_XORPAD], L"xorpad");
	wcscpy(strings[STR_NAND_XORPAD], L"NAND xorpad");
	wcscpy(strings[STR_EXHEADER], L"ExHeader");
	wcscpy(strings[STR_EXEFS], L"ExeFS");
	wcscpy(strings[STR_ROMFS], L"RomFS");
	wcscpy(strings[STR_TWLN], L"TWLN");
	wcscpy(strings[STR_TWLP], L"TWLP");
	wcscpy(strings[STR_AGB_SAVE], L"AGB_SAVE");
	wcscpy(strings[STR_FIRM0], L"FIRM0");
	wcscpy(strings[STR_FIRM1], L"FIRM1");
	wcscpy(strings[STR_CTRNAND], L"CTRNAND");
	wcscpy(strings[STR_CTR], L"CTR");
	wcscpy(strings[STR_TMD], L"TMD");
	wcscpy(strings[STR_JAPAN], L"Japan");
	wcscpy(strings[STR_USA], L"USA");
	wcscpy(strings[STR_EUROPE], L"Europe");
	wcscpy(strings[STR_CHINA], L"China");
	wcscpy(strings[STR_KOREA], L"Korea");
	wcscpy(strings[STR_TAIWAN], L"Taiwan");
	wcscpy(strings[STR_MSET], L"MSET");
	wcscpy(strings[STR_MSET4], L"MSET 4.x");
	wcscpy(strings[STR_MSET6], L"MSET 6.x");
	wcscpy(strings[STR_FBI], L"FBI");
}
/*
void preloadStringsU()
{
//Strings with special characters available only with unicode font
	wcscpy(strings[STR_BUTTON_A], L"Ⓐ");
	wcscpy(strings[STR_BUTTON_B], L"Ⓑ");
	wcscpy(strings[STR_BUTTON_X], L"Ⓧ");
	wcscpy(strings[STR_BUTTON_Y], L"Ⓨ");
	wcscpy(strings[STR_CURSOR], L"⮞");
	wcscpy(strings[STR_NO_CURSOR], L"　");
	wcscpy(strings[STR_ENABLED], L"✔");
	wcscpy(strings[STR_DISABLED], L"✘");
//	wcscpy(strings[STR_ENABLED], L"⦿");
//	wcscpy(strings[STR_DISABLED], L"⦾");
	wcscpy(strings[STR_PROGRESS], L"⬜");
	wcscpy(strings[STR_PROGRESS_OK], L"⬛");
	wcscpy(strings[STR_PROGRESS_FAIL], L"✖");
}

void switchStrings()
{
		preloadStringsA();
		preloadStringsU();
		loadStrings();
		mbstowcs(strings[STR_LANG_NAME], cfgs[CFG_LANGUAGE].val.s, STR_MAX_LEN);
}

int loadStrings()
{
	const size_t tokenNum = 1 + STR_NUM * 2;
	jsmntok_t t[tokenNum];
	char buf[8192];
	wchar_t path[_MAX_LFN];
	jsmn_parser p;
	unsigned int i, j, k;
	const char *s;
	int l, r, len;
	File fd;

	swprintf(path, _MAX_LFN, langPath, langDir, fontIsLoaded ? cfgs[CFG_LANGUAGE].val.s : "en");
	if (!FileOpen(&fd, path, 0))
		return 1;

	len = FileGetSize(&fd);
	if (len > sizeof(buf))
		return 1;

	FileRead(&fd, buf, len, 0);
	FileClose(&fd);

	jsmn_init(&p);
	r = jsmn_parse(&p, buf, len, t, tokenNum);
	if (r < 0)
		return r;

	for (i = 1; i < r; i++) {
		for (j = 0; j < STR_NUM; j++) {
			s = buf + t[i].start;

			len = t[i].end - t[i].start;
			if (!memcmp(s, keys[j], len) && !keys[j][len]) {
				i++;
				len = t[i].end - t[i].start;
				s = buf + t[i].start;
				for (k = 0; k + 1 < STR_MAX_LEN && len > 0; k++) {
					if (s[0] == '\\' && s[1] == 'n') {
						strings[j][k] = '\n';
						l = 2;
					} else {
						l = mbtowc(strings[j] + k, s, len);
						if (l < 0)
							break;
					}

					len -= l;
					s += l;
				}

				strings[j][k] = 0;
				mbtowc(NULL, NULL, 0);
				break;
			}
		}
	}
	return 0;
}
*/

#define STR_TRANS_CNT 8
static wchar_t wstr[STR_TRANS_CNT][STR_MAX_LEN];
static int itrans = 0;

#define LANG_CODE_NONE		""
#define LANG_CODE_DEFAULT	"en"

static Json langJson, *langJsonInit;
static const wchar_t *langDir, *langPattern;

uint_fast8_t langInit(Json *json, const wchar_t *path, const wchar_t *pattern) {
	return json && json->js && json->tok && (langJsonInit = json)->tok && (langDir = path) && (langPattern = pattern);
}

uint32_t langLoad(char *code, langSeek seek) {
	DIR dir;
	FILINFO fno;
	wchar_t *fn, *pathfn;
	wchar_t path[_MAX_LFN + 1];
	wchar_t targetfn[_MAX_LFN + 1];
	wchar_t prevfn[_MAX_LFN + 1] = L"";
	wchar_t lfn[_MAX_LFN + 1];
	fno.lfname = lfn;
	fno.lfsize = _MAX_LFN + 1;

	if (f_findfirst(&dir, &fno, langDir, langPattern) == FR_OK) {
		swprintf(path, _MAX_LFN + 1, L"%ls/%s%ls", langDir, code, wcsrchr(langPattern, L'.'));
		wcscpy(targetfn, pathfn = path + wcslen(langDir) + 1);
		wcscpy(pathfn, fn = *fno.lfname ? fno.lfname : fno.fname);
		do {
			if (wcscmp((fn = *fno.lfname ? fno.lfname : fno.fname), targetfn) == 0) {
				if (seek == LANG_SET)
					wcscpy(pathfn, targetfn);
				else if (seek == LANG_NEXT && f_findnext(&dir, &fno) == FR_OK && fno.fname[0] != 0)
					wcscpy(pathfn, *fno.lfname ? fno.lfname : fno.fname);
				else if (seek == LANG_PREV && !*prevfn)
					continue;
				break;
			}
			if (seek == LANG_PREV)
				wcscpy(prevfn, fn);
		} while (f_findnext(&dir, &fno) == FR_OK && fno.fname[0]);
		if (seek == LANG_PREV && *prevfn)
			wcscpy(pathfn, prevfn);
			
		langJson = *langJsonInit;
		if (jsonLoad(&langJson, path))
			code[wcstombs(code, pathfn, wcscspn(pathfn, L"."))] = 0;
		else {
			swprintf(path, _MAX_LFN + 1, L"%ls/%s%ls", langDir, LANG_CODE_DEFAULT, wcsrchr(langPattern, L'.'));
			langJson = *langJsonInit;
			strcpy(code, jsonLoad(&langJson, path) > 0 ? LANG_CODE_DEFAULT : LANG_CODE_NONE);
		}
	} else
		langJson.count = 0; //no money - no honey
	f_closedir(&dir);
	return langJson.count;
}

wchar_t *lang(const char *key) {
	return langn(key, -1);
}

wchar_t *langn(const char *key, int keylen) {
	int i, len = 0;
	char str[STR_MAX_LEN] = "";
	if (key != NULL) {
		if (keylen < 0)
			keylen = strlen(key);
		for (i = 1; i < langJson.count; i+=2) {
			len = langJson.tok[i].end - langJson.tok[i].start;
			if (langJson.tok[i].type == JSMN_STRING && keylen == len && memcmp(key, langJson.js + langJson.tok[i].start, len) == 0) {
				len = langJson.tok[i+1].end - langJson.tok[i+1].start;
				if (len > STR_MAX_LEN - 1)
					len = STR_MAX_LEN - 1;
				strncpy(str, langJson.js + langJson.tok[i+1].start, len);
				break;
			}
		}
		if (!*str) {
			if ((len = keylen) > STR_MAX_LEN - 1)
				len = STR_MAX_LEN - 1;
			strncpy(str, key, len);
		}
		str[len] = 0;
	}
	if (itrans >= STR_TRANS_CNT)
		itrans = 0;
	mbstowcs(wstr[itrans], str, len + 1);
	return wstr[itrans++];
}
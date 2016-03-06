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

#include <string.h>
#include <wchar.h>
#include "theme.h"

const wchar_t *themeDir = L"/rxTools/theme";
const wchar_t *themeFile = L"theme.json";
const wchar_t *themePath = L"%ls/%ls/%ls";
wchar_t themeName[_MAX_LFN + 1] = L"";

#define THEME_JSON_SIZE		0x4000
#define THEME_JSON_TOKENS	0x400

char jst[THEME_JSON_SIZE];
jsmntok_t tokt[THEME_JSON_TOKENS];
Json themeJson = {jst, THEME_JSON_SIZE, tokt, THEME_JSON_TOKENS};

themeStyle style, styleDefault = {
	L"",
	L"",
	L"",
	{WHITE, TRANSPARENT},
	{BLACK, WHITE},
	{GREY, TRANSPARENT},
	{BLACK, GREY},
	{WHITE, TRANSPARENT},
	{WHITE, TRANSPARENT}
};

enum {
	TOP1,
	TOP2,
	BOTTOM,
	COLORFG,
	COLORBG,
	SELECTEDFG,
	SELECTEDBG,
	DISABLEDFG,
	DISABLEDBG,
	UNSELECTEDFG,
	UNSELECTEDBG,
	HINTFG,
	HINTBG,
	VALUEFG,
	VALUEBG,
	IDX_COUNT
};

int colorParse(int s, char *key, int *idx, int coloridx);

int themeParse(int s, char *key, int *idx) {
	if (s == themeJson.count || key == NULL)
		return 0;
	if (themeJson.tok[s].type == JSMN_PRIMITIVE || themeJson.tok[s].type == JSMN_STRING)
		return 1;
	else if (themeJson.tok[s].type == JSMN_OBJECT) {
		int i, j = 0, k;
		bool isTarget = false;
		for (i = 0; i < themeJson.tok[s].size; i++) {
			j++;
			if (themeJson.tok[s+j+1].type == JSMN_OBJECT && strncmp(key, themeJson.js + themeJson.tok[s+j].start, themeJson.tok[s+j].end - themeJson.tok[s+j].start) == 0)
				isTarget = true;

			switch (themeJson.js[themeJson.tok[s+j].start]){
				case 'b': //"bottomimg"
					idx[BOTTOM] = s + ++j;
					break;
				case 'c': //"color"
					j += colorParse(s+j+1, key, idx, COLORFG);
					break;
				case 's': //"selected"
					j += colorParse(s+j+1, key, idx, SELECTEDFG);
					break;
				case 'd': //"disabled"
					j += colorParse(s+j+1, key, idx, DISABLEDFG);
					break;
				case 'u': //"unselected"
					j += colorParse(s+j+1, key, idx, UNSELECTEDFG);
					break;
				case 'h': //"hint"
					j += colorParse(s+j+1, key, idx, HINTFG);
					break;
				case 'v': //"value"
					j += colorParse(s+j+1, key, idx, VALUEFG);
					break;
				case 't': //"topimg"
					if (themeJson.tok[s+j+1].type == JSMN_STRING)
						idx[TOP1] = idx[TOP2] = s + j + 1;
					else if (themeJson.tok[s+j+1].type == JSMN_ARRAY && themeJson.tok[s+j+1].size > 0) {
						idx[TOP1] = s + j + 2;
						if (themeJson.tok[s+j+1].size > 1)
							idx[TOP2] = s + j + 3;
					}
					j += themeParse(s+j+1, key, idx);
					break;
				case 'm': //"menu"
				default:
					if (isTarget) { //target object - get member indexes and terminate
						themeParse(s+j+1, key, idx);
						return 0;
					} else {
						int localidx[IDX_COUNT] = {0};
						if ((k = themeParse(s+j+1, key, localidx)) == 0 || themeJson.js[themeJson.tok[s+j].start] == 'm') {
						//object is in target chain - set inherited indexes and terminate or apply root 'menu' for unset parameters
							for (int l = 0; l < IDX_COUNT; l++)
								if (idx[l] == 0)
									idx[l] = localidx[l];
							if (k == 0)
								return 0;
						}
					}
					j += k;
			}
		}
		return j + 1;
	} else if (themeJson.tok[s].type == JSMN_ARRAY) {
		int i, j = 0;
		for (i = 0; i < themeJson.tok[s].size; i++)
			j += themeParse(s+j+1, key, idx);
		return j + 1;
	}
	return 0;
}

int colorParse(int s, char *key, int *idx, int coloridx) {
	if (themeJson.tok[s].type == JSMN_STRING)
		idx[coloridx] = s;
	else if (themeJson.tok[s].type == JSMN_ARRAY && themeJson.tok[s].size > 0) {
		idx[coloridx] = s + 1;
		if (themeJson.tok[s].size > 1)
			idx[coloridx + 1] = s + 2;
	}
	return themeParse(s, key, idx);
}

void setImg(wchar_t *path, int index) {
	if (index > 0) {
		wchar_t fn[_MAX_LFN + 1];
		fn[mbstowcs(fn, themeJson.js + themeJson.tok[index].start, themeJson.tok[index].end - themeJson.tok[index].start)] = 0;
		swprintf(path, _MAX_LFN, themePath, themeDir, themeName, fn);
	}
}

void setColor(uint32_t *color, int index) {
	if (index > 0)
		*color = strtoul(themeJson.js + themeJson.tok[index].start, NULL, 16);
}

void themeStyleSet(char *key) {
	style = styleDefault;
	int idx[IDX_COUNT] = {0};
	themeParse(0, key, idx);
	setImg(style.top1img, idx[TOP1]);
	setImg(style.top2img, idx[TOP2]);
	setImg(style.bottomimg, idx[BOTTOM]);
	setColor(&style.color.fg, idx[COLORFG]);
	setColor(&style.color.bg, idx[COLORBG]);
	setColor(&style.selected.fg, idx[SELECTEDFG]);
	setColor(&style.selected.bg, idx[SELECTEDBG]);
	setColor(&style.disabled.fg, idx[DISABLEDFG]);
	setColor(&style.disabled.bg, idx[DISABLEDBG]);
	setColor(&style.unselected.fg, idx[UNSELECTEDFG]);
	setColor(&style.unselected.bg, idx[UNSELECTEDBG]);
	setColor(&style.hint.fg, idx[HINTFG]);
	setColor(&style.hint.bg, idx[HINTBG]);
	setColor(&style.value.fg, idx[VALUEFG]);
	setColor(&style.value.bg, idx[VALUEBG]);
}

int themeLoad(char *name, themeSeek seek) {
	DIR dir;
	FILINFO fno;
	wchar_t *fn;
	wchar_t path[_MAX_LFN + 1];
	wchar_t pathfn[_MAX_LFN + 1];
	wchar_t targetfn[_MAX_LFN + 1];
	wchar_t prevfn[_MAX_LFN + 1] = L"";
	wchar_t lfn[_MAX_LFN + 1];
	fno.lfname = lfn;
	fno.lfsize = _MAX_LFN + 1;

	targetfn[mbstowcs(targetfn, name, _MAX_LFN + 1)] = 0;

	if (f_findfirst(&dir, &fno, themeDir, L"*") == FR_OK) {
		wcscpy(pathfn, *fno.lfname ? fno.lfname : fno.fname);
		do {
			fn = *fno.lfname ? fno.lfname : fno.fname;
			if (swprintf(path, _MAX_LFN + 1, themePath, themeDir, fn, themeFile) > 0 && f_stat(path, NULL) == FR_OK) {
				if (wcscmp(fn, targetfn) == 0) {
					if (seek == THEME_SET)
						wcscpy(pathfn, targetfn);
					else if (seek == THEME_NEXT) {
						while (f_findnext(&dir, &fno) == FR_OK && *fno.fname) {
							fn = *fno.lfname ? fno.lfname : fno.fname;
							if (swprintf(path, _MAX_LFN + 1, themePath, themeDir, fn, themeFile) > 0 && f_stat(path, NULL) == FR_OK) {
								wcscpy(pathfn, fn);
								break;
							}
						}
					} else if (seek == THEME_PREV && !*prevfn)
						continue;
					break;
				} else if (seek == THEME_PREV)
					wcscpy(prevfn, fn);
			}
		} while (f_findnext(&dir, &fno) == FR_OK && *fno.fname);
		if (seek == THEME_PREV && *prevfn)
			wcscpy(pathfn, prevfn);
		swprintf(path, _MAX_LFN + 1, themePath, themeDir, pathfn, themeFile);
		themeJson = (Json){jst, THEME_JSON_SIZE, tokt, THEME_JSON_TOKENS};
		if (jsonLoad(&themeJson, path) > 0) {
			wcscpy(themeName, pathfn);
			name[wcstombs(name, pathfn, (wcslen(pathfn)+1)*2)] = 0;
		}
	} else
		themeJson.count = 0;
	f_closedir(&dir);
	return themeJson.count;
}
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

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "theme.h"

static Json themeJson, *themeJsonInit;
static const wchar_t *themeDir, *themePattern;

static themeStyle styleDefault;
themeStyle style = {
	L"",
	L"",
	L"",
	{WHITE, TRANSPARENT},
	{0, 0, 0, 24},
	0,
	{WHITE, TRANSPARENT},
	{BLACK, WHITE},
	{GREY, TRANSPARENT},
	{BLACK, GREY},
	{0, 24, 160, 0},
	0,
	{WHITE, TRANSPARENT},
	{160, 24, 0, 0},
	0,
	{WHITE, TRANSPARENT},
	{160, 24, 0, 16},
	0,
	{WHITE, TRANSPARENT},
	{0, 96, 320, 0},
	1,
	L"",
	L"",
	L"",
	{WHITE, TRANSPARENT},
	{16, 132, 288, 32},
	WHITE,
	GREY,
	TRANSPARENT
};

typedef enum {
	OBJ_NONE,
	OBJ_MENU,
	OBJ_CAPTION,
	OBJ_ITEMS,
	OBJ_DESCRIPTION,
	OBJ_VALUE,
	OBJ_ACTIVITY,
	OBJ_GAUGE
} objtype;

enum {
	TOP1,
	TOP2,
	BOTTOM,
	CAPTIONFG,
	CAPTIONBG,
	CAPTIONX,
	CAPTIONY,
	CAPTIONW,
	CAPTIONH,
	CAPTIONALIGN,
	ITEMSCOLORFG,
	ITEMSCOLORBG,
	ITEMSSELECTEDFG,
	ITEMSSELECTEDBG,
	ITEMSDISABLEDFG,
	ITEMSDISABLEDBG,
	ITEMSUNSELECTEDFG,
	ITEMSUNSELECTEDBG,
	ITEMSX,
	ITEMSY,
	ITEMSW,
	ITEMSH,
	ITEMSALIGN,
	DESCRIPTIONFG,
	DESCRIPTIONBG,
	DESCRIPTIONX,
	DESCRIPTIONY,
	DESCRIPTIONW,
	DESCRIPTIONH,
	DESCRIPTIONALIGN,
	VALUEFG,
	VALUEBG,
	VALUEX,
	VALUEY,
	VALUEW,
	VALUEH,
	VALUEALIGN,
	ACTIVITYFG,
	ACTIVITYBG,
	ACTIVITYX,
	ACTIVITYY,
	ACTIVITYW,
	ACTIVITYH,
	ACTIVITYALIGN,
	ACTIVITYTOP1,
	ACTIVITYTOP2,
	ACTIVITYBOTTOM,
	GAUGETEXTFG,
	GAUGETEXTBG,
	GAUGEX,
	GAUGEY,
	GAUGEW,
	GAUGEH,
	GAUGEFRAME,
	GAUGEDONE,
	GAUGEBACK,
	IDX_COUNT
};

static int colorParse(int s, char *key, int *idx, int coloridx);

int themeParse(int s, objtype type, char *key, int *idx) {
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

			switch (themeJson.js[themeJson.tok[s+j].start]) {
				case 'a':
					switch(type) {
						case OBJ_CAPTION:
							idx[CAPTIONALIGN] = s + ++j;
							break;
						case OBJ_ITEMS:
							idx[ITEMSALIGN] = s + ++j;
							break;
						case OBJ_DESCRIPTION:
							idx[DESCRIPTIONALIGN] = s + ++j;
							break;
						case OBJ_VALUE:
							idx[VALUEALIGN] = s + ++j;
							break;
						case OBJ_ACTIVITY: //"align"
							idx[ACTIVITYALIGN] = s + ++j;
							break;
						case OBJ_MENU: //"activity"
							j += themeParse(s+j+1, OBJ_ACTIVITY, key, idx);
							break;
						default:
							j += themeParse(s+j+1, type, key, idx);
					}
					break;
				case 'b': //"bottomimg"
					switch(type) {
						case OBJ_ACTIVITY:
							idx[ACTIVITYBOTTOM] = s + ++j;
							break;
						case OBJ_GAUGE:
							j += colorParse(s+j+1, key, idx, GAUGEBACK);
							break;
						default:
							idx[BOTTOM] = s + ++j;
					}
					break;
				case 'c':
					switch(type) {
						case OBJ_CAPTION: //"color"
							j += colorParse(s+j+1, key, idx, CAPTIONFG);
							break;
						case OBJ_ITEMS:
							j += colorParse(s+j+1, key, idx, ITEMSCOLORFG);
							break;
						case OBJ_DESCRIPTION:
							j += colorParse(s+j+1, key, idx, DESCRIPTIONFG);
							break;
						case OBJ_VALUE:
							j += colorParse(s+j+1, key, idx, VALUEFG);
							break;
						case OBJ_ACTIVITY:
							j += colorParse(s+j+1, key, idx, ACTIVITYFG);
							break;
						case OBJ_GAUGE:
							j += colorParse(s+j+1, key, idx, GAUGETEXTFG);
							break;
						case OBJ_MENU: //"caption"
							j += themeParse(s+j+1, OBJ_CAPTION, key, idx);
							break;
						default:
							j += themeParse(s+j+1, type, key, idx);
					}
					break;
				case 'd':
					switch(type) {
						case OBJ_ITEMS: //"disabled"
							j += colorParse(s+j+1, key, idx, ITEMSDISABLEDFG);
							break;
						case OBJ_GAUGE: //"done"
							j += colorParse(s+j+1, key, idx, GAUGEDONE);
							break;
						default: //"description"
							j += themeParse(s+j+1, OBJ_DESCRIPTION, key, idx);
					}
					break;
				case 'f':
					switch(type) {
						case OBJ_GAUGE:
							j += colorParse(s+j+1, key, idx, GAUGEFRAME);
							break;
						default:
							j += themeParse(s+j+1, type, key, idx);
					}
					break;
				case 'g':
					switch(type) {
						case OBJ_MENU: //"gauge"
							j += themeParse(s+j+1, OBJ_GAUGE, key, idx);
							break;
						default:
							j += themeParse(s+j+1, type, key, idx);
					}
					break;
				case 'i': //"items"
					j += themeParse(s+j+1, OBJ_ITEMS, key, idx);
					break;
				case 's': //"selected"
					j += colorParse(s+j+1, key, idx, ITEMSSELECTEDFG);
					break;
				case 'u': //"unselected"
					j += colorParse(s+j+1, key, idx, ITEMSUNSELECTEDFG);
					break;
/*				case 'h': //"hint"
					j += colorParse(s+j+1, key, idx, DESCRIPTIONFG);
					break;
*/				case 'v': //"value"
					j += themeParse(s+j+1, OBJ_VALUE, key, idx);
					break;
				case 't': //"topimg"
					if (themeJson.tok[s+j+1].type == JSMN_STRING)
						switch(type) {
							case OBJ_ACTIVITY:
								idx[ACTIVITYTOP1] = idx[ACTIVITYTOP2] = s + j + 1;
								break;
							default:
							idx[TOP1] = idx[TOP2] = s + j + 1;
						}
					else if (themeJson.tok[s+j+1].type == JSMN_ARRAY && themeJson.tok[s+j+1].size > 0) {
						switch(type) {
							case OBJ_ACTIVITY:
								idx[ACTIVITYTOP1] = s + j + 2;
								if (themeJson.tok[s+j+1].size > 1)
									idx[ACTIVITYTOP2] = s + j + 3;
								break;
							default:
							idx[TOP1] = s + j + 2;
							if (themeJson.tok[s+j+1].size > 1)
								idx[TOP2] = s + j + 3;
						}
					}
					j += themeParse(s+j+1, type, key, idx);
					break;
				case 'h': //"height"
					switch(type) {
						case OBJ_CAPTION:
							idx[CAPTIONH] = s + ++j;
							break;
						case OBJ_ITEMS:
							idx[ITEMSH] = s + ++j;
							break;
						case OBJ_DESCRIPTION:
							idx[DESCRIPTIONH] = s + ++j;
							break;
						case OBJ_VALUE:
							idx[VALUEH] = s + ++j;
							break;
						case OBJ_ACTIVITY:
							idx[ACTIVITYH] = s + ++j;
							break;
						case OBJ_GAUGE:
							idx[GAUGEH] = s + ++j;
							break;
						default:
							j += themeParse(s+j+1, type, key, idx);
					}
					break;
				case 'w': //"width"
					switch(type) {
						case OBJ_CAPTION:
							idx[CAPTIONW] = s + ++j;
							break;
						case OBJ_ITEMS:
							idx[ITEMSW] = s + ++j;
							break;
						case OBJ_DESCRIPTION:
							idx[DESCRIPTIONW] = s + ++j;
							break;
						case OBJ_VALUE:
							idx[VALUEW] = s + ++j;
							break;
						case OBJ_ACTIVITY:
							idx[ACTIVITYW] = s + ++j;
							break;
						case OBJ_GAUGE:
							idx[GAUGEW] = s + ++j;
							break;
						default:
							j += themeParse(s+j+1, type, key, idx);
					}
					break;
				case 'x': //"x"
					switch(type) {
						case OBJ_CAPTION:
							idx[CAPTIONX] = s + ++j;
							break;
						case OBJ_ITEMS:
							idx[ITEMSX] = s + ++j;
							break;
						case OBJ_DESCRIPTION:
							idx[DESCRIPTIONX] = s + ++j;
							break;
						case OBJ_VALUE:
							idx[VALUEX] = s + ++j;
							break;
						case OBJ_ACTIVITY:
							idx[ACTIVITYX] = s + ++j;
							break;
						case OBJ_GAUGE:
							idx[GAUGEX] = s + ++j;
							break;
						default:
							j += themeParse(s+j+1, type, key, idx);
					}
					break;
				case 'y': //"y"
					switch(type) {
						case OBJ_CAPTION:
							idx[CAPTIONY] = s + ++j;
							break;
						case OBJ_ITEMS:
							idx[ITEMSY] = s + ++j;
							break;
						case OBJ_DESCRIPTION:
							idx[DESCRIPTIONY] = s + ++j;
							break;
						case OBJ_VALUE:
							idx[VALUEY] = s + ++j;
							break;
						case OBJ_ACTIVITY:
							idx[ACTIVITYY] = s + ++j;
							break;
						case OBJ_GAUGE:
							idx[GAUGEY] = s + ++j;
							break;
						default:
							j += themeParse(s+j+1, type, key, idx);
					}
					break;
				case 'm': //"menu"
					type = OBJ_MENU;
				default:
					if (isTarget) { //target object - get member indexes and terminate
						themeParse(s+j+1, type, key, idx);
						return 0;
					} else {
						int localidx[IDX_COUNT] = {0};
						if ((k = themeParse(s+j+1, type, key, localidx)) == 0 || themeJson.js[themeJson.tok[s+j].start] == 'm') {
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
			j += themeParse(s+j+1, type, key, idx);
		return j + 1;
	}
	return 0;
}

static int colorParse(int s, char *key, int *idx, int coloridx) {
	if (themeJson.tok[s].type == JSMN_STRING)
		idx[coloridx] = s;
	else if (themeJson.tok[s].type == JSMN_ARRAY && themeJson.tok[s].size > 0) {
		idx[coloridx] = s + 1;
		if (themeJson.tok[s].size > 1)
			idx[coloridx + 1] = s + 2;
	}
	return themeParse(s, OBJ_NONE, key, idx);
}

static void setImg(wchar_t *path, int index) {
	if (index > 0) {
		wchar_t fn[_MAX_LFN + 1];
		fn[mbstowcs(fn, themeJson.js + themeJson.tok[index].start, themeJson.tok[index].end - themeJson.tok[index].start)] = 0;
		swprintf(path, _MAX_LFN + 1, L"%ls/%ls", themeDir, fn);
	}
}

static void setColor(uint32_t *color, int index) {
	if (index > 0)
		*color = strtoul(themeJson.js + themeJson.tok[index].start, NULL, 16);
}

static void setInt(uint_fast16_t *val, int index) {
	if (index > 0)
		*val = strtoul(themeJson.js + themeJson.tok[index].start, NULL, 0);
}

void themeStyleSet(char *key) {
	style = styleDefault;
	int idx[IDX_COUNT] = {0};
	themeParse(0, OBJ_NONE, key, idx);
	setImg(style.top1img, idx[TOP1]);
	setImg(style.top2img, idx[TOP2]);
	setImg(style.bottomimg, idx[BOTTOM]);
	setImg(style.activitytop1img, idx[ACTIVITYTOP1]);
	setImg(style.activitytop2img, idx[ACTIVITYTOP2]);
	setImg(style.activitybottomimg, idx[ACTIVITYBOTTOM]);
	setColor(&style.captionColor.fg.color, idx[CAPTIONFG]);
	setColor(&style.captionColor.bg.color, idx[CAPTIONBG]);
	setColor(&style.itemsColor.fg.color, idx[ITEMSCOLORFG]);
	setColor(&style.itemsColor.bg.color, idx[ITEMSCOLORBG]);
	setColor(&style.itemsSelected.fg.color, idx[ITEMSSELECTEDFG]);
	setColor(&style.itemsSelected.bg.color, idx[ITEMSSELECTEDBG]);
	setColor(&style.itemsDisabled.fg.color, idx[ITEMSDISABLEDFG]);
	setColor(&style.itemsDisabled.bg.color, idx[ITEMSDISABLEDBG]);
	setColor(&style.itemsUnselected.fg.color, idx[ITEMSUNSELECTEDFG]);
	setColor(&style.itemsUnselected.bg.color, idx[ITEMSUNSELECTEDBG]);
	setColor(&style.descriptionColor.fg.color, idx[DESCRIPTIONFG]);
	setColor(&style.descriptionColor.bg.color, idx[DESCRIPTIONBG]);
	setColor(&style.valueColor.fg.color, idx[VALUEFG]);
	setColor(&style.valueColor.bg.color, idx[VALUEBG]);
	setColor(&style.activityColor.fg.color, idx[ACTIVITYFG]);
	setColor(&style.activityColor.bg.color, idx[ACTIVITYBG]);
	setColor(&style.gaugeTextColor.fg.color, idx[GAUGETEXTFG]);
	setColor(&style.gaugeTextColor.bg.color, idx[GAUGETEXTBG]);
	setColor(&style.gaugeFrameColor.color, idx[GAUGEFRAME]);
	setColor(&style.gaugeDoneColor.color, idx[GAUGEDONE]);
	setColor(&style.gaugeBackColor.color, idx[GAUGEBACK]);
	setInt(&style.captionRect.x, idx[CAPTIONX]);
	setInt(&style.captionRect.y, idx[CAPTIONY]);
	setInt(&style.captionRect.w, idx[CAPTIONW]);
	setInt(&style.captionRect.h, idx[CAPTIONH]);
	setInt(&style.captionAlign, idx[CAPTIONALIGN]);
	setInt(&style.itemsRect.x, idx[ITEMSX]);
	setInt(&style.itemsRect.y, idx[ITEMSY]);
	setInt(&style.itemsRect.w, idx[ITEMSW]);
	setInt(&style.itemsRect.h, idx[ITEMSH]);
	setInt(&style.itemsAlign, idx[ITEMSALIGN]);
	setInt(&style.descriptionRect.x, idx[DESCRIPTIONX]);
	setInt(&style.descriptionRect.y, idx[DESCRIPTIONY]);
	setInt(&style.descriptionRect.w, idx[DESCRIPTIONW]);
	setInt(&style.descriptionRect.h, idx[DESCRIPTIONH]);
	setInt(&style.descriptionAlign, idx[DESCRIPTIONALIGN]);
	setInt(&style.valueRect.x, idx[VALUEX]);
	setInt(&style.valueRect.y, idx[VALUEY]);
	setInt(&style.valueRect.w, idx[VALUEW]);
	setInt(&style.valueRect.h, idx[VALUEH]);
	setInt(&style.valueAlign, idx[VALUEALIGN]);
	setInt(&style.activityRect.x, idx[ACTIVITYX]);
	setInt(&style.activityRect.y, idx[ACTIVITYY]);
	setInt(&style.activityRect.w, idx[ACTIVITYW]);
	setInt(&style.activityRect.h, idx[ACTIVITYH]);
	setInt(&style.activityAlign, idx[ACTIVITYALIGN]);
	setInt(&style.gaugeRect.x, idx[GAUGEX]);
	setInt(&style.gaugeRect.y, idx[GAUGEY]);
	setInt(&style.gaugeRect.w, idx[GAUGEW]);
	setInt(&style.gaugeRect.h, idx[GAUGEH]);
}

bool themeInit(Json *json, const wchar_t *path, const wchar_t *pattern) {
	styleDefault = style;
	return json && json->js && json->tok && (themeJsonInit = json)->tok && (themeDir = path) && (themePattern = pattern);
}

int themeLoad(char *name, themeSeek seek) {
	DIR dir;
	FILINFO fno;
	wchar_t *fn, *pathfn;
	wchar_t path[_MAX_LFN + 1];
	wchar_t targetfn[_MAX_LFN + 1];
	wchar_t prevfn[_MAX_LFN + 1] = L"";
	wchar_t lfn[_MAX_LFN + 1];
	fno.lfname = lfn;
	fno.lfsize = _MAX_LFN + 1;

	if (f_findfirst(&dir, &fno, themeDir, themePattern) == FR_OK) {
		swprintf(path, _MAX_LFN + 1, L"%ls/%s%ls", themeDir, name, wcsrchr(themePattern, L'.'));
		wcscpy(targetfn, pathfn = path + wcslen(themeDir) + 1);
		wcscpy(pathfn, fn = *fno.lfname ? fno.lfname : fno.fname);
		do {
			if (wcscmp((fn = *fno.lfname ? fno.lfname : fno.fname), targetfn) == 0) {
				if (seek == THEME_SET)
					wcscpy(pathfn, targetfn);
				else if (seek == THEME_NEXT && f_findnext(&dir, &fno) == FR_OK && fno.fname[0] != 0)
					wcscpy(pathfn, *fno.lfname ? fno.lfname : fno.fname);
				else if (seek == THEME_PREV && !*prevfn)
					continue;
				break;
			}
			if (seek == THEME_PREV)
				wcscpy(prevfn, fn);
		} while (f_findnext(&dir, &fno) == FR_OK && fno.fname[0]);
		if (seek == THEME_PREV && *prevfn)
			wcscpy(pathfn, prevfn);

		themeJson = *themeJsonInit;
		if (jsonLoad(&themeJson, path)) {
			wcstombs(name, pathfn, 31);
			*strrchr(name, '.') = 0;
		}
	} else
		themeJson.count = 0; //no money - no honey

	f_closedir(&dir);
	return themeJson.count;
}
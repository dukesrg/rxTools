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

#ifndef THEME_H

#include <stdint.h>
#include "json.h"
#include "fs.h"
#include "draw.h"

typedef enum {
	THEME_SET,
	THEME_NEXT,
	THEME_PREV
} themeSeek;

typedef struct {
	wchar_t top1img[_MAX_LFN + 1];
	wchar_t top2img[_MAX_LFN + 1];
	wchar_t bottomimg[_MAX_LFN + 1];
	Color captionColor;
	Rect captionRect;
	uint_fast16_t captionAlign;
	Color itemsColor;
	Color itemsSelected;
	Color itemsDisabled;
	Color itemsUnselected;
	Rect itemsRect;
	uint_fast16_t itemsAlign;
	Color descriptionColor;
	Rect descriptionRect;
	uint_fast16_t descriptionAlign;
	Color valueColor;
	Rect valueRect;
	uint_fast16_t valueAlign;
	Color activityColor;
	Rect activityRect;
	uint_fast16_t activityAlign;
	Rect activityStatusRect;
	uint_fast16_t activityStatusAlign;
	wchar_t activitytop1img[_MAX_LFN + 1];
	wchar_t activitytop2img[_MAX_LFN + 1];
	wchar_t activitybottomimg[_MAX_LFN + 1];
	Color gaugeTextColor;
	Rect gaugeRect;
	Color gaugeFrameColor;
	Color gaugeDoneColor;
	Color gaugeBackColor;
} themeStyle;

extern themeStyle style;

uint_fast8_t themeInit(Json *json, const wchar_t *path, const wchar_t *pattern);
uint32_t themeLoad(char *name, themeSeek seek);
void themeStyleSet(char *key);

#endif
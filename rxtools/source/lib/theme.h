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
	TextColors color;
	TextColors selected;
	TextColors disabled;
	TextColors unselected;
	TextColors hint;
	TextColors value;
} themeStyle;

extern themeStyle style;

int themeLoad(char *name, themeSeek seek);
void themeStyleSet(char *key);

#endif
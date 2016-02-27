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

#pragma once

#include "json.h"
#include "fs.h"
#include "draw.h"

#define THEME_JSON_SIZE		0x4000
#define THEME_JSON_TOKENS	0x400

typedef struct {
	wchar_t top1img[_MAX_LFN];
	wchar_t top2img[_MAX_LFN];
	wchar_t bottomimg[_MAX_LFN];
	TextColors color;
	TextColors selected;
	TextColors disabled;
	TextColors unselected;
	TextColors hint;
	TextColors value;
} themeStyle;

extern const wchar_t *themePath;
extern const char *themeFile;
extern Json themeJson;
extern themeStyle style;

void themeSet(int themeNum, char *key);
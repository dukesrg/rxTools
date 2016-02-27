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

#include <stdbool.h>
#include <wchar.h>
#include "console.h"
#include "json.h"

#define MENU_MASK_SELECTED	0x01
#define MENU_MASK_DISABLED	0x02
#define MENU_STATE_COUNT	4

#define MENU_JSON_SIZE		0x4000
#define MENU_JSON_TOKENS	0x400

extern Json menuJson;
extern const wchar_t *menuPath;

typedef struct{
	char Str[CONSOLE_MAX_LINE_LENGTH+1];
	void( *Func)();
	wchar_t *gfx_splash;
	TextColors color[MENU_STATE_COUNT];
} MenuEntry;

typedef struct{
	char Name[CONSOLE_MAX_LINE_LENGTH+1];
	MenuEntry *Option;
	int nEntryes;
	int Current;    //The current selected option
	bool Showed;    //Useful, to not refresh everything everytime
} Menu;

void MenuInit(Menu* menu);
void MenuShow();
void MenuNextSelection();
void MenuPrevSelection();
void MenuSelect();
void MenuClose();
void MenuRefresh();

extern bool bootGUI;
extern unsigned char Theme;
extern bool agb_bios;
extern bool theme_3d;
extern unsigned char language;
extern Menu* MyMenu;

int menuTry(int targetposition, int currentposition);
int menuLevel(int pos);

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
#include <stdlib.h>
#include <stdio.h>
#include "configuration.h"
#include "lang.h"
#include "menu.h"
#include "draw.h"
#include "hid.h"
#include "jsmn/jsmn.h"
#include "fs.h"
#include "json.h"

Menu* MyMenu;
Menu *MenuChain[100];
int openedMenus = 0;
int saved_index = 0;

char jsm[LANG_JSON_SIZE];
jsmntok_t tokm[LANG_JSON_TOKENS];
Json menuJson = {jsm, MENU_JSON_SIZE, tokm, MENU_JSON_TOKENS};
const TCHAR *menuPath = _T("") SYS_PATH "/gui.json";

#define MENU_MAX_LEVELS 4
#define MENU_LEVEL_BIT_WIDTH 8*sizeof(unsigned int)/MENU_MAX_LEVELS

typedef enum {
	OBJ_NONE,
	OBJ_MENU,
} objtype;

typedef enum{
	APPLY_NONE,
	APPLY_INHERIT,
	APPLY_TARGET
} menuapply;

int menuPosition;
int idescr, ihead;

void MenuInit(Menu* menu){
	MyMenu = menu;
	ConsoleInit();
	if (openedMenus == 0) MyMenu->Current = saved_index; //if we're entering the main menu, load the index
	else  MyMenu->Current = 0;
    MyMenu->Showed = 0;
	ConsoleSetTitle(lang(MyMenu->Name, -1));
	for(int i = 0; i < MyMenu->nEntryes; i++){
		ConsoleAddText(lang(MyMenu->Option[i].Str, -1));
	}
}

TextColors itemColor = {WHITE, TRANSPARENT},
	selectedColor = {BLACK, WHITE},
	descriptionColor = {YELLOW, TRANSPARENT};

void MenuShow(){
	wchar_t str[_MAX_LFN];

	//OLD TEXT MENU:
	/*int x = 0, y = 0;
	ConsoleGetXY(&x, &y);
	if (!MyMenu->Showed){
		sprintf(str, "/rxTools/Theme/%c/app.bin", Theme);
		DrawBottomSplash(str);
		ConsoleShow();
		MyMenu->Showed = 1;
	}
	for (int i = 0; i < MyMenu->nEntryes; i++){
		DrawString(BOT_SCREEN, i == MyMenu->Current ? L">" : L" ", x + CHAR_WIDTH*(ConsoleGetSpacing() - 1), (i)* CHAR_WIDTH + y + CHAR_WIDTH*(ConsoleGetSpacing() + 1), ConsoleGetSpecialColor(), ConsoleGetBackgroundColor());
	}*/

	//NEW GUI:
	/*
	swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/%ls",
		cfgs[CFG_THEME].val.i, MyMenu->Option[MyMenu->Current].gfx_splash);
	DrawBottomSplash(str);
	*/
	//NEW TEXT GUI :)
	swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/%ls",
		cfgs[CFG_THEME].val.i, MyMenu->Option[MyMenu->Current].gfx_splash);
	DrawSplash(&bottomTmpScreen, str);
	DrawStringRect(&bottomTmpScreen, lang(MyMenu->Name, -1), 0, 0, font24.h, 0, 0, &selectedColor, &font24);
//	DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[ihead].start, menuJson.tok[ihead].end - menuJson.tok[ihead].start), 0, 0, font24.h, 0, 0, &descriptionColor, &font24);
	for (int i = 0; i < MyMenu->nEntryes; i++)
		DrawStringRect(&bottomTmpScreen, lang(MyMenu->Option[i].Str, -1), 0, 0, font24.h + font16.h + font16.h * i, bottomTmpScreen.w / 2, 0, i == MyMenu->Current ? &selectedColor : &itemColor, &font16);
	DrawStringRect(&bottomTmpScreen, lang(menuJson.js + menuJson.tok[idescr].start, menuJson.tok[idescr].end - menuJson.tok[idescr].start), 0, bottomTmpScreen.w / 2, font24.h + font16.h, 0, 0, &descriptionColor, &font16);
	memcpy(bottomScreen.addr, bottomTmpScreen.addr, bottomScreen.size);
}

void MenuNextSelection(){
	menuPosition = menuNext(menuPosition);
	if(MyMenu->Current + 1 < MyMenu->nEntryes){
		MyMenu->Current++;
	}else{
		MyMenu->Current = 0;
	}

}

void MenuPrevSelection(){
	menuPosition = menuPrev(menuPosition);
	if(MyMenu->Current > 0){
		MyMenu->Current--;
	}else{
		MyMenu->Current = MyMenu->nEntryes - 1;
	}
}

void MenuSelect(){
	menuPosition = menuDown(menuPosition);
	if(MyMenu->Option[MyMenu->Current].Func != NULL){
		if (openedMenus == 0)saved_index = MyMenu->Current; //if leaving the main menu, save the index
		MenuChain[openedMenus++] = MyMenu;
		MyMenu->Option[MyMenu->Current].Func();
		MenuInit(MenuChain[--openedMenus]);
		MenuShow();
	}
}

void MenuClose(){
	menuPosition = menuUp(menuPosition);
	if (openedMenus > 0){
		OpenAnimation();
	}
}

void MenuRefresh(){
	ConsoleInit();
	MyMenu->Showed = 0;
	ConsoleSetTitle(lang(MyMenu->Name, -1));
	for (int i = 0; i < MyMenu->nEntryes; i++){
		print(L"%ls %ls\n", i == MyMenu->Current ? strings[STR_CURSOR] : strings[STR_NO_CURSOR], lang(MyMenu->Option[i].Str, -1));
	}
	int x = 0, y = 0;
	ConsoleGetXY(&x, &y);
	if (!MyMenu->Showed){
		ConsoleShow();
		MyMenu->Showed = 1;
	}
}

//New menu system

void menuInit(){
	menuPosition = menuDown(0);
}

int menuParse(int s, objtype type, int menulevel, int menuposition, int targetposition, int *foundposition) {
	if (s == menuJson.count)
		return 0;
	if (menuJson.tok[s].type == JSMN_PRIMITIVE || menuJson.tok[s].type == JSMN_STRING)
		return 1;
	else if (menuJson.tok[s].type == JSMN_OBJECT) {
		int mask, i, j = 0, k;
		menuapply apply = APPLY_NONE;
		if (type == OBJ_MENU)
			menulevel++;
		for (i = 0; i < menuJson.tok[s].size; i++) {
			j++;
			if (menuJson.tok[s+j+1].type == JSMN_OBJECT && menulevel > 0){
				menuposition += 1 << ((MENU_MAX_LEVELS - menulevel) * MENU_LEVEL_BIT_WIDTH);
				mask = (0xffffffff << (8 * sizeof(unsigned int) - menulevel * MENU_LEVEL_BIT_WIDTH));
				if((menuposition & mask) == (targetposition & mask)){
//					printf("menu=%.*s(%d)\n", t[j].end - t[j].start, js+t[j].start, t[j+1].size);
					apply = APPLY_INHERIT;
					if (menuposition == targetposition)
						*foundposition = menuposition; //at least, we found parent menu position
				}
			}else{
				mask = (0xffffffff << (8 * sizeof(unsigned int) - (menulevel-1) * MENU_LEVEL_BIT_WIDTH));
				if((menuposition & mask) == (targetposition & mask))
					apply = APPLY_INHERIT;
			}
			if(apply == APPLY_INHERIT && menuposition == targetposition)
				apply = APPLY_TARGET;

			switch (menuJson.js[menuJson.tok[s+j].start]){
				case 'c': //"caption"
					j++;
//					if (apply == APPLY_INHERIT)
//						ihead = menuJson.count - count + j;
//						printf("caption=%.*s\n", t[j].end - t[j].start, js+t[j].start);
					break;
				case 'd': //"description"
					j++;
					if (apply == APPLY_TARGET)
//						idescr = menuJson.count - count + j;
						idescr = s + j;
//						printf("description=%.*s\n", t[j].end - t[j].start, js+t[j].start);
					break;
				case 'f': //"function"
					j++;
//					if (apply == APPLY_TARGET || apply == APPLY_INHERIT)
//						printf("function=%.*s\n", t[j].end - t[j].start, js+t[j].start);
					break;
				case 'm': //"menu"
					k = menuParse(s+j+1, OBJ_MENU, menulevel, menuposition, targetposition, foundposition);
					if (k == 0)
						return 0;
					j += k;
					break;
				default:
					k = menuParse(s+j+1, type, menulevel, menuposition, targetposition, foundposition);
					if (k == 0)
						return 0;
					j += k;
			}
		}
		if (menuposition >= targetposition)
			return 0;
		return j+1;
	} else if (menuJson.tok[s].type == JSMN_ARRAY) {
		int i, j = 0;
		for (i = 0; i < menuJson.tok[s].size; i++)
			j += menuParse(s+j+1, type, menulevel, menuposition, targetposition, foundposition);
		return j+1;
	}
	return 0;
}

int menuTry(int targetposition, int currentposition) {
	int foundposition = 0;
	menuParse(0, OBJ_NONE, 0, 0, targetposition, &foundposition);
	if (foundposition == 0)
		menuParse(0, OBJ_NONE, 0, 0, currentposition, &foundposition);
	return foundposition;
}

int menuLevel(int pos) {
	int i;
	for (i = 0; pos != 0; pos <<= MENU_LEVEL_BIT_WIDTH, i++);
	return i;
}

int menuPrev(int pos) {
	int level = (sizeof(pos) * 8 - menuLevel(pos) * MENU_LEVEL_BIT_WIDTH);
	int newpos = pos - (1 << level);
	if ((newpos & (((1 << MENU_LEVEL_BIT_WIDTH) - 1) << level)) == 0)
		newpos = pos;
	return menuTry(newpos, pos);
}

int menuNext(int pos) {
	int level = (sizeof(pos) * 8 - menuLevel(pos) * MENU_LEVEL_BIT_WIDTH);
	int newpos = pos + (1 << level);
	if ((newpos & (((1 << MENU_LEVEL_BIT_WIDTH) - 1) << level)) == 0)
		newpos = pos;
	return menuTry(newpos, pos);
}

int menuUp(int pos) {
	int level = (sizeof(pos) * 8 - menuLevel(pos) * MENU_LEVEL_BIT_WIDTH);
	int mask = ((1 << MENU_LEVEL_BIT_WIDTH) - 1) << level;
	int newpos = pos & ~mask;
	if (newpos == 0)
		newpos = pos;
	return menuTry(newpos, pos);
}

int menuDown(int pos) {
	return menuTry(pos + (1 << (sizeof(pos) * 8 - (menuLevel(pos) + 1) * MENU_LEVEL_BIT_WIDTH)), pos);
}

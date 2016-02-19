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

Menu* MyMenu;
Menu *MenuChain[100];
int openedMenus = 0;
int saved_index = 0;

#define TOKEN_NUM_M 0x200
char js_m[0x2000];
jsmntok_t tok_m[TOKEN_NUM_M];
int r_m;

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


void MenuInit(Menu* menu){
	MyMenu = menu;
	ConsoleInit();
	if (openedMenus == 0) MyMenu->Current = saved_index; //if we're entering the main menu, load the index
	else  MyMenu->Current = 0;
    MyMenu->Showed = 0;
	ConsoleSetTitle(lang(MyMenu->Name));
	for(int i = 0; i < MyMenu->nEntryes; i++){
		ConsoleAddText(lang(MyMenu->Option[i].Str));
	}
}

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
	TextColors itemColor = {WHITE, TRANSPARENT},
		selectedColor = {BLACK, WHITE};
	Screen tmpScreen = bottomScreen;
	tmpScreen.addr = (uint8_t*)0x27000000;
	swprintf(str, _MAX_LFN, L"/rxTools/Theme/%u/%ls",
		cfgs[CFG_THEME].val.i, MyMenu->Option[MyMenu->Current].gfx_splash);
	DrawSplash(&tmpScreen, str);
	DrawStringRect(&tmpScreen, lang(MyMenu->Name), 0, 0, font24.h, 0, 0, &selectedColor, &font24);
	for (int i = 0; i < MyMenu->nEntryes; i++)
		DrawStringRect(&tmpScreen, lang(MyMenu->Option[i].Str), 0, 0, font24.h + font16.h + font16.h * i, tmpScreen.w / 2, 0, i == MyMenu->Current ? &selectedColor : &itemColor, &font16);
	memcpy(bottomScreen.addr, tmpScreen.addr, bottomScreen.size);
}

void MenuNextSelection(){
	if(MyMenu->Current + 1 < MyMenu->nEntryes){
		MyMenu->Current++;
	}else{
		MyMenu->Current = 0;
	}

}

void MenuPrevSelection(){
	if(MyMenu->Current > 0){
		MyMenu->Current--;
	}else{
		MyMenu->Current = MyMenu->nEntryes - 1;
	}
}

void MenuSelect(){
	if(MyMenu->Option[MyMenu->Current].Func != NULL){
		if (openedMenus == 0)saved_index = MyMenu->Current; //if leaving the main menu, save the index
		MenuChain[openedMenus++] = MyMenu;
		MyMenu->Option[MyMenu->Current].Func();
		MenuInit(MenuChain[--openedMenus]);
		MenuShow();
	}
}

void MenuClose(){
	if (openedMenus > 0){
		OpenAnimation();
	}
}

void MenuRefresh(){
	ConsoleInit();
	MyMenu->Showed = 0;
	ConsoleSetTitle(lang(MyMenu->Name));
	for (int i = 0; i < MyMenu->nEntryes; i++){
		print(L"%ls %ls\n", i == MyMenu->Current ? strings[STR_CURSOR] : strings[STR_NO_CURSOR], MyMenu->Option[i].Str);
	}
	int x = 0, y = 0;
	ConsoleGetXY(&x, &y);
	if (!MyMenu->Showed){
		ConsoleShow();
		MyMenu->Showed = 1;
	}
}

//New menu system
int menuLoad()
{
	jsmn_parser p;
	int len;
	File fd;

	if (!FileOpen(&fd, L"/rxTools/sys/gui.json", 0))
		return -1;

	len = FileGetSize(&fd);
	if (len > sizeof(js_m))
		return -1;

	FileRead(&fd, js_m, len, 0);
	FileClose(&fd);

	jsmn_init(&p);
	r_m = jsmn_parse(&p, js_m, len, tok_m, TOKEN_NUM_M);
	return r_m;
}

int menuParse(jsmntok_t *t, size_t count, objtype type, int menulevel, int menuposition, int targetposition, int *foundposition) {
	if (count == 0)
		return 0;
	if (t->type == JSMN_PRIMITIVE || t->type == JSMN_STRING)
		return 1;
	else if (t->type == JSMN_OBJECT) {
		int mask, i, j = 0, k;
		menuapply apply = APPLY_NONE;
		if (type == OBJ_MENU)
			menulevel++;
		for (i = 0; i < t->size; i++) {
			j++;
			if (t[j+1].type == JSMN_OBJECT && menulevel > 0){
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

			switch (*(js_m + t[j].start)){
				case 'c': //"caption"
					j++;
					if (apply == APPLY_TARGET || apply == APPLY_INHERIT)
//						printf("caption=%.*s\n", t[j].end - t[j].start, js+t[j].start);
					break;
				case 'd': //"description"
					j++;
					if (apply == APPLY_TARGET || apply == APPLY_INHERIT)
//						printf("description=%.*s\n", t[j].end - t[j].start, js+t[j].start);
					break;
				case 'f': //"function"
					j++;
					if (apply == APPLY_TARGET || apply == APPLY_INHERIT)
//						printf("function=%.*s\n", t[j].end - t[j].start, js+t[j].start);
					break;
				case 'm': //"menu"
					k = menuParse(t+j+1, count-j, OBJ_MENU, menulevel, menuposition, targetposition, foundposition);
					if (k == 0)
						return 0;
					j += k;
					break;
				default:
					k = menuParse(t+j+1, count-j, type, menulevel, menuposition, targetposition, foundposition);
					if (k == 0)
						return 0;
					j += k;
			}
		}
		if (menuposition >= targetposition)
			return 0;
		return j+1;
	} else if (t->type == JSMN_ARRAY) {
		int i, j = 0;
		for (i = 0; i < t->size; i++)
			j += menuParse(t+j+1, count-j, type, menulevel, menuposition, targetposition, foundposition);
		return j+1;
	}
	return 0;
}

int menuTry(int targetposition, int currentposition) {
	int foundposition = 0;
	menuParse(tok_m, r_m, OBJ_NONE, 0, 0, targetposition, &foundposition);
	if (foundposition == 0)
		menuParse(tok_m, r_m, OBJ_NONE, 0, 0, currentposition, &foundposition);
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

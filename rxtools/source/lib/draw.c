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
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include "fatfs/ff.h"
#include "fs.h"
#include "draw.h"
#include "lang.h"

uint32_t current_y = 1;

uint8_t *tmpscreen = (uint8_t*)0x26000000;
extern uint32_t _binary_font_ascii_bin_start[];
FontMetrics font16 = {8, 16, 16, 0x2000, _binary_font_ascii_bin_start}; //default font metrics
FontMetrics font24 = {8, 16, 16, 0x2000, _binary_font_ascii_bin_start}; //use default in case unicode glyps won't be loaded
Screen top1Screen = {320, 240, 3, 320*240*3, (uint8_t*)0x080FFFC0};
Screen top2Screen = {400, 240, 3, 400*240*3, (uint8_t*)0x080FFFC8};
Screen bottomScreen = {400, 240, 3, 400*240*3, (uint8_t*)0x080FFFD0};

void ClearScreen(Screen *screen, uint32_t color)
{
	uint32_t i = screen->size/sizeof(uint32_t);  //Surely this is an interger.
	uint32_t* tmpscr = (uint32_t*)screen->addr; //To avoid using array index which would decrease speed.
	color &= COLOR_MASK; //Ignore aplha
	//Prepared 3 uint32_t, that includes 4 24-bits color, cached. 4x(BGR)
	uint32_t color0 = (color) | (color << 24),
		color1 = (color << 16) | (color >> 8),
		color2 = (color >> 16) | (color << 8);
	while (i--) {
		*(tmpscr++) = color0;
		*(tmpscr++) = color1;
		*(tmpscr++) = color2;
	}
}

void DrawClearScreenAll(void) {
	ClearScreen(&top1Screen, BLACK);
	ClearScreen(&top2Screen, BLACK);
	ClearScreen(&bottomScreen, BLACK);
	current_y = 0;
}

static uint32_t DrawCharacter(Screen *screen, wchar_t character, uint32_t x, uint32_t y, uint32_t color, uint32_t bgcolor, FontMetrics *font)
{
	uint32_t char_width = character < font->dwstart ? font->sw : font->dw;
	if (screen->w < x + char_width || y < font->h)
		return 0;

	uint8_t (* pScreen)[screen->h][screen->bpp] = screen->addr;
	uint32_t fontX, fontY, charVal;

	struct {
		uint8_t a;
		uint8_t r;
		uint8_t g;
		uint8_t b;
	} fore, back;

	fore.a = color >> 24;
	fore.r = color >> 16;
	fore.g = color >> 8;
	fore.b = color;

	back.a = bgcolor >> 24;
	back.r = bgcolor >> 16;
	back.g = bgcolor >> 8;
	back.b = bgcolor;

//	pScreen = screen;
	uint32_t charOffs = character * font->dw * font->h;
	uint32_t *glyph = font->addr + (charOffs >> 5);
	charOffs &= (sizeof(glyph[0]) * 8) - 1; //0x0000001F
	charVal = *glyph++ >> charOffs;
	for (fontX = 0; fontX < char_width; fontX++) {
		for (fontY = 0; fontY < font->h; fontY++) {
			if (charVal & 1) {
				if (fore.a) {
					pScreen[x][SCREEN_HEIGHT - (y - fontY)][0] = fore.b;
					pScreen[x][SCREEN_HEIGHT - (y - fontY)][1] = fore.g;
					pScreen[x][SCREEN_HEIGHT - (y - fontY)][2] = fore.r;
				}
			} else {
				if (back.a) {
					pScreen[x][SCREEN_HEIGHT - (y - fontY)][0] = back.b;
					pScreen[x][SCREEN_HEIGHT - (y - fontY)][1] = back.g;
					pScreen[x][SCREEN_HEIGHT - (y - fontY)][2] = back.r;
				}
			}
			if ((++charOffs & 0x0000001F) == 0)
				charVal = *glyph++;
			else
				charVal >>= 1;
		}

		x++;
	}
	return char_width;
}

void DrawSubString(Screen *screen, const wchar_t *str, uint32_t count, uint32_t x, uint32_t y, uint32_t color, uint32_t bgcolor, FontMetrics *font)
{
	for (uint32_t i = 0; i < count; x += DrawCharacter(screen, str[i++], x, y, color, bgcolor, font));
}

uint32_t GetStringWidth(const wchar_t *str, uint32_t count, FontMetrics *font)
{
	uint32_t width = 0;
	for (uint32_t i = 0; i < count; width += str[i++] < font->dwstart ? font->sw : font->dw);
	return width;
}

void DrawStringRect(Screen *screen, const wchar_t *str, uint32_t x, uint32_t y, uint32_t color, uint32_t bgcolor, FontMetrics *font, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
/*	uint32_t width = right - left;
	uint32_t height = bottom - top;
	uint32_t i = 0, j, k;
	while (i < wcslen(str)) {
		while (GetStringWidth(str, j, font) < (j = wcscspn(str, L" "))) {
			k = j;
		}
	}
*/
}

void DrawStringWithFont(Screen *screen, const wchar_t *str, uint32_t x, uint32_t y, uint32_t color, uint32_t bgcolor, FontMetrics *font)
{
	DrawSubString(screen, str, wcslen(str), x, y, color, bgcolor, font);
}

void DrawString(Screen *screen, const wchar_t *str, uint32_t x, uint32_t y, uint32_t color, uint32_t bgcolor)
{
	DrawStringWithFont(screen, str, x, y, color, bgcolor, &font16);
}

/*//[Unused]
void DrawHex(uint8_t *screen, uint32_t hex, uint32_t x, uint32_t y, uint32_t color, uint32_t bgcolor)
{
	uint32_t i = sizeof(hex)*2;
	wchar_t HexStr[sizeof(hex)*2+1] = {0,};
	while (i){
		HexStr[--i] = hex & 0x0F;
		HexStr[i] += HexStr[i] > 9 ? '7' : '0';
		hex >>= 4;
	}
	DrawString(screen, HexStr, x, y, color, bgcolor);
}
//[Unused]
void DrawHexWithName(uint8_t *screen, const wchar_t *str, uint32_t hex, uint32_t x, uint32_t y, uint32_t color, uint32_t bgcolor)
{
	DrawString(screen, str, x, y, color, bgcolor);
	DrawHex(screen, hex, x + wcslen(str) * font16.sw, y, color, bgcolor);
}
*/
void Debug(const char *format, ...)
{
	char *str;
	va_list va;

	va_start(va, format);
	vasprintf(&str, format, va);
	va_end(va);
	wchar_t wstr[strlen(str)+1];
	mbstowcs(wstr, str, strlen(str)+1);
	free(str);
	DrawString(&top1Screen, wstr, 10, current_y, RGB(255, 255, 255), RGB(0, 0, 0));

	current_y += 10;
}

//No need to enter and exit again and again, isn't it
inline void writeByte(uint8_t *address, uint8_t value) {
	*(address) = value;
}
/*
void DrawPixel(uint8_t *screen, uint32_t x, uint32_t y, uint32_t color){
	if(x >= BOT_SCREEN_WIDTH || x < 0) return;
	if(y >= SCREEN_HEIGHT || y < 0) return;
	if(color & ALPHA_MASK){
		uint8_t *address  = screen + (SCREEN_HEIGHT * (x + 1) - y) * BYTES_PER_PIXEL;
		writeByte(address, color);
		writeByte(address+1, color >> 8);
		writeByte(address+2, color >> 16);
	}
	if(screen == TOP_SCREEN && TOP_SCREEN2){
		if(color & ALPHA_MASK){
			uint8_t *address = TOP_SCREEN2 + (SCREEN_HEIGHT * (x + 1) - y) * BYTES_PER_PIXEL;
			writeByte(address, color);
			writeByte(address+1, color >> 8);
			writeByte(address+2, color >> 16);
		}
	}
}

uint32_t GetPixel(uint8_t *screen, uint32_t x, uint32_t y){
	return *(uint32_t*)(screen + (SCREEN_HEIGHT * (x + 1) - y) * BYTES_PER_PIXEL) & COLOR_MASK;
}
*/

//----------------New Splash Screen Stuff------------------

void DrawTopSplash(TCHAR splash_file[], TCHAR splash_fileL[], TCHAR splash_fileR[]) {
	unsigned int n = 0, bin_size;
	File Splash, SplashL, SplashR;
	if (FileOpen(&SplashL, splash_fileL, 0)&&FileOpen(&SplashR, splash_fileR, 0))
	{
		//Load the spash image
		bin_size = 0;
		while ((n = FileRead(&SplashL, (void*)((uint32_t)TOP_SCREEN + bin_size), 0x100000, bin_size)) > 0) {
			bin_size += n;
		}
		FileClose(&SplashL);
		bin_size = 0;
		while ((n = FileRead(&SplashR, (void*)((uint32_t)TOP_SCREEN2 + bin_size), 0x100000, bin_size)) > 0) {
			bin_size += n;
		}
		FileClose(&SplashR);
	}
	else if (FileOpen(&Splash, splash_file, 0))
	{
		//Load the spash image
		bin_size = 0;
		while ((n = FileRead(&Splash, (void*)((uint32_t)TOP_SCREEN + bin_size), 0x100000, bin_size)) > 0) {
			bin_size += n;
		}
		FileClose(&Splash);
		memcpy(TOP_SCREEN2, TOP_SCREEN, bin_size);
	}
	else
	{
		wchar_t tmp[256];
		swprintf(tmp, sizeof(tmp)/sizeof(tmp[0]), strings[STR_ERROR_OPENING], splash_file);
		DrawString(&bottomScreen, tmp, font24.dw, SCREEN_HEIGHT - font24.h, RED, BLACK);
	}
}

void DrawBottomSplash(TCHAR splash_file[]) {
	DrawSplash(BOT_SCREEN, splash_file);
}

void DrawSplash(uint8_t *screen, TCHAR splash_file[]) {
	unsigned int n = 0, bin_size;
	File Splash;
	if(FileOpen(&Splash, splash_file, 0))
	{
		//Load the spash image
		bin_size = 0;
		while ((n = FileRead(&Splash, (void*)((uint32_t)screen + bin_size), 0x100000, bin_size)) > 0) {
			bin_size += n;
		}
		FileClose(&Splash);
	}
	else
	{
		wchar_t tmp[256];
		swprintf(tmp, sizeof(tmp)/sizeof(tmp[0]), strings[STR_ERROR_OPENING], splash_file);
		DrawString(&bottomScreen, tmp, font24.dw, SCREEN_HEIGHT - font24.h, RED, BLACK);
	}
}

void DrawFadeScreen(uint8_t *screen, uint16_t Width, uint16_t Height, uint32_t f)
{
	uint32_t *screen32 = (uint32_t *)screen;
	int i;
	for (i = 0; i<Width*Height * 3 / 4; i++)
	{
		*screen32 = (*screen32 >> 1) & 0x7F7F7F7F;
		*screen32 += (*screen32 >> 3) & 0x1F1F1F1F; 
		*screen32 += (*screen32 >> 1) & 0x7F7F7F7F; 
		screen32++; 
	}
}

void fadeOut(){
	for (int x = 255; x >= 0; x-=8){ 
		DrawFadeScreen(BOT_SCREEN, BOT_SCREEN_WIDTH, SCREEN_HEIGHT, x); 
		DrawFadeScreen(TOP_SCREEN, TOP_SCREEN_WIDTH, SCREEN_HEIGHT, x); 
		DrawFadeScreen(TOP_SCREEN2, TOP_SCREEN_WIDTH, SCREEN_HEIGHT, x); 
	} 
}

void OpenAnimation(){ //The Animation is here, but it leaves some shit on the screen so it has to be fixed
	/*for (int x = 255; x >= 0; x = x - 51){
		DrawFadeScreen(BOT_SCREEN, BOT_SCREEN_WIDTH, SCREEN_HEIGHT, x);
	}
	ClearScreen(BOT_SCREEN, RGB(0, 0, 0));*/
}

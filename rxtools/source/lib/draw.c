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
#include "fs.h"
#include "draw.h"
#include "lang.h"

Screen top1Screen = {400, 240, sizeof(Pixel), 400*240*sizeof(Pixel), (uint8_t*)0x080FFFC0, (uint8_t*)0x27000000};
Screen top2Screen = {400, 240, sizeof(Pixel), 400*240*sizeof(Pixel), (uint8_t*)0x080FFFC8, (uint8_t*)0x27000000+400*240*3};
Screen bottomScreen = {320, 240, sizeof(Pixel), 320*240*sizeof(Pixel), (uint8_t*)0x080FFFD0, (uint8_t*)0x27000000+400*240*3*2};

extern uint32_t _binary_font_ascii_bin_start[];
FontMetrics font16 = {8, 16, 16, 0x2000, _binary_font_ascii_bin_start}; //default font metrics
FontMetrics font24 = {8, 16, 16, 0x2000, _binary_font_ascii_bin_start}; //use default in case unicode glyps won't be loaded

void ClearScreen(Screen *screen, Color color)
{
	Pixel *pScreen = screen->buf2;
	if (color.r == color.g && color.g == color.b) {
		memset(pScreen, color.r, screen->size);
	} else {
		*pScreen = color.pixel;
		uint32_t i = 1;
		do {
			memcpy(pScreen + i, pScreen, i * sizeof(Pixel));
		} while ((i <<= 1) < screen->w * screen->h / 2);
		memcpy(pScreen + i, pScreen, (screen->w * screen->h - i) * sizeof(Pixel));
	}
	
}

void DisplayScreen(Screen *screen) {
	memcpy(screen->addr, screen->buf2, screen->size);
}

void DrawRect(Screen *screen, Rect *rect, Color color) {
	Pixel (*pScreen)[screen->h] = screen->buf2;

	if (!color.a) return;
	if (rect->x >= screen->w) rect->x = screen->w - 1;
	if (rect->y >= screen->h) rect->y = screen->h - 1;
	if (rect->x + rect->w > screen->w) rect->w = screen->w - rect->x;
	if (rect->y + rect->h > screen->h) rect->h = screen->h - rect->y;

	for (uint_fast16_t x = rect->x + 1; x < rect->x + rect->w - 1; pScreen[x][screen->h - rect->y - 1] = pScreen[x][screen->h - rect->y - rect->h] = color.pixel, x++);
	for (uint_fast16_t y = screen->h - rect->y - rect->h; y < screen->h - rect->y; pScreen[rect->x][y] = pScreen[rect->x + rect->w][y] = color.pixel, y++);
}

void FillRect(Screen *screen, Rect *rect, Color color) {
	Pixel (*pScreen)[screen->h] = screen->buf2;

	if (!color.a) return;
	if (rect->x >= screen->w) rect->x = screen->w - 1;
	if (rect->y >= screen->h) rect->y = screen->h - 1;
	if (rect->x + rect->w > screen->w) rect->w = screen->w - rect->x;
	if (rect->y + rect->h > screen->h) rect->h = screen->h - rect->y;
	for (uint_fast16_t x = rect->x; x < rect->x + rect->w; x++)
		for (uint_fast16_t y = screen->h - rect->y - rect->h; y < screen->h - rect->y; pScreen[x][y++] = color.pixel);
}

void DrawProgress(Screen *screen, Rect *rect, Color border, Color back, Color fill, TextColors *text, FontMetrics *font, uint32_t posmax, uint32_t pos) {
	uint_fast16_t x;
	wchar_t percent[5];
	
	if (!posmax) posmax = UINT32_MAX;
	if (pos > posmax) pos = posmax;	
	x = (rect->w - 2) * pos / posmax;
	swprintf(percent, 5, L"%u%%", 100 * pos / posmax);
	DrawRect(screen, rect, border);
	FillRect(screen, &(Rect){rect->x + 1, rect->y + 1, x, rect->h - 2}, fill);
	FillRect(screen, &(Rect){rect->x + 1 + x, rect->y + 1, rect->w - 2 - x, rect->h - 2}, back);
	DrawSubString(screen, percent, -1, rect->x + (rect->w - GetSubStringWidth(percent, -1, font)) / 2, rect->y + (rect->h - font->h) / 2, text, font);
}

static uint32_t DrawCharacter(Screen *screen, wchar_t character, uint32_t x, uint32_t y, TextColors *color, FontMetrics *font) {
	uint32_t char_width = character < font->dwstart ? font->sw : font->dw;
	y += font->h;
	if (screen->w < x + char_width || y < 0)
		return 0;

	Pixel (*pScreen)[screen->h] = screen->buf2;
	uint32_t fontX, fontY, charVal;

	uint32_t charOffs = character * font->dw * font->h;
	uint32_t *glyph = font->addr + (charOffs >> 5);
	charOffs &= (sizeof(glyph[0]) * 8) - 1; //0x0000001F
	charVal = *glyph++ >> charOffs;
	for (fontX = 0; fontX < char_width; fontX++) {
		for (fontY = 0; fontY < font->h; fontY++) {
			if (charVal & 1) {
				if (color->fg.a)
					pScreen[x][screen->h - (y - fontY)] = color->fg.pixel;
			} else {
				if (color->bg.a)
					pScreen[x][screen->h - (y - fontY)] = color->bg.pixel;
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

uint32_t DrawSubString(Screen *screen, const wchar_t *str, size_t count, uint32_t x, uint32_t y, TextColors *color, FontMetrics *font) {
	size_t len = wcslen(str);
	if (count < 0 || count > len)
		count = len;
	len = x;
	for (uint32_t i = 0; i < count && (str[i] < font->dwstart ? font->sw : font->dw) <= screen->w - x; x += DrawCharacter(screen, str[i], x, y, color, font), i++);
	return x - len;
}

uint32_t GetSubStringWidth(const wchar_t *str, size_t count, FontMetrics *font) {
	size_t len = wcslen(str);
	if (count < 0 || count > len)
		count = len;
	len = 0;		
	for (uint32_t i = 0; i < count; len += str[i++] < font->dwstart ? font->sw : font->dw);
	return len;
}

uint32_t DrawSubStringRect(Screen *screen, const wchar_t *str, size_t count, Rect *rect, TextColors *color, FontMetrics *font) {
	uint_fast16_t dy = 0;
	size_t len = wcslen(str);
	if (count < 0 || count > len)
		count = len;
	if (count == 0 || rect->x >= screen->w || rect->y >= screen->h)
		return dy;
	if (rect->w == 0 || rect->x + rect->w > screen->w)
		rect->w = screen->w - rect->x;
	if (rect->h == 0 || rect->y + rect->h > screen->h)
		rect->h = screen->h - rect->y;
	uint32_t i, j, k, sw;
	for (i = 0; i < count && (dy += font->h) <= rect->h; ) {
		sw = 0;
		k = 0;
		j = wcsspn(str + i, L" "); //include leading spaces
		j += wcscspn(str + i + j, L" ");
		for (; (sw += GetSubStringWidth(str + i + k, j, font)) < rect->w && i + j + k < count; ) {
			k += j;
			j = wcsspn(str + i + k, L" "); //next gap
			j += wcscspn(str + i + k + j, L" "); //next word
		}
		if (sw <= rect->w || k == 0) //word do not cross the area border or only one word - take all
			j += k;
		else //word crosses the area border - take all but last word
			j = k;                                                                                                	
		 //add trailing spaces, if any, to draw non-transparent background color and remove trailing word part or added spaces that won't fit
		for (j += wcsspn(str + i + j, L" "); GetSubStringWidth(str + i, j, font) > rect->w; j--);
		DrawSubString(screen, str + i, j, rect->x, rect->y, color, font);
		i += j + wcsspn(str + i + j, L" "); //skip the rest of spaces to the next line start
		rect->y += font->h;
	}
	return dy;
}

uint32_t DrawStringRect(Screen *screen, const wchar_t *str, Rect *rect, TextColors *color, FontMetrics *font) {
	return DrawSubStringRect(screen, str, -1, rect, color, font);
}

uint32_t DrawString(Screen *screen, const wchar_t *str, uint32_t x, uint32_t y, Color color, Color bgcolor) {
	TextColors c = {color, bgcolor};
	return DrawSubString(screen, str, -1, x, y - font16.h, &c, &font16);
}

void DrawSplash(Screen *screen, wchar_t *splash_file) {
	File Splash;
	if (!FileOpen(&Splash, splash_file, false) ||
		(FileRead2(&Splash, (void*)(screen->buf2), Splash.fsize) != Splash.fsize &&
		(FileClose(&Splash) || true)
	)) {
		wchar_t tmp[_MAX_LFN + 1];
		swprintf(tmp, _MAX_LFN + 1, strings[STR_ERROR_OPENING], splash_file);
		DrawString(&bottomScreen, tmp, font24.dw, SCREEN_HEIGHT - font24.h, RED, BLACK);
	} else
		FileClose(&Splash);
}

static void DrawFadeScreen(Screen *screen) {
	uint32_t *screen32 = (uint32_t*)screen->buf2;
	for (int i = 0; i++ < screen->size / sizeof(uint32_t); screen32++) {
		*screen32 = (*screen32 >> 1) & 0x7F7F7F7F;
		*screen32 += (*screen32 >> 3) & 0x1F1F1F1F; 
		*screen32 += (*screen32 >> 1) & 0x7F7F7F7F; 
	}
}

void fadeOut() {
	for (int i = 32; i--;) { 
		DrawFadeScreen(&bottomScreen); 
		DrawFadeScreen(&top1Screen); 
		DrawFadeScreen(&top2Screen); 
		DisplayScreen(&bottomScreen); 
		DisplayScreen(&top1Screen); 
		DisplayScreen(&top2Screen); 
	} 
}

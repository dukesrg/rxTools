/*
 * Copyright (C) 2015-2016 The PASTA Team, dukesrg
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
#include "draw.h"
#include "fs.h"
#include "hid.h"
#include "lang.h"
#include "strings.h"
#include "cfnt.h"

Screen top1Screen = {400, 240, sizeof(Pixel), 0, 400*240*sizeof(Pixel), (uint8_t*)0x080FFFC0, (uint8_t*)0x27000000, (uint8_t*)0x27000000+400*240*sizeof(Pixel), L""};
Screen top2Screen = {400, 240, sizeof(Pixel), 0, 400*240*sizeof(Pixel), (uint8_t*)0x080FFFC8, (uint8_t*)0x27000000+400*240*sizeof(Pixel)*2, (uint8_t*)0x27000000+400*240*sizeof(Pixel)*3, L""};
Screen bottomScreen = {320, 240, sizeof(Pixel), 0, 320*240*sizeof(Pixel), (uint8_t*)0x080FFFD0, (uint8_t*)0x27000000+400*240*sizeof(Pixel)*4, (uint8_t*)0x27000000+400*240*sizeof(Pixel)*4+320*240*sizeof(Pixel), L""};
//Screen top1Screen = {400, 240, sizeof(Pixel), 0, 400*240*sizeof(Pixel), (uint8_t*)0x23FFFE00, (uint8_t*)0x27000000, (uint8_t*)0x27000000+400*240*sizeof(Pixel), L""};
//Screen top2Screen = {400, 240, sizeof(Pixel), 0, 400*240*sizeof(Pixel), (uint8_t*)0x23FFFE04, (uint8_t*)0x27000000+400*240*sizeof(Pixel)*2, (uint8_t*)0x27000000+400*240*sizeof(Pixel)*3, L""};
//Screen bottomScreen = {320, 240, sizeof(Pixel), 0, 320*240*sizeof(Pixel), (uint8_t*)0x23FFFE08, (uint8_t*)0x27000000+400*240*sizeof(Pixel)*4, (uint8_t*)0x27000000+400*240*sizeof(Pixel)*4+320*240*sizeof(Pixel), L""};

static uint8_t *DrawTile(Screen *screen, uint8_t *in, uint_fast8_t iconsize, uint_fast8_t tilesize, uint_fast16_t ax, uint_fast16_t ay, uint_fast16_t dx, uint_fast16_t dy, uint_fast8_t cw, uint_fast8_t ch, Color color) {
	for (size_t y = 0; y < iconsize; y += tilesize) {
		if (tilesize == 1) {
			uint_fast16_t py = y + dy;
			if (py < ch) {
				Pixel (*pScreen)[screen->h] = screen->buf2;
				uint_fast8_t a = *in & 0x0F;
				py = screen->h - ay - py;
				uint_fast16_t px = dx;
				if (px < cw && a) {
					px += ax;
					pScreen[px][py].r = (pScreen[px][py].r * (0x0F - a) + color.pixel.r * (a + 1)) >> 4;
					pScreen[px][py].g = (pScreen[px][py].g * (0x0F - a) + color.pixel.g * (a + 1)) >> 4;
					pScreen[px][py].b = (pScreen[px][py].b * (0x0F - a) + color.pixel.b * (a + 1)) >> 4;
				}
				if ((px = dx + 1) < cw && (a = *in >> 4)) {
					px += ax;
					pScreen[px][py].r = (pScreen[px][py].r * (0x0F - a) + color.pixel.r * (a + 1)) >> 4;
					pScreen[px][py].g = (pScreen[px][py].g * (0x0F - a) + color.pixel.g * (a + 1)) >> 4;
					pScreen[px][py].b = (pScreen[px][py].b * (0x0F - a) + color.pixel.b * (a + 1)) >> 4;
				}
			}
			in++;
		} else for (size_t x = 0; x < iconsize; x += tilesize)
			in = DrawTile(screen, in, tilesize, tilesize / 2, ax, ay, dx + x, dy + y, cw, ch, color);
	}
	return in;
}

static uint8_t *DrawTileLA(Screen *screen, uint8_t *in, uint_fast8_t iconsize, uint_fast8_t tilesize, uint_fast16_t ax, uint_fast16_t ay, uint_fast16_t dx, uint_fast16_t dy, uint_fast8_t cw, uint_fast8_t ch, Color color) {
	if (!tilesize) {
		uint_fast16_t py = dy; //y + dy;
		if (py < ch) {
			Pixel (*pScreen)[screen->h] = screen->buf2;
			uint_fast8_t a = *in;// & 0x0F;
			py = screen->h - ay - py;
			uint_fast16_t px = dx;
			if (px < cw && a) {
				px += ax;
				pScreen[px][py].r = (pScreen[px][py].r * (0x0F - a) + color.pixel.r * (a + 1)) >> 4;
				pScreen[px][py].g = (pScreen[px][py].g * (0x0F - a) + color.pixel.g * (a + 1)) >> 4;
				pScreen[px][py].b = (pScreen[px][py].b * (0x0F - a) + color.pixel.b * (a + 1)) >> 4;
			}
		}
		in++;
	} else {
		for (size_t y = 0; y < iconsize; y += tilesize) {
			for (size_t x = 0; x < iconsize; x += tilesize)
				in = DrawTile(screen, in, tilesize, tilesize / 2, ax, ay, dx + x, dy + y, cw, ch, color);
		}
	}
	return in;
}

static uint8_t *DrawTile2(Screen *screen, uint8_t *in, uint_fast8_t iconsize, uint_fast8_t tilesize, uint_fast16_t ax, uint_fast16_t ay, uint_fast16_t dx, uint_fast16_t dy, uint_fast8_t cw, uint_fast8_t ch, Color color) {
	if (iconsize == 2) {
		if ((uint_fast16_t)(dy + 1) <= ch) {
			Pixel (*pScreen)[screen->h] = screen->buf2;
			uint_fast16_t a = *(uint16_t*)in;
			a = (a & 0x0F0F) + (a >> 4 & 0x0F0F);
			a = (a + (a >> 8)) >> 2 & 0x0F;
			uint_fast16_t py = screen->h - ay - (dy + 1) / 2;
			if ((uint_fast16_t)(dx + 1) <= cw && a) {
				uint_fast16_t px = ax + (dx + 1) / 2;
				pScreen[px][py].r = (pScreen[px][py].r * (0x0F - a) + color.pixel.r * (a + 1)) >> 4;
				pScreen[px][py].g = (pScreen[px][py].g * (0x0F - a) + color.pixel.g * (a + 1)) >> 4;
				pScreen[px][py].b = (pScreen[px][py].b * (0x0F - a) + color.pixel.b * (a + 1)) >> 4;
			}
		}
		in += 2;
	} else for (size_t y = 0; y < iconsize; y += tilesize)
		for (size_t x = 0; x < iconsize; x += tilesize)
			in = DrawTile2(screen, in, tilesize, tilesize / 2, ax, ay, dx + x, dy + y, cw, ch, color);
	return in;
}

static uint_fast8_t DrawGlyph(Screen *screen, uint_fast16_t x, uint_fast16_t y, Glyph glyph, Color color, uint_fast8_t fontsize) {
	if (!finf)
		return 0;
	uint_fast16_t charx, chary;
	tglp_header *tglp = finf->tglp_offset;
	uint_fast16_t glyphXoffs = glyphXoffs = glyph.code % tglp->number_of_columns * (tglp->cell_width + 1) + 1;
	uint_fast16_t glyphYoffs = glyph.code % (tglp->number_of_columns * tglp->number_of_rows) / tglp->number_of_columns * (tglp->cell_height + 1) + 1;
	uint8_t *sheetsrc = GlyphSheet(glyph.code);
	uint16_t tilex = glyphXoffs & 0xFFFFFFF8;
	uint16_t tilexend = (glyphXoffs + glyph.width->glyph) & 0xFFFFFFF8;
	uint16_t tiley = glyphYoffs & 0xFFFFFFF8;
	uint16_t tileyend = (glyphYoffs + finf->height) & 0xFFFFFFF8;

	glyphXoffs &= 0x00000007;
	glyphYoffs &= 0x00000007;
	for (chary = tiley; chary <= tileyend; chary += 8)
		for (charx = tilex; charx <= tilexend; charx += 8)
			if (fontsize >= finf->height) {
				if (tglp->sheet_image_format == FORMAT_A4)
					DrawTile(screen, sheetsrc + 4 * (charx + chary * tglp->sheet_width / 8), 8, 8, x + glyph.width->left, y, charx - tilex - glyphXoffs, chary - tiley - glyphYoffs, glyph.width->glyph, finf->height, color);
				else if (tglp->sheet_image_format == FORMAT_LA4)
					DrawTileLA(screen, sheetsrc + 4 * (charx + chary * tglp->sheet_width / 8), 8, 8, x + glyph.width->left, y, charx - tilex - glyphXoffs, chary - tiley - glyphYoffs, glyph.width->glyph, finf->height, color);
			} else {
				DrawTile2(screen, sheetsrc + 4 * (charx + chary * tglp->sheet_width / 8), 8, 8, x + (glyph.width->left + 1) / 2, y, charx - tilex - glyphXoffs, chary - tiley - glyphYoffs, glyph.width->glyph, finf->height, color);
			}

	if (fontsize >= finf->height)
		return glyph.width->character;
	else
		return (glyph.width->character + (glyph.width->glyph % 2) + (glyph.width->left % 2)) / 2;
}

uint_fast16_t GetSubStringWidth(Glyph *glyphs, size_t max, uint_fast8_t fontsize) {
	if (!(finf && glyphs))
		return 0;
	uint_fast16_t x = 0;
	if (fontsize >= finf->height)
		for (size_t i = max; i--; x += glyphs++->width->character);
	else for (size_t i = max; i--; glyphs++)
		x += (glyphs->width->character + (glyphs->width->glyph % 2) + (glyphs->width->left % 2)) / 2;
	return x;
}

uint_fast16_t DrawSubString(Screen *screen, uint_fast16_t x, uint_fast16_t y, const wchar_t *str, size_t max, Color color, uint_fast8_t fontsize) {
	if (!(finf && str && *str))
		return 0;
	size_t len = wcslen(str);
	if (!max || max > len)
		max = len;
	len = x;
	Glyph glyphs[max];
	max = wcstoglyphs(glyphs, str, max);
	fontsize = fontsize >= finf->height ? finf->height : (finf->height + 1) / 2;
	for (size_t i = 0 ; i < max; x += DrawGlyph(screen, x, y, glyphs[i++], color, fontsize));
	screen->updated = 1;
	return x - len;
}

void ClearScreen(Screen *screen, Color color) {
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
	screen->updated = 1;
}

void DisplayScreen(Screen *screen) {
	if (screen->updated) {
		memcpy(screen->addr, screen->buf2, screen->size);
		screen->updated = 0;
	}
}

void DrawRect(Screen *screen, Rect *rect, Color color) {
	Pixel (*pScreen)[screen->h] = screen->buf2;

	if (!color.a) return;
	if (rect->x >= screen->w) rect->x = screen->w - 1;
	if (rect->y >= screen->h) rect->y = screen->h - 1;
	if (rect->x + rect->w > screen->w) rect->w = screen->w - rect->x;
	if (rect->y + rect->h > screen->h) rect->h = screen->h - rect->y;

	for (uint_fast16_t x = rect->x + 1; x < rect->x + rect->w - 1; pScreen[x][screen->h - rect->y - 1] = pScreen[x][screen->h - rect->y - rect->h] = color.pixel, x++);
	for (uint_fast16_t y = screen->h - rect->y - rect->h; y < screen->h - rect->y; pScreen[rect->x][y] = pScreen[rect->x + rect->w - 1][y] = color.pixel, y++);
	screen->updated = 1;
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
	screen->updated = 1;
}

uint_fast16_t DrawSubStringRect(Screen *screen, const wchar_t *str, size_t max, Rect *rect, Color color, align a, uint_fast8_t fontsize) {
	if (!finf)
		return 0;
	uint_fast16_t dx = 0, dy = 0;
	size_t len = wcslen(str);
	if (!max || max > len)
		max = len;
	if (!max || rect->x >= screen->w || rect->y >= screen->h)
		return dy;
	if (!rect->w || rect->x + rect->w > screen->w)
		rect->w = screen->w - rect->x;
	if (!rect->h || rect->y + rect->h > screen->h)
		rect->h = screen->h - rect->y;
	uint32_t i, j, k, sw;
	Glyph glyphs[max];
	max = wcstoglyphs(glyphs, str, max);
	fontsize = fontsize >= finf->height ? finf->height : (finf->height + 1) / 2;
	for (i = 0; i < max && (dy + fontsize) <= rect->h; ) {
		sw = 0;
		k = 0;
		j = wcsspn(str + i, L" "); //include leading spaces
		j += wcscspn(str + i + j, L" ");
		for (; (sw += GetSubStringWidth(glyphs + i + k, j, fontsize)) < rect->w && i + j + k < max; ) {
			k += j;
			j = wcsspn(str + i + k, L" "); //next gap
			j += wcscspn(str + i + k + j, L" "); //next word
		}
		if (sw <= rect->w || k == 0) //word do not cross the area border or only one word - take all
			j += k;
		else //word crosses the area border - take all but last word
			j = k;                                                                                                	
		 //add trailing spaces, if any, to draw non-transparent background color and remove trailing word part or added spaces that won't fit
		for (j += wcsspn(str + i + j, L" "); (sw = GetSubStringWidth(glyphs + i, j, fontsize)) > rect->w; j--);
		switch (a) {
			case ALIGN_LEFT: dx = 0; break;
			case ALIGN_MIDDLE: dx = (rect->w - sw) / 2; break;
			case ALIGN_RIGHT: dx = rect->w - sw; break;
		}
		DrawSubString(screen, rect->x + dx, rect->y + dy, str + i, j, color, fontsize);
		i += j + wcsspn(str + i + j, L" "); //skip the rest of spaces to the next line start
		dy += fontsize;
	}
	screen->updated = 1;
	return dy;
}

uint_fast16_t DrawStringRect(Screen *screen, const wchar_t *str, Rect *rect, Color color, align a, uint_fast8_t fontsize) {
	return DrawSubStringRect(screen, str, 0, rect, color, a, fontsize);
}

uint_fast16_t DrawString(Screen *screen, const wchar_t *str, uint32_t x, uint32_t y, Color color, Color bgcolor) {
	return DrawSubString(screen, x, y, str, 0, color, 16);
}

uint_fast16_t DrawInfo(const wchar_t *info, const wchar_t *action, const wchar_t *format, ...){
	wchar_t str[_MAX_LFN + 1];
	Rect rect = {0};
	va_list va;
	va_start(va, format);
	vswprintf(str, _MAX_LFN + 1, format, va);
	va_end(va);
	rect.y += DrawStringRect(&bottomScreen, str, &rect, RED, ALIGN_LEFT, 30);
	if (info)
		rect.y += DrawStringRect(&bottomScreen, info, &rect, RED, ALIGN_LEFT, 30);
	if (action) {
		swprintf(str, _MAX_LFN + 1, lang(SF2_PRESS_BUTTON_ACTION), lang(S_ANY_BUTTON), action);
		rect.y += DrawStringRect(&bottomScreen, str, &rect, RED, ALIGN_LEFT, 16);
	}
	DisplayScreen(&bottomScreen);
	if (action)
		InputWait();

	return rect.y;
}

void DrawProgress(Screen *screen, Rect *rect, Color frame, Color done, Color back, Color textcolor, uint_fast8_t fontsize, uintmax_t posmax, uintmax_t pos, uint32_t timeleft) {
//	static uint_fast16_t oldx = -1;
	uint_fast16_t x;
	uint_fast8_t h, m, s;
//	wchar_t percent[5];
	wchar_t estimated[9];
	
	if (!posmax) posmax = UINTMAX_MAX;
	if (pos > posmax) pos = posmax;	
	x = (rect->w - 2) * pos / posmax;
//	if ((x = (rect->w - 2) * pos / posmax) == oldx) return;
//	oldx = x;
	DrawRect(screen, rect, frame);
	FillRect(screen, &(Rect){rect->x + 1, rect->y + 1, x, rect->h - 2}, done);
	FillRect(screen, &(Rect){rect->x + 1 + x, rect->y + 1, rect->w - 2 - x, rect->h - 2}, back);
//	swprintf(percent, 5, L"%u%%", 100 * pos / posmax);
//	DrawSubStringRect(screen, percent, 0, rect, textcolor, ALIGN_MIDDLE, fontsize);
	if (pos) {
		h = timeleft / 3600;
		m = (timeleft - h * 3600) / 60;
		s = timeleft % 60;
		swprintf(estimated, 9, L"%u:%02u:%02u", h, m, s);
		DrawSubStringRect(screen, h ? estimated : estimated + 2, 0, rect, textcolor, ALIGN_MIDDLE, fontsize);
	}
	screen->updated = 1;
}

void DrawSplash(Screen *screen, wchar_t *splash_file) {
	File Splash;
	if (wcscmp(splash_file, screen->bgpath)) {
		if (!FileOpen(&Splash, splash_file, 0) ||
			(FileRead2(&Splash, (void*)(screen->bgcache), Splash.fsize) != Splash.fsize &&
			(FileClose(&Splash) || 1)
		)) {
//			DrawInfo(NULL, lang(S_CONTINUE), lang(SF_FAILED_TO), lang(S_LOAD), splash_file);
			DrawInfo(NULL, lang(S_CONTINUE), lang("Failed to %ls %ls - %u!"), lang(S_LOAD), splash_file, FSGetLastError());

		} else {
			FileClose(&Splash);
			wcscpy(screen->bgpath, splash_file);
		}
	}
	memcpy(screen->buf2, screen->bgcache, screen->size);
	screen->updated = 1;
}

static void DrawFadeScreen(Screen *screen) {
	uint32_t *screen32 = (uint32_t*)screen->buf2;
	for (int i = 0; i++ < screen->size / sizeof(uint32_t); screen32++) {
		*screen32 = (*screen32 >> 1) & 0x7F7F7F7F;
		*screen32 += (*screen32 >> 3) & 0x1F1F1F1F; 
		*screen32 += (*screen32 >> 1) & 0x7F7F7F7F; 
	}
	screen->updated = 1;
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

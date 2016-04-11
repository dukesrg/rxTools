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
#include "draw.h"
#include "fs.h"
#include "hid.h"
#include "lang.h"
#include "strings.h"
#include "cfnt.h"

Screen top1Screen = {400, 240, sizeof(Pixel), 400*240*sizeof(Pixel), (uint8_t*)0x080FFFC0, (uint8_t*)0x27000000};
Screen top2Screen = {400, 240, sizeof(Pixel), 400*240*sizeof(Pixel), (uint8_t*)0x080FFFC8, (uint8_t*)0x27000000+400*240*3};
Screen bottomScreen = {320, 240, sizeof(Pixel), 320*240*sizeof(Pixel), (uint8_t*)0x080FFFD0, (uint8_t*)0x27000000+400*240*3*2};

extern uint32_t _binary_font_ascii_bin_start[];
FontMetrics font16 = {8, 16, 16, 0x2000, _binary_font_ascii_bin_start}; //default font metrics
FontMetrics font24 = {8, 16, 16, 0x2000, _binary_font_ascii_bin_start}; //use default in case unicode glyps won't be loaded

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
/*
uint8_t *decodetile(uint8_t *in, uint8_t *out, int iconsize, int tilesize, int ax, int ay, int w) {
	size_t x, y;
	uint8_t a;
	for (y = 0; y < iconsize; y += tilesize) {
		if (tilesize == 1) {
			a = *in++;
			out[ax + (ay + y) * w] = a & 0x0F;
			out[ax + (ay + y) * w + 1] = a >> 4;
		} else {
			for (x = 0; x < iconsize; x += tilesize)
				in = decodetile(in, out, tilesize, tilesize / 2, ax + x, ay + y, w);
		}
	}
	return in;
}

uint8_t *decodetile2(uint8_t *in, uint8_t *out, int iconsize, int tilesize, int ax, int ay, int w) {
	size_t x, y;
	uint_fast16_t a;
	if (iconsize == 2){
		a = *(uint16_t*)in;
		a = (a & 0x0F0F) + (a >> 4 & 0x0F0F);
		out[ax / 2 + ay * w / 2] = (a + (a >> 8)) >> 2 & 0x0F;
		in += 2;
	} else {
		for (y = 0; y < iconsize; y += tilesize)
			for (x = 0; x < iconsize; x += tilesize)
				in = decodetile2(in, out, tilesize, tilesize / 2, ax + x, ay + y, w);
	}
	return in;
}
*/
static uint8_t *DrawTile2(Screen *screen, uint8_t *in, uint_fast8_t iconsize, uint_fast8_t tilesize, uint_fast16_t ax, uint_fast16_t ay, uint_fast16_t dx, uint_fast16_t dy, uint_fast8_t cw, uint_fast8_t ch, Color color) {
	if (iconsize == 2) {
//		out[ax / 2 + ay * w / 2] = ;
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
		in+=2;
	} else for (size_t y = 0; y < iconsize; y += tilesize)
		for (size_t x = 0; x < iconsize; x += tilesize)
			in = DrawTile2(screen, in, tilesize, tilesize / 2, ax, ay, dx + x, dy + y, cw, ch, color);
	return in;
}

uint_fast8_t DrawGlyph(Screen *screen, uint_fast16_t x, uint_fast16_t y, Glyph glyph, Color color, uint_fast8_t fontsize) {
	uint_fast16_t charx, chary;
	tglp_header *tglp = finf->tglp_offset;
//	uint_fast16_t sheet_width = tglp->sheet_width;
	uint_fast16_t glyphXoffs = glyphXoffs = glyph.code % tglp->number_of_columns * (tglp->cell_width + 1) + 1;
	uint_fast16_t glyphYoffs = glyph.code % (tglp->number_of_columns * tglp->number_of_rows) / tglp->number_of_columns * (tglp->cell_height + 1) + 1;
//	uint_fast16_t height = tglp->cell_height;
//	glyph_width widths = *glyph.width;

//	uint8_t sheet[tglp->sheet_height * tglp->sheet_width];
	uint8_t *sheetsrc = GlyphSheet(glyph.code);
/*	for (chary = 0; chary < tglp->sheet_height; chary += 8)
		for (charx = 0; charx < tglp->sheet_width; charx += 8)
			if (fontsize == finf->height)
				sheetsrc = decodetile(sheetsrc, sheet, 8, 8, charx, chary, sheet_width);
			else			
				sheetsrc = decodetile2(sheetsrc, sheet, 8, 8, charx, chary, sheet_width);
*/				
	uint16_t tilex = glyphXoffs & 0xFFFFFFF8;
	uint16_t tilexend = (glyphXoffs + glyph.width->glyph) & 0xFFFFFFF8;
	glyphXoffs &= 0x00000007;
	uint16_t tiley = glyphYoffs & 0xFFFFFFF8;
	uint16_t tileyend = (glyphYoffs + finf->height) & 0xFFFFFFF8;
	glyphYoffs &= 0x00000007;

//	fontsize = fontsize >= finf->height ? finf->height : (finf->height + 1) / 2;
/*	if (fontsize < finf->height) {
		glyphXoffs >>= 1;
		glyphYoffs >>= 1;
		height = (height + 1) / 2;
		widths.left = (widths.left + 1) / 2;
		widths.glyph = (widths.glyph + 1) / 2;
		widths.character = (widths.character + 1) / 2;
		sheet_width >>= 1;
	}
*/
	for (chary = tiley; chary <= tileyend; chary += 8)
		for (charx = tilex; charx <= tilexend; charx += 8)
			if (fontsize >= finf->height)
				DrawTile(screen, sheetsrc + 4 * (charx + chary * tglp->sheet_width / 8), 8, 8, x + glyph.width->left, y, charx - tilex - glyphXoffs, chary - tiley - glyphYoffs, glyph.width->glyph, finf->height, color);
//				decodetile(sheetsrc + 4 * (charx + chary * tglp->sheet_width / 8), sheet, 8, 8, charx - tilex - glyphXoffs, chary - tiley - glyphYoffs, tglp->sheet_width);
			else
				DrawTile2(screen, sheetsrc + 4 * (charx + chary * tglp->sheet_width / 8), 8, 8, x + (glyph.width->left + 1) / 2, y, charx - tilex - glyphXoffs, chary - tiley - glyphYoffs, glyph.width->glyph, finf->height, color);
//				decodetile2(sheetsrc + 4 * (charx + chary * tglp->sheet_width / 8), sheet, 8, 8, charx - tilex - glyphXoffs, chary - tiley - glyphYoffs, tglp->sheet_width);
/*
*/
///	Pixel (*pScreen)[screen->h] = screen->buf2;
///	uint8_t fonta;
//	Color bg;
///	for (charx = 0; charx < widths.glyph; charx++) {
///		for (chary = 0; chary < height; chary++) {
//			fonta = sheet[charx + glyphXoffs + (glyphYoffs + chary) * sheet_width];
///			fonta = sheet[charx + chary * sheet_width];
///			if (fonta) {
///				pScreen[x + widths.left][screen->h - (y + chary)].r = (pScreen[x + widths.left][screen->h - (y + chary)].r * (0x0F - fonta) + color.pixel.r * (fonta + 1)) >> 4;
///				pScreen[x + widths.left][screen->h - (y + chary)].g = (pScreen[x + widths.left][screen->h - (y + chary)].g * (0x0F - fonta) + color.pixel.g * (fonta + 1)) >> 4;
///				pScreen[x + widths.left][screen->h - (y + chary)].b = (pScreen[x + widths.left][screen->h - (y + chary)].b * (0x0F - fonta) + color.pixel.b * (fonta + 1)) >> 4;

/*				bg.pixel = pScreen[x + (glyph.width->left+1)/2][screen->h - (y - chary)];
				bg.pixel.r = (bg.pixel.r*(0x0F-fonta) + color.pixel.r*(fonta+1)) >> 4;
				bg.pixel.g = (bg.pixel.g*(0x0F-fonta) + color.pixel.g*(fonta+1)) >> 4;
				bg.pixel.b = (bg.pixel.b*(0x0F-fonta) + color.pixel.b*(fonta+1)) >> 4;
				pScreen[x + (glyph.width->left+1)/2][screen->h - (y + chary)] = bg.pixel;
*/
///			}

/*			fonta = (sheet[charx + glyphXoffs + (glyphYoffs + chary) * tglp->sheet_width] + 1) >> 3;
			if (fonta == 1) {
				bg.pixel = pScreen[x][screen->h - (y - chary)];
				bg.color = (bg.color >> 1 & 0x007F7F7F) + (color.color >> 1 &0x007F7F7F);
//				bg.color = (((bg.color >> fonta) & fontmask[fonta]) + ((color.color >> (7 - fonta)) & fontmask[7-fonta])) >> 1;
//				bg.color = ((color.color >> (7 - fonta)) & fontmask[7-fonta]);
				pScreen[x + cw->left][screen->h - (y + chary)] = bg.pixel;
			} else if (fonta == 2) {
				pScreen[x + cw->left][screen->h - (y + chary)] = color.pixel;
			}
*/
///		}
///		x++;
///	}
	if (fontsize >= finf->height)
		return glyph.width->character;
	else
		return (glyph.width->character + (glyph.width->glyph % 2) + (glyph.width->left % 2)) / 2;
}
/*
uint_fast8_t DrawCharacterCFNT(Screen *screen, uint_fast16_t x, uint_fast16_t y, wchar_t c, Color color) {
	return DrawGlyph(screen, x, y, GlyphCode(c), color);
}

uint16_t *char2glyph(uint16_t *glyphs, wchar_t *str, size_t count) {
	if (!(glyphs && str && *str)) return NULL;
	for (size_t i = count; i--; *glyphs++ = GlyphCode(*str++));
	return glyphs;
}
*/

uint_fast16_t GetSubStringWidthGlyph(Glyph *glyphs, size_t max, uint_fast8_t fontsize) {
	if (!glyphs)
		return 0;
	uint_fast16_t x = 0;
	if (fontsize >= finf->height)
		for (size_t i = max; i--; x += glyphs++->width->character);
	else for (size_t i = max; i--; glyphs++)
		x += (glyphs->width->character + (glyphs->width->glyph % 2) + (glyphs->width->left % 2)) / 2;
	return x;
}

uint_fast16_t DrawSubStringGlyph(Screen *screen, uint_fast16_t x, uint_fast16_t y, const wchar_t *str, size_t max, Color color, uint_fast8_t fontsize) {
	if (!(str && *str))
		return 0;
	size_t len = wcslen(str);
	if (!max || max > len)
		max = len;
	len = x;
	Glyph glyphs[max];
	max = wcstoglyphs(glyphs, str, max);
	fontsize = fontsize >= finf->height ? finf->height : (finf->height + 1) / 2;
	for (size_t i = 0 ; i < max; x += DrawGlyph(screen, x, y, glyphs[i++], color, fontsize));
	return x - len;
}

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
	for (uint_fast16_t y = screen->h - rect->y - rect->h; y < screen->h - rect->y; pScreen[rect->x][y] = pScreen[rect->x + rect->w - 1][y] = color.pixel, y++);
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
/*
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
*/
//uint_fast16_t DrawSubString(Screen *screen, const wchar_t *str, size_t count, uint32_t x, uint32_t y, TextColors *color, FontMetrics *font) {
//	return DrawSubStringGlyph(screen, x, y, str, count, color->fg);
/*
	size_t len = wcslen(str);
	if (count < 0 || count > len)
		count = len;
	len = x;
	for (uint32_t i = 0; i < count && (str[i] < font->dwstart ? font->sw : font->dw) <= screen->w - x; x += DrawCharacter(screen, str[i], x, y, color, font), i++);
	return x - len;
*/
//}
/*
uint_fast16_t GetSubStringWidth(const wchar_t *str, size_t count, FontMetrics *font) {
	Glyph glyphs[count];
	return GetSubStringWidthGlyph(glyphs, wcstoglyphs(glyphs, str, count));
	size_t len = wcslen(str);
	uint_fast16_t dx = 0;
	if (count < 0 || count > len)
		count = len;
	for (uint32_t i = 0; i < count; dx += str[i++] < font->dwstart ? font->sw : font->dw);
	return dx;
*/
//}
/*
uint_fast16_t DrawSubStringRect(Screen *screen, const wchar_t *str, size_t count, Rect *rect, TextColors *color, FontMetrics *font, align a) {
	uint_fast16_t dx = 0, dy = 0;
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
	for (i = 0; i < count && (dy + font->h) <= rect->h; ) {
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
		for (j += wcsspn(str + i + j, L" "); (sw = GetSubStringWidth(str + i, j, font)) > rect->w; j--);
		switch (a) {
			case ALIGN_LEFT: dx = 0; break;
			case ALIGN_MIDDLE: dx = (rect->w - sw) / 2; break;
			case ALIGN_RIGHT: dx = rect->w - sw; break;
		}
		DrawSubString(screen, str + i, j, rect->x + dx, rect->y + dy, color, font);
		i += j + wcsspn(str + i + j, L" "); //skip the rest of spaces to the next line start
		dy += font->h;
	}
	return dy;
}
*/
uint_fast16_t DrawSubStringRectGlyph(Screen *screen, const wchar_t *str, size_t max, Rect *rect, Color color, align a, uint_fast8_t fontsize) {
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
		for (; (sw += GetSubStringWidthGlyph(glyphs + i + k, j, fontsize)) < rect->w && i + j + k < max; ) {
			k += j;
			j = wcsspn(str + i + k, L" "); //next gap
			j += wcscspn(str + i + k + j, L" "); //next word
		}
		if (sw <= rect->w || k == 0) //word do not cross the area border or only one word - take all
			j += k;
		else //word crosses the area border - take all but last word
			j = k;                                                                                                	
		 //add trailing spaces, if any, to draw non-transparent background color and remove trailing word part or added spaces that won't fit
		for (j += wcsspn(str + i + j, L" "); (sw = GetSubStringWidthGlyph(glyphs + i, j, fontsize)) > rect->w; j--);
		switch (a) {
			case ALIGN_LEFT: dx = 0; break;
			case ALIGN_MIDDLE: dx = (rect->w - sw) / 2; break;
			case ALIGN_RIGHT: dx = rect->w - sw; break;
		}
		DrawSubStringGlyph(screen, rect->x + dx, rect->y + dy, str + i, j, color, fontsize);
		i += j + wcsspn(str + i + j, L" "); //skip the rest of spaces to the next line start
		dy += fontsize;
	}
	return dy;
}

uint_fast16_t DrawStringRect(Screen *screen, const wchar_t *str, Rect *rect, TextColors *color, align a, uint_fast8_t fontsize) {
//	return DrawSubStringRect(screen, str, -1, rect, color, font, a);
	return DrawSubStringRectGlyph(screen, str, 0, rect, color->fg, a, fontsize);
}

uint_fast16_t DrawString(Screen *screen, const wchar_t *str, uint32_t x, uint32_t y, Color color, Color bgcolor) {
//	TextColors c = {color, bgcolor};
//	return DrawSubString(screen, str, -1, x, y - font16.h, &c, &font16);
	return DrawSubStringGlyph(screen, x, y, str, 0, color, 16);
}

uint_fast16_t DrawInfo(const wchar_t *info, const wchar_t *action, const wchar_t *format, ...){
	wchar_t str[_MAX_LFN + 1];
	Rect rect = {0};
	va_list va;
	va_start(va, format);
	vswprintf(str, _MAX_LFN + 1, format, va);
	va_end(va);
	rect.y += DrawStringRect(&bottomScreen, str, &rect, &(TextColors){RED, BLACK}, ALIGN_LEFT, 30);
	if (info)
		rect.y += DrawStringRect(&bottomScreen, info, &rect, &(TextColors){RED, BLACK}, ALIGN_LEFT, 30);
	if (action) {
		swprintf(str, _MAX_LFN + 1, lang(SF_PRESS_BUTTON_ACTION), lang(S_ANY_KEY), action);
		rect.y += DrawStringRect(&bottomScreen, str, &rect, &(TextColors){RED, BLACK}, ALIGN_LEFT, 30);
	}
	DisplayScreen(&bottomScreen);
	if (action)
		InputWait();

	return rect.y;
}

void DrawProgress(Screen *screen, Rect *rect, Color frame, Color done, Color back, TextColors *textcolor, FontMetrics *font, uint32_t posmax, uint32_t pos) {
	uint_fast16_t x;
	wchar_t percent[5];
	
	if (!posmax) posmax = UINT32_MAX;
	if (pos > posmax) pos = posmax;	
	x = (rect->w - 2) * pos / posmax;
	swprintf(percent, 5, L"%u%%", 100 * pos / posmax);
	DrawRect(screen, rect, frame);
	FillRect(screen, &(Rect){rect->x + 1, rect->y + 1, x, rect->h - 2}, done);
	FillRect(screen, &(Rect){rect->x + 1 + x, rect->y + 1, rect->w - 2 - x, rect->h - 2}, back);
//	DrawSubString(screen, percent, -1, rect->x + (rect->w - GetSubStringWidth(percent, -1, font)) / 2, rect->y + (rect->h - font->h) / 2, textcolor, font);
	DrawSubStringRectGlyph(screen, percent, 0, rect, textcolor->fg, ALIGN_MIDDLE, 30);
}

void DrawSplash(Screen *screen, wchar_t *splash_file) {
	File Splash;
	if (!FileOpen(&Splash, splash_file, false) ||
		(FileRead2(&Splash, (void*)(screen->buf2), Splash.fsize) != Splash.fsize &&
		(FileClose(&Splash) || true)
	))
		DrawInfo(NULL, lang(S_CONTINUE), lang(SF_FAILED_TO), lang(S_LOAD), splash_file);
	else
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

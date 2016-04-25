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

#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>
#include <wchar.h>

//Colors Macros
#define ARGB(a, r, g, b)	(Color){(uint32_t)(a<<24|r<<16|g<<8|b)} //console asks for B,G,R in bytes
//#define ARGB(a, r, g, b)	(Color){(uint32_t)(a<<24|r<<16|g<<8|b)}
//#define ARGB(a, r, g, b)	(Color){.b=b, .g=g, .r=r, .a=a}
#define RGB(r, g, b)	ARGB(255, r, g, b)
#define COLOR_MASK	ARGB(0, 255, 255, 255)
#define ALPHA_MASK	ARGB(255, 0, 0, 0)
#define TRANSPARENT	ARGB(0, 0, 0, 0)
#define BLACK		RGB(0, 0, 0)
#define WHITE		RGB(255,255,255)
#define RED		RGB(255, 0, 0)
#define GREEN		RGB(0, 255, 0)
#define BLUE		RGB(0, 0, 255)
#define YELLOW		RGB(255, 255, 0)
#define GREY		RGB(0x77, 0x77, 0x77) //GW Gray shade

typedef struct{
	uint_fast16_t w;
	uint_fast16_t h;
	uint_fast8_t bpp;
	uint32_t size;
	void *addr;
	void *buf2;
	uint_fast8_t updated;
} Screen;

typedef struct {
	uint8_t b;
	uint8_t g;
	uint8_t r;
} __attribute__ ((packed)) Pixel;

typedef union {
	uint32_t color;
	struct {
		union {
			Pixel pixel;
			struct {
				uint8_t b;
				uint8_t g;
				uint8_t r;
			};
		};
		uint8_t a;
	};
}  __attribute__ ((aligned, packed)) Color;

typedef struct {
	uint_fast16_t x;
	uint_fast16_t y;
	uint_fast16_t w;
	uint_fast16_t h;
} Rect;

extern Screen top1Screen, top2Screen, bottomScreen;

typedef enum{
	ALIGN_LEFT,
	ALIGN_MIDDLE,
	ALIGN_RIGHT,
} align;

void ClearScreen(Screen *screen, Color color);
void DisplayScreen(Screen *screen);
uint_fast16_t DrawString(Screen *screen, const wchar_t *str, uint32_t x, uint32_t y, Color color, Color bgcolor);
uint_fast16_t DrawStringRect(Screen *screen, const wchar_t *str, Rect *rect, Color color, align a, uint_fast8_t fontsize);
uint_fast16_t DrawSubString(Screen *screen, uint_fast16_t x, uint_fast16_t y, const wchar_t *str, size_t count, Color color, uint_fast8_t fontsize);
void DrawProgress(Screen *screen, Rect *rect, Color frame, Color done, Color back, Color textcolor, uint_fast8_t fontsize, uint32_t posmax, uint32_t pos, uint32_t timeleft);
void DrawSplash(Screen *screen, wchar_t *splash_file);
void fadeOut();
uint_fast16_t DrawInfo(const wchar_t *info, const wchar_t *action, const wchar_t *format, ...);

#endif

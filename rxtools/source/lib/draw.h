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

//Screen Macros
#define BYTES_PER_PIXEL	3  //Our color buffer accepts 24-bits color.
#define BOT_SCREEN_WIDTH	320
#define TOP_SCREEN_WIDTH	400
#define SCREEN_HEIGHT    	240
#define SCREEN_SIZE	(BYTES_PER_PIXEL*BOT_SCREEN_WIDTH*SCREEN_HEIGHT)
#define TOP_SCREEN	(uint8_t*)(*(uint32_t*)0x080FFFC0)
#define TOP_SCREEN2	(uint8_t*)(*(uint32_t*)0x080FFFC8)
#define BOT_SCREEN	(uint8_t*)(*(uint32_t*)0x080FFFD0)

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

//Unicode special characters
#define CHAR_BUTTON_A	"\u2496"
#define CHAR_BUTTON_B	"\u2497"
#define CHAR_BUTTON_L	"\u24C1"
#define CHAR_BUTTON_R	"\u24C7"
#define CHAR_BUTTON_X	"\u24CD"
#define CHAR_BUTTON_Y	"\u24CE"
#define CHAR_SELECTED	"\u2714"
#define CHAR_UNSELECTED	"\u2718"

typedef struct{
	uint_fast16_t w;
	uint_fast16_t h;
	uint_fast8_t bpp;
	uint32_t size;
	void *addr;
	void *buf2;
} Screen;

typedef struct{
	uint_fast8_t sw;
	uint_fast8_t h;
	uint_fast8_t dw;
	uint_fast8_t dwstart; 
	uint32_t *addr;
} FontMetrics;

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
	Color fg;
	Color bg;
} TextColors;

typedef struct {
	uint_fast16_t x;
	uint_fast16_t y;
	uint_fast16_t w;
	uint_fast16_t h;
} Rect;

extern Screen top1Screen, top2Screen, bottomScreen;
extern FontMetrics font16, font24;

/*
typedef enum{
	JUSTIFY_LEFT,
	JUSTIFY_MIDDLE,
	JUSTIFY_RIGHT,
} justify;
*/

void ClearScreen(Screen *screen, Color color);
void DisplayScreen(Screen *screen);
uint_fast16_t DrawString(Screen *screen, const wchar_t *str, uint32_t x, uint32_t y, Color color, Color bgcolor);
uint_fast16_t DrawSubStringRect(Screen *screen, const wchar_t *str, size_t count, Rect *rect, TextColors *color, FontMetrics *font);
uint_fast16_t DrawStringRect(Screen *screen, const wchar_t *str, Rect *rect, TextColors *color, FontMetrics *font);
uint_fast16_t DrawSubString(Screen *screen, const wchar_t *str, size_t count, uint32_t x, uint32_t y, TextColors *color, FontMetrics *font);
uint_fast16_t GetSubStringWidth(const wchar_t *str, size_t count, FontMetrics *font);
void DrawProgress(Screen *screen, Rect *rect, Color border, Color back, Color fill, TextColors *text, FontMetrics *font, uint32_t posmax, uint32_t pos);
void DrawSplash(Screen *screen, wchar_t *splash_file);
void fadeOut();
uint_fast16_t DrawInfo(const wchar_t *info, const wchar_t *action, const wchar_t *format, ...);
	
#endif

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
#define ARGB(a, r, g, b)	(a<<24|r<<16|g<<8|b) //console asks for B,G,R in bytes
#define RGB(r, g, b)	ARGB(255, r, g, b) //opaque color
#define COLOR_MASK	ARGB(0, 255, 255, 255)
#define ALPHA_MASK	ARGB(255, 0, 0, 0)
#define TRANSPARENT	ARGB(0, 0, 0, 0)
#define BLACK		RGB(0, 0, 0)
#define WHITE		RGB(255, 255, 255)
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
	uint32_t w;
	uint32_t h;
	uint32_t bpp;
	uint32_t size;
	void *addr;
} Screen;

typedef struct{
	uint32_t sw;
	uint32_t h;
	uint32_t dw;
	uint32_t dwstart; 
	uint32_t *addr;
} FontMetrics;

typedef struct{
	uint32_t fg;
	uint32_t bg;
} TextColors;

extern Screen top1Screen, top1TmpScreen, top2Screen, top2TmpScreen, bottomScreen, bottomTmpScreen;
extern FontMetrics font16, font24;

/*
typedef enum{
	JUSTIFY_LEFT,
	JUSTIFY_MIDDLE,
	JUSTIFY_RIGHT,
} justify;
*/

void ClearScreen(Screen *screen, uint32_t color);
//void DrawClearScreenAll(void);
uint32_t DrawString(Screen *screen, const wchar_t *str, uint32_t x, uint32_t y, uint32_t color, uint32_t bgcolor);
uint32_t DrawSubStringRect(Screen *screen, const wchar_t *str, size_t count, uint32_t x, uint32_t y, uint32_t w, uint32_t h, TextColors *color, FontMetrics *font);
uint32_t DrawStringRect(Screen *screen, const wchar_t *str, uint32_t x, uint32_t y, uint32_t w, uint32_t h, TextColors *color, FontMetrics *font);
uint32_t DrawSubString(Screen *screen, const wchar_t *str, size_t count, uint32_t x, uint32_t y, TextColors *color, FontMetrics *font);
//void DrawPixel(uint8_t *screen, uint32_t x, uint32_t y, uint32_t color);
//uint32_t GetPixel(uint8_t *screen, uint32_t x, uint32_t y);
//void Debug(const char *format, ...);

void DrawSplash(Screen *screen, wchar_t *splash_file);
void SplashScreen();
void DrawFadeScreen(Screen *screen, uint32_t f);
void fadeOut();
void OpenAnimation();
//Unused functions.
//void DrawHex(uint8_t *screen, uint32_t hex, uint32_t x, uint32_t y, uint32_t color, uint32_t bgcolor);
//void DrawHexWithName(uint8_t *screen, const wchar_t *str, uint32_t hex, uint32_t x, uint32_t y, uint32_t color, uint32_t bgcolor);

#endif

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

#ifndef HID_H
#define HID_H

#include <stdint.h>

#define HID_STATE (*(volatile uint32_t*)0x10146000)

#define BUTTON_A      (1 << 0)
#define BUTTON_B      (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START  (1 << 3)
#define BUTTON_RIGHT  (1 << 4)
#define BUTTON_LEFT   (1 << 5)
#define BUTTON_UP     (1 << 6)
#define BUTTON_DOWN   (1 << 7)
#define BUTTON_R1     (1 << 8)
#define BUTTON_L1     (1 << 9)
#define BUTTON_X      (1 << 10)
#define BUTTON_Y      (1 << 11)

enum {
	KEY_A,
	KEY_B,
	KEY_SELECT,
	KEY_START,
	KEY_DRIGHT,
	KEY_DLEFT,
	KEY_DUP,
	KEY_DDOWN,
	KEY_R,
	KEY_L,
	KEY_X,
	KEY_Y,
	KEY_12,
	KEY_13,
	KEY_ZL,
	KEY_ZR,
	KEY_16,
	KEY_17,
	KEY_18,
	KEY_19,
	KEY_TOUCH,
	KEY_21,
	KEY_22,
	KEY_23,
	KEY_CSTICK_RIGHT,
	KEY_CSTICK_LEFT,
	KEY_CSTICK_UP,
	KEY_CSTICK_DOWN,
	KEY_CPAD_RIGHT,
	KEY_CPAD_LEFT,
	KEY_CPAD_UP,
	KEY_CPAD_DOWN,
	KEY_COUNT
};

typedef struct {
	const char *key;
	const char *name;
	uint32_t mask;
} Key;

extern Key keys[KEY_COUNT];

uint32_t InputWait();
uint32_t GetInput();
void WaitForButton(uint32_t button);

#endif

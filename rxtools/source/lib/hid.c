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

#include "hid.h"
#include "i2c.h"
#include "screenshot.h"
#include "draw.h"

#define HID_PAD (*(volatile uint32_t*)0x10146000)

const Key keys[KEY_COUNT] = {
	{"KEY_A", "[A]", 1<<KEY_A},
	{"KEY_B", "[B]", 1<<KEY_B},
	{"KEY_SELECT", "[SELECT]", 1<<KEY_SELECT},
	{"KEY_START", "[START]", 1<<KEY_START},
	{"KEY_DRIGHT", "[RIGHT]", 1<<KEY_DRIGHT},
	{"KEY_DLEFT", "[LEFT]", 1<<KEY_DLEFT},
	{"KEY_DUP", "[UP]", 1<<KEY_DUP},
	{"KEY_DDOWN", "[DOWN]", 1<<KEY_DDOWN},
	{"KEY_R", "[R]", 1<<KEY_R},
	{"KEY_L", "[L]", 1<<KEY_L},
	{"KEY_X", "[X]", 1<<KEY_X},
	{"KEY_Y", "[Y]", 1<<KEY_Y},
	{"", "", 0},
	{"", "", 0},
	{"KEY_ZL", "[ZL]", 0},
	{"KEY_ZR", "[ZR]", 0},
	{"", "", 0},
	{"", "", 0},
	{"", "", 0},
	{"", "", 0},
	{"", "", 0},
	{"", "", 0},
	{"", "", 0},
	{"", "", 0},
	{"KEY_CSTICK_RIGHT", "[S-RIGHT]", 0},
	{"KEY_CSTICK_LEFT", "[S-LEFT]", 0},
	{"KEY_CSTICK_UP", "[S-UP]", 0},
	{"KEY_CSTICK_DOWN", "[S-DOWN]", 0},
	{"KEY_CPAD_RIGHT", "[P-RIGHT]", 0},
	{"KEY_CPAD_LEFT", "[P-LEFT]", 0},
	{"KEY_CPAD_UP", "[P-UP]", 0},
	{"KEY_CPAD_DOWN", "[P-DOWN]", 0}
};

static void bgWork()
{
	TryScreenShot();

	// Check whether HOME or POWER button has been pressed
	if (*(volatile uint8_t *)0x10147021 == 13) {
		fadeOut();
		// Return to HOME menu
		i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 2);
		while (1);
	}
}

uint32_t InputWait() {
	uint32_t pad_state_old = HID_PAD, pad_state;
	do {
		bgWork();
	} while ((pad_state = HID_PAD) == pad_state_old);
	return ~pad_state;
}

uint32_t GetInput() {
	return ~HID_PAD;
}

void WaitForButton(uint32_t mask) {
	while(InputWait() != mask);
}
/*
 * Copyright (C) 2016 The PASTA Team, dukesrg
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
#include "strings.h"

#define HID_PAD (*(volatile uint32_t*)0x10146000)

const Key keys[KEY_COUNT] = {
	{"KEY_A", &S_BUTTON_A, 1<<KEY_A},
	{"KEY_B", &S_BUTTON_B, 1<<KEY_B},
	{"KEY_SELECT", &S_BUTTON_SELECT, 1<<KEY_SELECT},
	{"KEY_START", &S_BUTTON_START, 1<<KEY_START},
	{"KEY_DRIGHT", &S_BUTTON_RIGHT, 1<<KEY_DRIGHT},
	{"KEY_DLEFT", &S_BUTTON_LEFT, 1<<KEY_DLEFT},
	{"KEY_DUP", &S_BUTTON_UP, 1<<KEY_DUP},
	{"KEY_DDOWN", &S_BUTTON_DOWN, 1<<KEY_DDOWN},
	{"KEY_R", &S_BUTTON_R, 1<<KEY_R},
	{"KEY_L", &S_BUTTON_L, 1<<KEY_L},
	{"KEY_X", &S_BUTTON_X, 1<<KEY_X},
	{"KEY_Y", &S_BUTTON_Y, 1<<KEY_Y},
	{NULL, NULL, 0},
	{NULL, NULL, 0},
	{"KEY_ZL", NULL, 0},
	{"KEY_ZR", NULL, 0},
	{NULL, NULL, 0},
	{NULL, NULL, 0},
	{NULL, NULL, 0},
	{NULL, NULL, 0},
	{NULL, NULL, 0},
	{NULL, NULL, 0},
	{NULL, NULL, 0},
	{NULL, NULL, 0},
	{"KEY_CSTICK_RIGHT", NULL, 0},
	{"KEY_CSTICK_LEFT", NULL, 0},
	{"KEY_CSTICK_UP", NULL, 0},
	{"KEY_CSTICK_DOWN", NULL, 0},
	{"KEY_CPAD_RIGHT", NULL, 0},
	{"KEY_CPAD_LEFT", NULL, 0},
	{"KEY_CPAD_UP", NULL, 0},
	{"KEY_CPAD_DOWN", NULL, 0}
};

void Shutdown(uint_fast8_t reboot) {
	fadeOut();
	i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << (2 * reboot));
	while (1);
}

static void bgWork()
{
	TryScreenShot();

	// Check whether HOME or POWER button has been pressed
	if (*(volatile uint8_t *)0x10147021 == 13)
		Shutdown(1); // Return to HOME menu
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

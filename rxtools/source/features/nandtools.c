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
#include "nandtools.h"
#include "console.h"
#include "fs.h"
#include "lang.h"
#include "hid.h"

const wchar_t *dumppath = L"rxTools/%ls";

void dumpCoolFiles(wchar_t *path)
{
	wchar_t dest[_MAX_LFN + 1], *fn = wcsrchr(path, L'/') + 1;

	ConsoleInit();
	ConsoleSetTitle(strings[STR_DUMP], strings[STR_FILES]);

	swprintf(dest, _MAX_LFN + 1, dumppath, fn);
	print(strings[STR_DUMPING], path, dest);
	ConsoleShow();

	switch ((FSFileCopy(dest, path) >> 8) & 0xFF)
	{
		case 0:
			print(strings[STR_COMPLETED]);
			break;
		case 1:
			print(strings[STR_ERROR_OPENING], path);
			break;
		case 2:
			print(strings[STR_ERROR_CREATING], dest);
			break;
		case 3:
		case 4:
			print(strings[STR_ERROR_READING], path);
			break;
		case 5:
		case 6:
			print(strings[STR_ERROR_WRITING], dest);
			break;
		default:
			print(strings[STR_FAILED]);
			break;
	}

	print(strings[STR_PRESS_BUTTON_ACTION], strings[STR_BUTTON_A], strings[STR_CONTINUE]);
	ConsoleShow();
	WaitForButton(BUTTON_A);
}

void restoreCoolFiles(wchar_t *path)
{
	wchar_t src[_MAX_LFN + 1], *fn = wcsrchr(path, L'/') + 1;

	ConsoleInit();
	ConsoleSetTitle(strings[STR_INJECT], strings[STR_FILES]);

	swprintf(src, _MAX_LFN + 1, dumppath, fn);

	print(strings[STR_INJECTING], src, path);
	ConsoleShow();

	switch ((FSFileCopy(path, src) >> 8) & 0xFF)
	{
		case 0:
			print(strings[STR_COMPLETED]);
			break;
		case 1:
			print(strings[STR_ERROR_OPENING], src);
			break;
		case 2:
			print(strings[STR_ERROR_CREATING], path);
			break;
		case 3:
		case 4:
			print(strings[STR_ERROR_READING], src);
			break;
		case 5:
		case 6:
			print(strings[STR_ERROR_WRITING], path);
			break;
		default:
			print(strings[STR_FAILED]);
			break;
	}

	print(strings[STR_PRESS_BUTTON_ACTION], strings[STR_BUTTON_A], strings[STR_CONTINUE]);
	ConsoleShow();
	WaitForButton(BUTTON_A);
}

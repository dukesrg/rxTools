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

#ifndef CFW_H
#define CFW_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <reboot.h>
#include "fs.h"
#include "aes.h"

#define FIRM_MAGIC 'MRIF'

typedef struct {
	aes_key_data keyX;
	aes_key_data keyY;
	aes_ctr_data ctr;
	char size[8];
	uint8_t pad[8];
	uint8_t control[AES_BLOCK_SIZE];
	union {
		uint32_t pad[8];
		struct {
			uint8_t unk[16];
			aes_key_data keyX_0x16;
		} s;
	} ext;
} Arm9Hdr;

extern const wchar_t *firmPathFmt;
extern const wchar_t *firmPatchPathFmt;

int PastaMode();
void FirmLoader();
int rxMode(int_fast8_t drive);
uint8_t* decryptFirmTitleNcch(uint8_t* title, size_t *size);
uint8_t *decryptFirmTitle(uint8_t *title, size_t size, size_t *firmSize, uint8_t key[16]);
FRESULT applyPatch(void *file, const char *patch);

static inline int getFirmPath(wchar_t *s, TitleIdLo id)
{
	return swprintf(s, _MAX_LFN, firmPathFmt, TID_HI_FIRM, id);
}

static inline int getFirmPatchPath(wchar_t *s, TitleIdLo id)
{
	return swprintf(s, _MAX_LFN, firmPatchPathFmt, TID_HI_FIRM, id);
}

#endif
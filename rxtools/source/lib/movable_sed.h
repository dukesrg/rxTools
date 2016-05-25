/*
 * Copyright (C) 2016 dukesrg
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

#ifndef MOVABLE_SED_H
#define MOVABLE_SED_H

#include <stdint.h>
#include "aes.h"

#define SEED_MAGIC 'DEES'

typedef struct {
	uint32_t magic;
	uint32_t flags;
	uint8_t signature[0x100];
	uint8_t reserved[8];
	aes_key_data keyY;
} movable_sed;

uint_fast8_t movablesedVerify(wchar_t *path);
uint_fast8_t movablesedGetID0(wchar_t *id0, wchar_t *path);

#endif
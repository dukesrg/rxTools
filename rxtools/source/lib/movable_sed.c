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

#include "movable_sed.h"
#include "fs.h"
#include "sha.h"

static uint_fast8_t movablesedLoad(movable_sed *seed, wchar_t *path) {
	FIL fil;
	
	if (!FileOpen(&fil, path, 0) || (
		(FileRead2(&fil, seed, sizeof(movable_sed)) != sizeof(movable_sed) ||
		seed->magic != SEED_MAGIC) &&
//todo: RSA check
		(FileClose(&fil) || 1)
	)) return 0;
	FileClose(&fil);

	return 1;
}

uint_fast8_t movablesedVerify(wchar_t *path) {
	movable_sed seed;
	return movablesedLoad(&seed, path);
}

uint_fast8_t movablesedGetID0(wchar_t *id0, wchar_t *path) {
	movable_sed seed;
	uint32_t hash[SHA_256_SIZE / sizeof(uint32_t)];

	if (!movablesedLoad(&seed, path))
		return 0;

	sha(hash, &seed.keyY, sizeof(seed.keyY), SHA_256_MODE);
	swprintf(id0, 33, L"%08x%08x%08x%08x", hash[0], hash[1], hash[2], hash[3]);

	return 1;
}
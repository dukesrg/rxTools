/*
 * Copyright (C) 2015-2016 b1l1s, dukesrg 
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

#include <string.h>
#include "rsa.h"
#include "sha.h"

static inline void rsa_wait() {
	while (*REG_RSA_CNT & RSA_CNT_START);
}

void rsa_set_keyslot(uint_fast8_t keyslot) {
	if (keyslot >= RSA_SLOT_COUNT)
		return;
	*REG_RSA_CNT = (*REG_RSA_CNT & ~RSA_CNT_KEYSLOTS) | (keyslot << 4);
}

void rsa_set_key(uint_fast8_t keyslot, void *mod, void *exp, uint32_t size, uint32_t mode) {
	if (keyslot >= RSA_SLOT_COUNT)
		return;
	rsa_wait();

	*REG_RSA_CNT = (*REG_RSA_CNT & ~RSA_CNT_KEYSLOTS) | (keyslot << 4) | mode;
	
	REG_RSA_SLOTS[keyslot].CNT &= ~(RSA_SLOTCNT_KEY_SET | RSA_SLOTCNT_WRITE_PROTECT);
	REG_RSA_SLOTS[keyslot].SIZE = size >> 2;

	memcpy((void*)(REG_RSA_MOD + RSA_MOD_SIZE - size), mod, size);
	
	size >>= 2;
	if (exp)
		for (uint32_t *exp32 = (uint32_t*)exp; size--; *REG_RSA_EXPFIFO = *exp32++);
	else {
		while (--size) *REG_RSA_EXPFIFO = 0;
		*REG_RSA_EXPFIFO = 0x01000100; // 65537 BE
	}
}

void rsa(void *dst, void *src, size_t sigSize, uint32_t mode) {
	if (!(REG_RSA_SLOTS[(*REG_RSA_CNT & RSA_CNT_KEYSLOTS) >> 4].CNT & RSA_SLOTCNT_KEY_SET))
		return;

	rsa_wait();
	*REG_RSA_CNT = (*REG_RSA_CNT & ~(RSA_CNT_ENDIAN | RSA_CNT_ORDER)) | mode;
	
	size_t padSize = (8 - sigSize) & 0x07;
	memset((void*)(REG_RSA_TXT + RSA_TXT_SIZE - sigSize - padSize), 0, padSize);
	memcpy((void*)(REG_RSA_TXT + RSA_TXT_SIZE - sigSize), src, sigSize);

	*REG_RSA_CNT |= RSA_CNT_START;
	
	rsa_wait();
	memcpy(dst, (void*)(REG_RSA_TXT + RSA_TXT_SIZE - sigSize), sigSize);
}

uint_fast8_t rsa_verify(void *data, size_t size, void *sig, uint32_t sigSize) {
	uint8_t hash[SHA_256_SIZE], decSig[sigSize];

	sha(hash, data, size, SHA_256_MODE);
	rsa(decSig, sig, sigSize, RSA_IO_BE_NORMAL);

	return !memcmp(hash, decSig + sigSize - SHA_256_SIZE, SHA_256_SIZE);
}

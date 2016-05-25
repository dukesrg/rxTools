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
#include "sha.h"

static inline void sha_wait() {
	while (*REG_SHA_CNT & (SHA_NORMAL_ROUND | SHA_FINAL_ROUND));
}

void sha_start(uint32_t mode, void *init) {
	sha_wait();
	*REG_SHA_CNT = mode | SHA_OUTPUT_BE | SHA_NORMAL_ROUND;
	if (init) memcpy((void*)REG_SHA_HASH, init, SHA_256_SIZE);
}

void sha_update(void *data, size_t size) {
	if (!data || !size) return;
	uint32_t *src32 = (uint32_t *)data;
	while (size >= SHA_INFIFO_SIZE) {
		sha_wait();
		memcpy((void*)REG_SHA_INFIFO, src32, SHA_INFIFO_SIZE);
		src32 += SHA_INFIFO_SIZE/sizeof(uint32_t);
		size -= SHA_INFIFO_SIZE;
	}
	sha_wait();
	memcpy((void*)REG_SHA_INFIFO, src32, size);
}

void sha_finish(void *hash) {
	sha_wait();
	uint32_t mode = *REG_SHA_CNT;
	*REG_SHA_CNT = mode | SHA_FINAL_ROUND;
	sha_wait();
	memcpy(hash, (void*)REG_SHA_HASH, mode & SHA_1_MODE ? SHA_1_SIZE : mode & SHA_224_MODE ? SHA_224_SIZE : SHA_256_SIZE);
}

void sha(void *hash, void *data, size_t size, uint32_t mode) {
	sha_start(mode, NULL);
	sha_update(data, size);
	sha_finish(hash);
}

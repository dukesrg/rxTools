/*
 * Copyright (C) 2017 dukesrg
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

#ifndef TICKET_H
#define TICKET_H

#include <stdint.h>
#include "aes.h"

typedef struct {
	char issuer[0x40];
	uint8_t public_key[0x3C];
	uint8_t version;
	uint8_t ca_crl_version;
	uint8_t signer_crl_version;
	aes_key_data key;
	uint8_t reserved_1;
	union {
		uint64_t ticket_id __attribute__((packed));
		struct {
			uint32_t ticket_id_hi;
			uint32_t ticket_id_lo;
		};
	};
	uint32_t console_id;
	union {
		uint64_t title_id __attribute__((packed));
		struct {
			uint32_t title_id_hi;
			uint32_t title_id_lo;
		};
	};
	uint8_t reserved_2[2];
	uint16_t title_version;
	uint8_t reserved_3[8];
	uint8_t license_type;
	uint8_t key_index;
	uint8_t reserved_4[0x2A];
	uint32_t eshop_account_id;
	uint8_t reserved_5;
	uint8_t audit;
	uint8_t reserved_6[0x42];
	uint8_t limits[0x40];
	uint8_t content_index[0xAC];
} __attribute__((packed)) ticket_data;

typedef struct {
	uint32_t sig_type;
	uint8_t *sig;
	ticket_data ticket;
} cetk_data;

uint_fast8_t ticketGetKey(aes_key *key, uint64_t titleid, uint_fast8_t drive);
uint_fast8_t ticketGetKeyCetk(aes_key *key, uint64_t titleid, wchar_t *path);

#endif

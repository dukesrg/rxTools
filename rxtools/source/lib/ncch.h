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

#ifndef NCCH_H
#define NCCH_H

#include <stdint.h>
#include "aes.h"
#include "crypto.h"

#define NCCH_MEDIA_UNIT_SIZE 0x200

typedef enum {
	NCCHTYPE_EXHEADER = 0x01000000,
	NCCHTYPE_EXEFS = 0x02000000,
	NCCHTYPE_ROMFS = 0x03000000
} ctr_ncchtype;

typedef enum {
	NCCHFLAG_PLATFORMCTR = 0x01,
	NCCHFLAG_PLATFORMKTR = 0x02,
	NCCHFLAG_CONTENTTYPEDATA = 0x01,
	NCCHFLAG_CONTENTTYPEXECUTABLE = 0x02,
	NCCHFLAG_CONTENTTYPSYSTEMUPDATE = 0x04,
	NCCHFLAG_CONTENTTYPEMANUAL = 0x08,
	NCCHFLAG_CONTENTTYPECHILD = 0x0C,
	NCCHFLAG_CONTENTTYPETRIAL = 0x10,
	NCCHFLAG_FIXEDCRYPTOKEY = 0x01,
	NCCHFLAG_NOMOUNTROMFS = 0x02,
	NCCHFLAG_NOCRYPTO = 0x04
} ctr_ncchflag;

typedef struct {
	uint8_t signature[0x100];
	uint32_t magic;
	uint32_t contentsize;
	uint64_t partitionid;
	uint16_t makercode;
	uint16_t version;
	uint8_t hash[4];
	union {
		uint64_t programid;
		struct {
			uint32_t programid_lo;
			uint32_t programid_hi;
		};
	};
	uint8_t reserved1[0x10];
	uint8_t logoregionhash[0x20];
	char productcode[0x10];
	uint8_t extendedheaderhash[0x20];
	uint32_t extendedheadersize;
	uint8_t reserved2[4];
	uint8_t flags0;
	uint8_t flags1;
	uint8_t flags2;
	uint8_t cryptomethod;
	uint8_t contentplatform;
	uint8_t contenttype;
	uint8_t contentunitsize;
	uint8_t flags7;
	uint32_t plainregionoffset;
	uint32_t plainregionsize;
	uint32_t logoregionoffset;
	uint32_t logoregionsize;
	uint32_t exefsoffset;
	uint32_t exefssize;
	uint32_t exefshashregionsize;
	uint8_t reserved4[4];
	uint32_t romfsoffset;
	uint32_t romfssize;
	uint32_t romfshashregionsize;
	uint8_t reserved5[4];
	uint8_t exefssuperblockhash[0x20];
	uint8_t romfssuperblockhash[0x20];
} __attribute__((packed)) ctr_ncchheader;

typedef struct {
	uint8_t system_control_info[0x200];
	uint8_t access_control_info[0x200];
	uint8_t accessdesc_signature[0x100];
	uint8_t ncch_hdr_public_key[0x100];
	uint8_t access_control_info2[0x200];
} __attribute__((packed)) ctr_ncchexheader;

void ncch_get_counter(ctr_ncchheader *header, aes_ctr *counter, ctr_ncchtype type);

#endif

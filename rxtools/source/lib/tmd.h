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

#ifndef TMD_H
#define TMD_H

#include <stdint.h>
#include <wchar.h>

#define TMD_MAX_CHUNKS 0x40

typedef enum {
	CONTENT_TYPE_ENCRYPTED = 0x0100,
	CONTENT_TYPE_DISC = 0x0200,
	CONTENT_TYPE_CFM = 0x0400,
	CONTENT_TYPE_OPTIONAL = 0x0040,
	CONTENT_TYPE_SHARED = 0x0080
} tmd_content_type; //BE order

typedef enum {
	CONTENT_INDEX_MAIN = 0x0000,
	CONTENT_INDEX_MANUAL = 0x0100,
	CONTENT_INDEX_DLP_CHILD = 0x0200,
	CONTENT_INDEX_ALL = 0xFFFF //Used in code only to indicate any type index filter
} tmd_contend_index; //BE order

typedef struct {
	uint8_t issuer[0x40];
	uint8_t version;
	uint8_t ca_crl_version;
	uint8_t signer_crl_version;
	uint8_t reserved_1;
	union {
		uint64_t system_version;
		struct {
			uint32_t system_version_hi;
			uint32_t system_version_lo;
		};
	};
	union {
		uint64_t title_id;
		struct {
			uint32_t title_id_hi;
			uint32_t title_id_lo;
		};
	};
	uint32_t title_type;
	uint16_t group_id;
	uint32_t srl_public_save_data_size;
	uint32_t srl_private_save_data_size;
	uint32_t reserved_2;
	uint8_t srl_flag;
	uint8_t reserved_3[0x31];
	uint32_t access_rights;
	uint16_t title_version;
	uint16_t content_count;
	uint16_t boot_content;
	uint16_t reserved_4;
	uint8_t content_info_hash[0x20];
} __attribute__((packed)) tmd_header;

typedef struct {
	uint16_t content_index_offset;
	uint16_t content_command_count;
	uint8_t content_chunk_hash[0x20];
} __attribute__((packed)) tmd_content_info;

typedef struct {
	uint32_t content_id;
	uint16_t content_index;
	uint16_t content_type;
	union {
		uint64_t content_size;
		struct {
			uint32_t content_size_hi;
			uint32_t content_size_lo;
		};
	};
	uint8_t content_hash[0x20];
} __attribute__((packed)) tmd_content_chunk;

typedef struct {
	uint32_t sig_type;
	uint8_t *sig;
	tmd_header header;
	tmd_content_info content_info[0x40];
	tmd_content_chunk *content_chunk;
} tmd_data;

//bool tmdLoad(wchar_t *apppath, tmd_data *data, uint32_t drive);
uint_fast8_t tmdLoadHeader(tmd_data *data, wchar_t *path);
uint_fast8_t tmdValidateChunk(tmd_data *data, wchar_t *path, uint_fast16_t content_index, uint_fast8_t drive);
size_t tmdGetChunkSize(tmd_data *data, wchar_t *path, uint_fast16_t content_index);
uint32_t tmdPreloadRecent(tmd_data *data, wchar_t *path);
uint32_t tmdLoadRecent(tmd_data *data, wchar_t *path);
size_t tmdPreloadHeader(tmd_data *data, wchar_t *path);
size_t tmdPreloadChunk(tmd_data *data, wchar_t *path, uint_fast16_t content_index);

#endif

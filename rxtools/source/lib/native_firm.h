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

#ifndef FIRM_H
#define FIRM_H

#include <stdint.h>

#define FIRM_MAGIC 'MRIF'

typedef enum {
	COPY_METHOD_NDMA = 0,
	COPY_METHOD_XDMA = 1,
	COPY_METHOD_CPU = 2
} firm_copy_method;

typedef struct {
	uint32_t offset;
	uint32_t load_address;
	uint32_t size;
	uint32_t copy_method;
	uint8_t hash[0x20];
} __attribute__((packed)) firm_section_header;

typedef struct {
	uint32_t magic;
	uint32_t boot_priority;
	uint32_t arm11_entry;
	uint32_t arm9_entry;
	uint8_t reserved[0x30];
	firm_section_header sections[4];
	uint8_t signature[0x100];
} __attribute__((packed)) firm_header;

firm_section_header *firmFindSection(firm_header *firm, uint32_t address);

#endif

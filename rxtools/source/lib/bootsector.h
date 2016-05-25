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

#ifndef BOOTSECTOR_H
#define BOOTSECTOR_H

#include <stdint.h>

#define END_OF_SECTOR_MARKER 0xAA55
#define PARTITION_MAX_COUNT 4
#define FAT_VOLUME_LABEL_OFFSET 3

enum {
	BOOT_INDICATOR_NOT_ACTIVE = 0,
	BOOT_INDICATOR_ACTIVE = 0x80
};

enum {
	PARTITION_TYPE_NONE = 0, 
	PARTITION_TYPE_FAT32 = 0x0C,
	PARTITION_TYPE_3DS_NAND = 0xD3
//	PARTITION_TYPE_2DS_NAND = 0x2D,
//	PARTITION_TYPE_3DS_NAND = 0x30,
//	PARTITION_TYPE_NEW3DS_NAND = 0x3E
};

typedef struct {
	uint8_t head;
	uint8_t sector;
	uint8_t cylinder;
} __attribute__((packed)) chs;

typedef struct {
	uint8_t boot_indicator;
	chs chs_starting;
	uint8_t system_id;
	chs chs_ending;
	uint32_t relative_sectors;
	uint32_t total_sectors;
} __attribute__((packed)) partition_table_entry;

typedef struct {
	partition_table_entry partition[PARTITION_MAX_COUNT];
	uint16_t marker;
} __attribute__((packed)) partition_table;

typedef struct {
	uint8_t bootstrap[0x1BE];
	partition_table partition_table;
} __attribute__((packed)) mbr;

typedef struct {
	uint8_t physical_drive_number;
	uint8_t reserved;
	uint8_t extended_boot_signature;
	uint32_t volume_serial_number;
	char volume_label[11];
	char file_system_type[8];
} __attribute__((packed)) extended_btb;

struct {
	uint8_t jump[3];
	uint8_t oem_id[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t number_of_fats;
	uint16_t root_entries;
	uint16_t small_sectors;
	uint8_t media_descriptor;
	uint16_t sectors_per_fat;
	uint16_t sectors_per_track;
	uint16_t number_of_heads;
	uint32_t hidden_sectors;
	uint32_t large_sectors;
	union {
		struct {
			extended_btb extended_btb;
			uint_fast8_t bootstrap[448];
		} fat16;
		struct {
			uint32_t sectors_per_fat;
			uint16_t extended_flags;
			uint16_t file_system_version;
			uint32_t root_cluster_number;
			uint32_t fsinfo_sector;
			uint16_t backup_boot_sector;
			uint8_t reserved[16];
			extended_btb extended_btb;
			uint_fast8_t bootstrap[420];
		} fat32;
	};
	uint16_t magic;
} __attribute__((packed)) boot_sector;

#endif

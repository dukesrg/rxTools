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

#include <stddef.h>
#include "bootsector.h"

#define SECTOR_SIZE 0x200
#define MEDIA_DESCRIPTOR_FIXED_DISK 0xF8
#define CHS_SECTORS_LIMIT 0xFB0400
#define CHS_MAX_CYLINDER 0x3FF
#define CHS_MAX_HEAD 0xFF
#define CHS_MAX_SECTOR 0x3F
#define FAT32_MAX_SECTORS_PER_CLUSTER 0x80
#define FAT32_BYTES 4
#define FAT16_BYTES 2

static struct {
	uint32_t max_sectors;
	uint8_t number_of_heads;
	uint8_t sectors_per_track;
} chs_param[] = {
	{0x1000, 0x2, 0x10}, //2MB
	{0x8000, 0x2, 0x20}, //16MB
	{0x10000, 0x4, 0x20}, //32MB
	{0x40000, 0x8, 0x20}, //128MB
	{0x80000, 0x10, 0x20}, //256MB
	{0xFC000, 0x10, CHS_MAX_SECTOR}, //504MB
	{0x1F8000, 0x20, CHS_MAX_SECTOR}, //1008MB
	{0x3F0000, 0x40, CHS_MAX_SECTOR}, //2016MB
	{0x7E0000, 0x80, CHS_MAX_SECTOR}, //4032MB
	{0xFFFFFFFF, CHS_MAX_HEAD, CHS_MAX_SECTOR} //2TB
};

static struct {
	uint32_t max_sectors;
	uint16_t sectors_per_cluster;
	uint32_t boundary_unit;
} sdparam[] = {
	{0x4000, 0x10, 0x10}, //8MB
	{0x20000, 0x20, 0x20}, //64MB
	{0x80000, 0x20, 0x40}, //256MB
	{0x200000, 0x20, 0x80}, //1GB
	{0x400000, 0x40, 0x80}, //2GB
	{0x1000000, 0x40, 0x2000}, //8GB
	{0x4000000, 0x40, 0x2000}, //32GB
	{0x10000000, 0x100, 0x8000}, //128GB
	{0x40000000, 0x200, 0x10000}, //512GB
	{0xFFFFFFFF, 0x400, 0x20000} //2TB
};

static void chs_get(chs *CHS, uint32_t sector, uint_fast8_t number_of_heads, uint_fast8_t sectors_per_track) {
	uint_fast16_t cylinder;
	if (sector < CHS_SECTORS_LIMIT) {
		CHS->head = sector % (number_of_heads * sectors_per_track) / sectors_per_track;
		cylinder = sector / (number_of_heads * sectors_per_track);
		CHS->cylinder = cylinder & 0xFF;
		CHS->sector = (sector % sectors_per_track + 1) | (cylinder >> 2 & 0xC0);
	} else
		*CHS = (chs){0xFF, 0xFF, 0xFF};
}

void chs_calc(partition_table_entry *partition, uint32_t drive_total_sectors) {
	size_t i;
	for (i = 0; drive_total_sectors > chs_param[i].max_sectors; i++);
	chs_get(&partition->chs_starting, partition->relative_sectors, chs_param[i].number_of_heads, chs_param[i].sectors_per_track);
	chs_get(&partition->chs_ending, partition->total_sectors, chs_param[i].number_of_heads, chs_param[i].sectors_per_track);
};

uint_fast8_t sdcalc(uint32_t drive_sectors, uint32_t partition_sectors) { //calculate FAT32 parameters according to SD File System Specification V3 and SDXC FAT32 trick
	uint32_t mbr_sectors_count, sectors_per_cluster, sectors_per_fat, sectors_per_fat_new, reserved_sectors_count, system_area_sectors, maximum_cluster_number;
	size_t i;

	for (i = 0; drive_sectors > sdparam[i].max_sectors; i++);
	if (drive_sectors == partition_sectors) //all data to single partition - reserve space for MBR
		mbr_sectors_count = sdparam[i].boundary_unit;
	else if (drive_sectors >= partition_sectors + sdparam[i].boundary_unit) //additional partition - ensure enough space for MBR was reserved
		mbr_sectors_count = 0;
	else
		return 0;
	sectors_per_cluster = sdparam[i].max_sectors;
	if (sectors_per_cluster > FAT32_MAX_SECTORS_PER_CLUSTER)
		sectors_per_cluster = FAT32_MAX_SECTORS_PER_CLUSTER;
	sectors_per_fat = (partition_sectors * FAT32_BYTES + sdparam[i].sectors_per_cluster * SECTOR_SIZE - 1) / (sdparam[i].sectors_per_cluster * SECTOR_SIZE);
	do {
		reserved_sectors_count = sdparam[i].boundary_unit - 2 * sectors_per_fat % sdparam[i].boundary_unit;
		if (reserved_sectors_count < 9)
			reserved_sectors_count += sdparam[i].boundary_unit;
		do {
			system_area_sectors = reserved_sectors_count + 2 * sectors_per_fat;
			maximum_cluster_number = (partition_sectors - mbr_sectors_count - system_area_sectors) / sdparam[i].sectors_per_cluster + 1;
			sectors_per_fat_new = (2 + maximum_cluster_number - 1) * FAT32_BYTES / SECTOR_SIZE;
		} while (sectors_per_fat_new > sectors_per_fat && (reserved_sectors_count += sdparam[i].boundary_unit));
	} while (sectors_per_fat_new != sectors_per_fat && sectors_per_fat--);
	return 1;
}
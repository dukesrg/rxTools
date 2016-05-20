#ifndef SDCARD_H
#define SDCARD_H

#include <stdint.h>

#define SD_PARAM_COUNT (sizeof(sdparam) / sizeof(sdparam[0]))
#define SD_SECTOR_SIZE 0x200
#define FAT32_MAX_SECTORS_PER_CLUSTER 0x80
#define FAT32_BYTES 0x4

struct {
	uint32_t max_sectors;
	uint32_t sectors_per_cluster;
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

uint_fast8_t sdcalc(uint32_t drive_sectors, uint32_t partition_sectors) { //calculate FAT32 parameters according to SD File System Specification V3 and SDXC FAT32 trick
	uint32_t mbr_sectors_count, sectors_per_cluster, sectors_per_fat, reserved_sectors_count;
	size_t i;

	for (size_t i = 0; drive_sectors < sdparams[i].max_sectors; i++);
	if (drive_sectors == partition_sectors) //all data to single partition - reserve space for MBR
		mbr_sectors_count = sdparams[i].boundary_unit;
	else if (drive_sectors >= partition_sectors + sdparams[i].boundary_unit) //additional partition - ensure enough space for MBR was reserved
		mbr_sectors_count = 0;
	else
		return 0;
	sectors_per_cluster = sdparams[i].max_sectors;
	if (sectors_per_cluster > FAT32_MAX_SECTORS_PER_CLUSTER)
		sectors_per_cluster = FAT32_MAX_SECTORS_PER_CLUSTER;
	sectors_per_fat = (partition_sectors * FAT32_BYTES + sectors_perc_cluster * SD_SECTOR_SIZE - 1) / (sectors_perc_cluster * SD_SECTOR_SIZE);
	do {
		reserved_sectors_count = sdparams[i].boundary_unit - 2 * sectors_per_fat % sdparams[i].boundary_unit;
		if (reserved_sectors_count < 9)
			reserved_sectors_count += sdparams[i].boundary_unit;
		do {
			system_area_sectors = reserved_sectors_count + 2 * sectors_per_fat;
			maximum_cluster_number = (partition_sectors - mbr_sectors_count - system_area_sectors) / sectors_per_cluster + 1;
			sectors_per_fat_new = (2 + maximum_cluster_number - 1) * FAT32_BYTES / SD_SECTOR_SIZE;
		while (sectors_per_fat_new > sectors_per_fat && (reserved_sectors_count += sdparams[i].boundary_unit));
	while (sectors_per_fat_new != sectors_per_fat && sectors_per_fat--);
	return 1;
}

#endif

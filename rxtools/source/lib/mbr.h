#ifndef MBR_H
#define MBR_H

#define MBR_BOOT_MAGIC = 0xAA55;

enum {
	MBR_PARTITION_NOT_ACTIVE 0,
	MBR_PARTITION_ACTIVE 0x80
};

enum {
	MBR_PARTITION_TYPE_NONE 0, 
	MBR_PARTITION_TYPE_FAT32 0x0C,
	MBR_PARTITION_TYPE_2DS_NAND 0x2D,
	MBR_PARTITION_TYPE_3DS_NAND 0x30,
	MBR_PARTITION_TYPE_NEW3DS_NAND 0x3E
};

typedef struct
	uint8_t active;
	uint8_t chs_start[3];
	uint8_t type;
	uint8_t chs_end[3];
	uint32_t lba_first_sector;
	uint32_t lba_sectors_count;
} __atribute__((packed)) mbr_partition_entry[4];

typedef struct {
	mbr_partition_entry partition[4];
	uint16_t magic;
} __atribute__((packed)) mbr_partition_table;

typedef struct {
	uint8_t bootstrap[0x160];
	mbr_partition_table partition_table;
} __atribute__((packed)) mbr;

#nedif

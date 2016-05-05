#ifndef NCSD_H
#define NCSD_H

#include "mbr.h"

#define NCSD_MAGIC 'DSCN'
#define NCSD_PARTITION_COUNT 8

enum {
 	NCSD_PARTITION_FS_NONE = 0,
 	NCSD_PARTITINO_FS_NORMAL = 1,
 	NCSD_PARTITION_FS_FIRM = 3,
 	NCSD_PARTITION_FS_AGB_FIRM_SAVE = 4
};

typedef struct {
	uint8_t signature[0x100];
	uint32_t magic;
	uint32_t size;
	union {
		int64_t media_id;
		struct {
			uint32_t media_id_lo;
			uint32_t media_id_hi;
		};
	};
	uint8_t partition_fs_type[8];
	uint8_t partition_crypt_type[8];
	struct {
		uint32_t offset;
		uint32_t size;
	} partition[8];
	union {
		struct {
			uint8_t reserved[0x5E];
			mbr_partition_table twl_partition_table;
		} nand;
		struct {
			uint8_t reserved[0xA0];
		} card;
	};
} __attribute__((packed)) ncsd_header;

#endif

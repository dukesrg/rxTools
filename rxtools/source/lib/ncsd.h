#ifndef NCSD_H
#define NCSD_H

#include "mbr.h"

enum {
 	PARTITION_FS_NONE = 0,
 	PARTITINO_FS_NORMAL = 1,
 	PARTITION_FS_FIRM = 3,
 	PARTITION_FS_AGB_FIRM_SAVE = 4
};

typedef struct {
	uint8_t signature[100];
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
			mbr twl_mbr;
		} nand;
		struct (
			uint8_t reserved[0xA0];
		} card;
	};
} __attribute__((packed)) ncsd_header;

#endif

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
 
#ifndef BLOBS_H
#define BLOBS_H

#include <stdint.h>

typedef struct {
	uint8_t cmac[0x10];
	uint8_t pad[0xf0];
} __attribute__((packed)) aes_cmac_header;	

#define DISA_MAGIC	'ASID'
#define DISA_PARTITION_PRIMARY	0
#define DISA_PARTITION_SECONDARY	1

typedef struct {
	uint32_t magic;
	uint32_t format_version;
	uint64_t partition_entries_count;
	uint64_t secondary_partition_offset;
	uint64_t primary_partition_offset;
	uint64_t partition_size;
	uint64_t save_partition_entry_offset;
	uint64_t save_partition_entry_size;
	uint64_t data_partition_entry_offset;
	uint64_t data_partition_entry_size;
	uint64_t save_partition_offset;
	uint64_t save_partition_size;
	uint64_t data_partition_offset;
	uint64_t data_partition_size;
	uint32_t active_partition;
	uint8_t hash[0x20];
	uint8_t reserved[0x74];
} __attribute__((packed)) disa_header;	

#define DIFF_MAGIC	'FFID'
#define DIFF_PARTITION_PRIMARY	0
#define DIFF_PARTITION_SECONDARY	1

typedef struct {
	uint32_t magic;
	uint32_t format_version;
	uint64_t secondary_partition_offset;
	uint64_t primary_partition_offset;
	uint64_t partition_size;
	uint64_t file_base_offset;
	uint64_t file_base_size;
	uint32_t active_partition;
	uint8_t hash[0x20];
	uint64_t extdata_uid;
	uint8_t reserved[0xA4];
} __attribute__((packed)) diff_header;

#define DIFI_MAGIC	'IFID'

typedef struct {
	uint32_t magic;
	uint32_t format_version;
	uint64_t ivfc_offset;
	uint64_t ivfc_size;
	uint64_t dpfs_offset;
	uint64_t dpfs_size;
	uint64_t hash_offset;
	uint64_t hash_size;
	uint32_t flags;
	uint64_t file_base_offset;
} __attribute__((packed)) difi_header;

#define IVFC_MAGIC	'CFVI'
#define IVFC_VERSION_1	0x00010000
#define IVFC_VERSION_2	0x00020000

typedef struct {
	uint32_t magic;
	uint32_t format_version;
	uint64_t master_hash_size;
	union {
		uint64_t l1_offset;
		struct {
			uint32_t l1_offset_lo;
			uint32_t l1_offset_hi;
		};
	};
	union {
		uint64_t l1_hashdata_size;
		struct {
			uint32_t l1_hashdata_size_lo;
			uint32_t l1_hashdata_size_hi;
		};
	};
	uint32_t l1_block_size_log2;
	uint32_t reserved1;
	union {
		uint64_t l2_offset;
		struct {
			uint32_t l2_offset_lo;
			uint32_t l2_offset_hi;
		};
	};
	union {
		uint64_t l2_hashdata_size;
		struct {
			uint32_t l2_hashdata_size_lo;
			uint32_t l2_hashdata_size_hi;
		};
	};
	uint32_t l2_block_size_log2;
	uint32_t reserved2;
	union {
		uint64_t l3_offset;
		struct {
			uint32_t l3_offset_lo;
			uint32_t l3_offset_hi;
		};
	};
	union {
		uint64_t l3_hashdata_size;
		struct {
			uint32_t l3_hashdata_size_lo;
			uint32_t l3_hashdata_size_hi;
		};
	};
	uint32_t l3_block_size_log2;
	uint32_t reserved3;
	union {
		uint64_t l4_offset;
		struct {
			uint32_t l4_offset_lo;
			uint32_t l4_offset_hi;
		};
	};
	union {
		uint64_t l4_hashdata_size;
		struct {
			uint32_t l4_hashdata_size_lo;
			uint32_t l4_hashdata_size_hi;
		};
	};
	uint32_t l4_block_size_log2;
	uint64_t unknown;
} __attribute__((packed)) ivfc_v2_header;

#define DPFS_MAGIC	'SFPD'

typedef struct {
	uint32_t magic;
	uint32_t format_version;
	uint64_t first_table_offset;
	uint64_t first_table_size;
	uint64_t first_table_block_size_log2;
	uint64_t second_table_offset;
	uint64_t second_table_size;
	uint64_t second_table_block_size_log2;
	uint64_t ivfc_partiton_offset;
	uint64_t ivfc_partiton_size;
	uint64_t ivfc_partiton_block_size_log2;
} __attribute__((packed)) dpfs_header;

#define TICK_MAGIC	'KCIT'

typedef struct {
	uint32_t magic;
	uint32_t unknown1; //0x00000001
	uint8_t unknown2[8];
} __attribute__((packed)) tick_header;

#define BDRI_MAGIC	'IRDB'

typedef struct {
	uint32_t magic;
	uint32_t format_version;
	uint64_t unknown1;
	uint64_t size_in_blocks;
	uint32_t block_size;
	uint32_t reserved;
	uint8_t unknown2[0x20];
	uint8_t unknown3[0x18];
	uint64_t title_entry_table_offset;
	uint8_t unknown4[0x20];
} __attribute__((packed)) bdri_header;	

typedef struct {
	uint32_t magic1;
	uint32_t magic2;
	uint8_t reserved1[0x24];
	uint32_t entries_used;
} __attribute__((packed)) title_entry_header_1;

typedef struct {
	uint32_t entries_total;
	uint32_t unknown; //0x2001
	uint8_t reserved2[0x20];
} __attribute__((packed)) title_entry_header_2;

typedef struct {
	uint32_t unknown1;
	uint32_t used;
	union {
		uint64_t title_id;
		struct {
			uint32_t title_id_lo;
			uint32_t title_id_hi;
		};
	};
	uint32_t index;
	uint32_t title_info_offset;
	uint32_t title_info_offset_block_size;
	uint32_t reserved;
	uint8_t unknown2[0x0C];
} __attribute__((packed)) title_entry;

typedef struct {
	uint64_t size;
	uint32_t type;
	uint32_t version;
	uint32_t flags0;
	uint32_t tmd_content_id;
	uint32_t cmd_content_id;
	uint32_t flags1;
	uint32_t extdata_id_lo;
	uint32_t reserved1;
	uint64_t flags2;
	uint8_t product_code[0x10];
	uint8_t reserved2[0x10];
	uint32_t unknown;
	uint8_t reserved3[0x2c];
} __attribute__((packed)) title_info;

typedef struct {
	uint32_t unknown1;
	uint32_t active;
	union {
		uint64_t title_id;
		struct {
			uint32_t title_id_lo;
			uint32_t title_id_hi;
		};
	};
	uint32_t index;
	uint32_t unknown2; //0x080936AC
	uint32_t title_info_offset; //relative to title entry header, in BRDI units (0x200 in fact)
	uint32_t title_info_size; //in bytes
	uint8_t unknown3[0x0C]; //0
} __attribute__((packed)) ticket_title_entry;

typedef struct {
	uint32_t count; //not sure, looks like always 1
	uint32_t size;
} __attribute__((packed)) ticket_title_info;

#endif

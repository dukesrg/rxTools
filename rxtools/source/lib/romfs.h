#ifndef ROMFS_H
#define ROMFS_H

#include <stdint.h>

typedef struct {
	uint32_t magic;
	uint32_t magic_number;
	uint32_t master_hash_size;
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
	uint32_t reserved4;
	uint32_t optional_info_size;
} __attribute__((packed)) ivfc_header;

typedef struct {
	uint32_t header_length;
	uint32_t dir_hash_offset;
	uint32_t dir_hash_length;
	uint32_t dir_metadata_offset;
	uint32_t dir_metadata_length;
	uint32_t file_hash_offset;
	uint32_t file_hash_length;
	uint32_t file_metadata_offset;
	uint32_t file_metadata_length;
	uint32_t file_data_offset;
} __attribute__((packed)) romfs_header;

typedef struct {
	uint32_t parent_dir_offset;
	uint32_t next_sibling_dir_offset;
	uint32_t first_child_dir_offset;
	uint32_t first_file_offset;
	uint32_t dir_hash_pointer;
	uint32_t name_length;
	uint16_t name[0];
} __attribute__((packed)) dir_metadata;

typedef struct {
	uint32_t parent_dir_offset;
	uint32_t next_sibling_file_offset;
	union {
		uint64_t file_data_offset;
		struct {
			uint32_t file_data_offset_lo;
			uint32_t file_data_offset_hi;
		};
	};
	union {
		uint64_t file_data_length;
		struct {
			uint32_t file_data_length_lo;
			uint32_t file_data_length_hi;
		};
	};
	uint32_t file_hash_pointer;
	uint32_t name_length;
	uint16_t name[0];
} __attribute__((packed)) file_metadata;

#endif

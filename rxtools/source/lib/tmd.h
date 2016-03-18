#ifndef TMD_H
#define TMD_H

#include <stdbool.h>
#include <stdint.h>
#include <wchar.h>

#define TMD_MAX_CHUNKS 0x40

typedef enum {
	RSA_4096_SHA1	= 0x00000100,
	RSA_2048_SHA1	= 0x01000100,
	ECDSA_SHA1	= 0x02000100,
	RSA_4096_SHA256	= 0x03000100,
	RSA_2048_SHA256	= 0x04000100,
	ECDSA_SHA256	= 0x05000100
} tmd_sig_type; //BE order

typedef enum {
	Encrypted = 0x0100,
	Disc = 0x0200,
	CFM = 0x0400,
	Optional = 0x0040,
	Shared = 0x0080
} tmd_content_type; //BE order

typedef enum {
	Main_Content = 0x0000,
	Home_Menu_Manual = 0x0100,
	DLP_Child_Container = 0x0200
} tmd_contend_index; //BE order

typedef struct {
	uint8_t issuer[0x40];
	uint8_t version;
	uint8_t ca_crl_version;
	uint8_t signer_crl_version;
	uint8_t reserved_1;
	uint64_t system_version;
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
	uint8_t hash[0x20];
} tmd_header;

typedef struct {
	uint16_t content_index_offset;
	uint16_t content_command_count;
	uint8_t hash[0x20];
} tmd_content_info;

typedef struct {
	uint32_t content_id;
	uint16_t content_index;
	uint16_t content_type;
	uint64_t content_size;
	uint8_t hash[0x20];
} tmd_content_chunk;

typedef struct {
	uint32_t sig_type;
	uint8_t *sig;
	tmd_header header;
	tmd_content_info content_info[TMD_MAX_CHUNKS];
	tmd_content_chunk content_chunk[TMD_MAX_CHUNKS];
} tmd_data;

bool tmdLoad(wchar_t *apppath, tmd_data *data, uint32_t drive);

#endif

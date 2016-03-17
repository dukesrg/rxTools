#ifndef TMD_H
#define TMD_H

#include <stdint.h>

#define TMD_MAX_CHUNKS 0x40

typedef enum {
	RSA_4096_SHA1	= 0x00010000,
	RSA_2048_SHA1	= 0x00010001,
	ECDSA_SHA1	= 0x00010002,
	RSA_4096_SHA256	= 0x00010003,
	RSA_2048_SHA256	= 0x00010004,
	ECDSA_SHA256	= 0x00010005
} tmd_sig_type;

typedef enum {
	Encrypted = 0x0001,
	Disc = 0x0002,
	CFM = 0x0004,
	Optional = 0x4000,
	Shared = 0x8000
} tmd_content_type;

typedef enum {
	MainContent = 0x0000,
	Home_Menu_Manual = 0x0001,
	DLP_Child_Container = 0x0002
} tmd_contend_index;

typedef struct {
	uint8_t issuer[0x40];
	uint8_t version;
	uint8_t ca_crl_version;
	uint8_t signer_crl_version;
	uint8_t reserved_1;
	uint64_t system_version;
	uint64_t title_id;
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

void tmdLoad(tmd_data *data, uint32_t drive, uint32_t tidh, uint32_t tidl);

#endif

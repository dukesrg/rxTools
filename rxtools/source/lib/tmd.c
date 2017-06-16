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

#include <string.h>
#include "tmd.h"
#include "fs.h"
#include "TitleKeyDecrypt.h"

#include "aes.h"
#include "draw.h"
#include "sha.h"
#include "theme.h"
#include "signature.h"

#define BUF_SIZE 0x100000
#define APP_EXT ".app"
#define CONTENT_VERSION_UNSET 0xFFFF

const wchar_t *titleDir = L"%d:title/%08lx/%08lx/content";

uint32_t tmdLoadRecent(tmd_data *data, wchar_t *path) { //returns tmd file name contentid
	DIR dir;
	FILINFO fno;
	fno.lfname = NULL;
	wchar_t *filename;
	tmd_data data_tmp;
	uint32_t contentid = 0xFFFFFFFF;

	if (f_findfirst(&dir, &fno, path, L"*.tmd") == FR_OK) {
		filename = path + wcslen(wcscat(path, L"/"));
		do {
			wcscat(filename, fno.fname);
			if (tmdLoadHeader(&data_tmp, path) &&
				(contentid == 0xFFFFFFFF ||
				__builtin_bswap16(data_tmp.header.title_version) > __builtin_bswap16(data->header.title_version))
			) {
				*data = data_tmp;
				contentid = wcstoul(filename, NULL, 16);
			}
		} while (f_findnext(&dir, &fno) == FR_OK && *fno.fname);
	}
	f_closedir(&dir);
	return contentid;
}

uint32_t tmdPreloadRecent(tmd_data *data, wchar_t *path) { //returns tmd file name contentid
	DIR dir;
	FILINFO fno;
	fno.lfname = NULL;
	wchar_t *filename;
	tmd_data data_tmp;
	uint32_t contentid = 0xFFFFFFFF;

	if (f_findfirst(&dir, &fno, path, L"*.tmd") == FR_OK) {
		filename = path + wcslen(wcscat(path, L"/"));
		do {
			wcscat(filename, fno.fname);
			if (tmdPreloadHeader(&data_tmp, path) &&
				(contentid == 0xFFFFFFFF ||
				__builtin_bswap16(data_tmp.header.title_version) > __builtin_bswap16(data->header.title_version))
			) {
				*data = data_tmp;
				contentid = wcstoul(filename, NULL, 16);
			}
		} while (f_findnext(&dir, &fno) == FR_OK && *fno.fname);
	}
	f_closedir(&dir);
	return contentid;
}
/*
bool tmdLoad(wchar_t *apppath, tmd_data *data, uint32_t drive) {
	FIL fil;
	DIR dir;
	FILINFO fno;
	size_t br;
	wchar_t path[_MAX_LFN + 1], lfn[_MAX_LFN + 1], tmdpath[_MAX_LFN + 1];
	size_t header_offset;
	fno.lfname = lfn;
	fno.lfsize = _MAX_LFN + 1;
	tmd_data data_tmp;

	data->header.title_version = CONTENT_VERSION_UNSET;
	swprintf(path, _MAX_LFN + 1, titleDir, drive, __builtin_bswap32(data->header.title_id_hi), __builtin_bswap32(data->header.title_id_lo));
	if (f_findfirst(&dir, &fno, path, L"*.tmd") == FR_OK) {
		do {
			swprintf(tmdpath, _MAX_LFN + 1, L"%ls/%ls", path, *fno.lfname ? fno.lfname : fno.fname);
			if (f_open(&fil, tmdpath, FA_READ | FA_OPEN_EXISTING) == FR_OK) {
				if (f_read(&fil, &data_tmp.sig_type, sizeof(data_tmp.sig_type), &br) == FR_OK && br == sizeof(data_tmp.sig_type)) {
					switch (data_tmp.sig_type){
						case RSA_4096_SHA1:
						case RSA_4096_SHA256:
							header_offset = 0x240;
							break;
						case RSA_2048_SHA1:
						case RSA_2048_SHA256:
							header_offset = 0x140;
							break;
						case ECDSA_SHA1:
						case ECDSA_SHA256:
							header_offset = 0x80;
							break;
						default:
							header_offset = 0;
					}
					if (header_offset != 0 &&
						f_lseek(&fil, header_offset) == FR_OK &&
						f_read(&fil, &data_tmp.header, sizeof(data_tmp.header), &br) == FR_OK && br == sizeof(data_tmp.header) &&
						(__builtin_bswap16(data_tmp.header.title_version) > __builtin_bswap16(data->header.title_version) || data->header.title_version == CONTENT_VERSION_UNSET) &&
						__builtin_bswap16(data_tmp.header.content_count) < TMD_MAX_CHUNKS &&
						f_read(&fil, &data_tmp.content_info, sizeof(data_tmp.content_info), &br) == FR_OK && br == sizeof(data_tmp.content_info) &&
						f_read(&fil, &data_tmp.content_chunk, sizeof(tmd_content_chunk) * __builtin_bswap16(data_tmp.header.content_count), &br) == FR_OK && br == sizeof(tmd_content_chunk) * __builtin_bswap16(data_tmp.header.content_count)
					) {
						for (int i = __builtin_bswap16(data_tmp.header.content_count) - 1; i >= 0; i--) {
							if (data_tmp.content_chunk[i].content_index == CONTENT_INDEX_MAIN) {
								swprintf(apppath, _MAX_LFN + 1, L"%ls/%08lx.app", path, __builtin_bswap32(data->content_chunk[i].content_id));
								if (f_stat(apppath, NULL) == FR_OK)
									*data = data_tmp;
								break;
							}
						}
					}
				}
				f_close(&fil);
			}
		} while (f_findnext(&dir, &fno) == FR_OK && *fno.fname);
	}
	f_closedir(&dir);
	return data->header.title_version != CONTENT_VERSION_UNSET;
}

static bool checkFileHash(wchar_t *path, uint8_t *checkhash) {
	FIL fil;
	uint8_t *buf;
	uint8_t hash[32];
	size_t size;
	mbedtls_sha256_context ctx;
	
	if (!FileOpen(&fil, path, false) || ((FileGetSize(&fil)) == 0 && (FileClose(&fil) || true))) return false;
	buf = __builtin_alloca(BUF_SIZE);
	mbedtls_sha256_init(&ctx);
	mbedtls_sha256_starts(&ctx, 0);
	while ((size = FileRead2(&fil, buf, BUF_SIZE))) mbedtls_sha256_update(&ctx, buf, size);
	FileClose(&fil);
	mbedtls_sha256_finish(&ctx, hash);
	return !memcmp(hash, checkhash, sizeof(hash));
}
*/
uint_fast8_t tmdValidateChunk(tmd_data *data, wchar_t *path, uint_fast16_t content_index, uint_fast8_t drive) { //validates loaded tmd content chunk records
	FIL fil;
	size_t size;
	wchar_t apppath[_MAX_LFN + 1];
	uint8_t hash[SHA_256_SIZE];
	uint8_t *buf = __builtin_alloca(BUF_SIZE);
	uint_fast16_t content_count;

	content_count = __builtin_bswap16(data->header.content_count);
	if (!data->content_chunk &&
		(data->content_chunk = __builtin_alloca(content_count * sizeof(tmd_content_chunk))) &&
		!tmdPreloadChunk(data, path, content_index)
	) return 0;

	for (uint_fast16_t info_index = 0, chunk_index = 0; chunk_index < content_count; info_index++) {
		for (uint_fast16_t chunk_count = __builtin_bswap16(data->content_info[info_index].content_command_count); chunk_count > 0; chunk_index++, chunk_count--) {
			if (content_index == CONTENT_INDEX_ALL || content_index == data->content_chunk[chunk_index].content_index) {
				sha_start(SHA_256_MODE, NULL);
				swprintf(apppath, _MAX_LFN + 1, L"%.*ls/%08lx%s", wcsrchr(path, L'/') - path, path, __builtin_bswap32(data->content_chunk[chunk_index].content_id), APP_EXT);
				size = __builtin_bswap32(data->content_chunk[chunk_index].content_size_lo);
				if (FileOpen(&fil, apppath, 0) && (FileGetSize(&fil) == size || (FileClose(&fil) && 0))) {
					while ((size = FileRead2(&fil, buf, BUF_SIZE))) sha_update(buf, size);
				} else if (!(apppath[wcslen(apppath) - strlen(APP_EXT)] = 0) && 
					FileOpen(&fil, apppath, 0) && (FileGetSize(&fil) == size || (FileClose(&fil) && 0))
				) {
					aes_key Key = {&(aes_key_data){{0}}, AES_CNT_INPUT_BE_NORMAL, 0x2C, NORMALKEY};
					getTitleKey2(&Key, data->header.title_id, drive);
					aes_ctr ctr = {{{0}}, AES_CNT_INPUT_BE_NORMAL};
					aes_set_key(&Key);
					while ((size = FileRead2(&fil, buf, BUF_SIZE))) {
						aes(buf, buf, size, &ctr, AES_CBC_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);
						sha_update(buf, size);
					}                    
				} else
					return 0;
				FileClose(&fil);
				sha_finish(hash);
				if (memcmp(hash, data->content_chunk[chunk_index].content_hash, sizeof(hash)))
					return 0;
			}
		}
	}
	return 1;
}

size_t tmdGetChunkSize(tmd_data *data, wchar_t *path, uint_fast16_t content_index) { //calculates loaded tmd content chunk size of content_index type
	FIL fil;
	size_t offset, size;
	tmd_content_chunk *content_chunk_tmp;
	uint_fast16_t content_count;
	uint32_t content_size = 0;
	
	if (!FileOpen(&fil, path, 0) || (offset = signatureAdvance(data->sig_type)) == 0) return 0;
	offset += sizeof(data->header) + sizeof(data->content_info);
	content_count = __builtin_bswap16(data->header.content_count);
	size = content_count * sizeof(tmd_content_chunk);
	content_chunk_tmp = __builtin_alloca(size);
	if (FileSeek(&fil, offset) && FileRead2(&fil, content_chunk_tmp, size) == size) {
		for (uint_fast16_t info_index = 0, chunk_index = 0; chunk_index < content_count; info_index++) {
			for (uint_fast16_t chunk_count = __builtin_bswap16(data->content_info[info_index].content_command_count); chunk_count > 0; chunk_index++, chunk_count--) {
				if (content_index == CONTENT_INDEX_ALL || content_index == content_chunk_tmp[chunk_index].content_index)
					content_size += __builtin_bswap32(content_chunk_tmp[chunk_index].content_size_lo);
			}
		}
	}
	FileClose(&fil);
	return content_size;
}

uint_fast8_t tmdLoadHeader(tmd_data *data, wchar_t *path) { //validate and load tmd header
	File fil;
	size_t offset, size;
	tmd_data data_tmp;
	tmd_content_chunk *content_chunk_tmp;
	uint_fast16_t content_count;
	uint8_t hash[SHA_256_SIZE];
	
	if (!FileOpen(&fil, path, 0) || (
		(FileRead2(&fil, &data_tmp.sig_type, sizeof(data_tmp.sig_type)) != sizeof(data_tmp.sig_type) ||
		(offset = signatureAdvance(data_tmp.sig_type)) == 0 ||
		!FileSeek(&fil, offset) ||
		FileRead2(&fil, &data_tmp.header, sizeof(data_tmp.header) + sizeof(data_tmp.content_info)) != sizeof(data_tmp.header) + sizeof(data_tmp.content_info)) &&
		(FileClose(&fil) || 1)
	)) return 0;
	sha(hash, &data_tmp.content_info, sizeof(data_tmp.content_info), SHA_256_MODE);
	if (memcmp(hash, data_tmp.header.content_info_hash, sizeof(hash))) return FileClose(&fil) && 0;
	size = (content_count = __builtin_bswap16(data_tmp.header.content_count)) * sizeof(tmd_content_chunk);
	content_chunk_tmp = __builtin_alloca(size);
	if (FileRead2(&fil, content_chunk_tmp, size) != size) return FileClose(&fil) && 0;
	FileClose(&fil);
	for (uint_fast16_t info_index = 0, chunk_index = 0, chunk_count; chunk_index < content_count; info_index++, chunk_index += chunk_count) {
		if (info_index >= sizeof(data_tmp.content_info)/sizeof(tmd_content_info) ||
			(chunk_count = __builtin_bswap16(data_tmp.content_info[info_index].content_command_count)) == 0
		) return 0;
		sha(hash, &content_chunk_tmp[chunk_index], chunk_count * sizeof(tmd_content_chunk), SHA_256_MODE);
		if (memcmp(hash, data_tmp.content_info[chunk_index].content_chunk_hash, sizeof(hash))) return 0;
	}
	*data = data_tmp;
	return 1;
}

size_t tmdPreloadHeader(tmd_data *data, wchar_t *path) { //loads tmd header, validatas content info hash and returns content chunks size on success
	File fil;
	size_t offset;
	tmd_data data_tmp;
	uint8_t hash[32];
	
	if (!FileOpen(&fil, path, 0) || (
		(FileRead2(&fil, &data_tmp.sig_type, sizeof(data_tmp.sig_type)) != sizeof(data_tmp.sig_type) ||
		(offset = signatureAdvance(data_tmp.sig_type)) == 0 ||
		!FileSeek(&fil, offset) ||
		FileRead2(&fil, &data_tmp.header, sizeof(data_tmp.header) + sizeof(data_tmp.content_info)) != sizeof(data_tmp.header) + sizeof(data_tmp.content_info)) &&
		(FileClose(&fil) || 1)
	)) return 0;
	FileClose(&fil);
	sha(hash, &data_tmp.content_info, sizeof(data_tmp.content_info), SHA_256_MODE);
	if (memcmp(hash, data_tmp.header.content_info_hash, sizeof(hash))) return 0;
	*data = data_tmp;
	data->content_chunk = NULL;
	return __builtin_bswap16(data->header.content_count) * sizeof(tmd_content_chunk);
}

size_t tmdPreloadChunk(tmd_data *data, wchar_t *path, uint_fast16_t content_index) { //loads tmd chunk records and returns total chunks size of content index type on success
	FIL fil;
	size_t offset, size = 0;
	uint_fast16_t content_count, chunk_count;
	uint8_t hash[SHA_256_SIZE];
	
	if (FileOpen(&fil, path, 0) && (offset = signatureAdvance(data->sig_type))) {
		size = (content_count = __builtin_bswap16(data->header.content_count)) * sizeof(tmd_content_chunk);
		if (!data->content_chunk)
			data->content_chunk = __builtin_alloca(size);
		if (FileSeek(&fil, offset + sizeof(data->header) + sizeof(data->content_info)) && FileRead2(&fil, data->content_chunk, size) == size) {
			size = 0;
			for (uint_fast16_t info_index = 0, chunk_index = 0; chunk_index < content_count; info_index++) {
				chunk_count = __builtin_bswap16(data->content_info[info_index].content_command_count);
				sha(hash, &data->content_chunk[chunk_index], chunk_count * sizeof(tmd_content_chunk), SHA_256_MODE);
				if (memcmp(hash, data->content_info[chunk_index].content_chunk_hash, sizeof(hash))) {
					size = 0;
					break;
				}
				for (; chunk_count > 0; chunk_index++, chunk_count--) {
					if (content_index == CONTENT_INDEX_ALL || content_index == data->content_chunk[chunk_index].content_index)
						size += __builtin_bswap32(data->content_chunk[chunk_index].content_size_lo);
				}
			}
		} else size = 0;
		FileClose(&fil);
	}
	return size;
}

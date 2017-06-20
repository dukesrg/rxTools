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

#include <stdio.h>
#include <string.h>
#include "aes.h"
#include "fs.h"
#include "signature.h"
#include "ticket.h"
#include "native_firm.h"
#include "nand.h"
#include "memory.h"

#include "tmd.h"
#include "firm.h"
#include "mpcore.h"

#define PROCESS9_SEEK_PENDING	0
#define PROCESS9_SEEK_FAILED	0xFFFFFFFF

static aes_key_data common_keyy[6] = {PROCESS9_SEEK_PENDING};

uint_fast8_t findCommonKeyY(void *data, uint32_t size) {
	const uint32_t keyy_magic = 0x55D76DA1; //last key octets 
	const uint32_t keyy_magic_dev = 0x630DCF76;

	for (data += size -= sizeof(keyy_magic); size--; data--) //make it backwards
		if (!memcmp(data, &keyy_magic, sizeof(keyy_magic)) || !memcmp(data, &keyy_magic_dev, sizeof(keyy_magic_dev))) {
			data -= sizeof(common_keyy[0]) - sizeof(keyy_magic); //advance to start of the last key data
			for (size_t i = sizeof(common_keyy)/sizeof(common_keyy)[0]; i--;) {
//				common_keyy[i] = *(aes_key_data)data; //alignment failure here, workaround with memcpy
				memcpy(&common_keyy[i], data, sizeof(common_keyy[0]));
				data -= sizeof(common_keyy[0]) + sizeof(uint32_t); //size + pad
			}
			return 1;
		}

	return 0;
}

uint_fast8_t decryptKey(aes_key *key, ticket_data *ticket) {
	void *data = __builtin_alloca(NAND_SECTOR_SIZE);
	firm_header *firm = (firm_header*)data;
	firm_section_header *arm9_section;
	wchar_t path[_MAX_LFN + 1], apppath[_MAX_LFN + 1];
	tmd_data tmd;
	tmd_content_chunk content_chunk;
	File fil;
	size_t size;

	switch (common_keyy[0].as32[0]) {
		case PROCESS9_SEEK_FAILED: //previous search failed, don't waste time
			return 0;
		case PROCESS9_SEEK_PENDING: //first search, try to fill keys
			nand_readsectors(0, 1, data, SYSNAND, NAND_PARTITION_FIRM0);
			if ((arm9_section = firmFindSection(firm, firm->arm9_entry)) && (data = __builtin_alloca(firm->sections[i].size))) {
				nand_readsectors(arm9_section->offset/NAND_SECTOR_SIZE, arm9_section->size/NAND_SECTOR_SIZE, data, SYSNAND, NAND_PARTITION_FIRM0);
				findCommonKeyY(data, arm9_section->size);
			}

			for (size_t drive = 1; drive <= 2 && common_keyy[0].as32[0] == PROCESS9_SEEK_PENDING; drive++) //todo: max NAND drive
				if ((swprintf(path, _MAX_LFN + 1, L"%u:title/00040138/%1x0000002/content", drive, getMpInfo() == MPINFO_KTR ? 2 : 0) > 0) &&
					(tmdPreloadRecent(&tmd, path) != 0xFFFFFFFF) &&
					FileOpen(&fil, path, 0) && (
						(FileSeek(&fil, signatureAdvance(tmd.sig_type) + sizeof(tmd.header) + sizeof(tmd.content_info)) &&
							FileRead2(&fil, &content_chunk, sizeof(content_chunk)) == sizeof(content_chunk) //FIRM title have only  one file, so just first chunk is needed
						) || (FileClose(&fil) && 0) //close on fail
					) && (FileClose(&fil) || 1) && //close on success
					wcscpy(wcsrchr(path, L'/'), L"/%08lx.app") && //make APP path
					(swprintf(apppath, _MAX_LFN + 1, path, __builtin_bswap32(content_chunk.content_id)) > 0) &&
					FileOpen(&fil, pathapp, 0) && (
						((size = FileGetSize(&fil)) &&
							(data = __builtin_alloca(size)) &&
							FileRead2(&fil, data, size) == size
						) || (FileClose(&fil) && 0)
					) && (FileClose(&fil) || 1) &&
					(firm = (firm_header*)(data = decryptFirmTitleNcch((uint8_t*)data, &size))) &&
					(arm9_section = firmFindSection(firm, firm->arm9_entry)) &&
					findCommonKeyY(data + arm9_section->offset, arm9_section->size)
				) break;
			
			if (common_keyy[0].as32[0] == PROCESS9_SEEK_PENDING) {
				common_keyy[0].as32[0] = PROCESS9_SEEK_FAILED;
				return 0;
			}
	}

	aes_set_key(&(aes_key){&common_keyy[ticket->key_index], AES_CNT_INPUT_BE_NORMAL, 0x3D, KEYY});
	*key->data = ticket->key; //make it aligned
//	memcpy(key->data, &ticket->key, sizeof(*key->data)); //looks like no alignment issue with structure assignment above
	aes(key->data, key->data, sizeof(*key->data), &(aes_ctr){.data.as64={ticket->title_id}, AES_CNT_INPUT_BE_NORMAL}, AES_CBC_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);
	*key = (aes_key){key->data, AES_CNT_INPUT_BE_NORMAL, 0x2C, NORMALKEY}; //set aes_key metadata

	return 1;
}

uint_fast8_t ticketGetKey(aes_key *key, uint64_t titleid, uint_fast8_t drive) {
#define BUF_SIZE 0xD0000 //todo: make more precise ticket.db parser
	File fil;
	size_t tick_size = 0x200;

	wchar_t path[_MAX_LFN + 1];
	ticket_data *ticket;

	swprintf(path, sizeof(path), L"%u:dbs/ticket.db", drive);

	if (!FileOpen(&fil, path, 0))
		return 0;
	void *buf = __builtin_alloca(BUF_SIZE);
	while (FileRead2(&fil, buf, tick_size))
		if (*(uint32_t*)buf == 'KCIT')
			tick_size = BUF_SIZE;
		else
			for (size_t i = 0; i < tick_size; i++)
				if ((ticket = (ticket_data*)(buf + i))->title_id == titleid && (
					!strncmp(ticket->issuer, "Root-CA00000003-XS0000000c", sizeof(ticket->issuer)) ||
					!strncmp(ticket->issuer, "Root-CA00000004-XS00000009", sizeof(ticket->issuer))
				)) {
					FileClose(&fil);
					return decryptKey(key, ticket);
				}
	FileClose(&fil);
	return 0;
}

uint_fast8_t ticketGetKeyCetk(aes_key *key, uint64_t titleid, wchar_t *path) {
	File fil;
	cetk_data data;
	
	return FileOpen(&fil, path, 0) && (
			(FileRead2(&fil, &data.sig_type, sizeof(data.sig_type)) == sizeof(data.sig_type) &&
			 	FileSeek(&fil, signatureAdvance(data.sig_type)) &&
			 	FileRead2(&fil, &data.ticket, sizeof(data.ticket)) == sizeof(data.ticket)
			) || (FileClose(&fil) && 0)
		) && (FileClose(&fil) || 1) &&
		data.ticket.title_id == titleid &&
		decryptKey(key, &data.ticket);
}

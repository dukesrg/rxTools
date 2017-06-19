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

#include "draw.h"
#include "lang.h"

#define PROCESS9_SEEK_PENDING	0
#define PROCESS9_SEEK_FAILED	0xFFFFFFFF

uint_fast8_t decryptKey(aes_key *key, ticket_data *ticket) {
	const uint32_t keyy_magic = 0x7F337BD0; //first key octets
	const uint32_t keyy_magic_dev = 0x72F8A355;
	static aes_key_data common_keyy[6] = {PROCESS9_SEEK_PENDING};
	uint8_t buf[NAND_SECTOR_SIZE], *data;
	firm_header *firm = (firm_header*)&buf[0];

	switch (common_keyy[0].as32[0]) {
		case PROCESS9_SEEK_FAILED: //previous search failed, don't waste time
			return 0;
		case PROCESS9_SEEK_PENDING: //first search, try to fill keys
			nand_readsectors(0, 1, buf, SYSNAND, NAND_PARTITION_FIRM0);
			if (firm->magic == FIRM_MAGIC)
				for (size_t i = sizeof(firm->sections)/sizeof(firm->sections[0]); i--;)
					if (firm->sections[i].load_address >= MEM_ARM9_RAM && 
						firm->sections[i].load_address + firm->sections[i].size <= MEM_ARM9_RAM + MEM_ARM9_RAM_SIZE + MEM_ARM9_RAM_KTR_SIZE
					) {
						data = __builtin_alloca(firm->sections[i].size);
						nand_readsectors(firm->sections[i].offset/NAND_SECTOR_SIZE, firm->sections[i].size/NAND_SECTOR_SIZE, data, SYSNAND, NAND_PARTITION_FIRM0);
						for (size_t j = firm->sections[i].size; j--; data++)
							if (!memcmp(data, &keyy_magic, sizeof(keyy_magic)) ||
								!memcmp(data, &keyy_magic_dev, sizeof(keyy_magic_dev))
							) {
								for (size_t k = 0; k < sizeof(common_keyy)/sizeof(common_keyy)[0]; k++) {
									memcpy(&common_keyy[k], data, sizeof(common_keyy[0]));
									data += sizeof(common_keyy[0]) + sizeof(uint32_t); //size + pad
								}
								break;
							}
						break;
					}

			for (size_t drive = 1; drive <= 2 && common_keyy[0].as32[0] == PROCESS9_SEEK_PENDING; drive++) {
//DrawInfo(NULL, lang(S_CONTINUE), lang("Common key search in FIRM0 failed"));
				tmd_data tmd;
				wchar_t path[_MAX_LFN + 1];
				swprintf(path, _MAX_LFN + 1, L"%u:title/00040138/%1x0000002/content", drive, getMpInfo() == MPINFO_KTR ? 2 : 0);
				uint32_t contentid = tmdPreloadRecent(&tmd, path);
				if (contentid != 0xFFFFFFFF) {
					wcscat(path, L"/%08lx.app");
					wchar_t pathapp[_MAX_LFN + 1];
					swprintf(pathapp, _MAX_LFN + 1, path, contentid);
					File fil;
					size_t size;
/*					if (FileOpen(&fil, pathapp, 0) && (
						((size = FileGetSize(&fil)) &&
						(data = __builtin_alloca(size)) &&
						FileRead2(&fil, data, size) == size) ||
						(FileClose(&fil) && 0)
					)) {
*/
					if (!FileOpen(&fil, pathapp, 0)) {
						DrawInfo(NULL, lang(S_CONTINUE), lang("Can't open file %ls"), pathapp);
						break;	
					}
					if (!(size = FileGetSize(&fil))) {
						DrawInfo(NULL, lang(S_CONTINUE), lang("Can't get file size %ls"), pathapp);
						break;	
					}
					if (!(data = __builtin_alloca(size))) {
						DrawInfo(NULL, lang(S_CONTINUE), lang("Can't allocate %u bytes"), size);
						break;	
					}
					if (!(FileRead2(&fil, data, size) == size)) {
						DrawInfo(NULL, lang(S_CONTINUE), lang("Can't read %u bytes from %ls"), size, pathapp);
						break;	
					} else {
//					)) {
						FileClose(&fil);
						data = decryptFirmTitleNcch((uint8_t*)data, &size);
						if (data == NULL) {
DrawInfo(NULL, lang(S_CONTINUE), lang("Firm title decrypt failed"));
						} else {
							firm = (firm_header*)data;
						}

			if (firm->magic == FIRM_MAGIC) {
				for (size_t i = sizeof(firm->sections)/sizeof(firm->sections[0]); i--;)
					if (firm->sections[i].load_address >= MEM_ARM9_RAM && 
						firm->sections[i].load_address + firm->sections[i].size <= MEM_ARM9_RAM + MEM_ARM9_RAM_SIZE + MEM_ARM9_RAM_KTR_SIZE
					) {
						data += firm->sections[i].offset;
						for (size_t j = firm->sections[i].size; j--; data++)
							if (!memcmp(data, &keyy_magic, sizeof(keyy_magic)) ||
								!memcmp(data, &keyy_magic_dev, sizeof(keyy_magic_dev))
							) {
								for (size_t k = 0; k < sizeof(common_keyy)/sizeof(common_keyy)[0]; k++) {
									memcpy(&common_keyy[k], data, sizeof(common_keyy[0]));
									data += sizeof(common_keyy[0]) + sizeof(uint32_t); //size + pad
								}
								break;
							}
						break;
					}
DrawInfo(NULL, lang(S_CONTINUE), lang("Firm title key seek failed"));
			} else {
DrawInfo(NULL, lang(S_CONTINUE), lang("Firm title magic not found"));
			}

//					} else {
//DrawInfo(NULL, lang(S_CONTINUE), lang("Firm title read failed"));
					}
				} else {
DrawInfo(NULL, lang(S_CONTINUE), lang("Firm title tmd preload falied"));
				}
			}
			
			if (common_keyy[0].as32[0] == PROCESS9_SEEK_PENDING) {
DrawInfo(NULL, lang(S_CONTINUE), lang("Common key search in firm title failed"));
				common_keyy[0].as32[0] = PROCESS9_SEEK_FAILED;
				return 0;
			}
	}
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
DrawInfo(NULL, lang(S_CONTINUE), lang("Ticket key search failed"));
	return 0;
}

uint_fast8_t ticketGetKeyCetk(aes_key *key, uint64_t titleid, wchar_t *path) {
	File fil;
	size_t offset;
	cetk_data data;
	
	if (!FileOpen(&fil, path, 0) || (
		(FileRead2(&fil, &data.sig_type, sizeof(data.sig_type)) != sizeof(data.sig_type) ||
		(offset = signatureAdvance(data.sig_type)) == 0 ||
		!FileSeek(&fil, offset) ||
		FileRead2(&fil, &data.ticket, sizeof(data.ticket)) != sizeof(data.ticket)) &&
		(FileClose(&fil) || 1) &&
		data.ticket.title_id != titleid
	))
{
DrawInfo(NULL, lang(S_CONTINUE), lang("CETK key search failed"));
		return 0;
}
	
	return decryptKey(key, &data.ticket);
}

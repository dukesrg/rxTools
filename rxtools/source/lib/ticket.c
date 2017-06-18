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

#define PROCESS9_SEEK_PENDING	0
#define PROCESS9_SEEK_FAILED	0xFFFFFFFF

uint_fast8_t decryptKey(aes_key *key, ticket_data *ticket) {
	const uint32_t keyy_magic = 0x55D76DA1; //last key octets 
	const uint32_t keyy_magic_dev = 0x630DCF76;

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
						uint32_t j = firm->sections[i].size - sizeof(keyy_magic);
						for (data += j; j--; data--) //make it backwards
							if (!memcmp(data, &keyy_magic, sizeof(keyy_magic)) ||
								!memcmp(data, &keyy_magic_dev, sizeof(keyy_magic_dev))
							) {
								data -= sizeof(common_keyy[0]) - sizeof(keyy_magic); //advance to start of the key data
								for (size_t k = sizeof(common_keyy)/sizeof(common_keyy)[0]; k--;) {
//									common_keyy[k] = *(aes_key_data)data; //alignment failure here, workaround with memcpy
									memcpy(&common_keyy[k], data, sizeof(common_keyy[0]));
									data -= sizeof(common_keyy[0]) + sizeof(uint32_t); //size + pad
								}
								break;
							}
						break;
					}
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
				if ((ticket = (ticket_data*)(buf + i))->title_id == titleid &&
					!strncmp(ticket->issuer, "Root-CA00000003-XS0000000c", sizeof(ticket->issuer))
				) {
					FileClose(&fil);
					return decryptKey(key, ticket);
				}
	FileClose(&fil);
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
	)) return 0;
	
	return decryptKey(key, &data.ticket);
}

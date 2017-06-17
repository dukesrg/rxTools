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

#include "draw.h"
#include "lang.h"

#define PROCESS9_SEEK_START	0x08080000
#define PROCESS9_SEEK_END	0x080A0000
#define PROCESS9_SEEK_PENDING	0
#define PROCESS9_SEEK_FAILED	0xFFFFFFFF

uint_fast8_t decryptKey(aes_key *key, ticket_data *ticket) {

/*
	static struct {
		aes_key_data key;
		uint32_t pad;
	} *keyYList = NULL;

	uintptr_t p;
	if (keyYList == NULL) {
		p = 0x08080000;
		while (((uint8_t *)p)[0] != 0xD0 || ((uint8_t *)p)[1] != 0x7B) {
			p++;
			if (p >= 0x080A0000)
{
ClearScreen(&top1Screen, RED);
DisplayScreen(&top1Screen);
				return 0;
}
		}

		keyYList = (void *)p;
	}
*/
	const uint32_t keyy_magic = 0x7F337BD0;
	const uint32_t keyy_magic_dev = 0x72F8A355;
	static aes_key_data common_keyy[6] = {PROCESS9_SEEK_PENDING};
	uintptr_t p9;

	switch (common_keyy[0].as32[0]) {
		case PROCESS9_SEEK_FAILED: //previous search failed, don't waste time
			return 0;
		case PROCESS9_SEEK_PENDING: //0 - first search, try to get keys
			for (p9 = 0x08080000; memcmp((void*)p9, &keyy_magic, sizeof(keyy_magic)) && memcmp((void*)p9, &keyy_magic_dev, sizeof(keyy_magic_dev)); p9++)
				if (p9 > 0x080A0000) {
ClearScreen(&top1Screen, RED);
DisplayScreen(&top1Screen);
					common_keyy[0].as32[0] = PROCESS9_SEEK_FAILED;
					return 0;
				}
			for (size_t i = 0; i < sizeof(common_keyy)/sizeof(common_keyy)[0]; i++) {
				memcpy(&common_keyy[i], (void*)p9, sizeof(common_keyy[0])); //ARM9 RAM -> FCRAM unaligned copy
				p9 += sizeof(common_keyy[0]) + sizeof(uint32_t);
			}	
	}

//	aes_key_data keyY;
//	memcpy(&keyY, &keyYList[ticket->key_index].key, sizeof(keyY)); //ARM9 RAM -> FCRAM unaligned structure assignment won't work
//	aes_set_key(&(aes_key){&keyY, AES_CNT_INPUT_BE_NORMAL, 0x3D, KEYY});
	aes_set_key(&(aes_key){&common_keyy[ticket->key_index], AES_CNT_INPUT_BE_NORMAL, 0x3D, KEYY});

	*key->data = ticket->key; //make it aligned
	aes(key->data, key->data, sizeof(*key->data), &(aes_ctr){.data.as64={ticket->title_id}, AES_CNT_INPUT_BE_NORMAL}, AES_CBC_DECRYPT_MODE | AES_CNT_INPUT_BE_NORMAL | AES_CNT_OUTPUT_BE_NORMAL);
/*
DrawInfo(NULL, lang(S_CONTINUE), lang("%08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x"),
	ticket->key.as32[0], ticket->key.as32[1], ticket->key.as32[2], ticket->key.as32[3],
//	ctr.data.as32[0], ctr.data.as32[1], ctr.data.as32[2], ctr.data.as32[3],
	keyY.as32[0], keyY.as32[1], keyY.as32[2], keyY.as32[3],
	key->data->as32[0], key->data->as32[1], key->data->as32[2], key->data->as32[3]
);
*/
	*key = (aes_key){key->data, AES_CNT_INPUT_BE_NORMAL, 0x2C, NORMALKEY}; //set aes_key metadata

	return 1;
}

uint_fast8_t ticketGetKey(aes_key *key, uint64_t titleid, uint_fast8_t drive) {
#define BUF_SIZE 0xD0000
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
ClearScreen(&top1Screen, BLUE);
DisplayScreen(&top1Screen);

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

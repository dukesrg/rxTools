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

#include <string.h>
#include "cetk.h"
#include "aes.h"
#include "signature.h"
#include "TitleKeyDecrypt.h"

uint_fast8_t ticketGetKeyCetk(aes_key *Key, wchar_t *path, uint64_t titleid) {
	File fil;
	size_t offset;
	cetk_data data;
	
	if (!FileOpen(&fil, path, 0) || (
		(FileRead2(&fil, &data.sig_type, sizeof(data.sig_type)) != sizeof(data.sig_type) ||
		(offset = signatureAdvance(data.sig_type)) == 0 ||
		!FileSeek(&fil, offset) ||
		FileRead2(&fil, &data.header, sizeof(data.header)) != sizeof(data.header)) &&
		(FileClose(&fil) || 1) &&
		data.title_id != titleid
	)) return 0;
	
	*Key->data = data.title_key;
	return DecryptTitleKey2(data.title_id, Key->data, data.key_index);
}

/*
 * Copyright (C) 2015 The PASTA Team
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

#include "ncch.h"

typedef union {
	struct {
		uint32_t offset;
		uint32_t type;
		uint64_t partitionid;
	};
	aes_ctr_data data;
} __attribute__((packed)) ncchcounter;

void ncch_get_counter(ctr_ncchheader *header, aes_ctr *counter, ctr_ncchtype type) {
	counter->mode = AES_CNT_INPUT_LE_REVERSE;
	switch (header->version) {
		case 0:
		case 2:
			counter->data = ((ncchcounter){{0, type, header->partitionid}}).data;
			break;
		case 1:
			switch (type) {
				case NCCHTYPE_EXHEADER:
					((ncchcounter*)&counter->data)->offset = sizeof(header);
					break;
				case NCCHTYPE_EXEFS:
					((ncchcounter*)&counter->data)->offset = header->exefsoffset * NCCH_MEDIA_UNIT_SIZE;
					break;
				case NCCHTYPE_ROMFS:
					((ncchcounter*)&counter->data)->offset = header->romfsoffset * NCCH_MEDIA_UNIT_SIZE;
					break;
			}
			counter->data = ((ncchcounter){{((ncchcounter*)counter)->offset, 0, __builtin_bswap64(header->partitionid)}}).data;
		break;
	}
}

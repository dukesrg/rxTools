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
		uint64_t partitionid;
		uint32_t type;
		uint32_t offset;
	};
	aes_ctr_data data;
} __attribute__((packed)) ncchcounter;

/*
void ncch_get_counter(ctr_ncchheader *header, aes_ctr *counter, ctr_ncchtype type) {
	switch (header->version) {
		case 0:
		case 2:
			*counter = ((ncchcounter){{__builtin_bswap64(header->partitionid), type, 0}}).ctr;
			break;
		case 1:
			switch (type) {
				case NCCHTYPE_EXHEADER:
					((ncchcounter*)counter)->offset = sizeof(header);
					break;
				case NCCHTYPE_EXEFS:
					((ncchcounter*)counter)->offset = header->exefsoffset * NCCH_MEDIA_UNIT_SIZE;
					break;
				case NCCHTYPE_ROMFS:
					((ncchcounter*)counter)->offset = header->romfsoffset * NCCH_MEDIA_UNIT_SIZE;
					break;
			}
			*counter = ((ncchcounter){{header->partitionid, 0, __builtin_bswap32(((ncchcounter*)counter)->offset)}}).ctr;
		break;
	}
}
*/
void ncch_get_counter(ctr_ncchheader *header, aes_ctr *counter, ctr_ncchtype type) {
	counter->mode = AES_CNT_INPUT_BE_NORMAL; //todo: switch to reverse LE
	switch (header->version) {
		case 0:
		case 2:
			counter->data = ((ncchcounter){{__builtin_bswap64(header->partitionid), type, 0}}).data;
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
			counter->data = ((ncchcounter){{header->partitionid, 0, __builtin_bswap32(((ncchcounter*)counter)->offset)}}).data;
		break;
	}
}
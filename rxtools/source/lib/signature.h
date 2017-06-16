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

#ifndef SIGNATURE_H
#define SIGNATURE_H

typedef enum {
	RSA_4096_SHA1	= 0x00000100,
	RSA_2048_SHA1	= 0x01000100,
	ECDSA_SHA1	= 0x02000100,
	RSA_4096_SHA256	= 0x03000100,
	RSA_2048_SHA256	= 0x04000100,
	ECDSA_SHA256	= 0x05000100
} signature_type; //BE order

size_t signatureAdvance(uint32_t sig_type) {
	switch (sig_type) {
		case RSA_4096_SHA1: case RSA_4096_SHA256: return 0x240;
		case RSA_2048_SHA1: case RSA_2048_SHA256: return 0x140;
		case ECDSA_SHA1: case ECDSA_SHA256: return 0x80;
		default: return 0;
	}
}

#endif

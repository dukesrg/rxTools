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

#ifndef CTR_DECRYPTOR_H
#define CTR_DECRYPTOR_H

#include <stddef.h>
#include <stdint.h>
#include "aes.h"
#include "fs.h"

uint_fast8_t decryptFile(File *outfile, File *infile, size_t size, size_t offset, aes_ctr *ctr, aes_key *key, uint32_t mode);
void CTRDecryptor();

#endif
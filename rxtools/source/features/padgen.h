/*
 * Copyright (C) 2015-2016 The PASTA Team, dukesrg
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

#ifndef PADGEN_H
#define PADGEN_H

#include "aes.h"

#define MAX_PAD_ENTRIES 1024

typedef struct {
    aes_ctr_data CTR;
    uint32_t  size_mb;
    char filename[180];
} __attribute__((packed)) SdInfoEntry;

typedef struct {
    uint32_t n_entries;
    SdInfoEntry entries[MAX_PAD_ENTRIES];
} __attribute__((packed, aligned(16))) SdInfo;

typedef struct {
    aes_ctr_data CTR;
    aes_key_data keyY;
    uint32_t  size_mb;
    uint8_t   reserved[8];
    uint32_t  uses7xCrypto;
    char filename[112];
} __attribute__((packed)) NcchInfoEntry;

typedef struct {
    uint32_t padding;
    uint32_t ncch_info_version;
    uint32_t n_entries;
    uint8_t  reserved[4];
    NcchInfoEntry entries[MAX_PAD_ENTRIES];
} __attribute__((packed, aligned(16))) NcchInfo;

uint_fast8_t CreatePad(aes_ctr *ctr, aes_key *key, uint32_t size, wchar_t *filename, int index);
uint_fast8_t PadGen(wchar_t *filename);

#endif

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

#ifndef NAND_DUMPER_H
#define NAND_DUMPER_H

#include "nand.h"

//#include "menu.h"

enum nand_type{
	UNK_NAND = -1,
	SYS_NAND = 1,
	EMU_NAND = 2
};

int NandSwitch();
void DumpNANDSystemTitles();

uint_fast8_t GenerateNandXorpad(nand_partition_index partition, wchar_t *path);
uint_fast8_t DumpNand(nand_type type, wchar_t *path);
uint_fast8_t InjectNand(nand_type type, wchar_t *path);
uint_fast8_t CopyNand(nand_type src, nand_type dst);
uint_fast8_t DumpPartition(nand_type type, nand_partition_index partition, wchar_t *path);
uint_fast8_t InjectPartition(nand_type type, nand_partition_index partition, wchar_t *path);
uint_fast8_t CopyPartition(nand_type src, nand_partition_index partition, nand_type dst);
uint_fast8_t CopyFile(wchar_t *srcpath, wchar_t *dstpath);

#endif

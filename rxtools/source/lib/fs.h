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

#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>
#include "fatfs/ff.h"
#include "nand.h"
#define File FIL
////////////////////////////////////////////////////////////////Basic FileSystem Operations
uint_fast8_t FSInit(void);
void FSDeInit(void);
uint_fast8_t FileOpen(File *Handle, const wchar_t *path, uint_fast8_t truncate);
size_t FileRead(File *Handle, void *buf, size_t size, size_t foffset);
size_t FileWrite(File *Handle, void *buf, size_t size, size_t foffset);
size_t FileGetSize(File *Handle);
uint_fast8_t FileClose(File *Handle);
uint_fast8_t FileExists(const wchar_t *path);
uint_fast8_t FileSeek(File *Handle, size_t foffset);
size_t FileRead2(File *Handle, void *buf, size_t size);
size_t FileSize(const wchar_t *path);
size_t FileMaxSize(const wchar_t *path, const wchar_t *pattern);
////////////////////////////////////////////////////////////////Advanced FileSystem Operations
uint32_t FSFileCopy(wchar_t *target, wchar_t *source);

#endif

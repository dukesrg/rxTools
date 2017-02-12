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

#include "fs.h"
#include "draw.h"

////////////////////////////////////////////////////////////////Basic FileSystem Operations
static FATFS fs[3];
static FRESULT lasterror;
/**Init FileSystems.*/
uint_fast8_t FSInit(void) {
	if (f_mount(&fs[0], L"0:", 0) != FR_OK //|| //SDCard
//		f_mount(&fs[1], L"1:", 1) != FR_OK //NAND
	) {
		ClearScreen(&bottomScreen, FUCHSIA);
		ClearScreen(&top1Screen, SALMON);
//		ClearScreen(&top2Screen, SALMON);
		DisplayScreen(&bottomScreen);
		DisplayScreen(&top1Screen);
//		DisplayScreen(&top2Screen);
		return 0;
	}
	f_mount(&fs[1], L"1:", 1);

	f_mount(&fs[2], L"2:", 0); //EmuNAND, Sometimes it doesn't exist

	return 1;
}
/**[Unused?]DeInit FileSystems.*/
void FSDeInit(void) {
	f_mount(NULL, L"0:", 0);
	f_mount(NULL, L"1:", 0);
	f_mount(NULL, L"2:", 0);
}

uint_fast8_t FileOpen(File *Handle, const wchar_t *path, uint_fast8_t truncate) {
	return (lasterror = f_open(Handle, path, FA_READ | FA_WRITE | (truncate ? FA_CREATE_ALWAYS : FA_OPEN_EXISTING))) == FR_OK && FileSeek(Handle, 0) && (f_sync(Handle) || 1);

}

uint_fast8_t FileSeek(File *Handle, size_t foffset) {
	return (lasterror = FR_OK) || f_tell(Handle) == foffset || (lasterror = f_lseek(Handle, foffset)) == FR_OK;
}

size_t FileRead(File *Handle, void *buf, size_t size, size_t foffset) {
	UINT bytes_read = 0;
	if (FileSeek(Handle, foffset))
		f_read(Handle, buf, size, &bytes_read);
	return bytes_read;
}

size_t FileRead2(File *Handle, void *buf, size_t size) {
	UINT bytes_read = 0;
	lasterror = f_read(Handle, buf, size, &bytes_read);
	return bytes_read;
}

size_t FileWrite(File *Handle, void *buf, size_t size, size_t foffset) {
	UINT bytes_written = 0;
	if (FileSeek(Handle, foffset) && f_write(Handle, buf, size, &bytes_written) == FR_OK)
		f_sync(Handle);
	return bytes_written;
}

size_t FileWrite2(File *Handle, void *buf, size_t size) {
	UINT bytes_written = 0;
	f_write(Handle, buf, size, &bytes_written);
	f_sync(Handle);
	return bytes_written;
}

size_t FileGetSize(File *Handle) {
	return f_size(Handle);
}

uint_fast8_t FileClose(File *Handle) {
	return f_close(Handle) == FR_OK;
}

uint_fast8_t FileExists(const wchar_t *path) {
	return f_stat(path, NULL) == FR_OK;
}

size_t FileSize(const wchar_t *path) {
	FILINFO fno = {0};
	f_stat(path, &fno);
	return fno.fsize;
}
////////////////////////////////////////////////////////////////Advanced FileSystem Operations
/** Copy Source File (source) to Target (target).
  * @param  target Target file, will be created if not exists.
  * @param  source Source file, must exists.
  * @retval Compounded value of (STEP<<8)|FR_xx, so it contains real reasons.
  * @note   directly FATFS calls. FATFS return value only ranges from 0 to 19.
  */
uint32_t FSFileCopy(wchar_t *target, wchar_t *source) {
	FIL src, dst;
	uint32_t step = 0; //Tells you where it failed
	FRESULT retfatfs = 0; //FATFS return value
	uint32_t blockSize = 0x4000;
	uint8_t *buffer = (uint8_t *)0x26000200; //Temp buffer for transfer.
	UINT byteI = 0, byteO = 0; //Bytes that read or written
	retfatfs = f_open(&src, source, FA_READ | FA_OPEN_EXISTING);
	if (retfatfs != FR_OK) {step = 1; goto closeExit;}
	retfatfs = f_open(&dst, target, FA_WRITE | FA_CREATE_ALWAYS);
	if (retfatfs != FR_OK) {step = 2; goto closeExit;}
	uint32_t totalSize = src.fsize;
	//If source file has no contents, nothing to be copied.
	if (!totalSize) { goto closeExit; }
	while (totalSize) {
		if (totalSize < blockSize) { blockSize = totalSize; }
		//FR_OK, FR_DISK_ERR, FR_INT_ERR, FR_INVALID_OBJECT, FR_TIMEOUT
		retfatfs = f_read(&src, buffer, blockSize, &byteI);
		if (retfatfs != FR_OK) {step = 3; goto closeExit;}
		//Unexpected
		if (byteI != blockSize) {step = 4; goto closeExit;}
		//FR_OK, FR_DISK_ERR, FR_INT_ERR, FR_INVALID_OBJECT, FR_TIMEOUT
		retfatfs = f_write(&dst, buffer, blockSize, &byteO);
		if (retfatfs != FR_OK) {step = 5; goto closeExit;}
		//Unexpected
		if (byteO != blockSize) {step = 6; goto closeExit;}
		totalSize -= blockSize;
	}
	step = 0; //Success return value.
closeExit:
	f_close(&src);
	f_close(&dst);
	return (step << 8) | retfatfs;
}

size_t FileMaxSize(const wchar_t *path, const wchar_t *pattern) {
	size_t maxSize = 0;
	DIR dir;
	wchar_t lfn[_MAX_LFN + 1];
	FILINFO fno;
	fno.lfname = lfn;
	fno.lfsize = _MAX_LFN + 1;

	if (f_findfirst(&dir, &fno, path, pattern) == FR_OK) {
		do {
			if (fno.fsize > maxSize)
				maxSize = fno.fsize;
		} while (f_findnext(&dir, &fno) == FR_OK && fno.fname[0]);
	}
	f_closedir(&dir);

	return maxSize;
}

uintmax_t FSFreeSpace(const wchar_t *path) {
	FATFS *fs;
	DWORD fre_clust;

	if (f_getfree(path, &fre_clust, &fs) != FR_OK)
		return 0;
	return (uintmax_t)fre_clust * fs->csize * _MIN_SS;
}

FRESULT FSGetLastError() {
	return lasterror;
}

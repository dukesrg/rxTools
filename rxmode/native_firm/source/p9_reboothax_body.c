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

#include <stddef.h>
#include <stdint.h>
#include <reboot.h>
#include <elf.h>
#include <emunand.h>
#ifndef PLATFORM_KTR
#include <keyx.h>
#endif

static void drainWriteBuffer()
{
	__asm__ volatile ("mcr p15, 0, %0, c7, c10, 4\n" :: "r"(0));
}

static void cleanDcacheLine(void *p)
{
	__asm__ volatile ("mcr p15, 0, %0, c7, c10, 1\n" :: "r"(p));
}

static void flushIcacheLine(void *p)
{
	__asm__ volatile ("mcr p15, 0, %0, c7, c5, 1\n" :: "r"(p));
}

static void *memcpy32(void *dst, const void *src, size_t n)
{
	const uint32_t *_src;
	uint32_t *_dst;
	uintptr_t btm;

	_dst = dst;
	_src = src;
	btm = (uintptr_t)dst + n;
	while ((uintptr_t)_dst < btm) {
		*_dst = *_src;
		_dst++;
		_src++;
	}

	return _dst;
}

static void loadFirm()
{
	const FirmSeg *seg;
	unsigned int i;

	seg = ((FirmHdr *)FIRM_ADDR)->segs;
	for (i = 0; i < FIRM_SEG_NUM; i++) {
		memcpy32((void *)seg->addr, (void *)FIRM_ADDR + seg->offset, seg->size);
		seg++;
	}
}

static void flushFirmData()
{
	uintptr_t dstCur, dstBtm;
	const FirmSeg *seg;
	unsigned int i;

	seg = ((FirmHdr *)FIRM_ADDR)->segs;
	for (i = 0; i < FIRM_SEG_NUM; i++) {
		dstCur = seg->addr;
		for (dstBtm = seg->addr + seg->size; dstCur < dstBtm; dstCur += 32)
			cleanDcacheLine((void *)dstCur);

		seg++;
	}

	drainWriteBuffer();
}

static void flushFirmInstr()
{
	uintptr_t dstCur, dstBtm;
	const FirmSeg *seg;
	unsigned int i;

	seg = ((FirmHdr *)FIRM_ADDR)->segs;
	for (i = 0; i < FIRM_SEG_NUM; i++) {
		if (!seg->isArm11) {
			dstCur = seg->addr;
			for (dstBtm = seg->addr + seg->size; dstCur < dstBtm; dstCur += 32)
				flushIcacheLine((void *)dstCur);
		}

		seg++;
	}
}

static void arm11Enter(uint32_t *arm11EntryDst)
{
	*arm11EntryDst = ((FirmHdr *)FIRM_ADDR)->arm11Entry;
	cleanDcacheLine(arm11EntryDst);
	drainWriteBuffer();
}

static _Noreturn void arm9Enter()
{
	__asm__ volatile ("ldr pc, %0\n" :: "m"(((FirmHdr *)FIRM_ADDR)->arm9Entry));
	__builtin_unreachable();
}

_Noreturn void __attribute__((section(".text.start")))
rebootFunc(uint32_t sector, const void *pkeyx, uint32_t *arm11EntryDst)
{
	if ((uint32_t)arm11EntryDst == 0x1FFFFFFC)
		loadFirm();

	if (sector > 0)
		nandSector = sector;

#ifndef PLATFORM_KTR
	if (pkeyx != NULL)
		memcpy32(keyx, pkeyx, sizeof(keyx));
#endif

	flushFirmData();
	arm11Enter(arm11EntryDst);
	flushFirmInstr();
	arm9Enter();
}

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

#include <stdint.h>
#include <wchar.h>
#include <reboot.h>
#include <svc.h>
#include <emunand.h>
#include <process9.h>


static _Noreturn void __attribute__((section(".patch.p9.reboot.entry")))
execReboot()
{
	__asm__ volatile (
		"mov r0, #0x1FFFFFFC\n"
#ifdef PLATFORM_KTR
		"mov sp, #0x08100000\n"
		"orr sp, #0x0007F000\n"
#else
		"mov sp, #0x08000000\n"
		"orr sp, #0x000FF000\n"
#endif
		"b rebootFunc\n");
	__builtin_unreachable();
}

_Noreturn void __attribute__((section(".patch.p9.reboot.entry.top")))
loadExecReboot(int r0, int r1, int r2, uint32_t hiId, uint32_t loId)
{
	const size_t pathLen = 64;
	wchar_t path[pathLen];
	size_t read;
	P9File f;

	p9FileInit(f);
	swprintf(path, pathLen, L"sdmc:/" FIRM_PATH_FMT, hiId, loId);
	p9Open(f, path, 1);
	p9Read(f, &read, (void *)FIRM_ADDR, FIRM_SIZE);
	p9Close(f);

	while (p9RecvPxi() != 0x44846);
	svcKernelSetState(SVC_KERNEL_STATE_INIT, hiId, loId,
		SVC_KERNEL_STATE_TITLE_COMPAT);

	svcBackdoor((void *)execReboot);
	__builtin_unreachable();
}

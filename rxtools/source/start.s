@ Copyright (C) 2015-2017 The PASTA Team, dukesrg
@
@ This program is free software; you can redistribute it and/or
@ modify it under the terms of the GNU General Public License
@ version 2 as published by the Free Software Foundation
@
@ This program is distributed in the hope that it will be useful,
@ but WITHOUT ANY WARRANTY; without even the implied warranty of
@ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
@ GNU General Public License for more details.
@
@ You should have received a copy of the GNU General Public License
@ along with this program; if not, write to the Free Software
@ Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <arm.h>
#include <memory.h>

.section .text.start
.arm
.global _start

_start:
	@ Vector Table for CakeHax bigpayload (GW)
	.rept	11
	nop
	.endr
	b	_start_al9h @skip bigpayload additional workaround

	@ bigpayload entry
_bigpayload:
	adr	r0, _start
	ldr	r1, =payload_start
	cmp	r0, r1
	beq	_start_al9h @ already launched from payload_start

	mov	r2, #0x10000
loop:
        subs	r2, #1
        bge	loop

	mov	r11, r0
	mov	r12, r1
	ldr	r2, =payload_length
copy:
	ldmia	r0!, {r3 - r10}
	stmia	r1!, {r3 - r10}
	subs	r2, r2, #32
	bne	copy

	adr	r3, _start_al9h
	sub	r3, r11
	add	r3, r12
	mcr	p15, 0, r2, c7, c5, 0 @ flush I-cache
	bx	r3 @ jump to payload_start + _start_al9h - _start

.align 4
_start_al9h:
	@ Disable IRQ and FIQ and enter supervisor mode
	msr	CPSR_c, #(PSR_FIQ_DISABLE | PSR_IRQ_DISABLE | PSR_MODE_SUPER)

	@ Load predefined values to registers for immediate use
	adr	r0, settings
	ldm	r0, {r0-r14}

	@ Give read/write access to all the memory regions
	mcr	p15, 0, r8, c5, c0, 2	@ write data access
	mcr	p15, 0, r8, c5, c0, 3	@ write instruction access

	@ Sets MPU permissions
	mcr	p15, 0, r0, c6, c0, 0
	mcr	p15, 0, r1, c6, c1, 0
	mcr	p15, 0, r2, c6, c2, 0
	mcr	p15, 0, r3, c6, c3, 0
	mcr	p15, 0, r4, c6, c4, 0
	mcr	p15, 0, r5, c6, c5, 0
	mcr	p15, 0, r6, c6, c6, 0
	mcr	p15, 0, r7, c6, c7, 0

	mcr	p15, 0, r9, c3, c0, 0	@ Write bufferable 0, 2, 5
	mcr	p15, 0, r9, c2, c0, 0	@ Data cacheable 0, 2, 5
	mcr	p15, 0, r9, c2, c0, 1	@ Inst cacheable 0, 2, 5

	@ Enable caches
	mrc	p15, 0, r4, c1, c0, 0
	orr	r4, r4, r10
	mcr	p15, 0, r4, c1, c0, 0

	@ Flush caches
	mcr	p15, 0, r14, c7, c5, 0	@ flush I-cache
	mcr	p15, 0, r14, c7, c6, 0	@ flush D-cache
	mcr	p15, 0, r14, c7, c10, 4	@ drain write buffer

	@ Fixes mounting of SDMC
	str	r12, [r11]

	bl	toolsmain

.die:
	b	.die

.align 4
settings:
	.word	(MEM_BOOTROM | PR_SZ_32K | PR_EN)	@ r0: memory region 0 base, size and protection
	.word	(MEM_ARM9_ITCM_KERNEL | PR_SZ_32K | PR_EN)	@ r1: memory region 1 base, size and protection
	.word	(MEM_ARM9_RAM | PR_SZ_2M | PR_EN)	@ r2: memory region 2 base, size and protection
	.word	(MEM_IO | PR_SZ_128K | PR_EN)		@ r3: memory region 3 base, size and protection
	.word	(MEM_IO + 0x00100000 | PR_SZ_512K | PR_EN)		@ r4: memory region 4 base, size and protection
	.word	(MEM_FCRAM | PR_SZ_128M | PR_EN)	@ r5: memory region 5 base, size and protection
	.word	(MEM_DSP | PR_SZ_1M | PR_EN)		@ r6: memory region 6 base, size and protection
	.word	(MEM_VRAM | PR_SZ_8M | PR_EN)		@ r7: memory region 7 base, size and protection
	.word	AP_ALL_ACCESS	@ r8: memory regions 0-7 access rights
	.word	(CB_0 | CB_2 | CB_5)	@ r9: memory regions 0-7 cache and write buffer control
	.word	(CR_M | CR_C | CR_I)	@ r10: instruction cache, data cache, MPU enable
	.word	0x10000020	@ r11: SDMC mount fix CFG9(?) register
	.word	0x00000340	@ r12: SDMC mount fix value
	.word	0x27000000	@ r13: SP init value
	.word	0x00000000	@ r14: just zero

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

.section .text.start
.align 4
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
@	ldr	r5, =0x33333333
@	mcr	p15, 0, r5, c5, c0, 2 @ write data access
@do we need this in al9h?
	msr	CPSR_c, #0xD3 @ Disable IRQ and FIQ and enter supervisor mode

@ todo: additional bigpayload job

	adr	r0, _start
	ldr	r1, =payload_start
	cmp	r0, r1
	beq	_start_al9h @ already launched from payload_start

@ todo: copy start -> payload_start 

	adr	r2, _start_al9h
	sub	r2, r0
	add	r1, r2
	bx	r1 @ jump to payload_start + _start_al9h - _start

_start_al9h:
    @ Change the stack pointer
    mov sp, #0x27000000

    @ Give read/write access to all the memory regions
    ldr r5, =0x33333333
    mcr p15, 0, r5, c5, c0, 2 @ write data access
    mcr p15, 0, r5, c5, c0, 3 @ write instruction access

    @ Sets MPU permissions and cache settings
    ldr r0, =0xFFFF001D	@ ffff0000 32k
    ldr r1, =0x01FF801D	@ 01ff8000 32k
    ldr r2, =0x08000027	@ 08000000 1M
    ldr r3, =0x10000021	@ 10000000 128k
    ldr r4, =0x10100025	@ 10100000 512k
    ldr r5, =0x20000035	@ 20000000 128M
    ldr r6, =0x1FF00027	@ 1FF00000 1M
    ldr r7, =0x1800002D	@ 18000000 8M
    mov r10, #0x25
    mov r11, #0x25
    mov r12, #0x25
    mcr p15, 0, r0, c6, c0, 0
    mcr p15, 0, r1, c6, c1, 0
    mcr p15, 0, r2, c6, c2, 0
    mcr p15, 0, r3, c6, c3, 0
    mcr p15, 0, r4, c6, c4, 0
    mcr p15, 0, r5, c6, c5, 0
    mcr p15, 0, r6, c6, c6, 0
    mcr p15, 0, r7, c6, c7, 0
    mcr p15, 0, r10, c3, c0, 0	@ Write bufferable 0, 2, 5
    mcr p15, 0, r11, c2, c0, 0	@ Data cacheable 0, 2, 5
    mcr p15, 0, r12, c2, c0, 1	@ Inst cacheable 0, 2, 5

    @ Enable caches
    mrc p15, 0, r4, c1, c0, 0  @ read control register
    orr r4, r4, #(1<<12)       @ - instruction cache enable
    orr r4, r4, #(1<<2)        @ - data cache enable
    orr r4, r4, #(1<<0)        @ - mpu enable
    mcr p15, 0, r4, c1, c0, 0  @ write control register

    @ Flush caches
    mov r5, #0
    mcr p15, 0, r5, c7, c5, 0  @ flush I-cache
    mcr p15, 0, r5, c7, c6, 0  @ flush D-cache
    mcr p15, 0, r5, c7, c10, 4 @ drain write buffer

    @ Fixes mounting of SDMC
	ldr r0, =0x10000020
	mov r1, #0x340
	str r1, [r0]

    bl toolsmain

.die:
    b .die
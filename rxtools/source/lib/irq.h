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

#ifndef IRQ_H
#define IRQ_H

#define IRQ_DMAC_1_0		0x00000001
#define IRQ_DMAC_1_1		0x00000002
#define IRQ_DMAC_1_2		0x00000004
#define IRQ_DMAC_1_3		0x00000008
#define IRQ_DMAC_1_4		0x00000010
#define IRQ_DMAC_1_5		0x00000020
#define IRQ_DMAC_1_6		0x00000040
#define IRQ_DMAC_1_7		0x00000080
#define IRQ_TIMER_0		0x00000100
#define IRQ_TIMER_1		0x00000200
#define IRQ_TIMER_2		0x00000400
#define IRQ_TIMER_3		0x00000800
#define IRQ_PXI_SYNC		0x00001000
#define IRQ_PXI_NOT_FULL	0x00002000
#define IRQ_PXI_NOT_EMPTY	0x00004000
#define IRQ_AES			0x00008000
#define IRQ_SDIO_1		0x00010000
#define IRQ_SDIO_1_ASYNC	0x00020000
#define IRQ_SDIO_3		0x00040000
#define IRQ_SDIO_3_ASYNC	0x00080000
#define IRQ_DEBUG_RECV		0x00100000
#define IRQ_DEBUG_SEND		0x00200000
#define IRQ_RSA			0x00400000
#define IRQ_CTR_CARD_1		0x00800000
#define IRQ_CTR_CARD_2		0x01000000
#define IRQ_CGC			0x02000000
#define IRQ_CGC_DET		0x04000000
#define IRQ_DS_CARD		0x08000000
#define IRQ_DMAC_2		0x10000000
#define IRQ_DMAC_2_ABORT	0x20000000

#define REG_IRQ_IE	*(volatile uint32_t*)0x10001000
#define REG_IRQ_IF	*(volatile uint32_t*)0x10001004

#define IRQ_ENABLE(x) REG_IRQ_IE |= x
#define IRQ_DISABLE(x) REG_IRQ_IE &= ~x
#define IRQ_ACK(x) REG_IRQ_IF = x
#define IRQ_WAIT() __asm__ volatile ("mcr p15, 0, %0, c7, c0, 4" :: "r"(0))

#endif

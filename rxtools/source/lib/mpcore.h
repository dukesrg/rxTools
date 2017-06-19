/*
 * Copyright (C) 2015, 2017 The PASTA Team, dukesrg
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

#ifndef MPCORE_H
#define MPCORE_H

#define REG_CFG11_SOCINFO	((volatile uint16_t*)0x10140FFC)

#define CFG11_SOCINFO_ALWAYS1		(1<<0)
#define CFG11_SOCINFO_KTR		(1<<1)
#define CFG11_SOCINFO_CLK3X		(1<<2)

typedef enum {
	MPINFO_CTR = 1,
	MPINFO_KTR = 7,
} MpInfo;

static inline MpInfo getMpInfo()
{
        return *(MpInfo *)0x10140FFC;
}

#endif

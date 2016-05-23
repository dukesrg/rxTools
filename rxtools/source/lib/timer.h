/*
 * Copyright (C) 2016 dukesrg
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

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#define TIMER_FREQ 67027964
#define TIMER_REG_COUNT 4
#define TIMER_REG_OFF 0x10003000
#define TIMER_VAL0 (TIMER_REG_OFF + 0)
#define TIMER_CNT0 (TIMER_REG_OFF + 2)
#define TIMER_VAL1 (TIMER_REG_OFF + 4)
#define TIMER_CNT1 (TIMER_REG_OFF + 6)
#define TIMER_VAL2 (TIMER_REG_OFF + 8)
#define TIMER_CNT2 (TIMER_REG_OFF + 10)
#define TIMER_VAL3 (TIMER_REG_OFF + 12)
#define TIMER_CNT3 (TIMER_REG_OFF + 14)
#define TIMER_DIV_1 0x0000
#define TIMER_DIV_64 0x0001
#define TIMER_DIV_256 0x0002
#define TIMER_DIV_1024 0x0003
#define TIMER_CASCADE 0x0004
#define TIMER_IRQ 0x0040 
#define TIMER_START 0x0080

void timerStart();
void timerStop();
uintmax_t timerGet();

#endif

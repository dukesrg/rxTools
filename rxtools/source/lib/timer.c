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

#include <stddef.h>
#include "timer.h"

static const struct {
	volatile uint16_t *val;
	volatile uint16_t *cnt;
} timer[TIMER_REG_COUNT] = {
	{(volatile uint16_t *)TIMER_VAL0, (volatile uint16_t *)TIMER_CNT0},
	{(volatile uint16_t *)TIMER_VAL1, (volatile uint16_t *)TIMER_CNT1},
	{(volatile uint16_t *)TIMER_VAL2, (volatile uint16_t *)TIMER_CNT2},
	{(volatile uint16_t *)TIMER_VAL3, (volatile uint16_t *)TIMER_CNT3}
};

static inline void timerSet(uint_fast8_t index, uint_fast16_t val, uint_fast16_t cnt) {
	*timer[index].val = val;
	*timer[index].cnt = cnt;
}

void timerStart() {
	timerStop();
	for (size_t i = 4; i--; timerSet(i, 0, TIMER_DIV_1024 | TIMER_CASCADE | TIMER_START));
}

void timerStop() {
	for (size_t i = TIMER_REG_COUNT ; i--; timerSet(i, 0, 0));
}

uintmax_t timerGet() {
	uintmax_t s = 0;
	for (size_t i = TIMER_REG_COUNT ; i--; s = (s << 16) | *timer[i].val);
	return s / (TIMER_FREQ >> 10);
}

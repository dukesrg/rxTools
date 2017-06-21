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

#include <stddef.h>
#include "native_firm.h"

firm_section_header *firmFindSection(firm_header *firm, uint32_t address) {
	if (firm->magic == FIRM_MAGIC)
		for (size_t i = sizeof(firm->sections)/sizeof(firm->sections[0]); i--;)
			if (address >= firm->sections[i].load_address && address < firm->sections[i].load_address + firm->sections[i].size)
				return &firm->sections[i];
	return NULL;
}

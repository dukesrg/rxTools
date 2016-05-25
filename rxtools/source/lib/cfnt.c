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

#include <stdio.h>
#include <string.h>
#include "cfnt.h"
#include "fs.h"

#define DEFAULT_GLYPH 0

finf_header *finf = NULL;

uint_fast16_t GlyphCode(wchar_t c) {
	cmap_header *cmap = finf->cmap_offset;
	uint_fast16_t glyph;
	do {
		if (c >= cmap->code_begin && c <= cmap->code_end) {
			switch(cmap->mapping_method) {
				case MAPPING_DIRECT:
					return cmap->direct_glyph_start - cmap->code_begin + c;
				case MAPPING_TABLE:
					return (glyph = cmap->table_glyphs[c - cmap->code_begin]) < UINT16_MAX ? glyph : DEFAULT_GLYPH;
				case MAPPING_SCAN:
					for (size_t i = 0; i < cmap->scan_pair_count; i++)
						if (cmap->scan_pairs[i].code == c)
							return cmap->scan_pairs[i].glyph;
			}
		}
	} while ((cmap = cmap->next_cmap_offset));
	return DEFAULT_GLYPH;
}

size_t wcstoglyphs(Glyph *glyphs, const wchar_t *str, size_t max) {
	if (!(glyphs && str && *str))
		return 0;
	size_t len = wcslen(str);
	if (!max || max > len)
		max = len;
	for (size_t i = max; i--;) {
		glyphs[i].code = DEFAULT_GLYPH;
		glyphs[i].width = &finf->default_glyph_width;
	}
	len = 0;
	cmap_header *cmap = finf->cmap_offset;
	uint_fast16_t glyphcode;
	do {
		for (size_t i = max; i--;) {
			if (str[i] >= cmap->code_begin && str[i] <= cmap->code_end) {
				glyphcode = DEFAULT_GLYPH;
				switch(cmap->mapping_method) {
					case MAPPING_DIRECT:
						glyphcode = cmap->direct_glyph_start - cmap->code_begin + str[i];
						break;
					case MAPPING_TABLE:
						glyphcode = cmap->table_glyphs[str[i] - cmap->code_begin];
						break;
					case MAPPING_SCAN:
						for (size_t j = cmap->scan_pair_count; j--;)
							if (cmap->scan_pairs[j].code == str[i]) {
								glyphcode = cmap->scan_pairs[j].glyph;
								break;
							}
					break;
				}
				if (glyphcode < UINT16_MAX)
					glyphs[i].code = glyphcode;
				len++;
			}
		}
	} while ((len < max ) && (cmap = cmap->next_cmap_offset));
	len = 0;
	cwdh_header *cwdh = finf->cwdh_offset;
	do {
		for (size_t i = max; i--;) {
			if (glyphs[i].code >= cwdh->start_index && glyphs[i].code <= cwdh->end_index) {
				glyphs[i].width = cwdh->widths + glyphs[i].code - cwdh->start_index;
				len++;
			}
		}
	} while ((len < max) && (cwdh = cwdh->next_cwdh_offset));
	return max;
}

glyph_width *GlyphWidth(uint_fast16_t glyphcode) {
	cwdh_header *cwdh = finf->cwdh_offset;
	do {
		if (glyphcode >= cwdh->start_index && glyphcode <= cwdh->end_index)
			return cwdh->widths + glyphcode - cwdh->start_index;
	} while ((cwdh = cwdh->next_cwdh_offset));
	return &finf->default_glyph_width;
}

inline uint8_t *GlyphSheet(uint_fast16_t glyphcode) {
	tglp_header *tglp = finf->tglp_offset;
	return (uint8_t*)tglp->sheet_data_offset + tglp->sheet_size * (glyphcode / (tglp->number_of_columns * tglp->number_of_rows));
}

size_t cfntPreload(wchar_t *path) {
	File f;
	cfnt_header cfnt;

	if (!(path && *path && FileOpen(&f, path, 0) &&
		(FileRead2(&f, &cfnt, sizeof(cfnt)) == sizeof(cfnt) || (FileClose(&f) && 0)) &&
		cfnt.magic == 'TNFC'
	)) return 0;
	FileClose(&f);
	return cfnt.file_size - sizeof(cfnt_header);
}

finf_header *cfntLoad(void *buf, wchar_t *path, size_t size) {
	File f;

	if (buf && path && *path && FileOpen(&f, path, 0) &&
		(FileRead(&f, buf, size, sizeof(cfnt_header)) == size || (FileClose(&f) && 0)) &&
		((finf_header*)buf)->magic == 'FNIF'
	) {
		FileClose(&f);
		finf = (finf_header*)buf;
		finf->tglp_offset = (tglp_header*)((uintptr_t)finf->tglp_offset + (uintptr_t)finf - sizeof(cfnt_header) - 8);
		finf->cwdh_offset = (cwdh_header*)((uintptr_t)finf->cwdh_offset + (uintptr_t)finf - sizeof(cfnt_header) - 8);
		finf->cmap_offset = (cmap_header*)((uintptr_t)finf->cmap_offset + (uintptr_t)finf - sizeof(cfnt_header) - 8);
		finf->tglp_offset->sheet_data_offset += (uintptr_t)finf - sizeof(cfnt_header);
		for (cwdh_header *cwdh = finf->cwdh_offset; cwdh->next_cwdh_offset; cwdh = (cwdh->next_cwdh_offset = (cwdh_header*)((uintptr_t)cwdh->next_cwdh_offset + (uintptr_t)finf - sizeof(cfnt_header) - 8)));
		for (cmap_header *cmap = finf->cmap_offset; cmap->next_cmap_offset; cmap = (cmap->next_cmap_offset = (cmap_header*)((uintptr_t)cmap->next_cmap_offset + (uintptr_t)finf - sizeof(cfnt_header) - 8)));
	}
	return finf;
}

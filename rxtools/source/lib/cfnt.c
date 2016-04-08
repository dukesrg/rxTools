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

uint8_t *decodetile(uint8_t *in, uint8_t *out, int iconsize, int tilesize, int ax, int ay, int w) {
	size_t x, y;
	uint8_t a;
	for (y = 0; y < iconsize; y += tilesize) {
		if (tilesize == 1) {
			a = *in++;
			out[ax + (ay + y) * w] = a & 0x0F;
			out[ax + (ay + y) * w + 1] = a >> 4;
		} else {
			for (x = 0; x < iconsize; x += tilesize)
				in = decodetile(in, out, tilesize, tilesize / 2, ax + x, ay + y, w);
		}
	}
	return in;
}

uint8_t *decodetile2(uint8_t *in, uint8_t *out, int iconsize, int tilesize, int ax, int ay, int w) {
	size_t x, y;
	uint_fast16_t a;
	if (iconsize == 2){
		a = *(uint16_t*)in;
		a = (a & 0x0F0F) + (a >> 4 & 0x0F0F);
		out[ax / 2 + ay * w / 2] = (a + (a >> 8)) >> 2 & 0x0F;
		in += 2;
	} else {
		for (y = 0; y < iconsize; y += tilesize)
			for (x = 0; x < iconsize; x += tilesize)
				in = decodetile2(in, out, tilesize, tilesize / 2, ax + x, ay + y, w);
	}
	return in;
}

/*
void printGlyph(uint_fast16_t glyph){
	size_t x, y ,i;
	uint8_t *sheet, val;
	tglp_header *tglp = finf->tglp_offset;

	sheet = (uint8_t*)malloc(tglp->sheet_width * tglp->sheet_height);

	uint8_t *sheets2 = GlyphSheet(glyph);

	for (y = 0; y < tglp->sheet_height; y += 8)
		for (x = 0; x < tglp->sheet_width; x += 8)
			decodetile(&sheets2, sheet, 8, 8, x, y, tglp->sheet_width);

	glyph_width *cw = GlyphWidth(glyph);
	
	uint_fast16_t glyphXoffs = glyph % tglp->number_of_columns * (tglp->cell_width + 1)+1;
	uint_fast16_t glyphYoffs = glyph % (tglp->number_of_columns * tglp->number_of_rows) / tglp->number_of_columns * (tglp->cell_height + 1)+1;

	for(y = 0; y < tglp->cell_height; y++) {
		printf("%.*s",cw->left,"........................");
		for(x = 0; x < tglp->cell_width - cw->left; x++) {
			val = sheet[x + glyphXoffs + (y + glyphYoffs) * tglp->sheet_width];
			printf(val ? "%X" : "%c", val ? val : ((x < cw->character - cw->left) ? ' ' : '#'));
		}
		printf("\n");
	}
	glyphXoffs = ((glyph % tglp->number_of_columns * (tglp->cell_width + 1)+1)+1)/2;
	glyphYoffs = (glyph % (tglp->number_of_columns * tglp->number_of_rows) / tglp->number_of_columns * (tglp->cell_height + 1)+1)+1/2;

	memset(sheet, 0, tglp->sheet_width * tglp->sheet_height);
	sheets2 = GlyphSheet(glyph);
	for (y = 0; y < tglp->sheet_height; y += 8)
		for (x = 0; x < tglp->sheet_width; x += 8)
			decodetile2(&sheets2, sheet, 8, 8, x, y, tglp->sheet_width);

	for (y = 0; y < (tglp->cell_height + 1) / 2; y++) {
		printf("%.*s",(cw->left+1)/2,"............");
		for (x = 0; x < (tglp->cell_width + 1)/2 - (cw->left + 1) / 2 ; x++) {
			val = sheet[x + glyphXoffs + (y + glyphYoffs) * tglp->sheet_width / 2];
			printf(val ? "%X" : "%c", val ? val : ((x < (cw->character+1)/2 - (cw->left +1 ) / 2) ? ' ' : '#'));
		}
		printf("\n");
	}
}	
*/
size_t cfntPreload(wchar_t *path) {
	File f;
	cfnt_header cfnt;

	if (!(path && *path && FileOpen(&f, path, false) &&
		(FileRead2(&f, &cfnt, sizeof(cfnt)) == sizeof(cfnt) || (FileClose(&f) && false)) &&
		cfnt.magic == 'TNFC'
	)) return 0;
	FileClose(&f);
	return cfnt.file_size - sizeof(cfnt_header);
}

finf_header *cfntLoad(void *buf, wchar_t *path, size_t size) {
	File f;

	if (buf && path && *path && FileOpen(&f, path, false) &&
		(FileRead(&f, buf, size, sizeof(cfnt_header)) == size || (FileClose(&f) && false)) &&
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

/*
int main(int argc, char *argv[]){
	size_t size = cfntPreload(argv[1]);
	if (!size)
		return 1;

	finf_header *finf = (finf_header*)malloc(size);

	if (!cfntLoad(finf, argv[1], size))
		return 1;

	uint_fast16_t glyph;

	for (size_t i = WCHAR_MIN; i <= WCHAR_MAX; i++) {
		glyph = GlyphCode(finf, i);
		glyph_width *cw = GlyphWidth(finf, glyph);
		if (glyph) {
			printf("Character: %u, Glyph: %u, left: %i, glyphw: %u, char: %u\n", i, glyph, cw->left, cw->glyph, cw->character);
			printGlyph(finf, glyph);
		}
	}
	return 0;
}
*/
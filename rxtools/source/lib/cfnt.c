#include <stdio.h>
#include <malloc.h>
#include "cfnt.h"

static uint16_t Glyph(finf_header *finf, wchar_t c) {
	cmap_header *cmap = finf->cmap_offset;
	do {
		if (c >= cmap->code_begin && c <= cmap->code_end) {
			switch(cmap->mapping_method) {
				case MAPPING_DIRECT: return cmap->direct_glyph_start - cmap->code_begin + c;
				case MAPPING_TABLE: return cmap->table_glyphs[c - cmap->code_begin];
				case MAPPING_SCAN:
					for (size_t i = 0; i < cmap->scan_pair_count; i++)
						if (cmap->scan_pairs[i].code == c)
							return cmap->scan_pairs[i].glyph;
			}
		}
	} while (cmap = cmap->next_cmap_offset);
	return 0;
}

static char_width *GlyphWidth(finf_header *finf, uint16_t glyph) {
	cwdh_header *cwdh = finf->cwdh_offset;
	do {
		if (glyph >= cwdh->start_index && glyph <= cwdh->end_index)
			return cwdh->widths + glyph - cwdh->start_index;
	} while (cwdh = cwdh->next_cwdh_offset);
	return &finf->default_char_width;
}

static inline uint8_t *GlyphSheet(finf_header *finf, uint16_t glyph) {
	tglp_header *tglp = finf->tglp_offset;
	return (uint8_t*)tglp->sheet_data_offset + tglp->sheet_size * (glyph / (tglp->number_of_columns * tglp->number_of_rows));
}

void decodetile(uint8_t **in, uint16_t *out, int iconsize, int tilesize, int ax, int ay, int w) {
	size_t x, y;
	uint16_t a;
	for (y = 0; y < iconsize; y += tilesize) {
		if (tilesize == 1) {
			a = *(*in)++;
			out[ax / 2 + (ay + y) * w / 2] = (a << 4 | a) & 0x0F0F;
//			out[ax / 2 + (ay + y) * w / 2] = (a << 4 | a >> 1) & 0x0707; //for simple alpha-blending
		} else {
			for (x = 0; x < iconsize; x += tilesize)
				decodetile(in, out, tilesize, tilesize / 2, ax + x, ay + y, w);
		}
	}
}

void decodetile2(uint16_t **in, uint8_t *out, int iconsize, int tilesize, int ax, int ay, int w) {
	size_t x, y;
	uint16_t a;
	if (iconsize == 2){
		a = *(*in)++;
		a = (a & 0x0F0F) + (a >> 4 & 0x0F0F);
		out[ax / 2 + ay * w / 4] = (a + (a >> 8)) >> 2 & 0x0F;
//		out[ax / 2 + ay * w / 4] = (a + (a >> 8)) >> 3 & 0x07; //for simple alpha-blending
	} else {
		for (y = 0; y < iconsize; y += tilesize)
			for (x = 0; x < iconsize; x += tilesize)
				decodetile2(in, out, tilesize, tilesize / 2, ax + x, ay + y, w);
	}
}

void printGlyph(finf_header *finf, uint16_t glyph){
	size_t x, y ,i;
	uint8_t *sheet, val;
	tglp_header *tglp = finf->tglp_offset;

	sheet = (uint8_t*)malloc(tglp->sheet_width * tglp->sheet_height);

	uint8_t *sheets2 = GlyphSheet(finf, glyph);

	for (y = 0; y < tglp->sheet_height; y += 8)
		for (x = 0; x < tglp->sheet_width; x += 8)
			decodetile(&sheets2, (uint16_t*)sheet, 8, 8, x, y, tglp->sheet_width);

	for(y = 0; y < tglp->sheet_height; y++) {
		for(x = 0; x < tglp->sheet_width; x++) {
			val = sheet[x + y * tglp->sheet_width];
			printf(val ? "%X" : "%c", val ? val : ' ');
		}
		printf("\n");
	}

	memset(sheet, 0, tglp->sheet_width * tglp->sheet_height);
	sheets2 = GlyphSheet(finf, glyph);
	for (y = 0; y < tglp->sheet_height; y += 8)
		for (x = 0; x < tglp->sheet_width; x += 8)
			decodetile2(&(uint16_t*)sheets2, sheet, 8, 8, x, y, tglp->sheet_width);

	for (y = 0; y < tglp->sheet_height / 2; y++) {
		for (x = 0; x < tglp->sheet_width / 2; x++) {
			val = sheet[x + y * tglp->sheet_width / 2];
			printf(val ? "%X" : "%c", val ? val : ' ');
		}
		printf("\n");
	}

}


size_t cfntPreload(char *path) {
	cfnt_header cfnt;
	FILE *fp = fopen(path, "rb");
	fread(&cfnt, sizeof(cfnt), 1, fp);
	fclose(fp);
	if (cfnt.magic != 'TNFC')
		return 0;
	return cfnt.file_size - sizeof(cfnt_header);
}

bool cfntLoad(finf_header *finf, char *path, size_t size) {
	FILE *fp = fopen(path, "rb");
	fseek(fp, sizeof(cfnt_header), SEEK_SET);
	fread(finf, size, 1, fp);
	fclose(fp);
	finf->tglp_offset = (tglp_header*)((uintptr_t)finf->tglp_offset + (uintptr_t)finf - sizeof(cfnt_header) - 8);
	finf->cwdh_offset = (cwdh_header*)((uintptr_t)finf->cwdh_offset + (uintptr_t)finf - sizeof(cfnt_header) - 8);
	finf->cmap_offset = (cmap_header*)((uintptr_t)finf->cmap_offset + (uintptr_t)finf - sizeof(cfnt_header) - 8);
	finf->tglp_offset->sheet_data_offset += (uintptr_t)finf - sizeof(cfnt_header);
	for (cwdh_header *cwdh = finf->cwdh_offset; cwdh->next_cwdh_offset; cwdh = (cwdh->next_cwdh_offset = (cwdh_header*)((uintptr_t)cwdh->next_cwdh_offset + (uintptr_t)finf - sizeof(cfnt_header) - 8)));
	for (cmap_header *cmap = finf->cmap_offset; cmap->next_cmap_offset; cmap = (cmap->next_cmap_offset = (cmap_header*)((uintptr_t)cmap->next_cmap_offset + (uintptr_t)finf - sizeof(cfnt_header) - 8)));
	return true;
}

int main(int argc, char *argv[]){
	size_t size = cfntPreload(argv[1]);
	if (!size)
		return 1;

	finf_header *finf = (finf_header*)malloc(size);

	if (!cfntLoad(finf, "cbf_std.bcfnt", size))
		return 1;

	uint16_t glyph = Glyph(finf, '%');
	char_width *cw = GlyphWidth(finf, glyph);
	printGlyph(finf, glyph);

	return 0;
};

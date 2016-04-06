#include <stdio.h>
#include <malloc.h>
#include "cfnt.h"

cfnt_header cfnt;
finf_header finf;
tglp_header tglp;
cwdh_header cwdh;
cmap_header cmap;
uint8_t sheets[2048];

void decodetile(uint8_t **in, uint8_t *out, int iconsize, int tilesize, int ax, int ay){
	int x,y;
	for (y = 0; y < iconsize; y += tilesize){
		if (tilesize==1){
			out[ax+(ay+y)*tglp.sheet_width] = (**in)&0x0f;
			out[ax+(ay+y)*tglp.sheet_width+1] = *((*in)++)>>4;
		} else {
			for (x = 0; x < iconsize; x += tilesize){
				decodetile(in, out, tilesize,tilesize / 2,ax + x,ay + y);
			}
		}
	}
}


void printGlyph(uint16_t glyph){
	size_t x,y,i;
	uint32_t offs=0;
	uint8_t sheet[4096] = {0};
	
			uint8_t *sheets2 = sheets;
					for (y = 0; y < tglp.sheet_height; y += 8)
					{
						for (x = 0; x < tglp.sheet_width; x += 8)
						{
							decodetile(&sheets2, sheet, 8, 8, x, y);
/*							for (i = 0; i < 64; i++)
							{
								int x2 = i % 8;
								if (x + x2 >= tglp.sheet_width) continue;
								int y2 = i / 8;
								if (y + y2 >= tglp.sheet_height) continue;
								int pos = tiles[x2 % 4 + y2 % 4 * 4] + 16 * (x2 / 4) + 32 * (y2 / 4);
								int shift = (pos & 1) * 4;
								sheet[(y + y2) * tglp.sheet_width + x + x2] = (((sheets[offs + pos / 2] >> shift) & 0xF) );//* 0x11
							}
							offs += 64 / 2;
*/						}
					}

	int val;
	for(y=0; y<tglp.sheet_height; y++){
		for(x=0; x<tglp.sheet_width; x++){
			val = sheet[x+y*tglp.sheet_width];
			printf(val?"%1X":"%c",val?val:' ');
		}
		printf("\n");
	}
}

int main(int argc, char *argv[]){
	FILE *fp = fopen(argv[1], "rb");

	fread(&cfnt, sizeof(cfnt), 1, fp);
	printf("\n=====CFNT=====\n");
	printf("Magic: %.*s\n", sizeof(cfnt.magic), &cfnt.magic);
	printf("Endianness: 0x%04x\n", cfnt.endianness);
	printf("Version: 0x%08x\n", cfnt.version);
	printf("File size: %u\n", cfnt.file_size);
	printf("Number of blocks: 0x%08x\n", cfnt.number_of_blocks);

	fread(&finf, sizeof(finf), 1, fp);
	printf("\n=====FINF=====\n");
	printf("Magic: %.*s\n", sizeof(finf.magic), &finf.magic);
	printf("Section size: %u\n", finf.section_size);
	printf("Font type: 0x%02x\n", finf.font_type);
	printf("Line feed: 0x%02x\n", finf.line_feed);
	printf("Alter char index: %u\n", finf.alter_char_index);
	printf("Default left width: %u\n", finf.default_char_width.left);
	printf("Default glyph width: %u\n", finf.default_char_width.glyph);
	printf("Default char width: %u\n", finf.default_char_width.character);
	printf("Encoding: %u\n", finf.encoding);
	printf("TGLP offset: 0x%08x\n", finf.tglp_offset);
	printf("CWDH offset: 0x%08x\n", finf.cwdh_offset);
	printf("CMAP offset: 0x%08x\n", finf.cmap_offset);
	printf("Height: %u\n", finf.height);
	printf("Width: %u\n", finf.width);
	printf("Ascent: %u\n", finf.ascent);
	printf("Reserved: 0x%02x\n", finf.reserved);

	fread(&tglp, sizeof(tglp), 1, fp);
	printf("\n=====TGLP=====\n");
	printf("Magic: %.*s\n", sizeof(tglp.magic), &tglp.magic);
	printf("Section size: %u\n", tglp.section_size);
	printf("Cell width: %u\n", tglp.cell_width);
	printf("Cell height: %u\n", tglp.cell_height);
	printf("Baseline position: %u\n", tglp.baseline_position);
	printf("Max character width: %u\n", tglp.max_character_width);
	printf("Sheet size: %u\n", tglp.sheet_size);
	printf("Number of sheets: %u\n", tglp.number_of_sheets);
	printf("Sheet image format: %u\n", tglp.sheet_image_format);
	printf("Number of columns: %u\n", tglp.number_of_columns);
	printf("Number of rows: %u\n", tglp.number_of_rows);
	printf("Sheet width: %u\n", tglp.sheet_width);
	printf("Sheet height: %u\n", tglp.sheet_height);
	printf("Sheet data offset: 0x%08x\n", tglp.sheet_data_offset);

	fseek(fp, tglp.sheet_data_offset, SEEK_SET);
//	sheets = malloc(tglp.sheet_size*tglp.number_of_sheets);
//	fread(&sheets, tglp.sheet_size, tglp.number_of_sheets, fp);
	fread(&sheets, tglp.sheet_size, 1, fp);

	fseek(fp, finf.cwdh_offset-8, SEEK_SET);
	fread(&cwdh, sizeof(cwdh), 1, fp);
	printf("\n=====CWDH=====\n");
	printf("Magic: %.*s\n", sizeof(cwdh.magic), &cwdh.magic);
	printf("Section size: %u\n", cwdh.section_size);
	printf("Start index: %u\n", cwdh.start_index);
	printf("End index: %u\n", cwdh.end_index);
	printf("Next cwdh offset: 0x%08x\n", cwdh.next_cwdh_offset);

	uint16_t char_count = cwdh.end_index - cwdh.start_index + 1;
	char_width character_widths[char_count];
	fread(character_widths, sizeof(char_width), char_count, fp);


	uint32_t cmap_offset = finf.cmap_offset;
	do {
		fseek(fp, cmap_offset-8, SEEK_SET);
		fread(&cmap, sizeof(cmap), 1, fp);

		printf("\n=====CMAP=====\n");
		printf("Magic: %.*s\n", sizeof(cmap.magic), &cmap.magic);
		printf("Section size: %u\n", cmap.section_size);
		printf("Code begin: %u\n", cmap.code_begin);
		printf("Code end: %u\n", cmap.code_end);
		printf("Mapping method: %u\n", cmap.mapping_method);
		printf("Reserved: 0x%04x\n", cmap.reserved);
		printf("Next CMAP offset: 0x%08x\n", cmap.next_cmap_offset);
	}
	while ((cmap_offset = cmap.next_cmap_offset));

	printGlyph(5);

	fclose(fp);
	return 0;
};
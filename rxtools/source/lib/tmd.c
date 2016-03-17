#include "tmd.h"
#include "fs.h"

const wchar_t *titleDir = L"%d:title/%08lx/%08lx/content";

void tmdLoad(tmd_data *data, uint32_t drive, uint32_t tidh, uint32_t tidl) {
	FIL fil;
	DIR dir;
	FILINFO fno;
	UINT br;
	wchar_t path[_MAX_LFN + 1], lfn[_MAX_LFN + 1], tmdpath[_MAX_LFN + 1];
	size_t header_offset = 0;
	fno.lfname = lfn;
	fno.lfsize = _MAX_LFN + 1;

	data->sig_type = 0; //mark data as void
	swprintf(path, _MAX_LFN + 1, titleDir, drive, tidh, tidl);
	if (f_findfirst(&dir, &fno, path, L"*.tmd") == FR_OK) {
		do {
			swprintf(tmdpath, _MAX_LFN + 1, L"%ls/%ls", path, *fno.lfname ? fno.lfname : fno.fname);
			if (f_open(&fil, tmdpath, FA_READ | FA_OPEN_EXISTING) == FR_OK) {
				if (f_read(&fil, &data->sig_type, sizeof(data->sig_type), &br) == FR_OK && br == sizeof(data->sig_type)) {
					switch (__builtin_bswap32(data->sig_type)){
						case RSA_4096_SHA1:
						case RSA_4096_SHA256:
							header_offset = 0x240;
							break;
						case RSA_2048_SHA1:
						case RSA_2048_SHA256:
							header_offset = 0x140;
							break;
						case ECDSA_SHA1:
						case ECDSA_SHA256:
							header_offset = 0x80;
							break;
					}
					if (header_offset == 0 ||
						f_lseek(&fil, header_offset) != FR_OK ||
						f_read(&fil, &data->header, sizeof(data->header), &br) != FR_OK ||
						br != sizeof(data->header) ||
						__builtin_bswap16(data->header.content_count) > TMD_MAX_CHUNKS ||
						f_read(&fil, &data->content_info, sizeof(data->content_info), &br) != FR_OK ||
						br != sizeof(data->content_info) ||
						f_read(&fil, &data->content_chunk, sizeof(tmd_content_chunk) * __builtin_bswap16(data->header.content_count), &br) != FR_OK ||
						br != sizeof(tmd_content_chunk) * __builtin_bswap16(data->header.content_count)
					)
						data->sig_type = 0; //mark data as void on any fail
				}
				f_close(&fil);
			}
		} while (f_findnext(&dir, &fno) == FR_OK && *fno.fname);
	}
	f_closedir(&dir);
}
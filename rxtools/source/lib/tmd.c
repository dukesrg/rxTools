#include "tmd.h"
#include "fs.h"

#define CONTENT_VERSION_UNSET 0xFFFF

const wchar_t *titleDir = L"%d:title/%08lx/%08lx/content";

bool tmdLoad(wchar_t *apppath, tmd_data *data, uint32_t drive) {
	FIL fil;
	DIR dir;
	FILINFO fno;
	UINT br;
	wchar_t path[_MAX_LFN + 1], lfn[_MAX_LFN + 1], tmdpath[_MAX_LFN + 1];
	size_t header_offset;
	fno.lfname = lfn;
	fno.lfsize = _MAX_LFN + 1;
	tmd_data data_tmp;

	data->header.title_version = CONTENT_VERSION_UNSET;
	swprintf(path, _MAX_LFN + 1, titleDir, drive, __builtin_bswap32(data->header.title_id_hi), __builtin_bswap32(data->header.title_id_lo));
	if (f_findfirst(&dir, &fno, path, L"*.tmd") == FR_OK) {
		do {
			swprintf(tmdpath, _MAX_LFN + 1, L"%ls/%ls", path, *fno.lfname ? fno.lfname : fno.fname);
			if (f_open(&fil, tmdpath, FA_READ | FA_OPEN_EXISTING) == FR_OK) {
				if (f_read(&fil, &data_tmp.sig_type, sizeof(data_tmp.sig_type), &br) == FR_OK && br == sizeof(data_tmp.sig_type)) {
					switch (data_tmp.sig_type){
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
						default:
							header_offset = 0;
					}
					if (header_offset != 0 &&
						f_lseek(&fil, header_offset) == FR_OK &&
						f_read(&fil, &data_tmp.header, sizeof(data_tmp.header), &br) == FR_OK && br == sizeof(data_tmp.header) &&
						(__builtin_bswap16(data_tmp.header.title_version) > __builtin_bswap16(data->header.title_version) || data->header.title_version == CONTENT_VERSION_UNSET) &&
						__builtin_bswap16(data_tmp.header.content_count) > TMD_MAX_CHUNKS &&
						f_read(&fil, &data_tmp.content_info, sizeof(data_tmp.content_info), &br) == FR_OK && br == sizeof(data_tmp.content_info) &&
						f_read(&fil, &data_tmp.content_chunk, sizeof(tmd_content_chunk) * __builtin_bswap16(data_tmp.header.content_count), &br) == FR_OK && br == sizeof(tmd_content_chunk) * __builtin_bswap16(data_tmp.header.content_count)
					) {
						for (int i = __builtin_bswap16(data_tmp.header.content_count) - 1; i >= 0; i--) {
							if (data_tmp.content_chunk[i].content_index == Main_Content) {
								swprintf(apppath, _MAX_LFN + 1, L"%ls/%08lx.app", path, __builtin_bswap32(data->content_chunk[i].content_id));
								if (f_stat(apppath, NULL) == FR_OK)
									*data = data_tmp;
								break;
							}
						}
					}
				}
				f_close(&fil);
			}
		} while (f_findnext(&dir, &fno) == FR_OK && *fno.fname);
	}
	f_closedir(&dir);
	return data->header.title_version == CONTENT_VERSION_UNSET;
}

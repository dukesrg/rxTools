#include "aes.h"

uint_fast8_t aes_set_key(aes_key *key) {
	if (!key || key->slot > AES_MAX_KEYSLOT) return 0;
	if (key->data) {
		*REG_AESCNT = (*REG_AESCNT & (~AES_CNT_INPUT_MODE)) | key->mode;
		if (key->slot > 0x04) {
			*REG_AESKEYCNT = AES_KEYCNT_FIFO_FLUSH_ENABLE | AES_KEYCNT_KEYGEN_3DS | key->slot;
			for (size_t i = 4; i--;) *(REG_AESKEYFIFO_ + key->type) = key->data->as32[3-i];
		} else if (key->mode & AES_CNT_INPUT_ORDER)
			for (size_t i = 4; i--;) REG_AESKEY[key->type + key->slot * 3].as32[3-i] = key->data->as32[i];
		else
			REG_AESKEY[key->type + key->slot * 3] = *key->data;
	}
	*REG_AESKEYSEL = key->slot;
	*REG_AESCNT |= AES_CNT_UPDATE_KEYSLOT;
	return 1;
}

void aes_add_ctr(aes_ctr *ctr, uint32_t carry) {
	if (!ctr) return;
	uint32_t counter;
	switch (ctr->mode) {
		case AES_CNT_INPUT_BE_NORMAL:
			for (size_t i = 4; i--;) {
				counter = __builtin_bswap32(ctr->data.as32[i]);
				carry += counter;
				ctr->data.as32[i] = __builtin_bswap32(carry);
				carry = carry < counter ? 1 : 0;
			}
			return;
		case AES_CNT_INPUT_LE_NORMAL:
			for (size_t i = 4; i--;) {
				counter = ctr->data.as32[i];
				carry += counter;
				ctr->data.as32[i] = carry;
				carry = carry < counter ? 1 : 0;
			}
			return;
		case AES_CNT_INPUT_BE_REVERSE:
			for (size_t i = 0; i < 4; i++) {
				counter = __builtin_bswap32(ctr->data.as32[i]);
				carry += counter;
				ctr->data.as32[i] = __builtin_bswap32(carry);
				carry = carry < counter ? 1 : 0;
			}
			return;
		case AES_CNT_INPUT_LE_REVERSE:
			for (size_t i = 0; i < 4; i++) {
				counter = ctr->data.as32[i];
				carry += counter;
				ctr->data.as32[i] = carry;
				carry = carry < counter ? 1 : 0;
			}
			return;
	}
}

static void aes_add_cbc(aes_ctr *ctr, void *lastblock, uint32_t mode) {
	if (!ctr || !lastblock) return;
	aes_ctr_data *data = (aes_ctr_data*)lastblock;
	switch ((mode ^ ctr->mode) & AES_CNT_INPUT_MODE) {
		case AES_CNT_INPUT_ORDER | AES_CNT_INPUT_ENDIAN:
			for (size_t i = 4; i--; ctr->data.as32[3-i] = __builtin_bswap32(data->as32[i]));
			return;
		case AES_CNT_INPUT_ORDER:
			for (size_t i = 4; i--; ctr->data.as32[3-i] = data->as32[i]);
			return;
		case AES_CNT_INPUT_ENDIAN:
			for (size_t i = 4; i--; ctr->data.as32[i] = __builtin_bswap32(data->as32[i]));
			return;
		default:
			ctr->data = *data;
	}
}

void aes(void *outbuf, void *inbuf, size_t size, aes_ctr *ctr, uint32_t mode) {
	if (!outbuf || !size) return;
	size_t rblocks, wblocks, blocks = 0;
	uint32_t *in = (uint32_t*)inbuf;
	uint32_t *out = (uint32_t*)outbuf;
	size = (size + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE;
	while (size) {
		rblocks = wblocks = size < 0xFFFF ? size : 0xFFFF;
		size -= rblocks;
		blocks += rblocks;
		if ((mode & AES_MODE) != AES_ECB_DECRYPT_MODE && (mode & AES_MODE) != AES_ECB_ENCRYPT_MODE) {
			*REG_AESCNT = (*REG_AESCNT & (~AES_CNT_INPUT_MODE)) | ctr->mode;
			if (ctr->mode & AES_CNT_INPUT_ORDER)
				for (size_t i = 4; i--; REG_AESCTR_->as32[3 - i] = ctr->data.as32[i]);
			else
				*REG_AESCTR_ = ctr->data;
		}
		switch (mode & AES_MODE) {
			case AES_CTR_ENCRYPT_MODE:
			case AES_CTR_DECRYPT_MODE:
				aes_add_ctr(ctr, rblocks);
				break;
			case AES_CBC_DECRYPT_MODE:
				aes_add_cbc(ctr, inbuf + (blocks - 1) * AES_BLOCK_SIZE, mode);
				break;
		}
		*REG_AESBLKCNT = rblocks << 16;
		*REG_AESCNT = mode | AES_CNT_FLUSH_WRITE | AES_CNT_FLUSH_READ | AES_CNT_START;
/*
		rblocks *= AES_BLOCK_SIZE / sizeof(uint32_t);
		wblocks = rblocks;
		while (rblocks) {
			for (size_t i = *REG_AESCNT & 0x1F; wblocks && i++ < 0x1F; *REG_AESWRFIFO = in ? *in++ : 0)
				wblocks--;
			for (size_t i = *REG_AESCNT >> 5 & 0x1F; rblocks && i--; *out++ = *REG_AESRDFIFO)
				rblocks--;
		}
*/
		while (rblocks) {
			if (wblocks && (*REG_AESCNT & 0x1F) <= 0x1F - 0x04) {
				for (size_t i = AES_BLOCK_SIZE / sizeof(uint32_t); i--; *REG_AESWRFIFO = in ? *in++ : 0);
				wblocks--;
			}
			if (rblocks && (0x1F << 5 & *REG_AESCNT) >= 0x04 << 5) {
				for (size_t i = AES_BLOCK_SIZE / sizeof(uint32_t); i--; *out++ = *REG_AESRDFIFO);
				rblocks--;
			}
		}
		if ((mode & AES_MODE) == AES_CBC_ENCRYPT_MODE)
			aes_add_cbc(ctr, outbuf + (blocks - 1) * AES_BLOCK_SIZE, mode << 1);
	}
}

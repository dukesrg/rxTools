#ifndef SHA_H
#define SHA_H

#include <stddef.h>
#include <stdint.h>

#define REG_SHA_CNT		((volatile uint32_t*)0x1000A000)
#define REG_SHA_BLKCNT		((volatile uint32_t*)0x1000A004)
#define REG_SHA_HASH		((volatile uint32_t*)0x1000A040)
#define REG_SHA_INFIFO		((volatile uint32_t*)0x1000A080)

#define SHA_NORMAL_ROUND	0x00000001
#define SHA_FINAL_ROUND		0x00000002
#define SHA_OUTPUT_BE		0x00000008
#define SHA_OUTPUT_LE		0
#define SHA_256_MODE		0
#define SHA_224_MODE		0x00000010
#define SHA_1_MODE		0x00000020

#define SHA_INFIFO_SIZE		0x40
#define SHA_256_SIZE		0x20
#define SHA_224_SIZE		0x1C
#define SHA_1_SIZE		0x14

void sha_start(uint32_t mode, void *init);
void sha_update(void *src, size_t size);
void sha_finish(void *hash);
void sha(void *hash, void *data, size_t size, uint32_t mode);

#endif
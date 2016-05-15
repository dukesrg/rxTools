#ifndef RSA_H
#define RSA_H

#include <stdint.h>

#define RSA_SLOT_COUNT 4

typedef struct {
	uint32_t CNT;
	uint32_t SIZE;
	uint32_t UNK1;
	uint32_t UNK2;
} REG_RSA_SLOT;

#define REG_RSA_CNT		((volatile uint32_t*)0x1000B000)
#define REG_RSA_SLOTS		((volatile REG_RSA_SLOT*)0x1000B100)
#define REG_RSA_EXPFIFO		((volatile uint32_t*)0x1000B200)
#define REG_RSA_MOD		((volatile uintptr_t*)0x1000B400)
#define REG_RSA_TXT		((volatile uintptr_t*)0x1000B800)

#define RSA_MOD_SIZE	0x100
#define RSA_TXT_SIZE	0x100

#define RSA_CNT_START		0x00000001
#define RSA_CNT_KEYSLOTS	0x000000F0
#define RSA_CNT_ENDIAN		0x00000100
#define RSA_CNT_ORDER		0x00000200

#define RSA_SLOTCNT_KEY_SET	0x00000001
#define RSA_SLOTCNT_WRITE_PROTECT	0x00000002

#define RSA_IO_BE_NORMAL	RSA_CNT_ENDIAN | RSA_CNT_ORDER
#define RSA_IO_BE_REVERSE	RSA_CNT_ENDIAN
#define RSA_IO_LE_NORMAL	RSA_CNT_ORDER
#define RSA_IO_LE_REVERSE

#define RSA_1024_SIZE		0x80
#define RSA_2048_SIZE		0x100
#define RSA_4096_SIZE		0x200

#endif

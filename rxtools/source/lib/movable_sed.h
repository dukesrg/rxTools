#ifndef MOVABLE_SED_H
#define MOVABLE_SED_H

#include <stdint.h>
#include "aes.h"

#define SEED_MAGIC 'DEES'

typedef struct {
	uint32_t magic;
	uint32_t flags;
	uint8_t signature[0x100];
	uint8_t reserved[8];
	aes_key_data keyY;
} movable_sed;

uint_fast8_t movablesedVerify(wchar_t *path);
uint_fast8_t movablesedGetID0(wchar_t *id0, wchar_t *path);

#endif
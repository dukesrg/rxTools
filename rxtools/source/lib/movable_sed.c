#include "movable_sed.h"
#include "fs.h"
#include "sha.h"

static uint_fast8_t movablesedLoad(movable_sed *seed, wchar_t *path) {
	FIL fil;
	
	if (!FileOpen(&fil, path, 0) || (
		(FileRead2(&fil, seed, sizeof(movable_sed)) != sizeof(movable_sed) ||
		seed->magic != SEED_MAGIC) &&
//todo: RSA check
		(FileClose(&fil) || 1)
	)) return 0;
	FileClose(&fil);

	return 1;
}

uint_fast8_t movablesedVerify(wchar_t *path) {
	movable_sed seed;
	return movablesedLoad(&seed, path);
}

uint_fast8_t movablesedGetID0(wchar_t *id0, wchar_t *path) {
	movable_sed seed;
	uint32_t hash[SHA_256_SIZE / sizeof(uint32_t)];

	if (!movablesedLoad(&seed, path))
		return 0;

	sha(hash, &seed.keyY, sizeof(seed.keyY), SHA_256_MODE);
	swprintf(id0, 33, L"%08x%08x%08x%08x", hash[0], hash[1], hash[2], hash[3]);

	return 1;
}
#ifndef PROGRESS_H
#define PROGRESS_H

#include <stdint.h>
#include "draw.h"

enum {
	STATUS_CANCELABLE = 1,
	STATUS_WAIT = 2,
	STATUS_FINISHED = 0x40,
	STATUS_FAILED = 0x80
};

uint_fast8_t progressSetPos(uintmax_t pos);
void progressPinOffset();
void progressSetMax(uintmax_t posmax);
void statusInit(uintmax_t gaugemax, uint_fast8_t features, wchar_t *format, ...);

#endif
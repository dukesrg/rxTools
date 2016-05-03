#ifndef PROGRESS_H
#define PROGRESS_H

#include <stdint.h>
#include "draw.h"

void progressInit(Screen *screen, Rect *rect, Color frame, Color done, Color back, Color textcolor, uint_fast8_t fontsize, uint32_t posmax);
uint_fast8_t progressSetPos(uint32_t pos);
void progressPinOffset();
void progressSetMax(uint32_t posmax);
void statusInit(uint32_t gaugemax, wchar_t *format, ...);

#endif

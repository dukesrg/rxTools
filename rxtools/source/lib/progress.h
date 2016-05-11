#ifndef PROGRESS_H
#define PROGRESS_H

#include <stdint.h>
#include "draw.h"

void progressInit(Screen *screen, Rect *rect, Color frame, Color done, Color back, Color textcolor, uint_fast8_t fontsize, uintmax_t posmax);
uint_fast8_t progressSetPos(uintmax_t pos);
void progressPinOffset();
void progressSetMax(uintmax_t posmax);
void statusInit(uintmax_t gaugemax, wchar_t *format, ...);

#endif

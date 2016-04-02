#ifndef PROGRESS_H
#define PROGRESS_H

#include <stdint.h>
#include "draw.h"

void progressInit(Screen *screen, Rect *rect, Color frame, Color done, Color back, TextColors *colortext, FontMetrics *font, uint32_t posmax);
void progressCallback(uint32_t pos);
void statusInit(uint_fast16_t gaugemax, wchar_t *format, ...);

#endif

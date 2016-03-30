#ifndef PROGRESS_H
#define PROGRESS_H

#include <stdint.h>
#include "draw.h"

void progressInit(Screen *screen, Rect *rect, Color border, Color back, Color fill, TextColors *text, FontMetrics *font, uint32_t posmax);
void progressCallback(uint32_t pos);

#endif

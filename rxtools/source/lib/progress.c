#include "progress.h"

static Screen *progress_screen;
static Rect progress_rect;
static Color progress_border;
static Color progress_back;
static Color progress_fill;
static FontMetrics *progress_font;
static TextColors progress_fontcolor;
static uint32_t progress_posmax;

void progressInit(Screen *screen, Rect *rect, Color border, Color back, Color fill, TextColors *text, FontMetrics *font, uint32_t posmax) {
	progress_screen = screen;
	progress_rect = *rect;
	progress_border = border;
	progress_back = back;
	progress_fill = fill;
	progress_fontcolor = *text;
	progress_font = font;
	progress_posmax = posmax;
	
	progressCallback(0);
}

void progressCallback(uint32_t pos) {
	static uint32_t oldpos;
	if (pos != oldpos || !pos) {
		DrawProgress(progress_screen, &progress_rect, progress_border, progress_back, progress_fill, &progress_fontcolor, progress_font, progress_posmax, pos);
		DisplayScreen(progress_screen);
		oldpos = pos;
	}
}

#include <stdarg.h>
#include "fs.h"
#include "progress.h"
#include "theme.h"

static Screen *progress_screen;
static Rect progress_rect;
static Color progress_frame;
static Color progress_done;
static Color progress_back;
static FontMetrics *progress_font;
static TextColors progress_textcolor;
static uint32_t progress_posmax;

void progressInit(Screen *screen, Rect *rect, Color frame, Color done, Color back, TextColors *textcolor, FontMetrics *font, uint32_t posmax) {
	progress_screen = screen;
	progress_rect = *rect;
	progress_frame = frame;
	progress_done = done;
	progress_back = back;
	progress_textcolor = *textcolor;
	progress_font = font;
	progress_posmax = posmax;
	
	progressCallback(0);
}

void progressCallback(uint32_t pos) {
	static uint32_t oldpos;
	if (pos != oldpos || !pos) {
		DrawProgress(progress_screen, &progress_rect, progress_frame, progress_done, progress_back, &progress_textcolor, progress_font, progress_posmax, oldpos = pos);
		DisplayScreen(&bottomScreen);
		DisplayScreen(&top1Screen);
//		DisplayScreen(&top2Screen);
	}
}

void statusInit(uint_fast16_t gaugemax, wchar_t *format, ...) {
	if (*style.activitytop1img) {
		DrawSplash(&top1Screen, style.activitytop1img);
//		if (*style.activitytop2img)
//			DrawSplash(&top2Screen, style.activitytop2img);
//		else
//			memcpy(top2Screen.buf2, top1Screen.buf2, top1Screen.size);
	} else if (*style.top1img) {
		DrawSplash(&top1Screen, style.top1img);
//		if (*style.top2img)
//			DrawSplash(&top2Screen, style.top2img);
//		else
//			memcpy(top2Screen.buf2, top1Screen.buf2, top1Screen.size);
	}
	if (*style.activitybottomimg)
		DrawSplash(&bottomScreen, style.activitybottomimg);
	else if (*style.bottomimg)
		DrawSplash(&bottomScreen, style.bottomimg);

	wchar_t str[_MAX_LFN + 1];
	va_list va;
	va_start(va, format);
	vswprintf(str, _MAX_LFN + 1, format, va);
	va_end(va);

	DrawStringRect(&bottomScreen, str, &style.activityRect, &style.activityColor, &font24, style.activityAlign);

	if (gaugemax)
		progressInit(&bottomScreen, &style.gaugeRect, style.gaugeFrameColor, style.gaugeDoneColor, style.gaugeBackColor, &style.gaugeTextColor, &font24, gaugemax);

	DisplayScreen(&bottomScreen);
	DisplayScreen(&top1Screen);
//	DisplayScreen(&top2Screen);
}
#include <stdarg.h>
#include "fs.h"
#include "hid.h"
#include "progress.h"
#include "screenshot.h"
#include "theme.h"
#include "timer.h"
#include "lang.h"
#include "strings.h"

static Screen *progress_screen;
static Rect progress_rect;
static Color progress_frame;
static Color progress_done;
static Color progress_back;
static uint_fast8_t progress_fontsize;
static Color progress_textcolor;
static uintmax_t progress_posmax;
static uintmax_t progress_pos;
static uintmax_t progress_offset;
static wchar_t status_caption[_MAX_LFN + 1];
static wchar_t status_value[_MAX_LFN + 1];
static uint_fast8_t status_features;

void progressSetMax(uintmax_t posmax) {
	if (posmax != progress_posmax) {
		progress_posmax = posmax;
	}
}

void progressPinOffset() {
	progress_offset = progress_pos;
}

static void statusDrawStart() {
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
	} else {
		ClearScreen(&top1Screen, BLACK);
//		ClearScreen(&top2Screen, BLACK);
	}
	if (*style.activitybottomimg)
		DrawSplash(&bottomScreen, style.activitybottomimg);
	else if (*style.bottomimg)
		DrawSplash(&bottomScreen, style.bottomimg);
	else
		ClearScreen(&bottomScreen, BLACK);
	DrawStringRect(&bottomScreen, status_caption, &style.activityRect, style.activityColor, style.activityAlign, 30);
}

static void statusDrawFinish() {
	DisplayScreen(&bottomScreen);
	DisplayScreen(&top1Screen);
//	DisplayScreen(&top2Screen);
	TryScreenShot();
}

uint_fast8_t progressSetPos(uintmax_t pos) {
	statusDrawStart();
	uint32_t timeleft = 0;
	if ((pos += progress_offset) >= progress_posmax) {
		pos = progress_posmax;
		timerStop();
		wcscpy(status_value, lang(S_COMPLETED));
		status_features |= STATUS_FINISHED;
	}
	if (pos != progress_pos || !pos) {
		if ((progress_pos = pos) && pos) {
			timeleft = timerGet();
			timeleft = progress_posmax * timeleft / progress_pos - timeleft;
		}
		DrawProgress(progress_screen, &progress_rect, progress_frame, progress_done, progress_back, progress_textcolor, progress_fontsize, progress_posmax, progress_pos, timeleft);
	}
	if (pos != progress_posmax && status_features & STATUS_CANCELABLE && GetInput() & keys[KEY_B].mask) {
		wcscpy(status_value, lang(S_CANCELED));
		status_features |= STATUS_WAIT | STATUS_FINISHED | STATUS_FAILED;
	}
	if (status_features & STATUS_FINISHED) {
		wcscat(status_value, L" ");
		swprintf(status_value + wcslen(status_value), _MAX_LFN + 1, lang(SF2_PRESS_BUTTON_ACTION), lang(S_ANY_BUTTON), lang(S_CONTINUE));
	}
	DrawStringRect(&bottomScreen, status_value, &style.activityStatusRect, style.activityColor, style.activityStatusAlign, 16);
	statusDrawFinish();
	if (status_features & STATUS_WAIT && status_features & STATUS_FINISHED)
		InputWait();
	return !(status_features & STATUS_FAILED);
}

void statusInit(uintmax_t gaugemax, uint_fast8_t features, wchar_t *format, ...) {
	va_list va;
	va_start(va, format);
	vswprintf(status_caption, _MAX_LFN + 1, format, va);
	va_end(va);

	*status_value = 0;
	if (gaugemax) {
		progress_screen = &bottomScreen;
		progress_rect = style.gaugeRect;
		progress_frame = style.gaugeFrameColor;
		progress_done = style.gaugeDoneColor;
		progress_back = style.gaugeBackColor;
		progress_textcolor = style.gaugeTextColor;
		progress_fontsize = 30;
		progress_posmax = gaugemax;
		progress_offset = 0;
		progress_pos = -1;
		if ((status_features = features) & STATUS_CANCELABLE)
			swprintf(status_value, _MAX_LFN + 1, lang(SF2_PRESS_BUTTON_ACTION), lang(S_BUTTON_B), lang(S_CANCEL));
		timerStart();
		progressSetPos(0);
	} else {
		statusDrawStart();
		statusDrawFinish();
	}
}
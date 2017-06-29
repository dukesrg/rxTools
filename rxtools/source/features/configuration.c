/*
 * Copyright (C) 2015-2017 The PASTA Team, dukesrg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <reboot.h>
#include "configuration.h"
#include "lang.h"
#include "screenshot.h"
#include "fs.h"
#include "console.h"
#include "draw.h"
#include "hid.h"
#include "mpcore.h"
#include "ncch.h"
#include "firm.h"
#include "menu.h"
#include "jsmn/jsmn.h"
#include "progress.h"
#include "strings.h"
#include "aes.h"
#include "ticket.h"

static char cfgLang[CFG_STR_MAX_LEN] = "en";
static char cfgTheme[CFG_STR_MAX_LEN] = "";

Cfg cfgs[] = {
	[CFG_BOOT_DEFAULT] = { "CFG_BOOT_DEFAULT", CFG_TYPE_INT, { .i = BOOT_UI } },
	[CFG_GUI_FORCE] = { "CFG_GUI_FORCE", CFG_TYPE_INT, { .i = KEY_L } },
	[CFG_EMUNAND_FORCE] = { "CFG_EMUNAND_FORCE", CFG_TYPE_INT, { .i = KEY_Y } },
	[CFG_SYSNAND_FORCE] = { "CFG_SYSNAND_FORCE", CFG_TYPE_INT, { .i = KEY_X } },
	[CFG_PASTA_FORCE] = { "CFG_PASTA_FORCE", CFG_TYPE_INT, { .i = KEY_B } },
	[CFG_THEME] = { "CFG_THEME", CFG_TYPE_STRING, { .s = cfgTheme } },
	[CFG_AGB_BIOS] = { "CFG_AGB_BIOS", CFG_TYPE_BOOLEAN, { .i = 0 } },
	[CFG_LANGUAGE] = { "CFG_LANGUAGE", CFG_TYPE_STRING, { .s = cfgLang } },
	[CFG_SYSNAND_WRITE_PROTECT] = { "CFG_SYSNAND_WRITE_PROTECT", CFG_TYPE_BOOLEAN, { .i = 1 } }
};

static const wchar_t *jsonPath= L"/rxTools/data/system.json";

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start && strncmp(json + tok->start, s, tok->end - tok->start) == 0)
		return 0;
	return -1;
}

int writeCfg()
{
	File fd;
	char buf[0x400];
	const char *p;
	char *jsonCur;
	unsigned int i;
	size_t len;
	int left, res;

	left = sizeof(buf);
	jsonCur = buf;

	*jsonCur = '{';

	left--;
	jsonCur++;

	i = 0;
	for (i = 0; i < CFG_NUM; i++) {
		if (i > 0) {
			if (left < 1)
				return 1;

			*jsonCur = ',';
			left--;
			jsonCur++;
		}

		res = snprintf(jsonCur, left, "\n\t\"%s\": ", cfgs[i].key);
		if (res < 0 || res >= left)
			return 1;

		left -= res;
		jsonCur += res;

		switch (cfgs[i].type) {
			case CFG_TYPE_INT:
				res = snprintf(jsonCur, left, "%d", cfgs[i].val.i);
				if (res < 0 || res >= left)
					return 1;

				len = res;
				break;

			case CFG_TYPE_BOOLEAN:
				if (cfgs[i].val.i) {
					len = sizeof("true");
					p = "true";
				} else {
					len = sizeof("false");
					p = "false";
				}

				if (len >= left)
					return -1;

				strcpy(jsonCur, p);
				len--;
				break;

			case CFG_TYPE_STRING:
				res = snprintf(jsonCur, left, "\"%s\"", cfgs[i].val.s);
				if (res < 0 || res >= left)
					return 1;

				len = res;
				break;

			default:
				return -1;
		}

		left -= len;
		jsonCur += len;
	}

	left -= 3;
	if (left < 0)
		return 1;

	*jsonCur = '\n';
	jsonCur++;
	*jsonCur = '}';
	jsonCur++;
	*jsonCur = '\n';
	jsonCur++;

	if (!FileOpen(&fd, jsonPath, 1))
		return 1;

	FileWrite(&fd, buf, (uintptr_t)jsonCur - (uintptr_t)buf, 0);
	FileClose(&fd);

	return 0;
}

int readCfg()
{
	const size_t tokenNum = 1 + CFG_NUM * 2;
	jsmntok_t t[tokenNum];
	char buf[256];
	jsmn_parser parser;
	File fd;
	unsigned int i, j;
	int r;
	size_t len;

	if (!FileOpen(&fd, jsonPath, 0))
		return 1;

	len = FileGetSize(&fd);
	if (len > sizeof(buf))
		return 1;

	FileRead(&fd, buf, len, 0);
	FileClose(&fd);

	jsmn_init(&parser);
	r = jsmn_parse(&parser, buf, len, t, tokenNum);
	if (r < 0)
		return r;

	if (r < 1)
		return 1;

	/* Loop over all keys of the root object */
	for (i = 1; i < r; i++) {
		for (j = 0; jsoneq(buf, &t[i], cfgs[j].key) != 0; j++)
			if (j >= CFG_NUM)
				return 1;

		i++;
		switch (cfgs[j].type) {
			case CFG_TYPE_INT:
				cfgs[j].val.i = strtoul(buf + t[i].start, NULL, 0);
				break;

			case CFG_TYPE_BOOLEAN:
				len = t[i].end - t[i].start;
				cfgs[j].val.i = buf[t[i].start] == 't';

				break;

			case CFG_TYPE_STRING:
				len = t[i].end - t[i].start;

				if (len + 1 > CFG_STR_MAX_LEN)
					break;

#ifdef DEBUG
				if (cfgs[j].val.s == NULL)
					break;
#endif

				
				memcpy(cfgs[j].val.s, buf + t[i].start, len);
				cfgs[j].val.s[len] = 0;
		}
	}

	return 0;
}
void InstallConfigData() {
	if (!FileExists(jsonPath))
		writeCfg();
}

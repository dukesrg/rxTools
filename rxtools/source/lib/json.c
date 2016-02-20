/*
 * Copyright (C) 2015 The PASTA Team
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

#include "json.h"

int jsonLoad(Json *json, const TCHAR *path) {
	jsmn_parser p;
	size_t len;
	File fd;

	if (FileOpen(&fd, path, 0) && (len = FileGetSize(&fd)) <= json->len) {
		json->len = len;
		FileRead(&fd, json->js, json->len, 0);
		FileClose(&fd);
		jsmn_init(&p);
		json->count = jsmn_parse(&p, json->js, json->len, json->tok, json->count);
	} else
		json->count = 0;
	return json->count;
}

/*
 * Copyright (C) 2016 dukesrg
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

#include "fs.h"
#include "json.h"

uint32_t jsonLoad(Json *json, const wchar_t *path) {
	jsmn_parser p;
	File f;

	if (!FileOpen(&f, path, 0) || f.fsize > json->len || (
		FileRead2(&f, json->js, f.fsize) != f.fsize &&
		(FileClose(&f) || 1)
	)) return json->count = 0;
	FileClose(&f);
	jsmn_init(&p);
	return json->count = jsmn_parse(&p, json->js, json->len = f.fsize, json->tok, json->count);
}
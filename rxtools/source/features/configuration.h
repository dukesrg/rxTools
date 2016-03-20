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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define CFG_STR_MAX_LEN 32

typedef enum {
	CFG_TYPE_INT,
	CFG_TYPE_BOOLEAN,
	CFG_TYPE_STRING
} CfgType;

typedef struct {
	const char *key;
	CfgType type;
	union {
		int i;
		int b;
		char *s;
	} val;
} Cfg;

enum {
	CFG_BOOT_DEFAULT,
	CFG_GUI_FORCE,
	CFG_EMUNAND_FORCE,
	CFG_SYSNAND_FORCE,
	CFG_PASTA_FORCE,
	CFG_THEME,
	CFG_AGB_BIOS,
	CFG_LANGUAGE,

	CFG_NUM
};

enum {
	BOOT_UI,
	BOOT_EMUNAND,
	BOOT_SYSNAND,
	BOOT_PASTA,
	BOOT_COUNT
};

extern Cfg cfgs[];

void InstallConfigData();

int writeCfg();
int readCfg();

#endif

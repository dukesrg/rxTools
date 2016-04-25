#ifndef STRINGS_H
#define STRINGS_H

#define STR(X, Y) static const char *const Y = X;

STR("Enabled", S_ENABLED)
STR("Disabled", S_DISABLED)
STR("|", S_PARENT_SEPARATOR)

STR("Japan", S_JAPAN)
STR("North America", S_NORTH_AMERICA)
STR("Europe/Australia", S_EUROPE_AUSTRALIA)
STR("China", S_CHINA)
STR("South Korea", S_SOUTH_KOREA)
STR("Taiwan", S_TAIWAN)
STR("Unknown", S_UNKNOWN)

//STR("Booting %ls...", SF_BOOTING)
STR("rxTools GUI", S_RXTOOLS_GUI)
STR("rxMode SysNAND", S_RXTOOLS_SYSNAND)
STR("rxMode EmuNAND", S_RXTOOLS_EMUNAND)
STR("Pasta mode", S_PASTA_MODE)

STR("Generating %ls", SF_GENERATING)
STR("SD XORpad", S_SD_XORPAD)
STR("NCCH XORpad", S_NCCH_XORPAD)
STR("FAT16 XORpad", S_FAT16_XORPAD)

//STR("Error reading %ls!", SF_ERROR_READING)
STR("Failed to %ls %ls!", SF_FAILED_TO)
STR("mount", S_MOUNT) //don't need a translation
//STR("read", S_READ)
//STR("write", S_WRITE)
STR("load", S_LOAD)
//STR("save", S_SAVE)
STR("file system", S_FILE_SYSTEM) //don't need a translation
//STR("font", S_FONT)
//STR("theme", S_THEME)
//STR("translation", S_TRANSLATION)
//STR("GUI", S_GUI)
STR("This file with a valid key must exist in the SD card root in order to boot rxMode on New 3DS.", S_NO_KTR_KEYS)
STR("This file with a valid key must exist in the SD card root in order to decrypt newer titles on firmware version below 7.0.0 and to use newer gamecard saves on firmware below 6.0.0.", S_NO_KEYX25)
STR("Press %ls to %ls.", SF_PRESS_BUTTON_ACTION)
STR("any key", S_ANY_KEY)
STR("continue", S_CONTINUE)
STR("reboot", S_REBOOT)

#endif

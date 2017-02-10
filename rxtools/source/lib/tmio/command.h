/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2016 173210
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2, as described below:
 *
 * This file is free software: you may copy, redistribute and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#ifndef COMMAND_H
#define COMMAND_H

enum {
	// Class 1
	MMC_GO_IDLE_STATE = 0,
	MMC_SEND_OP_COND = 1,
	MMC_ALL_SEND_CID = 2,
	MMC_SET_RELATIVE_ADDR = 3,
	MMC_SWITCH = 6,
	MMC_SELECT_CARD = 7,
	MMC_SEND_CSD = 9,
	MMC_SEND_CID = 10,
	MMC_SEND_STATUS = 13,

	// Class 2
	MMC_SET_BLOCKLEN = 16,
	MMC_READ_SINGLE_BLOCK = 17,
	MMC_READ_MULTIPLE_BLOCK = 18,

	// Class 4
	MMC_SET_BLOCK_COUNT = 23,
	MMC_WRITE_BLOCK = 24,
	MMC_WRITE_MULTIPLE_BLOCK = 25,

	// Class 8
	MMC_APP_CMD = 55
};

enum {
	// Class 0
	SD_SEND_IF_COND = 8,

	// Application command
	SD_APP_SET_BUS_WIDTH = 6,
	SD_APP_OP_COND = 41
};

#define MMC_READY	0x80000000
#define MMC_RCA_MASK	0xFFFF0000

//#define MMC_OCR_170_195	0x00000080
#define MMC_OCR_27_28	0x00008000
#define MMC_OCR_28_29	0x00010000
#define MMC_OCR_29_30	0x00020000
#define MMC_OCR_30_31	0x00040000
#define MMC_OCR_31_32	0x00080000
#define MMC_OCR_32_33	0x00100000
#define MMC_OCR_33_34	0x00200000
#define MMC_OCR_34_35	0x00400000
#define MMC_OCR_35_36	0x00800000
#define MMC_OCR_27_36	(MMC_OCR_27_28 | MMC_OCR_28_29 | MMC_OCR_29_30 | MMC_OCR_30_31 | MMC_OCR_31_32 | MMC_OCR_32_33 | MMC_OCR_33_34 | MMC_OCR_34_35 | MMC_OCR_35_36)

#define MMC_EXT_CSD_ACCESS_MODE_COMMAND_SET	0x00000000
#define MMC_EXT_CSD_ACCESS_MODE_SET_BITS	0x01000000
#define MMC_EXT_CSD_ACCESS_MODE_CLEAR_BITS	0x02000000
#define MMC_EXT_CSD_ACCESS_MODE_WRITE_BYTE	0x03000000

#define MMC_EXT_CSD_BUS_WIDTH	0x00B70000
#define MMC_BUS_WIDTH_1	0x00000000
#define MMC_BUS_WIDTH_4	0x00000100
#define MMC_BUS_WIDTH_8	0x00000200
#define MMC_BUS_WIDTH_4_DDR	0x00000500
#define MMC_BUS_WIDTH_8_DDR	0x00000600

#define MMC_EXT_CSD_HS_TIMING	0x00B90000
#define MMC_HS_TIMING	0x00000100

#define SD_VHS_27_36	0x00000100
//#define SD_VHS_LV	0x00000200
#define SD_CHECK_PATTERN	0x000000AA

#define SD_BUS_WIDTH_1	0x00000000
#define SD_BUS_WIDTH_4	0x00000002

#define SD_HCS_MASK	0x40000000
#define SD_HCS_SDSC	0
#define SD_HCS_SDHC	SD_HCS_MASK

#endif

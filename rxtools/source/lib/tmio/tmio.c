/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2014-2015, Normmatt, 173210
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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include "command.h"
#include "tmio.h"
#include "tmio_hardware.h"

#include "configuration.h"
#include "irq.h"

#include "draw.h"
#include "lang.h"

#define DATA32_SUPPORT

void waitcycles(uint32_t val);

_Static_assert(TMIO_DEV_NUM == 2, "TMIO device numer doesn't accord with the driver context.");

static tmio_device tmio_dev[] = {
	{SD_RCA_DEFAULT, TMIO_CLK_DIV_128, 9, TMIO_OPT_BUS_WIDTH_1, {}, {}},
	{MMC_RCA_DEFAULT, TMIO_CLK_DIV_128, 9, TMIO_OPT_BUS_WIDTH_1, {}, {}}
};

static uint_fast8_t waitDataend = 0;

static void setckl(uint_fast16_t data){
	REG_MMC_CLKCTL &= ~TMIO_CLK_ENABLE;
	REG_MMC_CLKCTL = (REG_MMC_CLKCTL & ~0x02FF) | (data & 0x02FF);
	REG_MMC_CLKCTL |= TMIO_CLK_ENABLE;
}

static inline uint_fast8_t tmio_wfi() {
	IRQ_ACK(IRQ_SDIO_1);
	IRQ_WAIT();
	return 1;
}

static void inittarget(enum tmio_dev_id target) {
	uint32_t status;

	if (waitDataend) {
		REG_MMC_IRQ_MASK = ~(TMIO_STAT_DATAEND | TMIO_MASK_GW);
		do {
			tmio_wfi();
			REG_MMC_IRQ_STATUS = ~TMIO_STAT_DATAEND;
		} while (
			!((status = REG_MMC_IRQ_STATUS) & TMIO_MASK_GW) &&
			(status & TMIO_STAT_CMD_BUSY)
		);
		waitDataend = 0;
	}

	REG_MMC_PORTSEL = (REG_MMC_PORTSEL & ~0x0003) | target;
	setckl(tmio_dev[target].clk);
	REG_MMC_OPT = (REG_MMC_OPT & TMIO_OPT_BUS_WIDTH_MASK) | tmio_dev[target].OPT;
}

static uint32_t tmio_wait_respend() {
	uint32_t status, error = 0;

	REG_MMC_IRQ_MASK = ~(TMIO_STAT_CMDRESPEND | TMIO_MASK_GW);
	while (
		tmio_wfi() &&
		!(error = (status = REG_MMC_IRQ_STATUS) & TMIO_MASK_GW) &&
		!(status & TMIO_STAT_CMDRESPEND)
	);

	return error;
}

static uint32_t tmio_send_command(uint16_t cmd, uint32_t args, uint_fast8_t cap_prev_error) {
	uint32_t error;

	if (
		(REG_MMC_IRQ_STATUS & TMIO_STAT_CMD_BUSY) &&
		(error = tmio_wait_respend()) &&
		cap_prev_error
	) return error;

	REG_MMC_IRQ_STATUS = 0;
	REG_MMC_CMDARG = args;
	REG_MMC_CMD = cmd;

	return 0;
}

uint32_t tmio_readsectors(enum tmio_dev_id target, uint32_t sector_no, uint_fast16_t numsectors, uint8_t *out) {
	uint32_t error = 0;
	do {
	uint_fast16_t count = numsectors;
	uint16_t *dataPtr = (uint16_t*)out;
	inittarget(target);
	REG_MMC_STOP = TMIO_STOP_AUTO;
#ifdef DATA32_SUPPORT
	uint32_t *dataPtr32 = (uint32_t*)out;
	REG_MMC_BLKCOUNT32 = count;
	REG_MMC_BLKLEN32 = TMIO_BBS;
	REG_MMC_BLKCOUNT = count;
	REG_MMC_DATACTL32 = TMIO32_ENABLE | TMIO32_IRQ_RXRDY;
	REG_MMC_IRQ_MASK = ~TMIO_MASK_GW;
#else
	REG_MMC_BLKCOUNT = count;
	REG_MMC_IRQ_MASK = ~(TMIO_MASK_GW | TMIO_STAT_RXRDY);
#endif

	tmio_send_command((count > 1 ? (MMC_READ_MULTIPLE_BLOCK | TMIO_CMD_TRANSFER_MULTI) : MMC_READ_SINGLE_BLOCK) | TMIO_CMD_RESP_R1 | TMIO_CMD_DATA_PRESENT | TMIO_CMD_TRANSFER_READ, sector_no << tmio_dev[target].addr_mul, 0);

#ifdef DATA32_SUPPORT
	if (!((uint32_t)dataPtr & 3)) {
		while (
			count &&
			tmio_wfi() &&
			!(error = REG_MMC_IRQ_STATUS & TMIO_MASK_GW)
		) {
			if (REG_MMC_DATACTL32 & TMIO32_STAT_RXRDY) {
				for (size_t i = TMIO_BBS; i; i -= sizeof(dataPtr32[0]), *dataPtr32++ = REG_MMC_FIFO32);
				count--;
			}
		}
	} else {
#endif
		while (
			count &&
			tmio_wfi() &&
			!(error = REG_MMC_IRQ_STATUS & TMIO_MASK_GW)
		) {
#ifdef DATA32_SUPPORT
			if (REG_MMC_DATACTL32 & TMIO32_STAT_RXRDY) {
#endif
				for (size_t i = TMIO_BBS; i; i -= sizeof(dataPtr[0]), *dataPtr++ = REG_MMC_FIFO);
				count--;
			}
#ifdef DATA32_SUPPORT
		}
	}
#endif
	if (error) {
//		tmio_send_command(MMC_STOP_TRANSMISSION | TMIO_CMD_RESP_R1B, 0, 0);
//		tmio_send_command(MMC_SELECT_CARD | TMIO_CMD_RESP_R1, 0, 0);
//		tmio_send_command(MMC_SELECT_CARD | TMIO_CMD_RESP_R1, dev->RCA, 0);

//		char tmpstr[32];
//		sprintf(tmpstr, "%08lX %08lX", REG_MMC_IRQ_STATUS, REG_MMC_ERROR_DETAIL);
//		DrawStringRect(&top1Screen, lang(tmpstr), &(Rect){0, 0, 400, 32}, BLUE, ALIGN_LEFT, 16);
//		DisplayScreen(&top1Screen);
//		while(1);
//		if (target == TMIO_DEV_SDMC) {
//			tmio_dev[TMIO_DEV_SDMC] = (tmio_device){0, TMIO_CLK_DIV_512, 9, TMIO_OPT_BUS_WIDTH_1, {}, {}};
//			tmio_init_dev(TMIO_DEV_SDMC);
//		}
		tmio_init();
		tmio_init_dev(TMIO_DEV_NAND);
		tmio_init_dev(TMIO_DEV_SDMC);
	}
	} while (error);
	return error;
}

uint32_t tmio_writesectors(enum tmio_dev_id target, uint32_t sector_no, uint_fast16_t numsectors, uint8_t *in) {
	uint32_t error = 0;
	if (target == TMIO_DEV_NAND && cfgs[CFG_SYSNAND_WRITE_PROTECT].val.i)
		return 0;
	do {
	uint_fast16_t count = numsectors;
	inittarget(target);
	REG_MMC_STOP = TMIO_STOP_AUTO;
#ifdef DATA32_SUPPORT
	uint32_t *dataPtr = (uint32_t*)in;
	volatile uint32_t *fifo = &REG_MMC_FIFO32;
	REG_MMC_BLKCOUNT32 = count;
	REG_MMC_BLKLEN32 = TMIO_BBS;
	REG_MMC_BLKCOUNT = count;
	REG_MMC_DATACTL32 = TMIO32_ENABLE | TMIO32_IRQ_TXRQ;
	REG_MMC_IRQ_MASK = ~TMIO_MASK_GW;
#else
	uint16_t *dataPtr = (uint16_t*)in;
	volatile uint32_t *fifo = &REG_MMC_FIFO;
	REG_MMC_BLKCOUNT = count;
	REG_MMC_IRQ_MASK = ~(TMIO_MASK_GW | TMIO_STAT_TXRQ);
#endif

	tmio_send_command((count > 1 ? (MMC_WRITE_MULTIPLE_BLOCK | TMIO_CMD_TRANSFER_MULTI) : MMC_WRITE_BLOCK) | TMIO_CMD_RESP_R1 | TMIO_CMD_DATA_PRESENT, sector_no << tmio_dev[target].addr_mul, 0);

	while (
		count &&
		tmio_wfi() &&
		!(error = REG_MMC_IRQ_STATUS & TMIO_MASK_GW)
	)
#ifdef DATA32_SUPPORT
		if (!(REG_MMC_DATACTL32 & TMIO32_STAT_BUSY)) {
#endif
			for (size_t i = TMIO_BBS; i; i -= sizeof(dataPtr[0]), *fifo = *dataPtr++);
			count--;
#ifdef DATA32_SUPPORT
		}
#endif
	waitDataend = 1;
	if (error) {
//		tmio_send_command(MMC_STOP_TRANSMISSION | TMIO_CMD_RESP_R1B, 0, 0);
//		tmio_send_command(MMC_SELECT_CARD | TMIO_CMD_RESP_R1, 0, 0);
//		tmio_send_command(MMC_SELECT_CARD | TMIO_CMD_RESP_R1, dev->RCA, 0);
//		char tmpstr[32];
//		sprintf(tmpstr, "%08lX %08lX", REG_MMC_IRQ_STATUS, REG_MMC_ERROR_DETAIL);
//		DrawStringRect(&top1Screen, lang(tmpstr), &(Rect){0, 0, 400, 32}, RED, ALIGN_LEFT, 16);
//		DisplayScreen(&top1Screen);
//		while(1);
		tmio_init();
		tmio_init_dev(TMIO_DEV_NAND);
		tmio_init_dev(TMIO_DEV_SDMC);
	}
	} while (error);
	return error;
}

void tmio_init() {
	IRQ_ENABLE(IRQ_SDIO_1);

	REG_MMC_DATACTL32 = (REG_MMC_DATACTL32 & ~(TMIO32_IRQ_RXRDY | TMIO32_IRQ_TXRQ)) | TMIO32_ENABLE | TMIO32_ABORT;
	REG_MMC_DATACTL = (REG_MMC_DATACTL & ~0x2200) | TMIO32_ENABLE;
#ifdef DATA32_SUPPORT
	REG_MMC_DATACTL &= ~0x0020;
	REG_MMC_BLKLEN32 = TMIO_BBS;
#else
	REG_MMC_DATACTL32 &= 0x0020;
	REG_MMC_DATACTL &= 0x0020;
	REG_MMC_BLKLEN32 = 0;
#endif
	REG_MMC_BLKCOUNT32 = 1;
	REG_MMC_RESET &= ~TMIO_RESET;
	REG_MMC_RESET |= TMIO_RESET;
	REG_MMC_IRQ_MASK = ~TMIO_MASK_ALL;
	REG_MMC_FC |= 0x00DB;
	REG_MMC_FE |= 0x00DB;
	REG_MMC_PORTSEL &= ~0x0002;
#ifdef DATA32_SUPPORT
	REG_MMC_CLKCTL = TMIO_CLK_DIV_128;
	REG_MMC_OPT = 0x40EE;
#else
	REG_MMC_CLKCTL = TMIO_CLK_DIV_256;
	REG_MMC_OPT = 0x40EB;
#endif
	REG_MMC_PORTSEL &= ~0x0002;
	REG_MMC_BLKLEN = TMIO_BBS;
	REG_MMC_STOP = 0;
	
	inittarget(TMIO_DEV_SDMC);
}

uint32_t tmio_init_dev(enum tmio_dev_id target) {
	uint32_t error;
	tmio_device *dev = &tmio_dev[target];
//	if (dev->CSD.sd1.CCC) //Already initialised if card class defined in CSD for both SD and eMMC
//		return 0;

	inittarget(target);
	waitcycles(0xF000);
	tmio_send_command(MMC_GO_IDLE_STATE, 0, 0);
	
	if (target == TMIO_DEV_NAND) {
		*dev = (tmio_device){MMC_RCA_DEFAULT, TMIO_CLK_DIV_128, 9, TMIO_OPT_BUS_WIDTH_1, {}, {}};
		while (
			tmio_send_command(MMC_SEND_OP_COND | TMIO_CMD_RESP_R3, MMC_OCR_32_33, 0) ||
			tmio_wait_respend() ||
			!(REG_MMC_RESP0 & MMC_READY)
		);
	} else {
		*dev = (tmio_device){SD_RCA_DEFAULT, TMIO_CLK_DIV_128, 9, TMIO_OPT_BUS_WIDTH_1, {}, {}};
		if ((error = tmio_send_command(SD_SEND_IF_COND | TMIO_CMD_RESP_R1, SD_VHS_27_36 | SD_CHECK_PATTERN, 1)))
			return error;

		uint32_t resp;
		uint32_t hcs = tmio_wait_respend() ? SD_HCS_SDSC : SD_HCS_SDHC;
		while (
			tmio_send_command(MMC_APP_CMD | TMIO_CMD_RESP_R1, dev->RCA, 0) ||
			tmio_send_command(SD_APP_OP_COND | TMIO_CMD_APP | TMIO_CMD_RESP_R3, MMC_OCR_27_36 | hcs, 1) ||
			tmio_wait_respend() ||
			!((resp = REG_MMC_RESP0) & MMC_READY)
		);
		if (resp & hcs & SD_HCS_MASK)
			dev->addr_mul = 0;
	}

	if (
		(error = tmio_send_command(MMC_ALL_SEND_CID | TMIO_CMD_RESP_R2, 0, 0)) ||
		(error = tmio_wait_respend())
	) return error;
	dev->CID = REG_MMC_RESP.CID;

	if (
		(error = tmio_send_command(MMC_SET_RELATIVE_ADDR | TMIO_CMD_RESP_R1, dev->RCA, 1)) ||
		(error = tmio_wait_respend())
	) return error;
	if (target == TMIO_DEV_SDMC)
		dev->RCA = REG_MMC_RESP0 & MMC_RCA_MASK;
	
	if (
		(error = tmio_send_command(MMC_SEND_CSD | TMIO_CMD_RESP_R2, dev->RCA, target != TMIO_DEV_NAND)) ||
		(error = tmio_wait_respend())
	) return error;
	dev->CSD = REG_MMC_RESP.CSD;
	setckl(dev->clk = TMIO_CLK_DIV_2);
	
	tmio_send_command(MMC_SELECT_CARD | TMIO_CMD_RESP_R1, dev->RCA, 0);

	if (
		(target == TMIO_DEV_NAND && (
			(error = tmio_send_command(MMC_SWITCH | TMIO_CMD_RESP_R1B, MMC_EXT_CSD_ACCESS_MODE_WRITE_BYTE | MMC_EXT_CSD_BUS_WIDTH | MMC_BUS_WIDTH_4, 1)) ||
			(error = tmio_send_command(MMC_SWITCH | TMIO_CMD_RESP_R1B, MMC_EXT_CSD_ACCESS_MODE_WRITE_BYTE | MMC_EXT_CSD_HS_TIMING | MMC_HS_TIMING, 1))
		)) || (
			target == TMIO_DEV_SDMC && (
			(error = tmio_send_command(MMC_APP_CMD | TMIO_CMD_RESP_R1, dev->RCA, 1)) ||
			(error = tmio_send_command(SD_APP_SET_BUS_WIDTH | TMIO_CMD_APP | TMIO_CMD_RESP_R1, SD_BUS_WIDTH_4, 1))
		)) ||
		(error = tmio_send_command(MMC_SEND_STATUS | TMIO_CMD_RESP_R1, dev->RCA, 1)) ||
		(error = tmio_send_command(MMC_SET_BLOCKLEN | TMIO_CMD_RESP_R1, TMIO_BBS, 1))
	) return error;
	dev->OPT = TMIO_OPT_BUS_WIDTH_4;
	
	dev->clk |= 0x200;
	if (target == TMIO_DEV_NAND)
		inittarget(TMIO_DEV_SDMC);
	    
	return 0;
}
/*
uint32_t tmio_wrprotected(enum tmio_dev_id target) {
	inittarget(target);
	return REG_MMC_IRQ_STATUS & TMIO_STAT_WRPROTECT;
}
*/
uint32_t tmio_get_size(enum tmio_dev_id target) {
	card_csd *csd = &tmio_dev[target].CSD;
	switch(csd->sd1.CSD_STRUCTURE) {
		case CSD_STRUCTURE_EMMC4:
			if (csd->emmc.SPEC_VERS != SPEC_VERS_EMMC4)
				return 0;
		case CSD_STRUCTURE_SD1:
			return (csd->sd1.C_SIZE_L + (csd->sd1.C_SIZE_H << 2) + 1) << (csd->sd1.C_SIZE_MULT + csd->sd1.READ_BL_LEN - 7); //2 from mult, -9 from sector size=512
		case CSD_STRUCTURE_SD2:
			return (csd->sd2.C_SIZE + 1) << 10;
	}
	return 0;
}

card_cid *tmio_get_cid(enum tmio_dev_id target) {
	return &tmio_dev[target].CID;
}

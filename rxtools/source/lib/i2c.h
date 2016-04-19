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

#ifndef I2C_H
#define I2C_H

#include <stddef.h>
#include <stdint.h>

#define I2C1_REG_OFF 0x10161000
#define I2C2_REG_OFF 0x10144000
#define I2C3_REG_OFF 0x10148000

#define I2C_REG_DATA  0
#define I2C_REG_CNT   1
#define I2C_REG_CNTEX 2
#define I2C_REG_SCL   4

#define I2C_DEV_MCU  3
#define I2C_DEV_GYRO 10
#define I2C_DEV_IR   13

const uint_fast8_t i2cGetDeviceBusId(uint_fast8_t device_id);
const uint_fast8_t i2cGetDeviceRegAddr(uint_fast8_t device_id);

volatile uint8_t* const i2cGetDataReg(uint_fast8_t bus_id);
volatile uint8_t* const i2cGetCntReg(uint_fast8_t bus_id);

void i2cWaitBusy(uint_fast8_t bus_id);
uint_fast8_t i2cGetResult(uint_fast8_t bus_id);
uint_fast8_t i2cGetData(uint_fast8_t bus_id);
void i2cStop(uint_fast8_t bus_id, uint_fast8_t arg0);

uint_fast8_t i2cSelectDevice(uint_fast8_t bus_id, uint_fast8_t dev_reg);
uint_fast8_t i2cSelectRegister(uint_fast8_t bus_id, uint_fast8_t reg);

uint_fast8_t i2cReadRegister(uint_fast8_t dev_id, uint_fast8_t reg);
uint_fast8_t i2cWriteRegister(uint_fast8_t dev_id, uint_fast8_t reg, uint_fast8_t data);

uint_fast8_t i2cReadRegisterBuffer(uint_fast8_t dev_id, uint_fast8_t reg, uint8_t* buffer, size_t buf_size);

#endif
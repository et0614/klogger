/**
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/*
 * MPLAB X / MCC Melody 移植版
 * TWI1の直接レジスタ操作を MCC生成 I2Cホストドライバ(TWI1_*)に置き換えた。
 * ドライバはポーリングモードのため、転送開始後は TWI1_Tasks() を回して完了を待つ。
 */

#include "vl53l1_platform.h"
#include "VL53L1X_api.h"
#include <string.h>
#include <stdbool.h>

#include "../mcc_generated_files/i2c_host/twi1.h"
#include "../mcc_generated_files/timer/delay.h"

//VL53L1CBの7bitアドレス
static const uint8_t ADDRESS = 0x29;

//WriteMultiの最大ペイロード（ULD APIはWrDWordの4バイトが最大）
#define WRITE_PAYLOAD_MAX 8

/**
 * @brief 開始済みのI2C転送の完了を待つ
 * @return true:正常完了, false:エラー（NACK/バスエラー等）
 */
static bool i2c_wait_done(void)
{
	while (TWI1_IsBusy()) {
		TWI1_Tasks();
	}
	return (TWI1_ErrorGet() == I2C_ERROR_NONE);
}

int8_t VL53L1_WriteMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
	(void)dev;

	if (count > WRITE_PAYLOAD_MAX) return -1;

	//先頭2バイトに16bitレジスタアドレス(MSB first)、続けてデータを詰める
	uint8_t buffer[2 + WRITE_PAYLOAD_MAX];
	buffer[0] = (uint8_t)(index >> 8);
	buffer[1] = (uint8_t)(index & 0x00FF);
	memcpy(&buffer[2], pdata, count);

	if (!TWI1_Write(ADDRESS, buffer, 2 + count)) return -1; //ドライバビジー
	if (!i2c_wait_done()) return -1;

	return 0;
}

int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count){
	(void)dev;

	if (count == 0) return 0;

	//レジスタアドレス書き込み→Repeated Start→読み込み
	uint8_t indexBuffer[2];
	indexBuffer[0] = (uint8_t)(index >> 8);
	indexBuffer[1] = (uint8_t)(index & 0x00FF);

	if (!TWI1_WriteRead(ADDRESS, indexBuffer, 2, pdata, count)) return -1;
	if (!i2c_wait_done()) return -1;

	return 0;
}

int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data) {
	uint8_t  buffer[1];

	buffer[0] = (uint8_t)(data);

	return VL53L1_WriteMulti(dev, index, buffer, 1);
}

int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data) {
	uint8_t  buffer[2];

	// Split 16-bit word into MS and LS uint8_t
	buffer[0] = (uint8_t)(data >> 8);
	buffer[1] = (uint8_t)(data & 0x00FF);

	return VL53L1_WriteMulti(dev, index, buffer, 2);
}

int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data) {
	uint8_t  buffer[4];

	// Split 32-bit word into MS ... LS bytes
	buffer[0] = (uint8_t) (data >> 24);
	buffer[1] = (uint8_t)((data &  0x00FF0000) >> 16);
	buffer[2] = (uint8_t)((data &  0x0000FF00) >> 8);
	buffer[3] = (uint8_t) (data &  0x000000FF);

	return VL53L1_WriteMulti(dev, index, buffer, 4);
}

int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *data) {
	uint8_t  buffer[1];

	VL53L1X_ERROR status = VL53L1_ReadMulti(dev, index, buffer, 1);

	*data = buffer[0];

	return status;
}

int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *data) {
	uint8_t  buffer[2];

	VL53L1X_ERROR status = VL53L1_ReadMulti(
		dev,
		index,
		buffer,
		2);

	*data = (uint16_t)(((uint16_t)(buffer[0])<<8) + (uint16_t)buffer[1]);

	return status;
}

int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data) {
	uint8_t  buffer[4];

	VL53L1X_ERROR status = VL53L1_ReadMulti(
		dev,
		index,
		buffer,
		4);

	*data = ((uint32_t)buffer[0]<<24) + ((uint32_t)buffer[1]<<16) + ((uint32_t)buffer[2]<<8) + (uint32_t)buffer[3];

	return status;
}

int8_t VL53L1_WaitUs(uint16_t dev, int32_t wait_us)
{
	(void)dev;

	while (wait_us > 0) {
		uint16_t chunk = (wait_us > 60000L) ? 60000U : (uint16_t)wait_us;
		DELAY_microseconds(chunk);
		wait_us -= chunk;
	}

	return 0;
}

int8_t VL53L1_WaitMs(uint16_t dev, int32_t wait_ms){
	(void)dev;

	while (wait_ms > 0) {
		uint16_t chunk = (wait_ms > 60000L) ? 60000U : (uint16_t)wait_ms;
		DELAY_milliseconds(chunk);
		wait_ms -= chunk;
	}

	return 0;
}

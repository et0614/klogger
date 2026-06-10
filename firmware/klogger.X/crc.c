/*
 * crc.c
 *
 * CRC計算ユーティリティ（Crc.cppのC言語移植版）
 * Author: e.togashi
 */

#include "crc.h"

uint8_t crc_crc8(const uint8_t *ptr, uint8_t len)
{
	uint8_t crc = 0xFF;
	for(uint8_t i = 0; i < len; i++) {
		crc ^= *ptr++;
		for(uint8_t bit = 8; bit > 0; --bit) {
			if(crc & 0x80) {
				crc = (uint8_t)((crc << 1) ^ 0x31u);
			} else {
				crc = (uint8_t)(crc << 1);
			}
		}
	}
	return crc;
}

uint16_t crc_crc16(const uint8_t *ptr, uint8_t len)
{
	uint16_t crc = 0xFFFF;
	const uint16_t poly = 0x1021;

	for (uint8_t i = 0; i < len; i++) {
		crc ^= (uint16_t)(*ptr++) << 8;
		for (uint8_t j = 0; j < 8; j++) {
			if (crc & 0x8000) {
				crc = (uint16_t)((crc << 1) ^ poly);
			} else {
				crc = (uint16_t)(crc << 1);
			}
		}
	}
	return crc;
}

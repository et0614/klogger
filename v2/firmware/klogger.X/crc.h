/*
 * crc.h
 *
 * CRC計算ユーティリティ（Crc.h/Crc.cppのC言語移植版）
 * Author: e.togashi
 */

#ifndef CRC_H_
#define CRC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CRC-8の巡回冗長検査値を生成する
 * SHT4xやAHT20などのセンサーで利用されるCRC-8チェックサムを計算する。
 * 多項式: 0x31 (x^8 + x^5 + x^4 + 1)
 * 初期値: 0xFF
 * @param ptr チェックサムを計算するデータ配列へのポインタ
 * @param len データ長（バイト）
 * @return 8ビットのCRCチェックサム値
 */
uint8_t crc_crc8(const uint8_t *ptr, uint8_t len);

/**
 * @brief CRC-16/CCITT-FALSEの巡回冗長検査値を生成する
 * XMODEMなどで利用される標準的なCRC-16チェックサムを計算する。
 * 多項式: 0x1021 (x^16 + x^12 + x^5 + 1)
 * 初期値: 0xFFFF
 * @param ptr チェックサムを計算するデータ配列へのポインタ
 * @param len データ長（バイト）
 * @return 16ビットのCRCチェックサム値
 */
uint16_t crc_crc16(const uint8_t *ptr, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRC_H_ */

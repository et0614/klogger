/*
 * File:   main.h
 * Author: e.togashi
 *
 * K-Logger v3 メイン (json_protocol から使う関数)
 */

#ifndef MAIN_H
#define	MAIN_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/// 電池電圧 [mV]
uint16_t getBatteryVoltage_mV(void);

/// 機体固有ID (AVRシリコンシリアル由来, 読み取り専用)
uint32_t Device_GetID(void);

/// 記録の開始/停止と状態取得 (USBコマンドから制御)
/// @return 開始時: true=センサ起動成功(記録モードへ移行), false=失敗(USB通信モードのまま)
bool setLoggingActive(bool on);
bool getLoggingActive(void);

/// ソフトウェア時計 (UNIX秒) の設定/取得
void     Clock_Set(uint32_t epoch);
uint32_t Clock_Get(void);

#ifdef	__cplusplus
}
#endif

#endif	/* MAIN_H */

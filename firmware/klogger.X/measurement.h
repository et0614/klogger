/*
 * File:   measurement.h
 * Author: e.togashi
 *
 * 距離の記録処理 (K-Logger v3)。
 *   - VL53L1X を記録間隔で自律測距させ、記録タイミングごとに最新の結果を1レコード(2バイト)にする。
 *   - レコードは RAM のページバッファ(128件)に溜め、満杯でフラッシュへまとめて書く
 *     (フラッシュの書き込み回数と起床を減らす省電力のため)。電池を抜くと最大128件を失う。
 *   - 記録停止・リセット・電池切れの前には必ず Measurement_Flush() を呼ぶ。
 *   - 時刻はレコードに持たない: 時刻 = start_dt + index * interval_sec (EEPROM)。
 */
#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "w25q128.h"
#include "tof.h"

/// 記録済み(フラッシュ+バッファ)のレコード数 = 次に書く index
extern uint32_t rec_latest;

/// フラッシュから保存件数を復元する (起動時に1回, SPI0_Open の後)
void Measurement_Initialize(void);

/// 記録を開始する: 既存データを消去し、センサを起動して自律測距を開始する。
/// @return true:センサ起動成功
bool Measurement_Start(void);

/// 1秒ごとに呼ぶ (記録中のみ)。記録タイミングで1レコード追加する。
/// @param epoch 現在時刻 (1レコード目の時刻 start_dt の確定に使う)
void Measurement_Tick(uint32_t epoch);

/// 記録を停止する (バッファを書き出し、センサを停止する)
void Measurement_Stop(void);

/// RAMバッファのレコードをフラッシュへ書き出す
void Measurement_Flush(void);

/// 保存済みデータを消去する (rec_latest=0)
void Measurement_ClearData(void);

/// フラッシュが満杯か
bool Measurement_IsFull(void);

#ifdef __cplusplus
}
#endif

#endif /* MEASUREMENT_H */

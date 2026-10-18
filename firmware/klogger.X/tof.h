/*
 * File:   tof.h
 * Author: e.togashi
 *
 * VL53L1X (ToF距離センサ) の制御。ST ULD API (VL53L1X/) のラッパ。
 *   - 電源: XSHUT(PF3) で ON/OFF。OFF中(HW standby)は数uA。
 *   - 割り込み: GPIO1(PA1) を「アクティブLow」に設定する。待機中は High(=プルアップ電流ゼロ)、
 *     測定完了で Low。v2 の既定(アクティブHigh)だと待機中にプルアップへ約330uA流れるため。
 *   - 測距条件 (モード/タイミングバジェット/ROI/オフセット) は EM_settings から適用する。
 */
#ifndef TOF_H
#define TOF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t  status;      // ULD レンジステータス (0=有効)
    uint16_t distance_mm; // 距離 [mm]
    uint16_t signal;      // 信号強度 [kcps/SPAD]
    uint16_t ambient;     // 外光 [kcps]
} TofResult_t;

/// センサを起動して初期化し、EM_settings の測距条件を適用する (既に起動済みなら何もしない)。
/// @return true:成功
bool Tof_PowerOn(void);

/// センサを停止し XSHUT=Low (HW standby) にする
void Tof_PowerOff(void);

/// 起動済みか
bool Tof_IsPowered(void);

/// EM_settings の測距条件を適用し直す (起動済みのとき)
bool Tof_ApplySettings(void);

/// センサID (0xEACC) を読む。@return true:I2C成功
bool Tof_GetSensorId(uint16_t *id);

/// 連続(自律)測距を開始する。inter_ms ごとに1回測定し、完了でGPIO1がLowになる。
bool Tof_StartContinuous(uint32_t inter_ms);

/// 測距を停止する
void Tof_Stop(void);

/// 新しい測定結果があれば読み出して割り込みをクリアする (GPIO1ピンで判定, 無ければ I2Cしない)。
/// @return true:新しい結果を読み出した
bool Tof_ReadIfReady(TofResult_t *res);

/// 1回測定して返す (内部で開始→完了待ち(最大timeout_ms)→停止)。
bool Tof_MeasureOnce(TofResult_t *res, uint16_t timeout_ms);

/// オフセット校正: target_mm の位置に対象物を置いた状態で samples 回測定し、
/// オフセット = target - 平均距離 を求めてセンサと EM_settings に設定・保存する。
/// @return true:成功
bool Tof_CalibrateOffset(uint16_t target_mm, uint8_t samples, int16_t *offset);

#ifdef __cplusplus
}
#endif

#endif /* TOF_H */

/*
 * File:   eeprom_manager.h
 * Author: e.togashi
 *
 * K-Logger v3 の設定を EEPROM に保存/読み込みする。CRC で破損を検出し、
 * 破損時やフォーマット変更時(version不一致)はデフォルト値で初期化する。
 */

#ifndef EEPROMMANAGER_H
#define	EEPROMMANAGER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define EM_SETTINGS_VERSION 3   // 構造体を変えたら上げる (古い内容はデフォルトで上書きされる)

// 測距モード (VL53L1X_SetDistanceMode の引数)
#define TOF_MODE_SHORT  1   // ~1.3m, 外光に強い
#define TOF_MODE_LONG   2   // ~4m (暗所), タイミングバジェット20ms以上

// 設定
typedef struct {
    uint16_t version;       // EM_SETTINGS_VERSION
    uint16_t interval_sec;  // 記録間隔 [sec] (1〜600)
    uint32_t start_dt;      // 1レコード目の時刻 (UNIX秒)。レコード i の時刻 = start_dt + i*interval_sec
    uint8_t  tof_mode;      // TOF_MODE_SHORT / TOF_MODE_LONG
    uint8_t  tof_roi;       // 受光領域 (4〜16, X=Y)
    uint16_t tof_tb_ms;     // タイミングバジェット [ms] (15,20,33,50,100,200,500)
    int16_t  tof_offset_mm; // 距離オフセット [mm] (calibrate で設定)
    uint8_t  crc;           // CRC-8 (このメンバを除く範囲)
} KLoggerSettings;

extern KLoggerSettings EM_settings;

/// 設定を読み込む (未初期化/破損ならデフォルトで初期化して保存)
void EM_loadEEPROM(void);

/// 現在の EM_settings を保存する
void EM_saveSettings(void);

#ifdef	__cplusplus
}
#endif

#endif	/* EEPROMMANAGER_H */

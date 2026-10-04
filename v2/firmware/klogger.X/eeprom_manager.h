/**
 * @file eeprom_manager.h
 * @brief AVR128DB32のEEPROMで設定値を管理する（EepromManager.h/.cppのC言語移植版）
 * @author E.Togashi
 */

#ifndef EEPROM_MANAGER_H_
#define EEPROM_MANAGER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 補正係数
typedef struct {
	uint16_t version;	//バージョン
	int16_t offset;		//距離オフセット[mm]（VL53L1X_SetOffsetに渡す符号付き値）
	uint8_t crc;		//CRC
} klogger_settings_t;

//補正係数（loadEEPROM後に有効）
extern klogger_settings_t kSettings;

//設定値を初期値に戻す
void EepromManager_initSettings(void);

//kSettingsをEEPROMに書き込む
void EepromManager_writeEEPROM(void);

//EEPROMからkSettingsへ読み込む（未初期化・CRC不一致時は初期値で再構築）
void EepromManager_loadEEPROM(void);

#ifdef __cplusplus
}
#endif

#endif /* EEPROM_MANAGER_H_ */

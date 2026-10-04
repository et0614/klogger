/**
 * @file eeprom_manager.c
 * @brief AVR128DB32のEEPROMで設定値を管理する（EepromManager.cppのC言語移植版）
 *
 * avr/eeprom.h(EEMEM)の代わりに MCC生成NVMドライバ(EEPROM_Read/EEPROM_Write)を使う。
 * EEPROM_Read/Writeはメモリマップ上の絶対アドレスを取るため、EEPROM_STARTからの
 * 固定オフセットでレイアウトを定義する。
 *
 * レイアウト：
 *   EEPROM_START + 0 : 初期化フラグ（'T'なら初期化済み）
 *   EEPROM_START + 1 : klogger_settings_t（version, offset, crc）
 */

#include <avr/io.h>
#include <string.h>

#include "eeprom_manager.h"
#include "crc.h"
#include "mcc_generated_files/nvm/nvm.h"

//EEPROM上の配置
#define EEP_ADDR_INITFLAG	((eeprom_address_t)(EEPROM_START + 0))
#define EEP_ADDR_SETTINGS	((eeprom_address_t)(EEPROM_START + 1))

//初期化済みマーカー
#define EEP_INIT_MARK		'T'

//設定値
klogger_settings_t kSettings;

//1バイト書き込み（完了待ち付き）
static void eep_write_byte(eeprom_address_t address, uint8_t data)
{
	while (EEPROM_IsBusy());
	EEPROM_Write(address, data);
	while (EEPROM_IsBusy());
}

//設定値を初期化する
void EepromManager_initSettings(void)
{
	kSettings.version = 1;	//バージョン
	kSettings.offset = 0;	//オフセット
	kSettings.crc = 0;		//CRC（一旦0で初期化）
}

//設定を書き込む
void EepromManager_writeEEPROM(void)
{
	// CRCを計算（crcメンバー自身のサイズは計算範囲から除外する）
	kSettings.crc = crc_crc8(
		(const uint8_t*)&kSettings,
		sizeof(klogger_settings_t) - sizeof(kSettings.crc)
	);

	const uint8_t* src = (const uint8_t*)&kSettings;
	for (uint8_t i = 0; i < sizeof(klogger_settings_t); i++) {
		eep_write_byte(EEP_ADDR_SETTINGS + i, src[i]);
	}
}

//設定を読み込む
void EepromManager_loadEEPROM(void)
{
	//初期化未了の場合は補正係数を初期化する
	while (EEPROM_IsBusy());
	if (EEPROM_Read(EEP_ADDR_INITFLAG) != EEP_INIT_MARK) {
		//設定を初期値に戻して保存
		EepromManager_initSettings();
		EepromManager_writeEEPROM();

		//初期化フラグOn
		eep_write_byte(EEP_ADDR_INITFLAG, EEP_INIT_MARK);
	}

	//読み込み処理
	uint8_t* dst = (uint8_t*)&kSettings;
	for (uint8_t i = 0; i < sizeof(klogger_settings_t); i++) {
		dst[i] = EEPROM_Read(EEP_ADDR_SETTINGS + i);
	}

	// 読み込んだデータのCRCを検証
	uint8_t expected_crc = kSettings.crc;
	uint8_t actual_crc = crc_crc8(
		(const uint8_t*)&kSettings,
		sizeof(klogger_settings_t) - sizeof(kSettings.crc)
	);

	// CRCが一致しない（データ破損）場合にはデフォルト値で再初期化
	if (expected_crc != actual_crc) {
		EepromManager_initSettings();
		EepromManager_writeEEPROM();
	}
}

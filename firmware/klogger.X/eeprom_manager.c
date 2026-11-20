#include "mcc_generated_files/system/clock.h" //F_CPUの設定
#include "mcc_generated_files/nvm/nvm.h"     //EEPROM処理
#include "eeprom_manager.h"
#include "crc.h"
#include <stddef.h>
#include <avr/io.h>
#include <util/atomic.h>

// AVR64DU32 EEPROM開始アドレス
#define EEPROM_BASE_ADDR  0x1400
#define ADDR_SETTINGS     EEPROM_BASE_ADDR

KLoggerSettings EM_settings;

// ブロック書き込み (内容が同じバイトは書かない = 書き換え回数の節約)
static void write_eep_block(const void* src, uint16_t dst_addr, size_t size)
{
    const uint8_t* pSrc = (const uint8_t*)src;
    for (size_t i = 0; i < size; i++)
    {
        while (EEPROM_IsBusy());
        if (EEPROM_Read(dst_addr + i) != pSrc[i])
        {
            while (EEPROM_IsBusy());
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                EEPROM_Write(dst_addr + i, pSrc[i]);
            }
        }
    }
    while (EEPROM_IsBusy());
}

static void read_eep_block(void* dst, uint16_t src_addr, size_t size)
{
    uint8_t* pDst = (uint8_t*)dst;
    for (size_t i = 0; i < size; i++) pDst[i] = EEPROM_Read(src_addr + i);
}

static uint8_t calcCrc(void)
{
    return CRC_calc8((uint8_t*)&EM_settings,
                     sizeof(KLoggerSettings) - sizeof(EM_settings.crc));
}

// デフォルト値 (測距条件: Short / 20ms / ROI 4x4, 1秒間隔)。
// 20ms は VL53L1X の最小タイミングバジェット (Short のみ)。15ms は datasheet の範囲外。
// 50ms から 20ms で記録中の平均電流は約1.2mA → 約0.6mA (実測)
static void initSettings(void)
{
    EM_settings = (KLoggerSettings){
        EM_SETTINGS_VERSION,
        1,              // interval_sec
        1609459200,     // start_dt (2021/1/1 00:00:00 UTC, 未計測)
        TOF_MODE_SHORT, // tof_mode
        4,              // tof_roi
        20,             // tof_tb_ms
        0,              // tof_offset_mm
        0               // crc
    };
}

void EM_saveSettings(void)
{
    EM_settings.crc = calcCrc();
    write_eep_block(&EM_settings, ADDR_SETTINGS, sizeof(KLoggerSettings));
}

void EM_loadEEPROM(void)
{
    read_eep_block(&EM_settings, ADDR_SETTINGS, sizeof(KLoggerSettings));
    if (EM_settings.crc != calcCrc() || EM_settings.version != EM_SETTINGS_VERSION)
    {
        initSettings();
        EM_saveSettings();
    }
}

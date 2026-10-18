/*
 * File:   w25q128.h
 * Author: e.togashi
 *
 * Winbond W25Q128JV (128M-bit / 16MB) Flashメモリ制御ドライバ (K-Logger v3)
 * SPI0 で距離データ(1レコード2バイト)の保存・読み出しを行う。満杯停止方式(古いデータは上書きしない)。
 *
 * アドレッシング: 16MB = 24bit 以内なので標準 3バイトアドレスコマンド
 *   0x03(Read) / 0x02(Page Program) / 0x20(4KB Sector Erase) / 0xD8(64KB Block Erase) を使う。
 * 省電力: 待機電流 typ 10uA(最大50uA) → Deep Power-Down(0xB9) で約1uA。
 *   W25_PowerDown() / W25_WakeUp() で明示的に切り替える (Power-Down中は他のコマンドを受け付けない)。
 */

#ifndef W25Q128_H
#define	W25Q128_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// ==========================================
// 容量・レイアウト
// ==========================================
#define PAGE_SIZE        256        // Page Program の単位
#define SECTOR_SIZE      4096       // 最小消去単位 (4KB)
#define BLOCK64_SIZE     65536      // 64KB ブロック消去単位
#define FLASH_TOTAL_SIZE 0x1000000UL // 16MB
#define DATA_START_ADDR  4096UL     // 先頭4KBは予約(セルフテスト用スクラッチ)。データはここから
#define RECORD_SIZE      2          // 1レコード = uint16_t (下記エンコード)
#define RECS_PER_PAGE    (PAGE_SIZE / RECORD_SIZE)                          // 128
#define MAX_RECORD_COUNT ((FLASH_TOTAL_SIZE - DATA_START_ADDR) / RECORD_SIZE) // 8,386,560 (1Hzで約97日)

// ==========================================
// レコードのエンコード (16bit, little-endian で保存)
//   bit15..13 : 状態コード (DIST_ST_*)
//   bit12..0  : 距離 [mm] (0..8189)
// 消去状態 0xFFFF は「空き」、0xFFFE は「計測失敗(データ無し)」に予約。
// 時刻はレコードに持たない: 時刻 = start_dt + index * interval (EEPROM に保存)
// ==========================================
#define DIST_REC_EMPTY    0xFFFFu   // 未書き込み(消去状態)
#define DIST_REC_FAILED   0xFFFEu   // 計測失敗 (I2Cエラー等で値が得られなかった)
#define DIST_MM_MASK      0x1FFFu
#define DIST_MM_MAX       8189u
#define DIST_ST_SHIFT     13

// 状態コード (VL53L1X のレンジステータスを3bitに集約)
#define DIST_ST_VALID     0   // ULD status 0 : 有効
#define DIST_ST_SIGMA     1   // ULD status 1 : sigma fail (ばらつき大)
#define DIST_ST_SIGNAL    2   // ULD status 2 : signal fail (反射光が弱い)
#define DIST_ST_OUTBOUND  3   // ULD status 4 : out of bounds (位相範囲外)
#define DIST_ST_WRAP      4   // ULD status 7 : wraparound
#define DIST_ST_MINRANGE  5   // ULD status 3 : min range fail
#define DIST_ST_OTHER     6   // 上記以外
#define DIST_ST_FAILED    7   // 計測失敗 (DIST_REC_FAILED のみ)

/// 距離[mm]とULDレンジステータスから16bitレコードを作る
uint16_t W25_EncodeDistance(uint16_t distance_mm, uint8_t uld_status);

// ==========================================
// 低レベル操作
// ==========================================
/// JEDEC ID (0x9F)。id[0]=メーカ(Winbond=0xEF), id[1]=タイプ, id[2]=容量(W25Q128=0x18)。
/// @return true:応答あり(0x00/0xFF以外)
bool W25_ReadJEDECID(uint8_t id[3]);

/// 予約領域(先頭4KB)を使った write/read/verify セルフテスト。保存データは消さない。
bool W25_SelfTest(void);

/// Deep Power-Down へ移行 (0xB9)。以降 W25_WakeUp() まで他のコマンドは無視される。
void W25_PowerDown(void);

/// Deep Power-Down から復帰 (0xAB) し tRES1(3us) 待つ。Power-Down中でなくても無害。
void W25_WakeUp(void);

/// 4KB セクタ消去 (blocking, typ 45ms)
void W25_SectorErase(uint32_t address);

/// 64KB ブロック消去 (blocking, typ 150ms)
void W25_BlockErase64K(uint32_t address);

/// 任意アドレスから len バイト読み出す
bool W25_ReadData(uint32_t address, uint8_t *buffer, uint16_t len);

/// 1ページ内に収まる範囲を書き込む (Page Program, blocking)
bool W25_WriteData(uint32_t address, const uint8_t *data, uint16_t len);

// ==========================================
// レコード操作
// ==========================================
/// レコード index の物理アドレス
static inline uint32_t W25_RecordAddress(uint32_t index)
{
    return DATA_START_ADDR + index * RECORD_SIZE;
}

/// index から count 件のレコードを書き込む (同一ページ内であること)。
/// 書き込み先がセクタ先頭なら事前にそのセクタを消去する。
bool W25_WriteRecords(uint32_t index, const uint16_t *recs, uint16_t count);

/// index から count 件のレコードを読み出す
bool W25_ReadRecords(uint32_t index, uint16_t *recs, uint16_t count);

/// 有効レコード数 (= 最初に DIST_REC_EMPTY になる index) を二分探索で返す
uint32_t W25_Count_Record(void);

/// レコード [0, count) が使っていた領域を消去する (64KBブロック消去を優先, 端数は4KB)
void W25_EraseRecords(uint32_t count);

#ifdef	__cplusplus
}
#endif

#endif	/* W25Q128_H */

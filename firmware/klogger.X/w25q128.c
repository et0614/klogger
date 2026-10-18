/*
 * File:   w25q128.c
 *
 * Winbond W25Q128JV ドライバ (K-Logger v3)。仕様・レコード形式は w25q128.h を参照。
 */

#include "w25q128.h"
#include "mcc_generated_files/system/system.h"
#include "mcc_generated_files/spi/spi0.h"
#include "mcc_generated_files/timer/delay.h"

// ==========================================
// CSピン (PA0 = FLASH_CS)
// ==========================================
#define SPI_CS_SetLow()   FLASH_CS_SetLow()   /* 選択 */
#define SPI_CS_SetHigh()  FLASH_CS_SetHigh()  /* 非選択 */

// ==========================================
// コマンド
// ==========================================
#define CMD_WRITE_ENABLE      0x06
#define CMD_READ_STATUS_REG1  0x05
#define CMD_SECTOR_ERASE      0x20   // 4KB
#define CMD_BLOCK_ERASE_64K   0xD8   // 64KB
#define CMD_PAGE_PROGRAM      0x02
#define CMD_READ_DATA         0x03
#define CMD_JEDEC_ID          0x9F
#define CMD_POWER_DOWN        0xB9
#define CMD_RELEASE_PD        0xAB

// ==========================================
// 内部ヘルパー
// ==========================================
static bool isValidRange(uint32_t addr, uint32_t len)
{
    return (addr + len) <= FLASH_TOTAL_SIZE;
}

static void sendAddress(uint32_t address)
{
    SPI0_ByteExchange((uint8_t)(address >> 16));
    SPI0_ByteExchange((uint8_t)(address >> 8));
    SPI0_ByteExchange((uint8_t)address);
}

static void writeEnable(void)
{
    SPI_CS_SetLow();
    SPI0_ByteExchange(CMD_WRITE_ENABLE);
    SPI_CS_SetHigh();
}

// BUSY(Status Reg1 bit0) が解けるまで待つ。timeout_ms は最大待ち時間の目安。
static bool waitForReady(uint16_t timeout_ms)
{
    uint32_t loops = (uint32_t)timeout_ms * 100; // 1ループ ~10us
    uint8_t status;
    do {
        SPI_CS_SetLow();
        SPI0_ByteExchange(CMD_READ_STATUS_REG1);
        status = SPI0_ByteExchange(0x00);
        SPI_CS_SetHigh();
        if ((status & 0x01) == 0) return true;
        DELAY_microseconds(10);
    } while (loops-- > 0);
    return false;
}

static void eraseCommand(uint8_t cmd, uint32_t address, uint16_t timeout_ms)
{
    writeEnable();
    SPI_CS_SetLow();
    SPI0_ByteExchange(cmd);
    sendAddress(address);
    SPI_CS_SetHigh();
    waitForReady(timeout_ms);
}

// ==========================================
// エンコード
// ==========================================
uint16_t W25_EncodeDistance(uint16_t distance_mm, uint8_t uld_status)
{
    uint8_t st;
    switch (uld_status)
    {
        case 0:  st = DIST_ST_VALID;    break;
        case 1:  st = DIST_ST_SIGMA;    break;
        case 2:  st = DIST_ST_SIGNAL;   break;
        case 4:  st = DIST_ST_OUTBOUND; break;
        case 7:  st = DIST_ST_WRAP;     break;
        case 3:  st = DIST_ST_MINRANGE; break;
        default: st = DIST_ST_OTHER;    break;
    }
    if (distance_mm > DIST_MM_MAX) distance_mm = DIST_MM_MAX;
    return (uint16_t)(((uint16_t)st << DIST_ST_SHIFT) | (distance_mm & DIST_MM_MASK));
}

// ==========================================
// 低レベル操作
// ==========================================
bool W25_ReadJEDECID(uint8_t id[3])
{
    SPI_CS_SetLow();
    SPI0_ByteExchange(CMD_JEDEC_ID);
    id[0] = SPI0_ByteExchange(0x00);
    id[1] = SPI0_ByteExchange(0x00);
    id[2] = SPI0_ByteExchange(0x00);
    SPI_CS_SetHigh();
    return (id[0] != 0x00 && id[0] != 0xFF);
}

void W25_PowerDown(void)
{
    SPI_CS_SetLow();
    SPI0_ByteExchange(CMD_POWER_DOWN);
    SPI_CS_SetHigh();
    DELAY_microseconds(5); // tDP = 3us
}

void W25_WakeUp(void)
{
    SPI_CS_SetLow();
    SPI0_ByteExchange(CMD_RELEASE_PD);
    SPI_CS_SetHigh();
    DELAY_microseconds(5); // tRES1 = 3us
}

void W25_SectorErase(uint32_t address)
{
    eraseCommand(CMD_SECTOR_ERASE, address, 500);   // max 400ms
}

void W25_BlockErase64K(uint32_t address)
{
    eraseCommand(CMD_BLOCK_ERASE_64K, address, 2500); // max 2000ms
}

bool W25_ReadData(uint32_t address, uint8_t *buffer, uint16_t len)
{
    if (!isValidRange(address, len)) return false;
    SPI_CS_SetLow();
    SPI0_ByteExchange(CMD_READ_DATA);
    sendAddress(address);
    SPI0_BufferRead(buffer, len);
    SPI_CS_SetHigh();
    return true;
}

bool W25_WriteData(uint32_t address, const uint8_t *data, uint16_t len)
{
    if (len == 0) return true;
    if (!isValidRange(address, len)) return false;
    // Page Program はページ境界を越えると先頭に折り返すので禁止
    if ((address % PAGE_SIZE) + len > PAGE_SIZE) return false;

    writeEnable();
    SPI_CS_SetLow();
    SPI0_ByteExchange(CMD_PAGE_PROGRAM);
    sendAddress(address);
    SPI0_BufferWrite((void *)data, len);
    SPI_CS_SetHigh();
    return waitForReady(5); // max 3ms
}

bool W25_SelfTest(void)
{
    const uint32_t addr = 0; // 予約領域の先頭セクタ (データ領域外)
    uint8_t wr[16], rd[16];
    for (uint8_t i = 0; i < sizeof(wr); i++) wr[i] = (uint8_t)(0xA5 ^ i);

    W25_SectorErase(addr);
    if (!W25_WriteData(addr, wr, sizeof(wr))) return false;
    if (!W25_ReadData(addr, rd, sizeof(rd)))  return false;
    for (uint8_t i = 0; i < sizeof(wr); i++)
        if (rd[i] != wr[i]) return false;
    return true;
}

// ==========================================
// レコード操作
// ==========================================
bool W25_WriteRecords(uint32_t index, const uint16_t *recs, uint16_t count)
{
    if (count == 0) return true;
    if (index + count > MAX_RECORD_COUNT) return false;
    uint32_t addr = W25_RecordAddress(index);

    // 書き込み先がセクタ先頭なら消去しておく (clear_memory 済みでも無害)
    if ((addr % SECTOR_SIZE) == 0) W25_SectorErase(addr);

    return W25_WriteData(addr, (const uint8_t *)recs, (uint16_t)(count * RECORD_SIZE));
}

bool W25_ReadRecords(uint32_t index, uint16_t *recs, uint16_t count)
{
    return W25_ReadData(W25_RecordAddress(index), (uint8_t *)recs, (uint16_t)(count * RECORD_SIZE));
}

static bool isRecordEmpty(uint32_t index)
{
    uint16_t v;
    W25_ReadRecords(index, &v, 1);
    return v == DIST_REC_EMPTY;
}

uint32_t W25_Count_Record(void)
{
    if (isRecordEmpty(0)) return 0;

    // 不変条件: record[low]=有効, record[high]=空き (high=MAX は満杯時の番兵)
    uint32_t low = 0;
    uint32_t high = MAX_RECORD_COUNT;
    while (high - low > 1)
    {
        uint32_t mid = low + (high - low) / 2;
        if (isRecordEmpty(mid)) high = mid;
        else                    low  = mid;
    }
    return high;
}

void W25_EraseRecords(uint32_t count)
{
    if (count == 0) return;
    uint32_t addr = DATA_START_ADDR;
    uint32_t end  = W25_RecordAddress(count); // 使用済み領域の終端(含まない)

    while (addr < end)
    {
        if ((addr % BLOCK64_SIZE) == 0 && (end - addr) >= BLOCK64_SIZE)
        {
            W25_BlockErase64K(addr);
            addr += BLOCK64_SIZE;
        }
        else
        {
            W25_SectorErase(addr);
            addr += SECTOR_SIZE;
        }
    }
}

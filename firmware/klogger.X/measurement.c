#include "measurement.h"
#include "eeprom_manager.h"

uint32_t rec_latest = 0;

// RAMページバッファ (1ページ=128レコード)。s_flash_count 番目から s_buf_n 件がフラッシュ未書き込み。
static uint16_t s_buf[RECS_PER_PAGE];
static uint8_t  s_buf_n = 0;
static uint32_t s_flash_count = 0;

static uint16_t s_sec = 0;          // 記録区間内の経過秒
static uint16_t s_last = DIST_REC_FAILED; // 最新の測定結果 (エンコード済み)
static bool     s_start_dt_set = false;

void Measurement_Initialize(void)
{
    W25_WakeUp();
    s_flash_count = W25_Count_Record();
    rec_latest = s_flash_count;
    s_buf_n = 0;
}

bool Measurement_IsFull(void)
{
    return rec_latest >= MAX_RECORD_COUNT;
}

void Measurement_Flush(void)
{
    if (s_buf_n == 0) return;
    W25_WakeUp();
    // バッファは常にページ内 (s_flash_count はページ境界から始まる) なので1回で書ける
    W25_WriteRecords(s_flash_count, s_buf, s_buf_n);
    s_flash_count += s_buf_n;
    s_buf_n = 0;
}

static void appendRecord(uint16_t rec)
{
    if (Measurement_IsFull()) return;
    s_buf[s_buf_n++] = rec;
    rec_latest++;
    // ページが埋まった / フラッシュ末尾 → 書き出してフラッシュは再び Deep Power-Down
    if (s_buf_n >= RECS_PER_PAGE || Measurement_IsFull())
    {
        Measurement_Flush();
        W25_PowerDown();
    }
}

void Measurement_ClearData(void)
{
    W25_WakeUp();
    s_buf_n = 0;
    W25_EraseRecords(W25_Count_Record());
    s_flash_count = 0;
    rec_latest = 0;
}

bool Measurement_Start(void)
{
    Measurement_ClearData();

    s_sec = 0;
    s_last = DIST_REC_FAILED;
    s_start_dt_set = false;

    if (!Tof_PowerOn()) return false;

    // センサの測定周期は記録間隔より約2%短くする: RTCとセンサ内部発振のずれで
    // 「記録タイミングに新しい結果が無い」ことを避けるため (余った結果は上書きされるだけ)。
    uint32_t period = (uint32_t)EM_settings.interval_sec * 1000UL;
    uint32_t margin = period / 50;
    if (margin < 10) margin = 10;
    if (!Tof_StartContinuous(period - margin)) return false;

    W25_PowerDown(); // 記録中はページが埋まるまでフラッシュを使わない
    return true;
}

void Measurement_Stop(void)
{
    Tof_PowerOff();
    Measurement_Flush();
    W25_WakeUp(); // USB通信などで続けてアクセスできるよう起こしておく
}

void Measurement_OnTofReady(void)
{
    // 新しい結果が出ていれば取り込む (GPIO1=Low のときだけ I2C で読む)
    TofResult_t r;
    if (Tof_ReadIfReady(&r)) s_last = W25_EncodeDistance(r.distance_mm, r.status);
}

void Measurement_Tick(uint32_t epoch)
{
    // 割り込みを取りこぼした場合の保険 (通常は Measurement_OnTofReady で取り込み済み)
    Measurement_OnTofReady();

    uint16_t interval = EM_settings.interval_sec ? EM_settings.interval_sec : 1;
    if (++s_sec < interval) return;
    s_sec = 0;

    if (!s_start_dt_set)
    {
        // 1レコード目の時刻を確定する (以降 start_dt + index*interval)
        EM_settings.start_dt = epoch;
        EM_saveSettings();
        s_start_dt_set = true;
    }
    appendRecord(s_last);
    s_last = DIST_REC_FAILED; // 次の区間で新しい結果が来なければ「計測失敗」とする
}

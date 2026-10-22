#include "tof.h"
#include "eeprom_manager.h"
#include "VL53L1X/VL53L1X_api.h"
#include "mcc_generated_files/system/system.h"
#include "mcc_generated_files/timer/delay.h"
#include <math.h>

#define DEV 0x29   // ULD API の dev 引数 (platform層では未使用)

static bool s_powered = false;

bool Tof_IsPowered(void) { return s_powered; }

static bool ok(VL53L1X_ERROR st) { return st == 0; }

bool Tof_ApplySettings(void)
{
    if (!s_powered) return false;

    uint8_t  mode = EM_settings.tof_mode;
    uint16_t tb   = EM_settings.tof_tb_ms;
    uint8_t  roi  = EM_settings.tof_roi;
    if (mode != TOF_MODE_LONG) mode = TOF_MODE_SHORT;
    if (mode == TOF_MODE_LONG && tb < 20) tb = 20;   // Longモードは15ms非対応
    if (roi < 4) roi = 4;
    if (roi > 16) roi = 16;

    // ULD の SetDistanceMode は現在のタイミングバジェットを再設定するため、
    // 15ms(Shortのみ有効)のまま Long へ切り替えると失敗する。両モードで有効な20msを一旦設定してから切り替える。
    bool r = true;
    r &= ok(VL53L1X_SetTimingBudgetInMs(DEV, (tb < 20) ? 20 : tb));
    r &= ok(VL53L1X_SetDistanceMode(DEV, mode));
    r &= ok(VL53L1X_SetTimingBudgetInMs(DEV, tb));
    r &= ok(VL53L1X_SetROI(DEV, roi, roi));
    r &= ok(VL53L1X_SetOffset(DEV, EM_settings.tof_offset_mm));
    return r;
}

bool Tof_PowerOn(void)
{
    if (s_powered) return true;

    TOF_XSHUT_SetHigh();
    DELAY_milliseconds(2); // tBOOT 1.2ms

    // ブート完了待ち (最大 約100ms)
    uint8_t booted = 0;
    for (uint8_t i = 0; i < 50 && !booted; i++)
    {
        if (!ok(VL53L1X_BootState(DEV, &booted))) booted = 0;
        if (!booted) DELAY_milliseconds(2);
    }
    if (!booted) { TOF_XSHUT_SetLow(); return false; }

    if (!ok(VL53L1X_SensorInit(DEV))) { TOF_XSHUT_SetLow(); return false; }

    // GPIO1 をアクティブLowに (SensorInit の既定はアクティブHigh)
    VL53L1X_SetInterruptPolarity(DEV, 0);

    s_powered = true;
    if (!Tof_ApplySettings()) { Tof_PowerOff(); return false; }
    return true;
}

void Tof_PowerOff(void)
{
    if (s_powered) VL53L1X_StopRanging(DEV);
    TOF_XSHUT_SetLow();
    s_powered = false;
}

bool Tof_GetSensorId(uint16_t *id)
{
    return ok(VL53L1X_GetSensorId(DEV, id));
}

bool Tof_StartContinuous(uint32_t inter_ms)
{
    if (!s_powered) return false;
    if (inter_ms < (uint32_t)EM_settings.tof_tb_ms + 5) inter_ms = EM_settings.tof_tb_ms + 5;
    VL53L1X_StopRanging(DEV);
    bool r = ok(VL53L1X_SetInterMeasurementInMs(DEV, inter_ms));
    r &= ok(VL53L1X_ClearInterrupt(DEV));
    r &= ok(VL53L1X_StartRanging(DEV));
    return r;
}

void Tof_Stop(void)
{
    if (s_powered) VL53L1X_StopRanging(DEV);
}

static bool readResult(TofResult_t *res)
{
    VL53L1X_Result_t r;
    if (!ok(VL53L1X_GetResult(DEV, &r))) return false;
    VL53L1X_ClearInterrupt(DEV);
    res->status      = r.Status;
    res->distance_mm = r.Distance;
    res->signal      = r.SigPerSPAD;
    res->ambient     = r.Ambient;
    return true;
}

bool Tof_ReadIfReady(TofResult_t *res)
{
    if (!s_powered) return false;
    if (TOF_GPIO_GetValue()) return false; // High = 未完了 (アクティブLow)
    return readResult(res);
}

bool Tof_MeasureOnce(TofResult_t *res, uint16_t timeout_ms)
{
    if (!s_powered && !Tof_PowerOn()) return false;

    VL53L1X_StopRanging(DEV);
    VL53L1X_SetInterMeasurementInMs(DEV, EM_settings.tof_tb_ms + 5);
    VL53L1X_ClearInterrupt(DEV);
    if (!ok(VL53L1X_StartRanging(DEV))) return false;

    bool got = false;
    for (uint16_t t = 0; t < timeout_ms && !got; t++)
    {
        if (!TOF_GPIO_GetValue()) got = readResult(res);
        else DELAY_milliseconds(1);
    }
    VL53L1X_StopRanging(DEV);
    return got;
}

bool Tof_CalibrateOffset(uint16_t target_mm, uint8_t samples, TofCalib_t *res)
{
    if (samples == 0) return false;
    if (!s_powered && !Tof_PowerOn()) return false;

    // オフセット0の状態で生の距離を測る
    if (!ok(VL53L1X_SetOffset(DEV, 0))) return false;

    // 平均と標準偏差 (桁あふれ防止のため1個目からの差で積算)
    int32_t sum = 0;
    int64_t sumsq = 0;
    int16_t first = 0;
    uint8_t n = 0;
    VL53L1X_StopRanging(DEV);
    VL53L1X_SetInterMeasurementInMs(DEV, EM_settings.tof_tb_ms + 5);
    VL53L1X_ClearInterrupt(DEV);
    VL53L1X_StartRanging(DEV);
    for (uint16_t t = 0; t < (uint16_t)samples * 600u && n < samples; t++)
    {
        if (!TOF_GPIO_GetValue())
        {
            TofResult_t r;
            if (readResult(&r) && r.status == 0)
            {
                if (n == 0) first = (int16_t)r.distance_mm;
                int32_t d = (int32_t)r.distance_mm - first;
                sum += d; sumsq += d * d; n++;
            }
        }
        else DELAY_milliseconds(1);
    }
    VL53L1X_StopRanging(DEV);

    if (n < samples)
    {
        VL53L1X_SetOffset(DEV, EM_settings.tof_offset_mm); // 失敗時は元に戻す
        return false;
    }

    float mean_d = (float)sum / n;
    float var = (float)sumsq / n - mean_d * mean_d;
    res->n        = n;
    res->raw_mm   = (uint16_t)(first + (int32_t)lroundf(mean_d));
    res->std_x10  = (uint16_t)lroundf(sqrtf(var > 0 ? var : 0) * 10.0f);
    res->prev_mm  = EM_settings.tof_offset_mm;
    res->offset_mm = (int16_t)((int32_t)target_mm - (int32_t)res->raw_mm);
    EM_settings.tof_offset_mm = res->offset_mm;
    EM_saveSettings();
    return ok(VL53L1X_SetOffset(DEV, res->offset_mm));
}

/**
 * @file main.c
 * @brief K-Logger v3 (AVR64DU32) 扉開閉記録装置 メインプログラム
 * @author E.Togashi
 *
 * 動作モード:
 *   USB通信モード : 起動時にUSB(VBUS)が接続されている場合。PCからのJSONコマンドを処理する。
 *                   LEDは1秒ごとに2回点滅(ハートビート)。
 *   記録モード     : start_logging コマンドで移行。USBを停止し、STANDBYスリープ + PIT(1秒)で
 *                   起床して距離を記録する。LEDは消灯。USBを抜いても記録を続ける。
 *   待機(ソフトOFF): USB非接続で起動した場合・電池切れ。STANDBYで眠り続ける(センサ/フラッシュも停止)。
 *
 * イベント:
 *   USB挿入(VBUS立ち上がり) : 記録中ならバッファを書き出してリセット → USB通信モード
 *   USB抜去(VBUS立ち下がり) : USB通信モードならリセット → 待機 (USB回路での電池消費を止める)
 *   ボタン(SW)押下中        : LED点灯 (生存確認)
 *   ボタン5秒長押し         : バッファを書き出してリセット (記録中なら記録停止 = ソフトOFF)
 *   電池電圧低下(1.0V未満が30秒継続, USB非接続時のみ) : 記録停止 → 待機
 */

// <editor-fold defaultstate="collapsed" desc="ヘッダインクルード">

#include "mcc_generated_files/system/clock.h" // F_CPU
#include "mcc_generated_files/system/system.h"
#include "mcc_generated_files/adc/adc0.h"
#include "mcc_generated_files/timer/delay.h"

#include "main.h"
#include "hal_io.h"
#include "eeprom_manager.h"
#include "usb_comm.h"
#include "measurement.h"

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="定数・広域変数">

#define LOW_BATTERY_MV          1000  // 電池電圧の下限 [mV] (単4×1本, 昇圧ICは0.5Vまで動くが余裕を持たせる)
#define BATTERY_CHECK_SEC       10    // 記録中の電池電圧チェック周期 [sec]
#define LOW_BATTERY_COUNT_LIMIT 3     // 下限割れがこの回数連続したら記録停止 (= 30秒)
#define RESET_HOLD_SEC          5     // ボタン長押しでリセットするまでの秒数

typedef enum { MODE_USB, MODE_LOGGING, MODE_STANDBY } run_mode_t;

static volatile bool s_tick = false;          // PIT(1秒)で立つ
static volatile bool s_vbus_changed = false;  // VBUS検出ピンの変化で立つ
static run_mode_t s_mode = MODE_STANDBY;
static uint8_t s_reset_timer = 0;
static uint8_t s_lowbat_count = 0;
static uint8_t s_batt_check_sec = 0;

// ソフトウェア時計 (UNIX秒)。set_time で設定し、PIT(1秒)ごとに進める。
static uint32_t g_epoch = 0;

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="割り込みコールバック">

static void oneSecHandler(void)
{
    s_tick = true;
}

// ボタンSW(PA7, 両エッジ): 押している間LEDを点灯する
static void buttonHandler(void)
{
    if (!SW_GetValue()) turnOnGreenLED();
    else                turnOffGreenLED();
}

// VBUS検出(PF2, 両エッジ, 完全非同期ピンなのでSTANDBY中でも起床できる)
static void vbusHandler(void)
{
    s_vbus_changed = true;
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="モード遷移">

static bool isUsbConnected(void)
{
    return VBUS_DET_GetValue() != 0;
}

// 記録中のデータを確実に保存してからソフトウェアリセットする
static void safeReset(void)
{
    if (s_mode == MODE_LOGGING) Measurement_Stop();
    W25_PowerDown();
    _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRST_bm);
}

// 待機(ソフトOFF)へ: センサ・フラッシュ・USBを止める
static void enterStandby(void)
{
    if (s_mode == MODE_LOGGING) Measurement_Stop();
    Tof_PowerOff();
    W25_PowerDown();
    USB_Comm_Disable();
    turnOffGreenLED();
    s_mode = MODE_STANDBY;
}

bool setLoggingActive(bool on)
{
    if (on)
    {
        s_lowbat_count = 0;
        s_batt_check_sec = 0;
        if (Measurement_Start()) { s_mode = MODE_LOGGING; return true; }
        Measurement_Stop(); // センサ起動失敗: USB通信モードのまま
        return false;
    }
    if (s_mode == MODE_LOGGING)
    {
        Measurement_Stop();
        s_mode = isUsbConnected() ? MODE_USB : MODE_STANDBY;
    }
    return true;
}

bool getLoggingActive(void)
{
    return s_mode == MODE_LOGGING;
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="1秒ごとの処理">

static void handleButton(void)
{
    if (!SW_GetValue())
    {
        turnOnGreenLED();
        if (++s_reset_timer >= RESET_HOLD_SEC)
        {
            while (!SW_GetValue()) ; // 離されるまで待ってからリセット
            safeReset();
        }
    }
    else
    {
        s_reset_timer = 0;
        if (s_mode == MODE_USB) blinkGreenLEDHeartbeat();
        else                    turnOffGreenLED();
    }
}

static void checkBattery(void)
{
    if (++s_batt_check_sec < BATTERY_CHECK_SEC) return;
    s_batt_check_sec = 0;

    if (getBatteryVoltage_mV() < LOW_BATTERY_MV) s_lowbat_count++;
    else                                         s_lowbat_count = 0;

    // 電圧崩落域でのフラッシュ書き込みはデータ破損の恐れがあるため、早めに止める
    if (s_lowbat_count >= LOW_BATTERY_COUNT_LIMIT) enterStandby();
}

static void executeSecondlyTask(void)
{
    g_epoch++;
    handleButton();

    if (s_mode == MODE_LOGGING)
    {
        Measurement_Tick(g_epoch);
        if (Measurement_IsFull()) enterStandby(); // 満杯停止
        else if (!isUsbConnected()) checkBattery(); // USB給電中は電池電圧を見ない(電池無しでも動くため)
    }
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="main">

int main(void)
{
    SYSTEM_Initialize();   // クロック / ピン / ADC / I2C / SPI / USB / RTC 等
    ADC0_Disable();        // ADC(内部基準電圧 ~175uA)は電池電圧を測るときだけ有効にする
    SPI0_Open(SPI0_DEFAULT);

    EM_loadEEPROM();
    Measurement_Initialize(); // フラッシュの保存件数を復元
    Tof_PowerOff();           // センサは使うときだけ起動する

    RTC_SetPITIsrCallback(oneSecHandler);
    SW_SetInterruptHandler(buttonHandler);
    VBUS_DET_SetInterruptHandler(vbusHandler);

    // STANDBY中もPITで毎秒起床できるよう RUNSTDBY を有効化 (MCC生成コードは無効で初期化する)
    while (RTC.STATUS > 0) ;
    RTC.CTRLA |= RTC_RUNSTDBY_bm;

    sei();

    if (isUsbConnected())
    {
        USB_Comm_Initialize();
        s_mode = MODE_USB;
    }
    else
    {
        enterStandby();
    }

    while (true)
    {
        if (s_vbus_changed)
        {
            s_vbus_changed = false;
            DELAY_milliseconds(20); // チャタリング除去
            bool usb = isUsbConnected();
            // USB挿入: 記録中/待機中 → USB通信モードへ。USB抜去: USB通信モード → 待機へ。
            // (記録中の抜去は無視して記録を続ける)
            if ((usb && s_mode != MODE_USB) || (!usb && s_mode == MODE_USB)) safeReset();
        }

        if (s_tick)
        {
            s_tick = false;
            executeSecondlyTask();
        }

        if (s_mode == MODE_USB)
        {
            USB_Comm_Task();
        }
        else
        {
            set_sleep_mode(SLEEP_MODE_STANDBY);
            sleep_mode();
        }
    }
}

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="ユーティリティ">

void     Clock_Set(uint32_t epoch) { g_epoch = epoch; }
uint32_t Clock_Get(void)           { return g_epoch; }

// 電池電圧 [mV]。PF4(AIN20)に電池+が直結。基準電圧 内部2.048V、10bit×64回積算 (MCC設定)。
// mV = acc * 2048 / (1024*64) = acc / 32
uint16_t getBatteryVoltage_mV(void)
{
    ADC0_Enable();
    DELAY_microseconds(100); // 基準電圧の立ち上がり待ち
    ADC0_ChannelSelect(ADC0_CHANNEL_AIN20);
    ADC0_ConversionStart();
    while (!ADC0_IsConversionDone()) ;
    adc_accumulate_t acc = ADC0_AccumulatedResultGet();
    ADC0_Disable();
    if (acc < 0) acc = 0;
    return (uint16_t)(acc / 32);
}

// 機体固有ID: シリコンシリアル(SIGROW.SERNUM, 16バイト)の FNV-1a(32bit) ハッシュ
uint32_t Device_GetID(void)
{
    const volatile uint8_t *sn = &SIGROW.SERNUM0;
    uint32_t hash = 2166136261UL;
    for (uint8_t i = 0; i < 16; i++)
    {
        hash ^= sn[i];
        hash *= 16777619UL;
    }
    return hash;
}

// </editor-fold>

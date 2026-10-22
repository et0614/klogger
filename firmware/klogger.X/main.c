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

// 診断用ビルド (USB不具合の切り分け用)。0=通常ファームウェア。
//   1: USBのみ (c-loggerと同じ起動手順)。LEDは1秒ごとに点灯/消灯を切り替え
//   2: 1 + EEPROM読込・フラッシュ件数復元・センサOFF・ADC停止。LEDは1秒ごとに切替
//   3: 2 + ボタン/VBUS割り込み登録 + 2回点滅ハートビート (通常FWのUSBモードとほぼ同じ)
#ifndef DIAG_LEVEL
#define DIAG_LEVEL 0
#endif

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

#if DIAG_LEVEL == 4
// 診断4: USBの制御要求がどこまで届いたかをLEDの点滅回数で表示する (USBのみ, delayなし)
//   点滅回数 = 到達段階 + 1 を約2秒おきに繰り返す
//   1回: 何も受信していない
//   2回: SET_CONFIGURATION 受信 (列挙完了)
//   3回: CDCクラス要求を受信 (GET_LINE_CODING 等)
//   4回: SET_LINE_CODING のSETUPを受信
//   5回: SET_LINE_CODING のデータ(7バイト)を受信完了 (PC→機器のデータ転送が成功)
//   6回: SET_CONTROL_LINE_STATE を受信
#include "mcc_generated_files/usb/usb_common/usb_core_events.h"
#include "mcc_generated_files/usb/usb_cdc/usb_cdc.h"
#include "mcc_generated_files/usb/usb_common/usb_core_transfer.h"

static volatile uint8_t s_stage = 0;
static uint8_t s_lc_buf[7];

static void diagStage(uint8_t st) { if (st > s_stage) s_stage = st; }

static void diagSetConfig(USB_SETUP_REQUEST_t *req) { (void)req; diagStage(1); }

static void diagLineCodingDone(void) { diagStage(4); }

static RETURN_CODE_t diagClassRequest(USB_SETUP_REQUEST_t *req)
{
    diagStage(2);
    if (req->bmRequestType.type == USB_REQUEST_TYPE_CLASS &&
        req->bmRequestType.dataPhaseTransferDirection == USB_REQUEST_DIR_OUT)
    {
        if (req->bRequest == USB_CDC_REQUEST_SET_LINE_CODING)
        {
            diagStage(3);
            return USB_TransferControlDataSet(s_lc_buf, sizeof(s_lc_buf), diagLineCodingDone);
        }
        if (req->bRequest == USB_CDC_REQUEST_SET_CONTROL_LINE_STATE) diagStage(5);
    }
    return USB_CDCRequestHandler(req);
}

int main(void)
{
    SYSTEM_Initialize();
    RTC_SetPITIsrCallback(oneSecHandler);
    // PITを8Hzにする (点滅表示用)
    while (RTC.PITSTATUS > 0) ;
    RTC.PITCTRLA = RTC_PERIOD_CYC4096_gc | RTC_PITEN_bm;
    DELAY_milliseconds(100);
    sei();
    DELAY_milliseconds(100);
    USB_Comm_Initialize();
    USB_SetConfigurationCallbackRegister(diagSetConfig);
    USB_ClassRequestCallbackRegister(diagClassRequest);

    uint8_t phase = 0; // 1周期 = (段階+1)回 × 4tick + 休み16tick
    while (true)
    {
        if (s_tick)
        {
            s_tick = false;
            uint8_t blinks = (uint8_t)(s_stage + 1);
            uint8_t len = (uint8_t)(blinks * 4 + 16);
            if (phase < blinks * 4 && (phase % 4) == 0) turnOnGreenLED();
            else                                       turnOffGreenLED();
            if (++phase >= len) phase = 0;
        }
        USB_Comm_Task();
    }
}
#elif DIAG_LEVEL == 5
// 診断5: CDCの制御要求ごとに「受信回数」と「完了回数(ステータス段階まで終了)」を数え、
//        LEDの点滅で4つの数を順に表示する (各最大9回, 0回は長めの1回点灯で表す)
//   [A:SET_LINE_CODING受信] 休 [B:同 完了] 休 [C:GET_LINE_CODING受信] 休 [D:同 完了] 長休
#include "mcc_generated_files/usb/usb_common/usb_core_events.h"
#include "mcc_generated_files/usb/usb_cdc/usb_cdc.h"
#include "mcc_generated_files/usb/usb_common/usb_core_transfer.h"

static volatile uint8_t s_cnt[4] = {0}; // A,B,C,D
static uint8_t s_lc_buf[7] = {0x00, 0xC2, 0x01, 0x00, 0, 0, 8}; // 115200bps 8N1

static void diagInc(uint8_t i) { if (s_cnt[i] < 9) s_cnt[i]++; }
static void diagSetLcDone(void) { diagInc(1); }
static void diagGetLcDone(void) { diagInc(3); }

static RETURN_CODE_t diagClassRequest(USB_SETUP_REQUEST_t *req)
{
    if (req->bmRequestType.type == USB_REQUEST_TYPE_CLASS)
    {
        if (req->bRequest == USB_CDC_REQUEST_SET_LINE_CODING &&
            req->bmRequestType.dataPhaseTransferDirection == USB_REQUEST_DIR_OUT)
        {
            diagInc(0);
            return USB_TransferControlDataSet(s_lc_buf, sizeof(s_lc_buf), diagSetLcDone);
        }
        if (req->bRequest == USB_CDC_REQUEST_GET_LINE_CODING &&
            req->bmRequestType.dataPhaseTransferDirection == USB_REQUEST_DIR_IN)
        {
            diagInc(2);
            return USB_TransferControlDataSet(s_lc_buf, sizeof(s_lc_buf), diagGetLcDone);
        }
    }
    return USB_CDCRequestHandler(req);
}

int main(void)
{
    SYSTEM_Initialize();
    RTC_SetPITIsrCallback(oneSecHandler);
    // PITを8Hzにする (点滅表示用)
    while (RTC.PITSTATUS > 0) ;
    RTC.PITCTRLA = RTC_PERIOD_CYC4096_gc | RTC_PITEN_bm;
    DELAY_milliseconds(100);
    sei();
    DELAY_milliseconds(100);
    USB_Comm_Initialize();
    USB_ClassRequestCallbackRegister(diagClassRequest);

    // 表示: 数n → n回の短い点滅(1tick点灯+3tick消灯)。n=0 → 4tick点灯。各数の後に8tick休み、最後に24tick休み
    uint8_t idx = 0, phase = 0;
    while (true)
    {
        if (s_tick)
        {
            s_tick = false;
            uint8_t n = s_cnt[idx];
            uint8_t body = (n == 0) ? 4 : (uint8_t)(n * 4);
            uint8_t gap = (idx == 3) ? 24 : 8;
            bool on = (phase < body) && ((n == 0) || (phase % 4) == 0);
            if (on) turnOnGreenLED(); else turnOffGreenLED();
            if (++phase >= body + gap) { phase = 0; idx = (uint8_t)((idx + 1) & 3); }
        }
        USB_Comm_Task();
    }
}
#elif DIAG_LEVEL > 0
int main(void)
{
    SYSTEM_Initialize();
#if DIAG_LEVEL >= 2
    ADC0_Disable();
    SPI0_Open(SPI0_DEFAULT);
    EM_loadEEPROM();
    Measurement_Initialize();
    Tof_PowerOff();
#endif
    RTC_SetPITIsrCallback(oneSecHandler);
#if DIAG_LEVEL >= 3
    SW_SetInterruptHandler(buttonHandler);
    VBUS_DET_SetInterruptHandler(vbusHandler);
#endif
    DELAY_milliseconds(100);
    sei();
    DELAY_milliseconds(100);
    USB_Comm_Initialize();
    s_mode = MODE_USB;

    while (true)
    {
        if (s_tick)
        {
            s_tick = false;
            g_epoch++;
#if DIAG_LEVEL >= 3
            blinkGreenLEDHeartbeat();
#else
            turnToggleGreenLED();
#endif
        }
        USB_Comm_Task();
    }
}
#else
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
#endif // DIAG_LEVEL

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="ユーティリティ">

void     Clock_Set(uint32_t epoch) { g_epoch = epoch; }
uint32_t Clock_Get(void)           { return g_epoch; }

// 電池電圧 [mV]。PF5(AIN21, MCCのピン名 VBAT)に電池+が直結。基準電圧 内部2.048V、10bit×64回積算 (MCC設定)。
// mV = acc * 2048 / (1024*64) = acc / 32
uint16_t getBatteryVoltage_mV(void)
{
    ADC0_Enable();
    DELAY_microseconds(100); // 基準電圧の立ち上がり待ち
    ADC0_ChannelSelect(VBAT);
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

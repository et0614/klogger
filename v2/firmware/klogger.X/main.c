/**
 * @file main.c
 * @brief AVR(AVR128DB32)を使用した距離計ロガープログラム
 * @author E.Togashi
 *
 * Microchip Studio版(main.cpp)の MPLAB X / MCC Melody 移植版。
 * ペリフェラル初期化はSYSTEM_Initialize()（MCC生成）が行い、
 * 割り込み処理はMCCのコールバック登録機構を使う。
 *  - 1秒周期処理   : RTC PIT       → pit_1sec_callback()
 *  - 10ms周期処理  : TCA0 OVF      → tca0_10ms_callback() (FatFs用)
 *  - 距離計割り込み : PD4立ち上がり → distance_ready_callback()
 *  - UART          : MCC生成リングバッファ(USART0_Read/Write)
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/atomic.h>

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "mcc_generated_files/system/system.h"
#include "mcc_generated_files/timer/rtc.h"
#include "mcc_generated_files/timer/tca0.h"
#include "mcc_generated_files/timer/delay.h"
#include "mcc_generated_files/adc/adc0.h"

#include "VL53L1X/VL53L1X_api.h"	//距離計
#include "VL53L1X/VL53L1X_calibration.h"

//FatFs関連
#include "ff/ff.h"
#include "ff/diskio.h"

//EEPROM
#include "eeprom_manager.h"

//関数前方宣言************************************
static void configure_twi_extra(void);
static bool initialize_sensor(void);
static bool configure_sensor(void);
static bool isLowBattery(void);
static void sleepVL53L(void);
static void wakeupVL53L(void);
static void handle_uart_command(const char* command);
static void parse_settime_command(const char* command);
static void parse_calibrate_command(const char* command);
static void uart_send_chars(const char* data);

//割り込みコールバック
static void pit_1sec_callback(void);
static void tca0_10ms_callback(void);
static void distance_ready_callback(void);

//以下はinline関数************************************
static inline bool checkStatus(VL53L1X_ERROR status);
static inline void turnOnGreenLED(void);
static inline void turnOffGreenLED(void);
static inline void blinkGreenLED(int iterNum);

//定数宣言************************************************************
#define WRITE_INTERVAL_SEC      60   // SDカードへの書き込み間隔 [sec]
#define STARTUP_IDLE_TIME_SEC   60   // 起動後にUART待機する時間 [sec]
#define LOW_BATTERY_LIMIT_SEC   10   // 電圧低下を許容する時間 [sec]
#define MEASURE_INTERVAL        1000 //通常・SD記録時の計測間隔 [msec]
#define CONTINUOUS_INTERVAL     100  //連続計測時の計測間隔 [msec] (100ms = 1秒に10回)
#define DEV                     0x29 //VL53L1Xのアドレス（結局使わないが、とりあえずサンプルに合わせておく）

//広域変数定義********************************************************
volatile time_t current_time_t = 0; //現在時間（ff/rtc.cからも参照する）
static volatile bool isTimeSet = false; //時刻が設定されたかを管理するフラグ
static volatile bool initFM = false; //Flash Memory初期化フラグ
static volatile bool fmOutputRequest = false; //Flash Memory書き出しフラグ
static volatile bool hasMeasuredData = false; //距離計測結果有りフラグ
static volatile bool calibrating = false; //キャリブレーション中フラグ
static volatile uint8_t blinkCount = 0; //FM書き出し時のLED点滅時間間隔保持変数
static volatile bool continuous_measure = false; //連続測定

//Flashメモリ
static FATFS fSystem;
static FIL fl;

//UART通信可能時間
static volatile uint16_t startUpTime = STARTUP_IDLE_TIME_SEC;

//文字列配列
static char charBuff[15 * WRITE_INTERVAL_SEC + 1 + 50]; //"HH:mm:ss,dist\r\n"(15文字) x WRITE_INTERVAL_SEC + \0 = +ゆとりとして50 words

//UARTコマンド受信用バッファ
#define UART_COMMAND_BUFFER_SIZE 64
static char uart_command_buffer[UART_COMMAND_BUFFER_SIZE];
static uint8_t uart_command_len = 0;

int main(void)
{
	//ペリフェラル初期化（クロック・ピン・ADC・I2C・SPI・TCA0・NVM・RTC・USART・VREF・CPUINT）
	SYSTEM_Initialize();

	//MCCでは設定できないTWI追加設定（SDAホールド50ns・バスタイムアウト200us）
	configure_twi_extra();

	//EEPROM読み込み
	EepromManager_loadEEPROM();

	//割り込みコールバック登録
	RTC_SetPITIsrCallback(pit_1sec_callback);           //1秒周期（ロギング）
	TCA0_OverflowCallbackRegister(tca0_10ms_callback);  //10ms周期（FatFs）
	IO_PD4_SetInterruptHandler(distance_ready_callback);//距離計データレディ

	//距離計起動（XSHUT=High。PIN_MANAGERで初期Highだが明示的に設定）
	wakeupVL53L();

	//初期設定が終わったら少し待機
	DELAY_milliseconds(100);

	//割り込み開始
	sei();

	//距離計の設定**********************
	initialize_sensor();
	configure_sensor();

	//計測開始
	if(checkStatus(VL53L1X_SetInterMeasurementInMs(DEV, MEASURE_INTERVAL))) return -1; //計測時間間隔
	if(checkStatus(VL53L1X_StartRanging(DEV))) return -1;

	//10秒以上電圧不足時間が継続したら終了
	uint16_t distance = 0;
	uint8_t lowBatteryTime = 0;
	uint16_t fileNum = 0; //ファイル番号
	while (lowBatteryTime <= LOW_BATTERY_LIMIT_SEC)
	{
		//起動直後のUARTコマンド受信処理
		if (startUpTime > 0) {
			while (USART0_IsRxReady()) {
				char c = (char)USART0_Read();
				// Enterキー(CRまたはLF)が押されたらコマンド処理
				if (c == '\r' || c == '\n') {
					if (uart_command_len > 0) {
						uart_command_buffer[uart_command_len] = '\0';
						// コマンド処理関数を呼び出す
						handle_uart_command(uart_command_buffer);
						uart_command_len = 0; // バッファクリア
					}
				}
				// バッファに文字を追加
				else if (uart_command_len < UART_COMMAND_BUFFER_SIZE - 1)
					uart_command_buffer[uart_command_len++] = c;
			}
		}

		//連続測定時
		if(continuous_measure)
		{
			//連続測定中はUART通信を有効にする
			startUpTime = STARTUP_IDLE_TIME_SEC;

			if(hasMeasuredData && !calibrating){
				hasMeasuredData = false;
				checkStatus(VL53L1X_GetDistance(DEV, &distance));
				checkStatus(VL53L1X_ClearInterrupt(DEV)); //次の計測に備えさせる

				snprintf(charBuff, sizeof(charBuff), "MES %u\r\n", distance);
				uart_send_chars(charBuff);
			}
		}
		else
		{
			//マウントできていなければとにかくマウント
			if(!initFM)
				initFM = (f_mount(&fSystem, "", 1) == FR_OK);

			//SD書き出しフラグがあれば書き込む
			if(fmOutputRequest){
				fmOutputRequest = false;

				//電圧確認（限界まで使うとFlashメモリのデータが破損することがある）
				if(isLowBattery()) lowBatteryTime++;
				else lowBatteryTime = 0;

				//距離計読み込みフラグがあれば読む:未計測であれば以前のデータを使う
				if(hasMeasuredData && !calibrating){
					hasMeasuredData = false;
					checkStatus(VL53L1X_GetDistance(DEV, &distance));
					checkStatus(VL53L1X_ClearInterrupt(DEV)); //次の計測に備えさせる
				}

				//日時を作成
				time_t time_snapshot;
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
					time_snapshot = current_time_t;
				}
				struct tm time_tm;
				gmtime_r(&time_snapshot, &time_tm);

				//バッファの末尾に加える
				size_t len = strlen(charBuff);
				if(isTimeSet)
					snprintf(charBuff + len, sizeof(charBuff) - len, "%02d:%02d:%02d,%u\r\n",
						time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec, distance);
				else snprintf(charBuff + len, sizeof(charBuff) - len, "%lu,%u\r", (unsigned long)time_snapshot, distance);

				//WRITE_INTERVAL_SEC[sec]ごとに書き出し
				if(time_snapshot % WRITE_INTERVAL_SEC == 0)
				{
					//書き出すファイル名称
					char fileName[13]={0}; //yyyyMMdd.csv
					if(isTimeSet) {
						time_t file_timestamp = time_snapshot > 0 ? time_snapshot - 1 : 0;
						struct tm file_tm;
						gmtime_r(&file_timestamp, &file_tm);
						snprintf(fileName, sizeof(fileName), "%04d%02d%02d.csv", file_tm.tm_year + 1900, file_tm.tm_mon + 1, file_tm.tm_mday);
					}
					else {
						snprintf(fileName, sizeof(fileName), "%04d.csv", fileNum);
						//必要に応じてファイル名称を更新
						if(time_snapshot == WRITE_INTERVAL_SEC){
							FILINFO fno;
							for (int fn = fileNum; fn <= 9999; fn++) {
								snprintf(fileName, sizeof(fileName), "%04d.csv", fn);

								// ファイルの存在を確認
								if(f_stat(fileName, &fno) == FR_NO_FILE) {
									fileNum = fn;
									break;
								}
							}
						}
					}

					if(f_open(&fl, fileName, FA_OPEN_APPEND | FA_WRITE) == FR_OK)
					{
						bool writeOk = (0 <= f_puts(charBuff, &fl));	/* EOF(-1) on write error */
						if(f_close(&fl) != FR_OK) writeOk = false;
						if(writeOk) charBuff[0] = '\0';	/* clear only after the data is safely on the card */
						else initFM = false;	/* write failed -> force re-mount / re-init */
					}
					else initFM = false; //エラー時は再マウントを試みる

				}

				//86400たまったら戻す
				if(!isTimeSet && 86400 <= time_snapshot) {
					ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
						current_time_t = 0;
					}
				}
			}
		}

		//スリープ処理
		if(startUpTime) set_sleep_mode(SLEEP_MODE_IDLE); //起動してしばらくはUART通信のために浅いスリープ
		else set_sleep_mode(SLEEP_MODE_PWR_DOWN); //その後、省電力化のための深いスリープ
		sleep_mode();
	}

	cli(); //割り込み終了
	return 0;
}

/**
 * @brief MCC(Melody)のUIでは設定できないTWI1の追加設定を行う
 * SDAホールド時間50ns・バスタイムアウト200usを現行版（Microchip Studio版）に合わせる
 */
static void configure_twi_extra(void)
{
	TWI1.MCTRLA &= ~TWI_ENABLE_bm;          //一旦停止
	TWI1.CTRLA |= TWI_SDAHOLD_50NS_gc;      //SDAホールド時間 50ns
	TWI1.MCTRLA |= TWI_TIMEOUT_200US_gc;    //バスタイムアウト 200us
	TWI1.MCTRLA |= TWI_ENABLE_bm;           //再開
	TWI1.MSTATUS = TWI_BUSSTATE_IDLE_gc;    //バスをIDLE状態へ
}

//センサ初期化
static bool initialize_sensor(void){
	uint8_t hasBooted = 0;
	while(hasBooted == 0){
		if (checkStatus(VL53L1X_BootState(DEV, &hasBooted))) return -1;
		DELAY_milliseconds(100);
	}
	if(checkStatus(VL53L1X_SensorInit(DEV))) return -1; //初期化

	return 0;
}

//センサ設定
static bool configure_sensor(void){
	if(checkStatus(VL53L1X_SetOffset(DEV, kSettings.offset))) return -1; //距離補正
	if(checkStatus(VL53L1X_SetDistanceMode(DEV, 1))) return -1; //距離モード（1=short, 2=long）
	if(checkStatus(VL53L1X_SetTimingBudgetInMs(DEV, 15))) return -1; //計測時間（15, 20, 50, 100, 200, 500）
	if(checkStatus(VL53L1X_SetROI(DEV,4,4))) return -1; //視野角（4-16で小さいほど狭い。小さいと計測距離が短くなるらしい）

	return 0;
}

//距離計データレディ割り込み（PD4立ち上がり：MCC生成ISRから呼ばれる）
static void distance_ready_callback(void)
{
	hasMeasuredData = true;
	//割り込みフラグはpins.cのISRが解除する
}

//FatFS用0.01secタイマ割り込み（TCA0 OVF：MCC生成ISRから呼ばれる）
static void tca0_10ms_callback(void)
{
	disk_timerproc();	/* Drive timer procedure of low level disk I/O module */
	//割り込みフラグはtca0.cのISRが解除する
}

//ロギング用の1秒毎の処理（RTC PIT：MCC生成ISRから呼ばれる）
static void pit_1sec_callback(void)
{
	//割り込み要求フラグはrtc.cのISRが解除する

	if(0 < startUpTime)	startUpTime--; //起動時間
	current_time_t++; //1秒進める

	//連続計測中は10秒毎に2回点滅
	if(continuous_measure) {
		blinkCount++;
		if(10 <= blinkCount)
		{
			blinkGreenLED(2);
			blinkCount = 0;
		}
	}
	//マウント未了の場合には毎秒1回点滅
	else if(!initFM) blinkGreenLED(1);
	//マウント済の場合は10秒毎に1回点滅
	else{
		blinkCount++;
		if(10 <= blinkCount)
		{
			blinkGreenLED(1);
			blinkCount = 0;
		}
	}

	//距離測定+microSD書き出しフラグ
	fmOutputRequest = true;
}

//電池残量が小さくなったか否か
static bool isLowBattery(void)
{
	ADC0_ChannelSelect(ADC0_CHANNEL_AIN5); //AIN5(PD5)を計測
	DELAY_milliseconds(5);
	ADC0_ConversionStart(); //変換開始
	while (!ADC0_IsConversionDone()); //変換終了待ち

	// 電圧をミリボルト単位の整数で計算する (3.3V -> 3300mV)
	// RESは最大16368 (10bit分解能1023 * 16回平均) なので、32bitで計算すればオーバーフローしない
	uint16_t res = (uint16_t)ADC0_ConversionResultGet();
	uint32_t vdd_mV = (3300UL * res) / 16384;

	// 1.0V (1000mV) を下回ったら電池残量不足
	return vdd_mV < 1000;
}

//VL53Lを休止させる
static void sleepVL53L(void){
	IO_XSHT_SetLow();
}

//VL53Lを起動する
static void wakeupVL53L(void){
	IO_XSHT_SetHigh();
}

//文字配列を送信（MCC生成の送信リングバッファに積む）
static void uart_send_chars(const char* data)
{
	for(int i = 0; data[i] != '\0'; i++){
		while (!USART0_IsTxReady()); //送信バッファ満杯時は空き待ち
		USART0_Write((uint8_t)data[i]);
	}
}

static void handle_uart_command(const char* command) {
	//コマンドを受けるたびにUART通信待機時間を延長
	startUpTime = STARTUP_IDLE_TIME_SEC;

	// IDコマンド
	if (strcmp(command, "ID?") == 0)
		uart_send_chars("K-Logger v2.2\r\n");
	// SETTIMEコマンド
	else if (strncmp(command, "SETTIME ", 8) == 0)
		parse_settime_command(command);
	// CALIBRATEコマンド
	else if (strncmp(command, "CALIBRATE ", 10) == 0)
		parse_calibrate_command(command);
	// 連続測定コマンド
	else if (strncmp(command, "CMEASURE", 8) == 0)
	{
		VL53L1X_StopRanging(DEV);
		VL53L1X_SetInterMeasurementInMs(DEV, CONTINUOUS_INTERVAL); // 高速モード(100ms)へ
		VL53L1X_StartRanging(DEV);

		continuous_measure = true;
		uart_send_chars("Start continuous measuring\r\n");
	}
	// 連続測定停止コマンド
	else if (strncmp(command, "STOP", 4) == 0)
	{
		VL53L1X_StopRanging(DEV);
		VL53L1X_SetInterMeasurementInMs(DEV, MEASURE_INTERVAL); // 通常モード(1000ms)へ
		VL53L1X_StartRanging(DEV);

		continuous_measure = false;
		uart_send_chars("Stop continuous measuring\r\n");
	}
	// 不明なコマンド
	else
		uart_send_chars("Unknown command\r\n");
}

/**
 * @brief 文字列先頭の10進数を読み取る軽量パーサ（sscanfはコードサイズが大きいため不使用）
 * @param p [in/out] 解析位置。成功時は数字の直後まで進む
 * @param out [out] 読み取った値
 * @return 1文字以上の数字を読めたらtrue
 */
static bool parse_uint(const char** p, int* out) {
	const char* s = *p;
	int value = 0;
	bool hasDigit = false;
	while ('0' <= *s && *s <= '9') {
		value = value * 10 + (*s - '0');
		s++;
		hasDigit = true;
	}
	*p = s;
	*out = value;
	return hasDigit;
}

/**
 * @brief 区切り文字を1つ読み飛ばす
 * @return 期待した区切り文字ならtrue
 */
static bool parse_sep(const char** p, char sep) {
	if (**p != sep) return false;
	(*p)++;
	return true;
}

static void parse_settime_command(const char* command) {
	if (strncmp(command, "SETTIME ", 8) == 0) {
		struct tm new_time_tm;
		memset(&new_time_tm, 0, sizeof(new_time_tm)); // 構造体をゼロクリア
		const char* time_str = command + 8;

		// "yyyy-MM-ddTHH:mm:ss"を解析（一旦int変数に格納）
		int year, mon, mday, hour, min, sec;
		if (parse_uint(&time_str, &year) && parse_sep(&time_str, '-') &&
			parse_uint(&time_str, &mon)  && parse_sep(&time_str, '-') &&
			parse_uint(&time_str, &mday) && parse_sep(&time_str, 'T') &&
			parse_uint(&time_str, &hour) && parse_sep(&time_str, ':') &&
			parse_uint(&time_str, &min)  && parse_sep(&time_str, ':') &&
			parse_uint(&time_str, &sec)) {

			// 読み取った値をtm構造体の仕様に合わせる
			new_time_tm.tm_year = year - 1900; // 年: 1900年からの年数
			new_time_tm.tm_mon = mon - 1;      // 月: 0-11 (0が1月)
			new_time_tm.tm_mday = mday;
			new_time_tm.tm_hour = hour;
			new_time_tm.tm_min = min;
			new_time_tm.tm_sec = sec;

			// tm構造体からtime_tに変換
			time_t new_time = mktime(&new_time_tm);

			if (new_time != (time_t)-1) { // 変換成功
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
					current_time_t = new_time;
					isTimeSet = true;
				}
				uart_send_chars("Time set OK\r\n");
				charBuff[0] = '\0'; //未保存データを削除
			}
			else uart_send_chars("Time conversion failed\r\n");
		}
		else uart_send_chars("Invalid time format\r\n");
	}
}

static void parse_calibrate_command(const char* command) {
	int dist_mm;
	// Extract the distance from the command string ("CALIBRATE "は10文字)
	const char* num_str = command + 10;
	if (parse_uint(&num_str, &dist_mm)) {

		//入力値が10mm～500mmの範囲内かチェック
		if (dist_mm < 10 || 500 < dist_mm) {
			uart_send_chars("ERROR: Distance must be between 10 and 500 mm.\r\n");
			return; // 処理を中断して終了
		}

		// Stop ranging before calibration
		VL53L1X_StopRanging(DEV);
		initialize_sensor();

		// Perform calibration
		int16_t offset;
		calibrating = true;
		VL53L1X_ERROR status = VL53L1X_CalibrateOffset(DEV, (uint16_t)dist_mm, &offset);
		calibrating = false;

		if (status == 0) {
			// Save the new offset
			kSettings.offset = offset;
			EepromManager_writeEEPROM();

			// 送信文字列："CAL OK, offset:[値]"
			char response_buffer[32];
			snprintf(response_buffer, sizeof(response_buffer), "CAL OK, offset:%d\r\n", offset);
			uart_send_chars(response_buffer);
		}
		else uart_send_chars("CAL FAIL\r\n");

		// Restart ranging
		configure_sensor();
		VL53L1X_SetInterMeasurementInMs(DEV, MEASURE_INTERVAL); //計測時間間隔
		VL53L1X_StartRanging(DEV);
	}
	else uart_send_chars("Invalid calibrate format\r\n");
}

//以下はinline関数************************************
static inline bool checkStatus(VL53L1X_ERROR status){
	if(status != 0){
		blinkGreenLED(2);
		return 1;
	}
	return 0;
}

static inline void turnOnGreenLED(void)
{
	IO_LED_SetHigh(); //点灯
}

static inline void turnOffGreenLED(void)
{
	IO_LED_SetLow(); //消灯
}

static inline void blinkGreenLED(int iterNum)
{
	if(iterNum < 1) return;

	//初回
	turnOffGreenLED(); //一旦必ず消灯して
	//点滅
	for(int i=0;i<iterNum;i++)
	{
		DELAY_milliseconds(100);
		turnOnGreenLED(); //点灯
		DELAY_milliseconds(15);
		turnOffGreenLED(); //消灯
	}
}

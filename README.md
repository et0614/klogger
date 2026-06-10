# K-Logger — 扉開閉記録装置

VL53L1X ToF距離センサで扉の開閉を検出し、タイムスタンプ付きで microSD に記録するデータロガー。
詳細仕様は「[扉開閉記録装置K-Logger_説明書.pdf](扉開閉記録装置K-Logger_説明書.pdf)」を参照。

## ディレクトリ構成

```
firmware/klogger.X/   MPLAB X プロジェクト（ファームウェア）
hardware/             回路図・基板 (Eagle)、BOM、ガーバー
software/             PC側操作ツール
  windows-console/    PowerShell + バッチファイル版
  python/             Python 版 (クロスプラットフォーム)
```

## ファームウェア開発環境

| 項目 | 内容 |
|---|---|
| MCU | AVR32DB32 (Flash 32KB / RAM 4KB / EEPROM 512B, F_CPU = 4MHz) |
| IDE | MPLAB X IDE v6.30 + MCC Melody (設定ファイル: `klogger.mc3`) |
| コンパイラ | XC8 v3.10 (Free ライセンス, -O2 + リンカ `-mrelax`) |
| DFP | AVR-Dx_DFP 2.7.321 |
| 書き込み | UPDI (PICkit 等) |

ビルドは MPLAB X でプロジェクト `firmware/klogger.X` を開いて Clean and Build。
Flash 使用量が上限 32KB の約 90% のため、機能追加時はサイズに注意。

### 主要ペリフェラル

- USART0 (PA0/PA1): 9600 bps 8N1 — PC との通信
- TWI1 (PF2/PF3): VL53L1X 距離センサ (XSHUT=PD3, INT=PD4)
- SPI1 (PC0–PC2, CS=PC3): microSD (FatFs + ChaN MMC ドライバ)
- RTC: 外部 32.768kHz クリスタル、PIT 1 秒周期で時刻管理
- ADC0 (AIN5=PD5): 電源電圧監視
- PD2: 状態表示 LED

## UART コマンド

起動後 60 秒間（コマンド受信ごとに延長）だけ UART 待機する。9600 bps 8N1、行末 CR+LF。

| コマンド | 応答 | 説明 |
|---|---|---|
| `ID?` | `K-Logger v2.2` | 機器識別 |
| `SETTIME yyyy-mm-ddTHH:MM:SS` | `Time set OK` | 時計設定 |
| `CALIBRATE <mm>` | `CAL OK, offset:<n>` | 距離校正 (10–500 mm) |
| `CMEASURE` | `Start continuous measuring` 後、`MES <mm>` を連続送出 | 連続測定開始 |
| `STOP` | `Stop continuous measuring` | 連続測定停止 |

## PC 側ツール

USB-シリアル変換は CP210x (`software/CP210x_Universal_Windows_Driver.zip`)。

- **Windows**: `software/windows-console/` の `SetTime.bat` / `Calibrate.bat` / `Measure.bat`
- **Python**: `software/python/` の `klogger_tool.py`（要 pyserial、使い方は同フォルダの README 参照）

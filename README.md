# K-Logger v3 — 扉開閉記録装置

VL53L1X ToF距離センサで扉の開閉を検出し、内蔵フラッシュメモリに記録するデータロガー。
v2 からの主な変更点は以下のとおり（開発中）。

| 項目 | v2 | v3 |
|---|---|---|
| MCU | AVR32DB32 | AVR64DU32（ネイティブUSB） |
| PC通信 | CP2102N（USB-UART、要ドライバ） | USB-CDC（ドライバ不要） |
| 記録媒体 | microSD（FatFs、CSV） | W25Q128 16MB（バイナリ、PCツールでCSV化） |
| 電源 | 単3×1 | 単4×1、USB接続中はUSB給電（自動切替） |
| USB端子 | micro-B | Type-C |
| 電源スイッチ | スライド | なし（ボタン長押しでソフトOFF） |
| 基板CAD | Eagle | Fusion |

旧版（v2）は [v2/](v2/README.md) を参照。

![K-Logger v3 基板（3D）](docs/images/3d_top.png)

## ディレクトリ構成

```
docs/
  images/               基板・3D外観画像
hardware/
  BOM.xlsx              設計用部品表（JLCPCB部品番号・リンク付き）
  datasheets/           主要部品データシート（ファイル名=参照記号）
  pcb/                  Fusion設計データ(klogger.f3z)、回路図PDF
  fab/                  JLCPCB入稿データ（ガーバー、JLCPCB_BOM.xlsx、JLCPCB_CPL.xlsx）
firmware/klogger.X/     MPLAB X プロジェクト（AVR64DU32, MCC Melody: klogger.mc3）
software/python/        PCツール: klogger_gui.py（GUI）、klogger.py（コマンド版・通信ライブラリ）、build_exe.bat（exe化）
v2/                     旧版 K-Logger v2 一式（凍結）
```

## バージョン

- 製品世代: K-Logger v3（v2 の後継）
- 基板: 3.1（シルク表記「Version 3.1」。3.0 からはシルクのみ変更、履歴は `hardware/fab/README.md`）。修正ごとに 3.2, 3.3 … と上げる。リポジトリには最新版のみを置き、過去の版は Git の履歴で参照する
- ファームウェア: 現在 3.1.0（`hello` 応答の `fw`。`firmware/klogger.X/version.h`）

## 使い方

### 動作モード
| 状態 | 入り方 | 動作 | LED |
|---|---|---|---|
| USB通信 | USBを挿す | PCツールからの操作を受け付ける（USB給電） | 1秒ごとに2回点滅 |
| 記録 | PCツールで記録開始 | 記録間隔ごとに距離を記録。USBを抜いても続ける | 消灯 |
| 待機（ソフトOFF） | USB無しで電池を入れる / 記録中にボタン5秒長押し / 電池切れ | 何もしない（省電力） | 消灯 |

- ボタンを押している間はLEDが点灯する（生存確認）。5秒長押しでリセット（記録中なら記録停止）。
- 記録中にUSBを挿すと記録を止めてUSB通信モードに戻る（データは保存される）。
- 記録開始時に既存データは消去される。先に読み出しておくこと。
- 電池を抜くと、まだフラッシュに書いていない最大128件（1秒間隔で約2分）が失われる。

### 消費電流・電池寿命
単4アルカリ1本、記録間隔1秒で記録中の電流は電池側で約0.5mA（実測）。約2か月の連続記録が目安。
電流の大部分は距離センサの測距で、記録間隔を延ばすとほぼ比例して減る。

### PCツール（GUI）
[Releases](https://github.com/et0614/klogger/releases) から `KLogger.exe` をダウンロードしてダブルクリックで起動する（Pythonは不要）。
署名の無いexeのため、初回に Windows SmartScreen が出たら「詳細情報」→「実行」を選ぶ。
Pythonがある場合は `pip install pyserial` の後に `python klogger_gui.py` でも起動できる。画面表示は英語。

1. USBを挿して **Connect**（ポートは自動検出）。上部に機器情報（FW、機体ID、電池電圧、保存件数、記録期間、測距条件）が出る。
2. **Logging** タブ: 記録間隔を入れて **Start logging**。時刻はPCに合わせられ、既存データは消去される。開始すると機器はUSBから切り離されるので、そのまま抜いて設置する。
3. 回収したらUSBを挿して（記録は自動で止まる）**Connect**。**Data** タブの **Save as CSV** で保存する（v2互換の日別CSVも選べる）。
4. **Live view** タブ: 距離をリアルタイム表示（設置位置の確認用）。
5. **Settings** タブ: 測距モード（Short/Long）、タイミングバジェット、ROI、自己診断。

### 距離校正（GUIの Calibration タブ）
距離のオフセット（測定値のずれ）を補正する。結果は機器に保存される。

1. 先に **Settings** タブで、実際に使う測距条件にしておく（条件を変えたら校正し直す）。
2. グレーの反射板（反射率17%前後。写真用18%グレーカードなど、10cm角以上）をセンサ正面に垂直に置く。距離は140mmを推奨。直射日光は避ける。
3. 目標距離（Target distance）を入力し **Start live reading**。直近20回の平均・ばらつき（SD）・目標との差が表示される。
4. ランプが緑（全数が有効かつ SD 3mm 以下）になったら **Calibrate**。50回測定して オフセット = 目標距離 − 平均 を保存する。
5. 校正後は補正済みの距離が表示されるので、反射板を別の距離に動かしてずれを確認する。結果が悪ければ **Restore previous offset**（元に戻す）や **Reset offset to 0** を使う。

### PCツール（コマンド版 `software/python/klogger.py`）
`pip install pyserial` の後、USBを挿して実行する（COMポートは自動検出）。

```
python klogger.py info              機器情報・状態・電池電圧
python klogger.py start -i 1        時刻合わせ＋記録開始（記録間隔1秒）
python klogger.py read              保存データをCSVに保存（--daily フォルダ でv2互換の日別CSV）
python klogger.py measure           距離を連続表示（保存しない）
python klogger.py calibrate 140     オフセット校正（対象物を140mmに置く）
python klogger.py tof --mode long --tb 33 --roi 8   測距条件の変更（--offset でオフセットを直接設定）
python klogger.py selftest          自己診断
python klogger.py clear             保存データの消去
```

exeの作り方: `pip install pyinstaller` の後に `build_exe.bat` を実行すると `dist\KLogger.exe` ができる。

### 記録データ形式
1レコード2バイト（W25Q128に連続保存、16MBで約838万件＝1秒間隔で約97日）。
bit15–13が状態コード（0=有効, 1=sigma fail, 2=signal fail, 3=out of bounds, 4=wraparound, 5=min range, 6=その他）、
bit12–0が距離[mm]。0xFFFEは計測失敗。時刻は 記録開始時刻 + index × 記録間隔 で復元する。

## 開発メモ
- USB: AVR64DU のエラッタ 2.8.1（マルチパケットOUT転送は偶数アドレス必須）により、MCC生成の `usbCDCLineCoding` が奇数アドレスに置かれるとPCでCOMポートが開けなくなる。`usb_comm.c` で偶数アドレスのバッファを使って回避している。MCCで再生成しても消えない。
- センサの GPIO1（10kプルアップ、アクティブLow）は測定完了の立ち下がり割り込み（PA1）ですぐ読み取って解除する。放置するとプルアップに電流が流れ続ける。

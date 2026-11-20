# K-Logger 用 USB ブートローダ

K-Logger v3 (AVR64DU32) のファームウェアを USB 経由で書き換えるためのブートローダ。
初回だけ PICkit でブートローダを書き込めば、以降のファームウェア更新は USB と PCツール (または `avrdude`) だけで済む。

## 出典

[askn37/euboot](https://github.com/askn37/euboot) (MIT License, Copyright (c) 2024 askn (K.Sato) multix.jp) を取り込み、
`Makefile` に K-Logger のピン配置の variant `BUILTIN_LC3_SA7` を追加したもの。ソースコード (`euboot/`, `gencrc.pl`) と `LICENSE` は無改変。
オリジナルの説明は `README_euboot.md` / `README_euboot_jp.md` を参照。K-Logger (`c-logger/firmware/bootloader`) と同じ構成。

## ピン配置

| 役割 | ピン | K-Logger での用途 |
|---|---|---|
| ブートローダ起動スイッチ (SW_BUILTIN) | PA7 | ボタン (プルアップ、押すと GND) |
| 進捗表示 LED (LED_BUILTIN) | PC3 | 赤 LED |

ブートローダに入るのは、**ボタンを押したまま起動したとき**（電源投入・リセットとも）。そのため、アプリでボタン長押しによるリセットをするときは、ボタンが離されるまで待ってからリセットする（`main.c` の `handleButton` で対応済み）。

ブートローダ動作中の LED: 1回点滅 = USB 接続待ち、2回点滅 = 待機（PC からの通信待ち）、4回点滅 = 読み書き中。

USB の VID:PID は euboot の既定値 `04D8:0B12`（Microchip にブートローダ用の PID を申請中。取得後に `euboot/src/configuration.h` の `CONFIG_USB_VIDPID` を変える）。

## ビルド (WSL)

arduino-cli と modernAVR SDK が必要（セットアップは M-Logger の `firmware/bootloader/README.md` を参照）。

```sh
cd /mnt/c/git_repositories/klogger/firmware/bootloader
make hex/euboot_LC3_SA7.hex
```

`hex/euboot_LC3_SA7.hex`（2368 バイト）と `hex/euboot_LC3_SA7.fuse` ができる。

## 初回の書き込み (PICkit)

MPLAB IPE か avrdude で `hex/euboot_LC3_SA7.hex` を書き込む。その後、ファームウェア (`klogger.X`) を MPLAB X から書き込めばヒューズも `config_bits.c` の値（euboot の `.fuse` と同じ）になる。

```cmd
avrdude -cpkobn_updi -pavr64du32 -Uflash:w:hex\euboot_LC3_SA7.hex:i
```

## ファームウェア側 (`klogger.X`) の設定

ブートローダがフラッシュの先頭 `0x0000-0x09FF` (2.5 KB) を使うので、アプリを `0x0A00` 以降に置く。

1. **リンカ**: Project Properties → XC8 Linker → Additional options → Extra linker options に `-Wl,--defsym=__TEXT_REGION_ORIGIN__=0xA00`
2. **ヒューズ** (MCC の Configuration Bits): BOOTSIZE = 0x05、SYSCFG0 = 0xD9 (EESAVE 有効)。euboot の `.fuse` と一致させる
3. **MPLAB X から PICkit で書き込むとき**にブートローダを消さない設定: Project Properties → PICkit → Memories to Program で
   - Manually select memories and ranges
   - Preserve Program Memory: チェック、範囲 `0-9ff`
   - Preserve EEPROM Memory: チェック、範囲 `1400-14ff`（測距条件・オフセット・記録の設定を残す）

## 量産用の 1 ファイル (ブートローダ + ファームウェア + ヒューズ)

`klogger.X` を Build (production) した後に:

```sh
python build_production_hex.py
```

`hex/klogger_combined.hex` ができる（`.gitignore` 済み）。JLC の書き込みサービスや IPE にはこの 1 ファイルを渡す。

## USB でのファームウェア更新

1. 電池を外し、USB ケーブルも抜く。
2. ボタンを押したまま、USB ケーブルをつなぐ。
3. LED がブートローダの点滅になったら、ボタンを離す。
4. PCツール (GUI 版の Firmware タブ) で書き込む。コマンドなら:

   ```sh
   avrdude -Pusb:04d8:0b12 -cjtag3updi -pavr64du32 -D -Uflash:w:klogger.X.production.hex:i
   ```

   **`-D`（自動消去しない）が必須**。付けないとブートローダも消え、復旧に PICkit が必要になる。

5. 書き込みが終わったら USB を抜き差しすると、新しいファームウェアで起動する。

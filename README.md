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

## ディレクトリ構成

```
docs/                 仕様・設計メモ・取扱説明書
hardware/
  bom/BOM.xlsx        設計用部品表（JLCPCB部品番号・リンク付き）
  pcb/                Fusionアーカイブ(.f3z)、回路図PDF
  fab/rev-X/          JLCPCB入稿一式（ガーバー、JLCPCB_BOM、CPL）基板リビジョンごと
  datasheet/          主要部品データシート
firmware/klogger.X/   MPLAB X プロジェクト（AVR64DU32）
software/python/      PC側ツール（USB-CDC通信、データ読出し→CSV）
v2/                   旧版 K-Logger v2 一式（凍結）
```

## バージョン

- 製品世代: K-Logger v3（v2 の後継）
- ファームウェア: 3.0.0 から開始（`ID?` 応答例: `K-Logger v3.0.0`）

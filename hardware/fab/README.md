# K-Logger v3 基板 製造データ（JLCPCB入稿用）

基板バージョン: **3.1**（シルク表記「Version 3.1」）

リポジトリには最新版のみを置く。以前の版は Git の履歴（タグ）で参照する。基板を修正して再発注するときは 3.1, 3.2 … と上げ、シルク表記と下記を更新する。

| ファイル | 内容 |
|---|---|
| `klogger_gerber.zip` | ガーバー一式（Fusion Electronics 出力、2層、63 × 13 mm） |
| `JLCPCB_BOM.xlsx` | 入稿用BOM（Comment / Designator / Footprint / JLCPCB Part #） |
| `JLCPCB_CPL.xlsx` | 入稿用 部品配置データ（Designator / Mid X / Mid Y / Layer / Rotation） |

- BOM・CPL は Fusion の部品配置出力（参照記号・パッケージ・座標）と `hardware/BOM.xlsx`（部品番号）から作成。UPDIテストパッドは実装部品ではないため除外。
- 設計データ: `hardware/pcb/klogger.f3z`、回路図: `hardware/pcb/klogger_schematic.pdf`、外観: `docs/images/`

## 発注時の指定
- 板厚 **1.2 mm**（ガーバージョブファイルは 1.27 mm になっているので発注画面で指定する）
- USB-C（J1）のシェル足用長穴は外形レイヤ（GKO）のミリングで定義。**めっきスルーホール（PTH）として加工**されるよう備考で指定し、プレビューで確認する
- AVR64DU32（U1）は JLC在庫なし → Global Sourcing
- BT1（電池ホルダ）は裏面（Layer=Bottom）。JLCで実装すると両面実装になる。手はんだにする場合は BOM と CPL の BT1 行を削除
- 部品の向き（Rotation）は Fusion の値のまま。**JLCのプレビューで、特にU1〜U6・Q1・D1・D2・J1・X1の向きと1番ピン位置を確認**し、ずれていれば発注画面で補正する

## 版の履歴
| 版 | 日付 | 変更内容 |
|---|---|---|
| 3.1 | 2026-10-22 | 表面シルクのみ変更: ブランドロゴ「Popolo」、製品名「K-Logger」、「Togashi Research Institute LLC」を追加、版表記を「Version 3.1」に。銅箔・レジスト・メタルマスク・穴・外形・部品配置は 3.0 と同一のため、BOM・CPL は変更なし |
| 3.0 | 2026-10-08 | 初版（5台試作） |

- ロゴの元データ: popolo3 リポジトリの `assets/logo/`（基板には `popolo_silk_2.5mm.svg` を Fusion の「配置 > SVGを挿入」で tPlace に配置）

## 3.0 チェック結果（2026-10-08）
- 電池ホルダ位置決め穴 φ1.6（X6.70 Y1.13、基板端まで約0.33 mm）
- C12/C13 は 0603
- USB-C は基板端から約0.2 mm 突出、長穴 0.6×1.4 / 0.6×1.7 mm、位置決め穴 φ0.65

## 未対応
- [ ] 回路図の Value が未設定（コンデンサ・抵抗が「CAPACITOR0402」等、Q1=AO3401A、RN=EXB-28V103JX 表記）。入稿用BOMは BOM.xlsx から作成しているため発注には影響しない
- [ ] 回路図PDF（`hardware/pcb/klogger_schematic.pdf`）は修正前の版。Value修正後に再出力する

# K-Logger Python ツール

K-Logger を PC からシリアル経由で操作するツール。Windows / macOS / Linux で動作する。
`windows-console/` の PowerShell 版と同等の機能。

## セットアップ

```
pip install -r requirements.txt
```

Windows では CP210x ドライバ（`software/CP210x_Universal_Windows_Driver.zip`）が必要。

## 使い方

ポートは自動探索される（各ポートへ `ID?` を送って `K-Logger` 応答を確認）。
`--port COM3` のように明示指定も可能。

```
python klogger_tool.py settime               # 時刻設定 (PC の現在時刻を送信)
python klogger_tool.py calibrate 100         # 距離校正 (基準距離 100mm)
python klogger_tool.py measure               # 連続測定 (Ctrl+C で停止)
python klogger_tool.py measure -o log.csv    # 連続測定 + CSV 保存
python klogger_tool.py id                    # 機器識別のみ
```

## 注意

- K-Logger は**起動後 60 秒間**しか UART を待ち受けない（コマンド受信ごとに延長）。
  電源投入またはリセット直後に実行すること。
- `measure -o` の CSV には PC 側の受信時刻と距離 [mm] が記録される。

@echo off
rem K-Logger GUI を単一の exe にまとめる (PyInstaller)。
rem 事前に:  pip install pyserial pyinstaller
rem 出力:    dist\KLogger.exe  (Python の無いPCでもダブルクリックで起動できる)
rem ファームウェア更新 (Firmware タブ) 用に、このフォルダに avrdude.exe と avrdude.conf があれば exe に同梱する。
cd /d %~dp0
set EXTRA=
if exist avrdude.exe if exist avrdude.conf set EXTRA=--add-binary avrdude.exe;. --add-data avrdude.conf;.
python -m PyInstaller --noconfirm --onefile --windowed --name KLogger %EXTRA% klogger_gui.py

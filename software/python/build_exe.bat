@echo off
rem K-Logger GUI を単一の exe にまとめる (PyInstaller)。
rem 事前に:  pip install pyserial pyinstaller
rem 出力:    dist\KLogger.exe  (Python の無いPCでもダブルクリックで起動できる)
cd /d %~dp0
python -m PyInstaller --noconfirm --onefile --windowed --name KLogger klogger_gui.py

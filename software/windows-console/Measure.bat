@echo off

rem K-Loggerで連続計測を開始し、データを表示する

powershell -ExecutionPolicy Bypass -File "%~dp0K-Logger-Tool.ps1" -ContinuousMode

pause
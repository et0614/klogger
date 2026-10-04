@echo off
rem K-Loggerの距離センサーをキャリブレーションする。
rem 引数なしで実行すると、メーカー推奨の140mmで実行される。
rem 使い方: Calibrate.bat [距離[mm]]

set "DISTANCE=%1"

rem 引数が指定されていないかチェック
if "%DISTANCE%"=="" (
    echo No distance specified. Using default value: 140 mm.
    set "DISTANCE=140"
) else (
    echo Using specified distance: %DISTANCE% mm.
)

powershell -ExecutionPolicy Bypass -File "%~dp0K-Logger-Tool.ps1" -CalibrateDistance %DISTANCE%

pause
#!/usr/bin/env python3
"""K-Logger 操作ツール (Python 版)

windows-console/K-Logger-Tool.ps1 と同等の機能をクロスプラットフォームで提供する。

  python klogger_tool.py settime               時刻設定 (PC の現在時刻を送信)
  python klogger_tool.py calibrate 100         距離校正 (基準距離 mm を指定)
  python klogger_tool.py measure               連続測定 (Ctrl+C で停止)
  python klogger_tool.py measure -o log.csv    連続測定しつつ CSV に保存
  python klogger_tool.py id                    機器識別のみ

ポートは自動探索する (各 COM ポートに "ID?" を送り "K-Logger" 応答を確認)。
--port で明示指定も可能。要 pyserial (pip install -r requirements.txt)。

注意: K-Logger は起動後 60 秒間しか UART を待ち受けない (コマンド受信ごとに
延長される)。電源投入またはリセット直後に実行すること。
"""

import argparse
import csv
import sys
import time
from datetime import datetime

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("ERROR: pyserial is not installed. Run: pip install pyserial")
    sys.exit(1)

BAUDRATE = 9600
IDENTIFY_CMD = "ID?"
IDENTIFY_EXPECT = "K-Logger"
WRITE_DELAY_SEC = 0.05  # 送信後の小休止 (PS1 版の WriteDelayMs と同じ)


def send_line(port, line):
    """1 行送信する。送信前に受信バッファを捨てる。"""
    port.reset_input_buffer()
    port.write((line + "\r\n").encode("ascii"))
    port.flush()
    time.sleep(WRITE_DELAY_SEC)


def read_line(port):
    """1 行受信して返す。タイムアウト時は None。"""
    raw = port.readline()
    if not raw:
        return None
    return raw.decode("ascii", errors="replace").strip()


def open_port(name, timeout):
    return serial.Serial(
        port=name, baudrate=BAUDRATE,
        bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE, timeout=timeout, write_timeout=0.5)


def find_device(explicit_port, timeout):
    """K-Logger が接続されたポートを探して open 済みの Serial を返す。"""
    if explicit_port:
        candidates = [explicit_port]
    else:
        candidates = sorted(p.device for p in list_ports.comports())
        if not candidates:
            print("ERROR: No serial port found. Check USB connection and driver.")
            sys.exit(1)
        print("Candidate ports: " + ", ".join(candidates))

    for name in candidates:
        print(f"Trying: {name} ...")
        try:
            port = open_port(name, timeout)
        except (serial.SerialException, OSError):
            print("  Could not open.")
            continue
        try:
            send_line(port, IDENTIFY_CMD)
            resp = read_line(port)
        except (serial.SerialException, OSError):
            port.close()
            continue
        if resp and IDENTIFY_EXPECT in resp:
            print(f"  Identification OK: '{resp}'")
            return port
        print(f"  Identification failed: response='{resp}'")
        port.close()

    print("ERROR: Target device not found.")
    sys.exit(2)


def cmd_id(port, args):
    # find_device() で識別済みなのでここでは何もしない
    return 0


def cmd_settime(port, args):
    timestamp = datetime.now().strftime("%Y-%m-%dT%H:%M:%S")
    cmd = f"SETTIME {timestamp}"
    print(f"Sending: {cmd}")
    send_line(port, cmd)
    resp = read_line(port)
    if resp and "Time set OK" in resp:
        print(f"SUCCESS: Time set. Response='{resp}'")
        return 0
    print(f"ERROR: No valid response. Response='{resp}'")
    return 4


def cmd_calibrate(port, args):
    cmd = f"CALIBRATE {args.distance}"
    print(f"Sending: {cmd}")
    send_line(port, cmd)
    resp = read_line(port)
    if resp and "CAL OK" in resp:
        print(f"SUCCESS: Calibration complete. Response='{resp}'")
        return 0
    print(f"ERROR: Calibration failed. Response='{resp}'")
    return 4


def cmd_measure(port, args):
    writer = None
    csv_file = None
    if args.output:
        csv_file = open(args.output, "w", newline="", encoding="utf-8")
        writer = csv.writer(csv_file)
        writer.writerow(["timestamp", "distance_mm"])

    print("Sending: CMEASURE")
    send_line(port, "CMEASURE")
    resp = read_line(port)
    if resp and "Start continuous measuring" in resp:
        print("SUCCESS: Measurement started. Press Ctrl+C to stop.")
    else:
        print(f"WARNING: Unexpected start response. Response='{resp}'")
        # ACK がおかしくてもデータが来る可能性があるので続行する

    exit_code = 0
    try:
        port.timeout = 0.5  # Ctrl+C に素早く反応できるよう短いタイムアウトで回す
        while True:
            line = read_line(port)
            if line is None:
                continue
            if line.startswith("MES "):
                value = line[4:]
                print(f"{value} mm")
                if writer:
                    writer.writerow(
                        [datetime.now().strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3], value])
            elif line:
                print(line)  # エラーや ACK などはそのまま表示
    except KeyboardInterrupt:
        print("\nStopping requested by user...")
    finally:
        if csv_file:
            csv_file.close()
            print(f"CSV saved: {args.output}")

    print("Sending: STOP")
    port.timeout = args.timeout
    send_line(port, "STOP")
    resp = read_line(port)
    if resp and "Stop continuous measuring" in resp:
        print("SUCCESS: Measurement stopped cleanly.")
    else:
        print(f"WARNING: Stopped but unexpected response. Response='{resp}'")
    return exit_code


def main():
    parser = argparse.ArgumentParser(
        description="K-Logger control tool",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("--port", help="serial port (e.g. COM3, /dev/ttyUSB0); "
                        "omit to auto-detect")
    parser.add_argument("--timeout", type=float, default=7.0,
                        help="read timeout in seconds")
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("id", help="identify device only")
    sub.add_parser("settime", help="set device clock to PC time")

    p_cal = sub.add_parser("calibrate", help="calibrate distance offset")
    p_cal.add_argument("distance", type=int,
                       help="actual target distance in mm (10-500)")

    p_mes = sub.add_parser("measure", help="continuous measurement (Ctrl+C to stop)")
    p_mes.add_argument("-o", "--output", help="save measurements to CSV file")

    args = parser.parse_args()

    if args.command == "calibrate" and not 10 <= args.distance <= 500:
        parser.error("distance must be between 10 and 500 mm")

    handlers = {"id": cmd_id, "settime": cmd_settime,
                "calibrate": cmd_calibrate, "measure": cmd_measure}

    port = find_device(args.port, args.timeout)
    try:
        return handlers[args.command](port, args)
    finally:
        port.close()


if __name__ == "__main__":
    sys.exit(main())

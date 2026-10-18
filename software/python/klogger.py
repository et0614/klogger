#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
K-Logger v3 PCツール (USB-CDC / JSONプロトコル)

USB接続した K-Logger v3 の設定・記録開始・データ読み出しを行う。
ドライバ不要 (Windows 10以降は標準のUSBシリアルとして認識される)。

使い方:
    python klogger.py info                    機器情報・状態・電池電圧を表示
    python klogger.py start [-i 秒]           時刻を合わせて記録開始 (既存データは消去される)
    python klogger.py stop                    記録停止
    python klogger.py read [出力.csv]         保存データを CSV に保存
    python klogger.py read --daily 出力フォルダ  日ごとのCSV (v2互換: yyyyMMdd.csv, "HH:MM:SS,距離")
    python klogger.py measure [-n 回数]       距離を連続表示 (保存しない, Ctrl+Cで終了)
    python klogger.py calibrate 距離mm        オフセット校正 (対象物を指定距離に置いて実行)
    python klogger.py tof [--mode short|long] [--tb ms] [--roi n]  測距条件の表示/変更
    python klogger.py selftest                フラッシュ/距離センサの自己診断
    python klogger.py clear                   保存データの消去
    共通オプション: -p COM3 (ポート指定。省略時は自動検出)

記録中の機器はUSBに応答しない。読み出すときはUSBを挿すと自動でUSB通信モードに戻る
(記録はその時点で停止し、データは保存される)。

CSV列: index, epoch, datetime, distance_mm, status
  status: valid / sigma_fail / signal_fail / out_of_bounds / wraparound / min_range / other / failed
  (failed = 計測できなかった記録。distance_mm は空欄)

依存: pip install pyserial
"""
import sys
import os
import json
import time
import csv
import struct
import argparse
from datetime import datetime

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("pyserial が必要です:  pip install pyserial")
    sys.exit(1)

# USB識別子 (c-logger と共通なので、接続後に hello の device 名で K-Logger か確認する)
USB_VID = 0x04D8
USB_PID = 0x0B15
DEVICE_NAME = "K-Logger"

# レコード形式 (firmware w25q128.h)
REC_FAILED = 0xFFFE
STATUS_NAMES = ["valid", "sigma_fail", "signal_fail", "out_of_bounds",
                "wraparound", "min_range", "other", "failed"]
# measure 応答の status (VL53L1X ULD のレンジステータス番号) の名前
ULD_STATUS_NAMES = {0: "valid", 1: "sigma_fail", 2: "signal_fail", 3: "min_range",
                    4: "out_of_bounds", 7: "wraparound"}

DUMP_CHUNK = 32768  # dump_range 1回あたりのレコード数 (64KB)


def decode_record(v):
    """16bitレコード → (distance_mm or None, status名)"""
    if v == REC_FAILED:
        return None, "failed"
    return v & 0x1FFF, STATUS_NAMES[(v >> 13) & 0x7]


class KLogger:
    def __init__(self, port, timeout=2.0):
        # USB-CDC なのでボーレートは無視されるが pyserial には必要
        self.ser = serial.Serial(port, baudrate=115200, timeout=timeout, write_timeout=timeout)
        time.sleep(0.2)
        try:
            self.ser.reset_input_buffer()
        except Exception:
            pass
        self._id = 0

    def close(self):
        try:
            self.ser.close()
        except Exception:
            pass

    def _send(self, command, params=None):
        self._id += 1
        req = {"id": self._id, "command": command}
        if params is not None:
            req["params"] = params
        self.ser.write((json.dumps(req) + "\n").encode("ascii"))
        return self._id

    def _read_json_line(self, wait_s):
        """JSON行を1つ読む (wait_s 秒まで待つ)。取れなければ None。"""
        deadline = time.time() + wait_s
        while time.time() < deadline:
            line = self.ser.readline().decode("ascii", errors="replace").strip()
            if line.startswith("{"):
                try:
                    return json.loads(line)
                except json.JSONDecodeError:
                    return None
        return None

    def request(self, command, params=None, wait_s=3.0):
        """コマンドを送り応答(dict)を返す。応答が無ければ RuntimeError。"""
        rid = self._send(command, params)
        while True:
            j = self._read_json_line(wait_s)
            if j is None:
                raise RuntimeError(f"{command}: 応答がありません")
            if j.get("id") == rid:
                if not j.get("ok", False) and "error" in j:
                    raise RuntimeError(f"{command}: エラー {j['error']}")
                return j

    def dump_range(self, start, count, progress=None):
        """レコード [start, start+count) を取得して uint16 のリストで返す。ヘッダも返す。"""
        rid = self._send("dump_range", {"start": start, "count": count})
        hdr = self._read_json_line(5.0)
        if not hdr or hdr.get("id") != rid or hdr.get("dump") != "begin":
            raise RuntimeError(f"dumpヘッダの取得に失敗しました: {hdr}")
        n = hdr.get("count", 0)
        nbytes = n * hdr.get("rec_size", 2)
        raw = bytearray()
        idle = 0
        while len(raw) < nbytes:
            chunk = self.ser.read(nbytes - len(raw))
            if chunk:
                raw += chunk
                idle = 0
                if progress:
                    progress(len(raw) // 2)
            else:
                idle += 1
                if idle >= 3:
                    break
        self._read_json_line(3.0)  # フッタ
        if len(raw) < nbytes:
            raise RuntimeError(f"受信が不足しました: {len(raw)}/{nbytes} バイト")
        return hdr, list(struct.unpack(f"<{n}H", bytes(raw)))


def find_port():
    """K-Logger の COMポートを自動検出する (VID/PIDで候補を探し hello で確認)。"""
    for p in list_ports.comports():
        if p.vid == USB_VID and p.pid == USB_PID:
            try:
                dev = KLogger(p.device, timeout=1.0)
                try:
                    h = dev.request("hello")
                    if h.get("device") == DEVICE_NAME:
                        return p.device
                finally:
                    dev.close()
            except (serial.SerialException, RuntimeError):
                pass
    return None


def fmt_time(epoch):
    return datetime.fromtimestamp(epoch).strftime("%Y-%m-%d %H:%M:%S")


# ============================================================
# サブコマンド
# ============================================================

def cmd_info(dev, args):
    h = dev.request("hello")
    print(f"{h.get('device')}  FW {h.get('fw')}  proto {h.get('proto')}  build {h.get('build')}")
    print(f"機体ID: {dev.request('get_id').get('device_id')}")
    s = dev.request("get_status")
    print(f"記録中: {'はい' if s['logging'] else 'いいえ'}  記録間隔: {s['interval']}秒")
    days = s["count"] * s["interval"] / 86400
    print(f"保存件数: {s['count']:,} / {s['capacity']:,}  (約{days:.1f}日分)")
    if s["count"] > 0:
        print(f"記録開始: {fmt_time(s['start_dt'])}")
    print(f"機器の時計: {fmt_time(s['epoch'])}")
    t = dev.request("get_tof")
    print(f"測距条件: {t['mode']} / {t['tb_ms']}ms / ROI {t['roi']}x{t['roi']} / オフセット {t['offset_mm']}mm")
    b = dev.request("get_battery")
    print(f"電池電圧: {b['vbat_mV']/1000:.3f} V")


def cmd_start(dev, args):
    if args.interval:
        dev.request("set_interval", {"sec": args.interval})
    now = int(time.time())
    dev.request("set_time", {"epoch": now})
    print(f"時刻設定: {fmt_time(now)}")
    print("既存データを消去して記録を開始します (データ量により最大1分程度かかります)...")
    dev.request("start_logging", wait_s=120.0)
    print("記録を開始しました。USBを抜いても記録を続けます。")


def cmd_stop(dev, args):
    dev.request("stop_logging")
    print("記録を停止しました。")


def cmd_read(dev, args):
    s = dev.request("get_status")
    total = s["count"]
    if total == 0:
        print("保存データがありません (0件)。")
        return
    print(f"{total:,} 件を読み出します...")
    recs = []
    hdr = None
    for start in range(0, total, DUMP_CHUNK):
        cnt = min(DUMP_CHUNK, total - start)
        def progress(n, base=start):
            print(f"\r受信中... {base + n:,}/{total:,}", end="", flush=True)
        hdr, part = dev.dump_range(start, cnt, progress)
        recs += part
    print()
    start_dt, interval = hdr["start_dt"], hdr["interval"]
    n_failed = sum(1 for v in recs if v == REC_FAILED)

    if args.daily:
        # v2互換: 日ごとのファイル yyyyMMdd.csv に "HH:MM:SS,距離[mm]"
        os.makedirs(args.daily, exist_ok=True)
        files = {}
        try:
            for i, v in enumerate(recs):
                ts = datetime.fromtimestamp(start_dt + i * interval)
                name = ts.strftime("%Y%m%d.csv")
                if name not in files:
                    files[name] = open(os.path.join(args.daily, name), "w", newline="")
                dist, _ = decode_record(v)
                files[name].write(f"{ts.strftime('%H:%M:%S')},{'' if dist is None else dist}\r\n")
        finally:
            for f in files.values():
                f.close()
        print(f"{len(files)} 個のファイルを {args.daily} に保存しました。")
    else:
        out = args.output
        if not out:
            did = dev.request("get_id").get("device_id", "")
            out = f"klogger_{did}_{datetime.fromtimestamp(start_dt).strftime('%Y%m%d_%H%M%S')}.csv"
        with open(out, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["index", "epoch", "datetime", "distance_mm", "status"])
            for i, v in enumerate(recs):
                ts = start_dt + i * interval
                dist, st = decode_record(v)
                w.writerow([i, ts, fmt_time(ts), "" if dist is None else dist, st])
        print(f"CSV保存: {out}")
    print(f"{len(recs):,} 件 (計測失敗 {n_failed} 件)  {fmt_time(start_dt)} 〜 "
          f"{fmt_time(start_dt + (len(recs) - 1) * interval)}")


def cmd_measure(dev, args):
    print("距離を連続表示します (保存しません)。Ctrl+C で終了。")
    i = 0
    try:
        while args.count == 0 or i < args.count:
            r = dev.request("measure")
            st = ULD_STATUS_NAMES.get(r["status"], f"status {r['status']}")
            print(f"{r['distance_mm']:5d} mm  {st:12s} signal {r['signal']:5d}  ambient {r['ambient']:5d}")
            i += 1
            if args.wait:
                time.sleep(args.wait)
    except KeyboardInterrupt:
        print()


def cmd_calibrate(dev, args):
    print(f"{args.mm} mm の位置に対象物(反射率17%程度のグレーが標準)を置いた状態で校正します...")
    r = dev.request("calibrate", {"mm": args.mm}, wait_s=60.0)
    print(f"校正完了: オフセット {r['offset_mm']} mm (機器に保存しました)")


def cmd_tof(dev, args):
    params = {}
    if args.mode:
        params["mode"] = args.mode
    if args.tb:
        params["tb_ms"] = args.tb
    if args.roi:
        params["roi"] = args.roi
    t = dev.request("set_tof", params) if params else dev.request("get_tof")
    print(f"測距条件: {t['mode']} / {t['tb_ms']}ms / ROI {t['roi']}x{t['roi']} / オフセット {t['offset_mm']}mm")


def cmd_selftest(dev, args):
    r = dev.request("selftest", wait_s=10.0)
    ok = lambda b: "OK" if b else "NG"
    fl, tf, ms = r["flash"], r["tof"], r["measure"]
    print(f"フラッシュ   : {ok(fl['ok'] and fl['rw'])}  (JEDEC {fl['jedec']}, 読み書き {ok(fl['rw'])})")
    print(f"距離センサ   : {ok(tf['ok'])}  (ID {tf['sensor_id']})")
    print(f"測定         : {ok(ms['ok'])}  ({ms['distance_mm']} mm, status {ms['status']})")
    print(f"総合         : {ok(r['ok'])}")


def cmd_clear(dev, args):
    print("保存データを消去します (データ量により最大1分程度かかります)...")
    dev.request("clear_memory", wait_s=120.0)
    print("消去しました。")


def main():
    ap = argparse.ArgumentParser(description="K-Logger v3 PCツール")
    ap.add_argument("-p", "--port", help="COMポート (省略時は自動検出)")
    sub = ap.add_subparsers(dest="cmd", required=True)

    sub.add_parser("info", help="機器情報・状態を表示")
    p = sub.add_parser("start", help="時刻を合わせて記録開始 (既存データは消去)")
    p.add_argument("-i", "--interval", type=int, help="記録間隔 [秒] (1〜600)")
    sub.add_parser("stop", help="記録停止")
    p = sub.add_parser("read", help="保存データを CSV に保存")
    p.add_argument("output", nargs="?", help="出力CSVファイル名 (省略時は自動命名)")
    p.add_argument("--daily", metavar="DIR", help="日ごとのCSV(v2互換)をこのフォルダに保存")
    p = sub.add_parser("measure", help="距離を連続表示 (保存しない)")
    p.add_argument("-n", "--count", type=int, default=0, help="回数 (0=無限)")
    p.add_argument("-w", "--wait", type=float, default=0.0, help="測定間隔 [秒]")
    p = sub.add_parser("calibrate", help="オフセット校正")
    p.add_argument("mm", type=int, help="対象物までの距離 [mm] (10〜1000)")
    p = sub.add_parser("tof", help="測距条件の表示/変更")
    p.add_argument("--mode", choices=["short", "long"])
    p.add_argument("--tb", type=int, choices=[15, 20, 33, 50, 100, 200, 500], help="タイミングバジェット [ms]")
    p.add_argument("--roi", type=int, help="受光領域 4〜16")
    sub.add_parser("selftest", help="自己診断")
    sub.add_parser("clear", help="保存データの消去")
    args = ap.parse_args()

    port = args.port or find_port()
    if port is None:
        print("K-Logger が見つかりません。USB接続を確認してください。")
        return 1
    try:
        dev = KLogger(port)
    except serial.SerialException as e:
        print(f"ポート {port} を開けません: {e}")
        return 1

    handlers = {"info": cmd_info, "start": cmd_start, "stop": cmd_stop, "read": cmd_read,
                "measure": cmd_measure, "calibrate": cmd_calibrate, "tof": cmd_tof,
                "selftest": cmd_selftest, "clear": cmd_clear}
    try:
        handlers[args.cmd](dev, args)
    except RuntimeError as e:
        print(f"エラー: {e}")
        return 1
    finally:
        dev.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())

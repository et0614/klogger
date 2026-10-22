#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
K-Logger v3 GUI ツール (tkinter, 画面表示は英語)

klogger.py の KLogger クラス(USB-CDC / JSONプロトコル)を使い、以下を1画面で行う。
  - 機器情報の表示 (FW, 機体ID, 電池電圧, 保存件数, 測距条件)
  - 記録開始 (時刻合わせ + 記録間隔)
  - データ読み出し (CSV / 日別CSV)、データ消去
  - 距離のリアルタイム表示
  - 距離校正 (グレー反射板を使ったオフセット校正)
  - 測距条件の設定、自己診断

通信はすべてワーカースレッドで行い、結果はキュー経由でUIスレッドに渡す (tkinterはUIスレッド専用)。
機器へのアクセスは Device.lock で直列化する (リアルタイム表示中に他の操作をしても衝突しない)。

実行: python klogger_gui.py   (依存: pyserial)
exe化: build_exe.bat (PyInstaller)
"""
import os
import queue
import statistics
import threading
import time
from collections import deque
from datetime import datetime

import tkinter as tk
from tkinter import ttk, filedialog, messagebox

import serial
from serial.tools import list_ports

import klogger as kl

APP_TITLE = "K-Logger"
TB_CHOICES = [15, 20, 33, 50, 100, 200, 500]
LIVE_INTERVAL_S = 0.1          # リアルタイム表示の測定間隔
CAL_WINDOW = 20                # 校正画面の統計に使う直近の測定数
CAL_READY_STD_MM = 3.0         # これ以下のばらつきなら「校正できる」とみなす
CAL_WARN_STD_MM = 5.0          # 校正結果のばらつきがこれを超えたら警告
AUTO_PORT = "Auto"


def fmt_time(epoch):
    return datetime.fromtimestamp(epoch).strftime("%Y-%m-%d %H:%M:%S")


class Device:
    """KLogger をスレッド間で共有するための入れ物 (操作は lock で直列化)"""

    def __init__(self):
        self.dev = None
        self.port = None
        self.lock = threading.Lock()

    @property
    def connected(self):
        return self.dev is not None

    def open(self, port):
        dev = kl.KLogger(port)
        try:
            h = dev.request("hello")
        except Exception:
            dev.close()
            raise
        if h.get("device") != kl.DEVICE_NAME:
            dev.close()
            raise RuntimeError(f"{port} is not a K-Logger")
        with self.lock:
            self.dev, self.port = dev, port

    def close(self):
        with self.lock:
            if self.dev:
                self.dev.close()
            self.dev, self.port = None, None

    def call(self, fn):
        """fn(KLogger) を排他実行する"""
        with self.lock:
            if self.dev is None:
                raise RuntimeError("not connected")
            return fn(self.dev)


class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title(APP_TITLE)
        self.minsize(640, 560)
        self.device = Device()
        self.q = queue.Queue()
        self.busy = False
        self.info = {}
        self.live_on = False
        self.live_thread = None
        self.cal_buf = deque(maxlen=CAL_WINDOW)
        self.prev_offset = None

        self._build_ui()
        self._set_connected(False)
        self.after(50, self._poll_queue)
        self.protocol("WM_DELETE_WINDOW", self._on_close)
        self.after(200, self._refresh_ports)

    # ------------------------------------------------------------------
    # UI 構築
    # ------------------------------------------------------------------
    def _build_ui(self):
        pad = {"padx": 6, "pady": 3}

        # 接続
        top = ttk.Frame(self)
        top.pack(fill="x", padx=8, pady=(8, 4))
        ttk.Label(top, text="Port:").pack(side="left")
        self.port_var = tk.StringVar(value=AUTO_PORT)
        self.port_combo = ttk.Combobox(top, textvariable=self.port_var, width=12, state="readonly")
        self.port_combo.pack(side="left", padx=4)
        ttk.Button(top, text="↻", width=3, command=self._refresh_ports).pack(side="left")
        self.connect_btn = ttk.Button(top, text="Connect", command=self._on_connect)
        self.connect_btn.pack(side="left", padx=6)
        self.refresh_btn = ttk.Button(top, text="Refresh", command=self._refresh_info)
        self.refresh_btn.pack(side="left")
        self.device_buttons = [self.refresh_btn]  # 接続中だけ有効にするボタン (各タブで追加)

        # 機器情報
        box = ttk.LabelFrame(self, text="Device")
        box.pack(fill="x", padx=8, pady=4)
        self.info_vars = {}
        fields = [("device", "Device"), ("id", "Device ID"), ("battery", "Battery"),
                  ("records", "Stored records"), ("period", "Recorded period"), ("tof", "Ranging")]
        for i, (key, label) in enumerate(fields):
            r, c = divmod(i, 2)
            ttk.Label(box, text=label + ":").grid(row=r, column=c * 2, sticky="w", **pad)
            v = tk.StringVar(value="-")
            ttk.Label(box, textvariable=v).grid(row=r, column=c * 2 + 1, sticky="w", **pad)
            self.info_vars[key] = v
        box.columnconfigure(1, weight=1)
        box.columnconfigure(3, weight=1)

        # タブ
        self.nb = ttk.Notebook(self)
        self.nb.pack(fill="both", expand=True, padx=8, pady=4)
        self._build_logging_tab()
        self._build_data_tab()
        self._build_live_tab()
        self._build_cal_tab()
        self._build_settings_tab()

        # ステータスバー
        bar = ttk.Frame(self)
        bar.pack(fill="x", padx=8, pady=(0, 8))
        self.status_var = tk.StringVar(value="Not connected.")
        ttk.Label(bar, textvariable=self.status_var).pack(side="left")
        self.progress = ttk.Progressbar(bar, length=180, mode="determinate")
        self.progress.pack(side="right")

    def _build_logging_tab(self):
        f = ttk.Frame(self.nb, padding=10)
        self.nb.add(f, text="Logging")
        ttk.Label(f, text="Recording interval (s):").grid(row=0, column=0, sticky="w")
        self.interval_var = tk.IntVar(value=1)
        sp = ttk.Spinbox(f, from_=1, to=600, width=6, textvariable=self.interval_var,
                         command=self._update_capacity)
        sp.grid(row=0, column=1, sticky="w", padx=6)
        sp.bind("<KeyRelease>", lambda e: self._update_capacity())
        self.capacity_var = tk.StringVar()
        ttk.Label(f, textvariable=self.capacity_var, foreground="gray").grid(row=1, column=0, columnspan=3,
                                                                           sticky="w", pady=(2, 10))
        self.start_btn = ttk.Button(f, text="Start logging…", command=self._on_start_logging)
        self.start_btn.grid(row=2, column=0, sticky="w")
        ttk.Label(f, wraplength=560, justify="left", text=(
            "Starting erases all stored data and synchronizes the device clock to this PC.\n"
            "After starting, the K-Logger disconnects from USB and keeps logging on battery. "
            "To stop logging, plug it into USB again (data is saved automatically) "
            "or hold the button for 5 seconds.")).grid(row=3, column=0, columnspan=3, sticky="w", pady=10)
        self.device_buttons.append(self.start_btn)
        self._update_capacity()

    def _build_data_tab(self):
        f = ttk.Frame(self.nb, padding=10)
        self.nb.add(f, text="Data")
        self.daily_var = tk.BooleanVar(value=False)
        self.save_btn = ttk.Button(f, text="Save as CSV…", command=self._on_save_csv)
        self.save_btn.grid(row=0, column=0, sticky="w")
        ttk.Checkbutton(f, text="Also save daily CSV files (legacy v2 format: yyyyMMdd.csv)",
                        variable=self.daily_var).grid(row=0, column=1, sticky="w", padx=10)
        ttk.Label(f, wraplength=560, justify="left", foreground="gray", text=(
            "CSV columns: index, epoch, datetime, distance_mm, status\n"
            "status: valid / sigma_fail / signal_fail / out_of_bounds / wraparound / min_range / failed "
            "(failed = no measurement, distance is empty)")).grid(row=1, column=0, columnspan=2,
                                                                 sticky="w", pady=10)
        ttk.Separator(f).grid(row=2, column=0, columnspan=2, sticky="ew", pady=10)
        self.erase_btn = ttk.Button(f, text="Erase all data…", command=self._on_erase)
        self.erase_btn.grid(row=3, column=0, sticky="w")
        self.device_buttons += [self.save_btn, self.erase_btn]

    def _build_live_tab(self):
        f = ttk.Frame(self.nb, padding=10)
        self.nb.add(f, text="Live view")
        self.live_btn = ttk.Button(f, text="Start", command=self._toggle_live)
        self.live_btn.grid(row=0, column=0, sticky="w")
        self.live_dist = tk.StringVar(value="---- mm")
        ttk.Label(f, textvariable=self.live_dist, font=("Segoe UI", 36, "bold")).grid(
            row=1, column=0, columnspan=3, sticky="w", pady=10)
        self.live_detail = tk.StringVar(value="")
        ttk.Label(f, textvariable=self.live_detail, font=("Consolas", 11)).grid(
            row=2, column=0, columnspan=3, sticky="w")
        ttk.Label(f, wraplength=560, justify="left", foreground="gray", text=(
            "Short mode measures up to about 1.3 m. 'signal_fail' means the reflected light is too weak "
            "(target too far, too dark or too small).")).grid(row=3, column=0, columnspan=3, sticky="w",
                                                              pady=10)
        self.device_buttons.append(self.live_btn)

    def _build_cal_tab(self):
        f = ttk.Frame(self.nb, padding=10)
        self.nb.add(f, text="Calibration")
        ttk.Label(f, wraplength=600, justify="left", text=(
            "1. Select the ranging settings you will actually use (Settings tab) before calibrating.\n"
            "2. Place a grey target (about 17 % reflectance, e.g. an 18 % grey card, 10 cm square or larger) "
            "perpendicular to the sensor at a known distance. 140 mm is recommended. Avoid direct sunlight.\n"
            "3. Start the live reading and wait until the indicator turns green, then press Calibrate.")
                  ).grid(row=0, column=0, columnspan=4, sticky="w")

        row = ttk.Frame(f)
        row.grid(row=1, column=0, columnspan=4, sticky="w", pady=8)
        ttk.Label(row, text="Target distance (mm):").pack(side="left")
        self.cal_target = tk.IntVar(value=140)
        ttk.Spinbox(row, from_=10, to=1000, width=6, textvariable=self.cal_target).pack(side="left", padx=6)
        self.cal_live_btn = ttk.Button(row, text="Start live reading", command=self._toggle_live)
        self.cal_live_btn.pack(side="left", padx=6)

        stat = ttk.LabelFrame(f, text=f"Live reading (last {CAL_WINDOW} measurements, current offset applied)")
        stat.grid(row=2, column=0, columnspan=4, sticky="ew", pady=4)
        self.cal_ready = tk.Label(stat, text="  ", width=2, bg="gray")
        self.cal_ready.grid(row=0, column=0, rowspan=2, padx=8, pady=6)
        self.cal_stat1 = tk.StringVar(value="-")
        self.cal_stat2 = tk.StringVar(value="")
        ttk.Label(stat, textvariable=self.cal_stat1, font=("Consolas", 11)).grid(row=0, column=1, sticky="w")
        ttk.Label(stat, textvariable=self.cal_stat2).grid(row=1, column=1, sticky="w")

        btns = ttk.Frame(f)
        btns.grid(row=3, column=0, columnspan=4, sticky="w", pady=8)
        self.cal_btn = ttk.Button(btns, text="Calibrate", command=self._on_calibrate)
        self.cal_btn.pack(side="left")
        self.cal_restore_btn = ttk.Button(btns, text="Restore previous offset", command=self._on_restore_offset)
        self.cal_restore_btn.pack(side="left", padx=6)
        self.cal_zero_btn = ttk.Button(btns, text="Reset offset to 0", command=lambda: self._set_offset(0))
        self.cal_zero_btn.pack(side="left")

        self.cal_result = tk.StringVar(value="")
        ttk.Label(f, textvariable=self.cal_result, wraplength=600, justify="left").grid(
            row=4, column=0, columnspan=4, sticky="w")
        ttk.Label(f, wraplength=600, justify="left", foreground="gray", text=(
            "After calibrating, the live reading shows the corrected distance. "
            "Move the target to another distance to check the result.")).grid(row=5, column=0, columnspan=4,
                                                                              sticky="w", pady=8)
        f.columnconfigure(3, weight=1)
        self.device_buttons += [self.cal_live_btn, self.cal_btn, self.cal_zero_btn]

    def _build_settings_tab(self):
        f = ttk.Frame(self.nb, padding=10)
        self.nb.add(f, text="Settings")
        ttk.Label(f, text="Distance mode:").grid(row=0, column=0, sticky="w", pady=3)
        self.mode_var = tk.StringVar(value="short")
        mf = ttk.Frame(f)
        mf.grid(row=0, column=1, sticky="w")
        ttk.Radiobutton(mf, text="Short (up to ~1.3 m, robust to ambient light)", value="short",
                        variable=self.mode_var).pack(anchor="w")
        ttk.Radiobutton(mf, text="Long (up to ~4 m in the dark, timing budget ≥ 20 ms)", value="long",
                        variable=self.mode_var).pack(anchor="w")
        ttk.Label(f, text="Timing budget (ms):").grid(row=1, column=0, sticky="w", pady=3)
        self.tb_var = tk.StringVar(value="15")
        ttk.Combobox(f, textvariable=self.tb_var, values=[str(v) for v in TB_CHOICES], width=6,
                     state="readonly").grid(row=1, column=1, sticky="w")
        ttk.Label(f, text="ROI (4–16):").grid(row=2, column=0, sticky="w", pady=3)
        self.roi_var = tk.IntVar(value=4)
        ttk.Spinbox(f, from_=4, to=16, width=6, textvariable=self.roi_var).grid(row=2, column=1, sticky="w")
        self.apply_btn = ttk.Button(f, text="Apply", command=self._on_apply_tof)
        self.apply_btn.grid(row=3, column=1, sticky="w", pady=8)
        ttk.Label(f, wraplength=560, justify="left", foreground="gray", text=(
            "A longer timing budget gives more stable readings but uses more battery. "
            "Recalibrate after changing these settings.")).grid(row=4, column=0, columnspan=2, sticky="w")

        ttk.Separator(f).grid(row=5, column=0, columnspan=2, sticky="ew", pady=10)
        self.selftest_btn = ttk.Button(f, text="Run self-test", command=self._on_selftest)
        self.selftest_btn.grid(row=6, column=0, sticky="w")
        self.selftest_var = tk.StringVar(value="")
        ttk.Label(f, textvariable=self.selftest_var, font=("Consolas", 10), justify="left").grid(
            row=7, column=0, columnspan=2, sticky="w", pady=6)
        self.device_buttons += [self.apply_btn, self.selftest_btn]

    # ------------------------------------------------------------------
    # 非同期実行
    # ------------------------------------------------------------------
    def _run(self, text, work, done=None, progress_mode=None):
        """work() をワーカースレッドで実行し、結果を done(result) に渡す (UIスレッド)"""
        if self.busy:
            return
        self.busy = True
        self._update_buttons()
        self.status_var.set(text)
        if progress_mode == "indeterminate":
            self.progress.configure(mode="indeterminate")
            self.progress.start(15)

        def worker():
            try:
                r = work()
                self.q.put(("done", done, r))
            except Exception as e:  # noqa: BLE001 (UIに表示する)
                self.q.put(("error", None, e))

        threading.Thread(target=worker, daemon=True).start()

    def _poll_queue(self):
        try:
            while True:
                kind, cb, payload = self.q.get_nowait()
                if kind == "live":
                    self._on_live_result(payload)
                elif kind == "live_error":
                    self._stop_live()
                    self._handle_error(payload)
                elif kind == "progress":
                    n, total = payload
                    self.progress.configure(mode="determinate", maximum=max(total, 1), value=n)
                    self.status_var.set(f"Downloading… {n:,} / {total:,}")
                else:
                    self._task_finished()
                    if kind == "done":
                        if cb:
                            cb(payload)
                    else:
                        self._handle_error(payload)
        except queue.Empty:
            pass
        self.after(50, self._poll_queue)

    def _task_finished(self):
        self.busy = False
        self.progress.stop()
        self.progress.configure(mode="determinate", value=0)
        self._update_buttons()

    def _handle_error(self, e):
        # ポートが消えた(USBを抜いた)などの通信異常は切断扱いにする
        if isinstance(e, (serial.SerialException, OSError)) or "no response" in str(e):
            self._disconnect(f"Connection lost: {e}")
            messagebox.showerror(APP_TITLE, f"Connection lost.\n\n{e}")
        else:
            self.status_var.set(f"Error: {e}")
            messagebox.showerror(APP_TITLE, str(e))

    # ------------------------------------------------------------------
    # 接続
    # ------------------------------------------------------------------
    def _refresh_ports(self):
        ports = [p.device for p in list_ports.comports() if p.vid == kl.USB_VID and p.pid == kl.USB_PID]
        self.port_combo["values"] = [AUTO_PORT] + ports
        if self.port_var.get() not in self.port_combo["values"]:
            self.port_var.set(AUTO_PORT)

    def _on_connect(self):
        if self.device.connected:
            self._disconnect("Disconnected.")
            return
        port = self.port_var.get()

        def work():
            p = kl.find_port() if port == AUTO_PORT else port
            if p is None:
                raise RuntimeError("K-Logger not found. Check the USB connection.\n"
                                   "(A K-Logger that is logging does not respond. Re-plug it to stop logging.)")
            self.device.open(p)
            try:
                return self._read_info()
            except Exception:
                self.device.close()
                raise

        self._run("Connecting…", work, self._on_info, "indeterminate")

    def _disconnect(self, msg):
        self._stop_live()
        self.device.close()
        self._set_connected(False)
        self.status_var.set(msg)

    def _set_connected(self, on):
        self.connect_btn.configure(text="Disconnect" if on else "Connect")
        self.port_combo.configure(state="disabled" if on else "readonly")
        if not on:
            for v in self.info_vars.values():
                v.set("-")
            self.info = {}
        self._update_buttons()

    def _update_buttons(self):
        st = "normal" if (self.device.connected and not self.busy) else "disabled"
        for b in self.device_buttons:
            b.configure(state=st)
        # リアルタイム表示の停止はいつでもできるように
        if self.live_on:
            self.live_btn.configure(state="normal")
            self.cal_live_btn.configure(state="normal")
        restore_ok = st == "normal" and self.prev_offset is not None
        self.cal_restore_btn.configure(state="normal" if restore_ok else "disabled")
        self.connect_btn.configure(state="disabled" if self.busy else "normal")

    # ------------------------------------------------------------------
    # 機器情報
    # ------------------------------------------------------------------
    def _read_info(self):
        def f(d):
            return {"hello": d.request("hello"), "id": d.request("get_id").get("device_id"),
                    "status": d.request("get_status"), "tof": d.request("get_tof"),
                    "battery": d.request("get_battery")}
        return self.device.call(f)

    def _refresh_info(self):
        self._run("Reading device information…", self._read_info, self._on_info)

    def _on_info(self, info):
        self.info = info
        self._set_connected(True)
        h, s, t = info["hello"], info["status"], info["tof"]
        self.info_vars["device"].set(f"{h.get('device')}  FW {h.get('fw')}  ({self.device.port})")
        self.info_vars["id"].set(info["id"])
        self.info_vars["battery"].set(f"{info['battery']['vbat_mV'] / 1000:.2f} V")
        cap_days = s["capacity"] * s["interval"] / 86400
        self.info_vars["records"].set(f"{s['count']:,} / {s['capacity']:,}  ({cap_days:,.0f} days at {s['interval']} s)")
        if s["count"] > 0:
            end = s["start_dt"] + (s["count"] - 1) * s["interval"]
            self.info_vars["period"].set(f"{fmt_time(s['start_dt'])} – {fmt_time(end)}")
        else:
            self.info_vars["period"].set("(no data)")
        self.info_vars["tof"].set(f"{t['mode']}, {t['tb_ms']} ms, ROI {t['roi']}, offset {t['offset_mm']} mm")
        # 設定画面に反映
        self.interval_var.set(s["interval"])
        self.mode_var.set(t["mode"])
        self.tb_var.set(str(t["tb_ms"]))
        self.roi_var.set(t["roi"])
        self._update_capacity()
        self.status_var.set(f"Connected to {self.device.port}.")

    def _update_capacity(self):
        try:
            sec = int(self.interval_var.get())
        except (tk.TclError, ValueError):
            self.capacity_var.set("")
            return
        cap = self.info.get("status", {}).get("capacity", 8386560)
        self.capacity_var.set(f"Memory lasts about {cap * sec / 86400:,.0f} days at this interval.")

    # ------------------------------------------------------------------
    # 記録
    # ------------------------------------------------------------------
    def _on_start_logging(self):
        try:
            sec = int(self.interval_var.get())
        except (tk.TclError, ValueError):
            sec = 0
        if not 1 <= sec <= 600:
            messagebox.showwarning(APP_TITLE, "Recording interval must be 1–600 seconds.")
            return
        n = self.info.get("status", {}).get("count", 0)
        msg = f"Start logging every {sec} s?\n\nThe device clock is set to this PC's time."
        if n > 0:
            msg += f"\n\n{n:,} stored records will be ERASED. Save them first if needed."
        if not messagebox.askyesno(APP_TITLE, msg, icon="warning" if n else "question"):
            return
        self._stop_live()

        def work():
            def f(d):
                d.request("set_interval", {"sec": sec})
                now = int(time.time())
                d.request("set_time", {"epoch": now})
                d.request("start_logging", wait_s=120.0)
                return now
            return self.device.call(f)

        def done(now):
            self._disconnect(f"Logging started at {fmt_time(now)}.")
            messagebox.showinfo(APP_TITLE, (
                f"Logging started at {fmt_time(now)} (every {sec} s).\n\n"
                "The K-Logger has disconnected from USB. You can unplug it now.\n"
                "To stop logging, plug it into USB again or hold the button for 5 seconds."))

        self._run("Erasing memory and starting logging (may take up to a minute)…", work, done, "indeterminate")

    # ------------------------------------------------------------------
    # データ
    # ------------------------------------------------------------------
    def _on_save_csv(self):
        s = self.info.get("status", {})
        if s.get("count", 0) == 0:
            messagebox.showinfo(APP_TITLE, "No stored data.")
            return
        path = filedialog.asksaveasfilename(
            title="Save CSV", defaultextension=".csv", filetypes=[("CSV", "*.csv")],
            initialfile=kl.default_csv_name(self.info.get("id", ""), s.get("start_dt", 0)))
        if not path:
            return
        daily_dir = None
        if self.daily_var.get():
            daily_dir = filedialog.askdirectory(title="Folder for daily CSV files")
            if not daily_dir:
                return
        self._stop_live()

        def work():
            start_dt, interval, recs = self.device.call(
                lambda d: d.download(lambda n, total: self.q.put(("progress", None, (n, total)))))
            kl.write_csv(path, start_dt, interval, recs)
            ndaily = kl.write_daily_csv(daily_dir, start_dt, interval, recs) if daily_dir else 0
            nfail = sum(1 for v in recs if v == kl.REC_FAILED)
            return len(recs), nfail, ndaily

        def done(r):
            n, nfail, ndaily = r
            msg = f"Saved {n:,} records ({nfail:,} failed measurements) to\n{path}"
            if daily_dir:
                msg += f"\n\n{ndaily} daily files saved to\n{daily_dir}"
            self.status_var.set(f"Saved {n:,} records.")
            messagebox.showinfo(APP_TITLE, msg)

        self._run("Downloading…", work, done)

    def _on_erase(self):
        n = self.info.get("status", {}).get("count", 0)
        if not messagebox.askyesno(APP_TITLE, f"Erase all {n:,} stored records?\nThis cannot be undone.",
                                   icon="warning"):
            return
        self._run("Erasing memory (may take up to a minute)…",
                  lambda: self.device.call(lambda d: d.request("clear_memory", wait_s=120.0)),
                  lambda r: self._refresh_info(), "indeterminate")

    # ------------------------------------------------------------------
    # リアルタイム表示 (Live view / Calibration 共通)
    # ------------------------------------------------------------------
    def _toggle_live(self):
        if self.live_on:
            self._stop_live()
        else:
            self._start_live()

    def _start_live(self):
        if not self.device.connected or self.live_on:
            return
        self.live_on = True
        self.cal_buf.clear()
        self.live_btn.configure(text="Stop")
        self.cal_live_btn.configure(text="Stop live reading")

        def loop():
            while self.live_on and self.device.connected:
                try:
                    r = self.device.call(lambda d: d.request("measure"))
                    self.q.put(("live", None, r))
                except Exception as e:  # noqa: BLE001
                    if self.live_on:
                        self.q.put(("live_error", None, e))
                    return
                time.sleep(LIVE_INTERVAL_S)

        self.live_thread = threading.Thread(target=loop, daemon=True)
        self.live_thread.start()
        self._update_buttons()

    def _stop_live(self):
        self.live_on = False
        self.live_btn.configure(text="Start")
        self.cal_live_btn.configure(text="Start live reading")
        self.cal_ready.configure(bg="gray")
        self._update_buttons()

    def _on_live_result(self, r):
        if not self.live_on:
            return
        st = kl.ULD_STATUS_NAMES.get(r["status"], f"status {r['status']}")
        valid = r["status"] == 0
        self.live_dist.set(f"{r['distance_mm']:4d} mm" if valid else f"({r['distance_mm']} mm)")
        self.live_detail.set(f"status {st:13s} signal {r['signal']:6d}   ambient {r['ambient']:5d}")

        # 校正画面の統計
        self.cal_buf.append(r["distance_mm"] if valid else None)
        vals = [v for v in self.cal_buf if v is not None]
        nvalid = len(vals)
        if nvalid >= 2:
            mean = statistics.fmean(vals)
            sd = statistics.stdev(vals)
            try:
                target = int(self.cal_target.get())
                err = f"   error {mean - target:+.1f} mm"
            except (tk.TclError, ValueError):
                target, err = None, ""
            self.cal_stat1.set(f"now {r['distance_mm']:5d} mm ({st})   mean {mean:7.1f} mm   SD {sd:4.1f} mm{err}")
        else:
            sd = None
            self.cal_stat1.set(f"now {r['distance_mm']:5d} mm ({st})")
        full = len(self.cal_buf) == CAL_WINDOW
        ready = full and nvalid == CAL_WINDOW and sd is not None and sd <= CAL_READY_STD_MM
        if ready:
            self.cal_ready.configure(bg="#2e9d48")
            self.cal_stat2.set("Stable. Ready to calibrate.")
        else:
            self.cal_ready.configure(bg="#d9a400" if nvalid else "#c62828")
            if not full:
                self.cal_stat2.set(f"Collecting… ({len(self.cal_buf)}/{CAL_WINDOW})")
            elif nvalid < CAL_WINDOW:
                self.cal_stat2.set(f"Some readings are invalid ({CAL_WINDOW - nvalid}/{CAL_WINDOW}). "
                                   "Check the target position and size.")
            else:
                self.cal_stat2.set(f"Readings are not stable (SD > {CAL_READY_STD_MM:.0f} mm). "
                                   "Keep the target still.")

    # ------------------------------------------------------------------
    # 校正
    # ------------------------------------------------------------------
    def _on_calibrate(self):
        try:
            target = int(self.cal_target.get())
        except (tk.TclError, ValueError):
            target = 0
        if not 10 <= target <= 1000:
            messagebox.showwarning(APP_TITLE, "Target distance must be 10–1000 mm.")
            return
        if not messagebox.askyesno(APP_TITLE, (
                f"Calibrate with the target at {target} mm?\n\n"
                "The sensor takes 50 readings and stores the new offset in the device.")):
            return

        def done(r):
            self.cal_buf.clear()
            prev = r.get("prev_offset_mm", self.info.get("tof", {}).get("offset_mm"))
            self.prev_offset = prev
            lines = [f"Calibrated at {target} mm:  offset {prev} mm → {r['offset_mm']} mm (saved to the device)."]
            if "raw_mm" in r:
                sd = r["std_x10"] / 10
                lines.append(f"Raw mean {r['raw_mm']} mm, SD {sd:.1f} mm, {r['n']} readings.")
                if sd > CAL_WARN_STD_MM:
                    lines.append(f"WARNING: readings were not stable (SD > {CAL_WARN_STD_MM:.0f} mm). "
                                 "Consider restoring the previous offset and calibrating again.")
            self.cal_result.set("\n".join(lines))
            self._refresh_info()

        self._run("Calibrating…", lambda: self.device.call(
            lambda d: d.request("calibrate", {"mm": target}, wait_s=60.0)), done, "indeterminate")

    def _on_restore_offset(self):
        if self.prev_offset is not None:
            self._set_offset(self.prev_offset)

    def _set_offset(self, mm):
        def done(t):
            if t.get("offset_mm") != mm:
                messagebox.showerror(APP_TITLE, "This firmware cannot set the offset directly.\n"
                                                "Please update the firmware to 3.1.0 or later.")
            else:
                self.cal_buf.clear()
                self.cal_result.set(f"Offset set to {mm} mm.")
            self._refresh_info()

        self._run("Setting offset…", lambda: self.device.call(
            lambda d: d.request("set_tof", {"offset_mm": mm})), done)

    # ------------------------------------------------------------------
    # 設定・自己診断
    # ------------------------------------------------------------------
    def _on_apply_tof(self):
        mode, tb = self.mode_var.get(), int(self.tb_var.get())
        try:
            roi = int(self.roi_var.get())
        except (tk.TclError, ValueError):
            roi = 0
        if not 4 <= roi <= 16:
            messagebox.showwarning(APP_TITLE, "ROI must be 4–16.")
            return
        if mode == "long" and tb < 20:
            messagebox.showwarning(APP_TITLE, "Long mode requires a timing budget of 20 ms or more.")
            return
        self._run("Applying settings…", lambda: self.device.call(
            lambda d: d.request("set_tof", {"mode": mode, "tb_ms": tb, "roi": roi})),
            lambda t: self._refresh_info())

    def _on_selftest(self):
        def done(r):
            ok = lambda b: "OK" if b else "NG"  # noqa: E731
            fl, tf, ms = r["flash"], r["tof"], r["measure"]
            self.selftest_var.set(
                f"Flash   : {ok(fl['ok'] and fl['rw'])}  (JEDEC {fl['jedec']}, read/write {ok(fl['rw'])})\n"
                f"Sensor  : {ok(tf['ok'])}  (ID {tf['sensor_id']})\n"
                f"Measure : {ok(ms['ok'])}  ({ms['distance_mm']} mm, status {ms['status']})\n"
                f"Overall : {ok(r['ok'])}")
            self.status_var.set(f"Self-test: {ok(r['ok'])}")

        self._run("Running self-test…", lambda: self.device.call(
            lambda d: d.request("selftest", wait_s=10.0)), done, "indeterminate")

    # ------------------------------------------------------------------
    def _on_close(self):
        self.live_on = False
        try:
            self.device.close()
        finally:
            self.destroy()


def main():
    app = App()
    app.mainloop()


if __name__ == "__main__":
    main()

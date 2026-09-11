#!/usr/bin/env python3
"""LED render benchmark: sync baseline (xewe-led-os-sync) vs async render task (xewe-led-os).

Both firmwares print, once per FPS window (3 s), when built with DEBUG_RenderPerf=1:
    [BENCH] variant=async frame_ms=20 leds=500 fps_x100=4998 loop_hz=41230 mode_us=812 output_us=95 show_us=15020

Usage:
    python3 bench_fps.py run --port /dev/ttyACM0 --out results/s3-async-capped.csv
    python3 bench_fps.py report results/*.csv

See README.md in this folder for the full test set.
"""

import argparse
import csv
import re
import statistics
import sys
import time
from collections import defaultdict
from datetime import datetime, timezone

try:
    import serial
except ImportError:
    sys.exit("pyserial is required: pip install pyserial")

BENCH_RE     = re.compile(r"\[BENCH\]\s+(.*)$")
MODE_NAMES   = {0: "Solid", 1: "Color Fade", 2: "Color Fade Two Zone", 3: "Brightness Fade", 4: "Pulse", 5: "Rainbow", 6: "Christmas Lights"}
CSV_FIELDS   = ["timestamp", "variant", "frame_ms", "leds", "mode", "sample", "fps", "loop_hz", "mode_us", "output_us", "show_us", "frame_us"]
METRICS      = ["fps", "loop_hz", "mode_us", "output_us", "show_us", "frame_us"]


# ------------------------------------------------------------------------------------------------ serial helpers
class Device:
    def __init__(self, port, baud, verbose):
        self.port    = port
        self.baud    = baud
        self.verbose = verbose
        self.ser     = None
        self.open()

    def open(self, timeout_s=30):
        deadline = time.monotonic() + timeout_s
        while True:
            try:
                self.ser = serial.Serial(self.port, self.baud, timeout=0.2)
                return
            except (serial.SerialException, OSError):
                if time.monotonic() > deadline:
                    raise
                time.sleep(0.5)

    def close(self):
        if self.ser:
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None

    def send(self, cmd):
        if self.verbose:
            print(f"  > {cmd}")
        self.ser.write((cmd + "\n").encode())
        self.ser.flush()
        time.sleep(0.3)

    def readline(self):
        try:
            raw = self.ser.readline()
        except (serial.SerialException, OSError):
            # native USB CDC (S3/C3/C6) disappears on restart; reopen and keep reading
            self.close()
            self.open()
            return ""
        line = raw.decode(errors="replace").strip()
        if line and self.verbose > 1:
            print(f"  < {line}")
        return line

    def restart(self, boot_timeout_s):
        self.send("$system restart")
        self.close()
        time.sleep(2.0)
        self.open()
        self.wait_ready(boot_timeout_s)

    def wait_ready(self, timeout_s):
        """Poll `$led status` until the CLI answers, i.e. setup() finished and loop() runs."""
        deadline  = time.monotonic() + timeout_s
        next_poll = 0.0
        while time.monotonic() < deadline:
            if time.monotonic() >= next_poll:
                try:
                    self.ser.write(b"$led status\n")
                except (serial.SerialException, OSError):
                    self.close()
                    self.open()
                next_poll = time.monotonic() + 3.0
            if "FPS:" in self.readline():
                self.drain(1.0)
                return
        raise TimeoutError(f"device did not answer `$led status` within {timeout_s}s")

    def drain(self, seconds):
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            self.readline()

    def bench_samples(self, leds, count, warmup, window_timeout_s):
        """Collect `count` BENCH reports for `leds`, discarding the first `warmup` matching ones."""
        # drop reports already queued from before the last command; warmup then skips the partial windows
        self.ser.reset_input_buffer()
        samples  = []
        skipped  = 0
        deadline = time.monotonic() + window_timeout_s * (count + warmup + 1)
        while len(samples) < count:
            if time.monotonic() > deadline:
                raise TimeoutError(f"no [BENCH] lines for leds={leds}; is the firmware built with DEBUG_RenderPerf=1?")
            match = BENCH_RE.search(self.readline())
            if not match:
                continue
            fields = dict(kv.split("=", 1) for kv in match.group(1).split() if "=" in kv)
            if int(fields.get("leds", -1)) != leds:
                continue
            if skipped < warmup:
                skipped += 1
                continue
            samples.append(fields)
        return samples


# ------------------------------------------------------------------------------------------------ run
def cmd_run(args):
    dev    = Device(args.port, args.baud, args.verbose)
    out    = open(args.out, "a", newline="")
    writer = csv.DictWriter(out, fieldnames=CSV_FIELDS)
    if out.tell() == 0:
        writer.writeheader()

    try:
        print("waiting for device...")
        dev.wait_ready(args.boot_timeout)

        for leds in args.leds:
            print(f"\n=== leds={leds}")
            # FastLED is registered with num_led at boot, so a new length needs a restart
            dev.send(f"$led set_length {leds}")
            if not args.no_restart:
                dev.restart(args.boot_timeout)
            # sync baseline sizes the mode buffer only on set_length, so apply it again after boot
            dev.send(f"$led set_length {leds}")
            dev.send("$led turn_on")
            dev.send(f"$led set_brightness {args.brightness}")

            for mode in args.modes:
                dev.send(f"$led set_mode {mode}")
                samples = dev.bench_samples(leds, args.samples, args.warmup, args.window_timeout)
                for i, s in enumerate(samples):
                    frame_us = int(s["mode_us"]) + int(s["output_us"]) + int(s["show_us"])
                    writer.writerow({
                        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
                        "variant":   s["variant"],
                        "frame_ms":  s["frame_ms"],
                        "leds":      leds,
                        "mode":      mode,
                        "sample":    i,
                        "fps":       int(s["fps_x100"]) / 100,
                        "loop_hz":   s["loop_hz"],
                        "mode_us":   s["mode_us"],
                        "output_us": s["output_us"],
                        "show_us":   s["show_us"],
                        "frame_us":  frame_us,
                    })
                out.flush()
                fps  = statistics.median(int(s["fps_x100"]) / 100 for s in samples)
                loop = statistics.median(int(s["loop_hz"]) for s in samples)
                print(f"  mode {mode} {MODE_NAMES.get(mode, '?'):<20} "
                      f"variant={samples[0]['variant']} frame_ms={samples[0]['frame_ms']} fps={fps:7.2f} loop_hz={loop:8.0f}")
    finally:
        out.close()
        dev.close()

    print(f"\nwrote {args.out}")


# ------------------------------------------------------------------------------------------------ report
def load(paths):
    groups = defaultdict(lambda: defaultdict(list))
    for path in paths:
        with open(path, newline="") as f:
            for row in csv.DictReader(f):
                key = (int(row["frame_ms"]), int(row["leds"]), int(row["mode"]), row["variant"])
                for m in METRICS:
                    groups[key][m].append(float(row[m]))
    return {k: {m: statistics.median(v) for m, v in metrics.items()} for k, metrics in groups.items()}


def ratio(a, b):
    return f"{b / a:.2f}x" if a else "-"


def cmd_report(args):
    med   = load(args.csv)
    cases = sorted({k[:3] for k in med})

    print("| frame_ms | leds | mode | sync fps | async fps | fps gain | sync loop Hz | async loop Hz | loop gain "
          "| sync mode/out/show us | async mode/out/show us |")
    print("|---:|---:|---|---:|---:|---:|---:|---:|---:|---|---|")
    for frame_ms, leds, mode in cases:
        s = med.get((frame_ms, leds, mode, "sync"))
        a = med.get((frame_ms, leds, mode, "async"))

        def cell(r, m, fmt):
            return format(r[m], fmt) if r else "-"

        def split(r):
            return f"{r['mode_us']:.0f} / {r['output_us']:.0f} / {r['show_us']:.0f}" if r else "-"

        print(f"| {frame_ms} | {leds} | {MODE_NAMES.get(mode, mode)} "
              f"| {cell(s, 'fps', '.2f')} | {cell(a, 'fps', '.2f')} | {ratio(s['fps'], a['fps']) if s and a else '-'} "
              f"| {cell(s, 'loop_hz', '.0f')} | {cell(a, 'loop_hz', '.0f')} | {ratio(s['loop_hz'], a['loop_hz']) if s and a else '-'} "
              f"| {split(s)} | {split(a)} |")


# ------------------------------------------------------------------------------------------------ main
def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub    = parser.add_subparsers(dest="cmd", required=True)

    run = sub.add_parser("run", help="drive a flashed board over serial and append results to a CSV")
    run.add_argument("--port", required=True)
    run.add_argument("--baud", type=int, default=115200)
    run.add_argument("--out", required=True, help="CSV file (appended)")
    run.add_argument("--leds", type=int, nargs="+", default=[100, 500, 1000, 2000])
    run.add_argument("--modes", type=int, nargs="+", default=[0, 5, 2], help="0 Solid, 5 Rainbow, 2 Color Fade Two Zone")
    run.add_argument("--samples", type=int, default=5, help="reports per case (3 s each)")
    run.add_argument("--warmup", type=int, default=2, help="reports discarded after each mode change")
    run.add_argument("--brightness", type=int, default=20, help="keeps a connected strip's current low; does not change timing")
    run.add_argument("--boot-timeout", type=int, default=120)
    run.add_argument("--window-timeout", type=int, default=10, help="max seconds to wait per report")
    run.add_argument("--no-restart", action="store_true", help="skip restart after set_length (send time will not follow N)")
    run.add_argument("-v", "--verbose", action="count", default=0)
    run.set_defaults(func=cmd_run)

    rep = sub.add_parser("report", help="summarize CSVs into a sync vs async markdown table (medians)")
    rep.add_argument("csv", nargs="+")
    rep.set_defaults(func=cmd_report)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()

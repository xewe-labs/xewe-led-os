# LED render benchmark: sync vs async

Compares LED rendering inside `loop()` (**sync**, the code up to commit `35933f0`) with rendering in a dedicated FreeRTOS task (**async**, the render-task change).

It answers two questions:

1. **Render speed:** frames per second, and where each frame's time goes (mode computation, output conversion, `FastLED.show()`).
2. **Main loop freedom:** how many times per second `loop()` runs. This stands in for how responsive serial, the web UI, HomeKit and buttons are.

## Firmware under test

| Image | Source | Rendering | `frame_delay` |
|---|---|---|---|
| `bench-s3-sync-f20`  | `../xewe-led-os-sync` | in `loop()` | 20 ms (shipped 50 fps cap) |
| `bench-s3-sync-f1`   | `../xewe-led-os-sync` | in `loop()` | 1 ms (uncapped) |
| `bench-s3-async-f20` | `xewe-led-os`         | render task | 20 ms (shipped 50 fps cap) |
| `bench-s3-async-f1`  | `xewe-led-os`         | render task | 1 ms (uncapped) |

All four images use the same build settings:
- ESP32-S3 with the release board options from `build/scripts/mac/compile.sh`
- `LED_PIN_DATA 48`, `LED_PIN_CLOCK 12`
- `LED_STRIP_NUM_LEDS_MAX 2000`
- `DEBUG_RenderPerf 1`

`build_bench.sh` sets these in a staged copy of each tree, so the source trees stay unchanged.

### Benchmark patch (identical in both trees)
- **FastLED buffer length:** FastLED is registered with `num_led` LEDs instead of `LED_STRIP_NUM_LEDS_MAX`, so the data sent to the strip scales with the configured length. It's read at boot, so **a length change needs a restart**. The `num_led` read from NVS is clamped to the maximum.
- **Loop counter:** `ModuleController::loop_iterations` counts main loop passes, only when `DEBUG_RenderPerf` is on.
- **Report line:** one line every FPS window (3 s), in the same format in both variants:
  ```
  [BENCH] variant=async frame_ms=20 leds=500 fps_x100=4998 loop_hz=41230 mode_us=812 output_us=95 show_us=15020
  ```
- **Sync only:** `set_all(leds)` is inlined in `LedStrip::loop()` so `show()` can be timed separately. It does the same work as before.

## Hardware and setup
- **Board:** ESP32-S3 SuperMini, data on GPIO48 (the on-board WS2812B, or an external strip on GPIO48). You don't need 2000 physical LEDs: the send time is the same whether or not LEDs are attached.
- **First-boot setup:** flashing erases NVS, so the serial setup runs after every flash. Give **identical answers for all four images**: no clock line, chip **WS2812B**, 5V, no parallel strips, 2000 LEDs. The send timing depends on the chipset.
- **Network:** WiFi and the smart home modules must be in the same state for all runs (same network, same modules enabled). `loop_hz` depends on it.
- **Brightness:** the runner sets brightness 20 to keep a connected strip's current draw low. Brightness does not change timing.

## Test matrix

| Dimension | Values |
|---|---|
| firmware | 4 images above |
| LEDs | 100, 500, 1000, 2000 |
| mode | 0 Solid (least computation), 5 Rainbow (FastLED fill), 2 Color Fade Two Zone (most: noise and blend per LED) |
| samples | 5 reports of 3 s each, after discarding 2 warmup reports following each mode change (covers the 900 ms transition) |

That's 48 cases in total, about 5–6 minutes per image including the restarts.

## Metrics (per 3 s window)

| Field | Meaning |
|---|---|
| `fps` | frames rendered per second |
| `loop_hz` | main `loop()` passes per second |
| `mode_us` | average mode computation per frame, including transition blending |
| `output_us` | average brightness scaling and color-order conversion per frame |
| `show_us` | average `FastLED.show()` time per frame |
| `frame_us` | sum of the three above; the rest of each frame period is waiting |

## Procedure

```bash
# 0. once: toolchain and libraries (build/scripts/<os>/setup_build_environment.sh), plus pyserial
pip install pyserial

# 1. build the four images (or use the prebuilt ones in build/builds/bench/)
cd build/tools/render_bench
./build_bench.sh --sync-root ../../../../xewe-led-os-sync

# 2. for each image: erase, flash, complete first-boot setup over serial, then run
python3 -m esptool --chip esp32s3 --port /dev/ttyACM0 erase_flash
python3 -m esptool --chip esp32s3 --port /dev/ttyACM0 --baud 921600 write_flash 0x0 ../../builds/bench/bench-s3-sync-f20.bin
python3 bench_fps.py run --port /dev/ttyACM0 --out results/s3-sync-f20.csv
#    ...repeat for sync-f1, async-f20, async-f1

# 3. compare (medians)
python3 bench_fps.py report results/*.csv > results/report.md
```

For each LED count the runner does the following:
1. `$led set_length N`, then `$system restart`.
2. Waits until `$led status` answers.
3. Applies `set_length` again. The sync code only resizes the mode buffer on `set_length`, so without this it would keep computing 2000 LEDs.
4. For each mode: `$led set_mode M`, then collects reports.

Close any serial monitor before running, since the runner needs the port.

## Expected results (to confirm or refute)
WS2812B takes about 30 µs per LED to send: roughly 3 / 15 / 30 / 60 ms for 100 / 500 / 1000 / 2000 LEDs.

- **FPS at `f20`:**
  - 100 and 500 LEDs: both variants hit the 50 fps cap.
  - 1000 and 2000 LEDs: send-limited, about 30 and 16 fps.
  - Async should match or slightly beat sync, because its output stage is cheaper.
- **FPS at `f1`:**
  - Sync is capped near 200 fps: `AsyncTimer` only recalculates every 5 ms, which is a quirk of the old frame timer, not of rendering.
  - Async is capped only by `frame_us` plus a 1 ms tick.
- **`loop_hz`:**
  - Sync: once a frame takes most of its slot, every `loop()` pass renders a frame, so expect tens of Hz at 2000 LEDs.
  - Async on S3: the render task is on core 0 and `loop()` on core 1. Expect a high rate that barely changes with LED count.
  - **This is the headline metric.**
- **`output_us`:** clearly lower in async (no per-pixel `millis()`, one lookup table).
- **`mode_us`** for Color Fade Two Zone: lower in async (parameters cached).
- **`show_us`:** the same in both, because it's the hardware. If it's about the send time, `show()` blocks while sending. If it's much smaller, the RMT driver sends in the background and the wait shows up as idle time.

## Acceptance criteria for async
- `fps` is at least the sync value (within 2 %) in every case.
- `loop_hz` is at least the sync value in every case.
- There are no resets, watchdog messages or Guru Meditation errors in the serial log during any run.

## Not covered here
- Race and stress testing (fast mode, brightness and on/off changes during transitions).
- Long-run stability.
- Single-core boards (C3/C6).

Run those separately. `build_bench.sh --chip c3 --data-pin 4 --clock-pin 3` builds the C3 images.

# XeWe LED OS — one ESP32 firmware for addressable LED strips

Personal project (XeWe Labs) · 2024-07-22 → ongoing (this repository since 2026-01-25) · Solo: Max Dokukin · Status: Ongoing (release 2.3.0)

<p align="center">
  <img src="static/media/resources/main_img.png" alt="XeWe LED OS boot banner on the serial console" height="400">
</p>

## Overview

XeWe LED OS is modular firmware for addressable LED strips on ESP32-C3, ESP32-C6 and ESP32-S3 boards. It provides
reusable control, connectivity and persistence, so an LED project does not need its own glue code for WiFi, voice
assistants, storage and buttons. The firmware centers on a serial command line (`$<module> <command> [args]`) and layers
more interfaces on top of it — a local web UI with a weekly scheduler, Apple HomeKit, Amazon Alexa, Home Assistant over
MQTT discovery and physical buttons — all kept in sync, with every setting stored in ESP32 NVS. The LED chip (46
FastLED chipsets), colour order and strip length are chosen at first boot, so one precompiled image per pin layout covers
most strips; release 2.3.0 ships 43 such images and can be flashed from the browser. It grew out of two 2024 drafts and
the [xewe-led-os-draft](https://github.com/xewe-labs/xewe-led-os-draft) repository (2025), was re-based on
[XeWe OS](https://github.com/xewe-labs/xewe-os) in January 2026, and moved to the XeWe OS 2.0 framework in release 2.3.0.

## Highlights

- 13 modules behind one `Module` lifecycle; 5 of them (LED strip, web, Home Assistant, HomeKit, Alexa) share one sync contract, so a change from any interface reaches all the others (`src/Modules/Module/ModuleController.h`)
- 46 LED chipsets (32 one-wire, 14 with a clock line), 6 colour orders and up to 2,000 LEDs selected at runtime (`src/Modules/Hardware/LedStrip/LedStrip.h`, `doc/about_project/supported_led_chips.txt`)
- 7 animation modes with smooth cross-fades between them, rendered by a dedicated FreeRTOS task at a 20 ms frame period (`LedStripConfig`)
- 45 module commands on the CLI (30 of them for the LED strip) plus `status` / `reset` / `enable` / `disable` per module
- Release 2.3.0: 43 prebuilt images for C3 (7), C6 (14) and S3 (22) pin layouts, each a 4 MB merged image for the web flasher (`build/release_matrix.csv`, `static/firmware/releases/2.3.0/`)

## How it works

```
Serial CLI ─┐
Web UI  ────┤                       ┌─ LedStrip ─ ModeController ─ Mode (7) ─┐
HomeKit ────┼─→ CommandExecutor ───→│  Brightness                            ├─→ render task ─→ FastLED.show() ─→ strip
Alexa  ─────┤   / sync_* calls      └─ Wifi · Time · Scheduler · Buttons     ┘
Home Asst. ─┘         │
                      └─→ ModuleController.sync_*() fans every change out to the other interfaces; Nvs persists it
```

- **Module framework** (`src/Modules/Module/`) — `Module` gives every feature the same lifecycle (`begin_routines_required` / `_init` on first boot / `_regular` / `_common`, `loop`, `enable`, `disable`, `reset`, `status`), dependency checks (`add_requirement`) and generic CLI commands. `SyncModule` adds `sync_color`, `sync_brightness`, `sync_state`, `sync_mode`, `sync_length`. `ModuleController` owns all 13 modules and fans sync calls out with per-interface flags.
- **Core** (`src/Modules/Core/`) — `SerialPort` (non-blocking UART I/O, menus, prompts at 115,200 baud), `CommandExecutor` (parses `$group command args`, quoted strings, argument-count checks), `Nvs` (scalar, blob and typed-object storage; `FlexData` structs serialise to a compact blob for flash and to JSON for the API — see `src/Modules/Core/Nvs/README.md`), `System` (restart, chip/build info, MAC, UID, device name).
- **LED strip** (`src/Modules/Hardware/LedStrip/`) — first-boot setup asks for the data/clock pins, chip, voltage, parallel lines, LED count and colour order; `ModeController` blends the old and new mode over 900 ms; `Brightness` fades over 500 ms; since 2026-09-10 frames are produced by a pinned `led_render` FreeRTOS task, so the main loop never waits for the strip.
- **Interfaces** (`src/Modules/Software/SmartHome/`) — `WebInterface` (HTTP on port 80, WebSocket push on port 81, schedule editor), `HomeKit` (HomeSpan, Lighting accessory), `Alexa` (Espalexa colour device), `HomeAssistant` (MQTT discovery: a JSON-schema light, a mode select and per-mode number entities; broker credentials provisioned over HTTP by the [Home Assistant integration](https://github.com/xewe-labs/xewe-led-os-homeassistant)).
- **Time and scheduling** (`src/Modules/Software/Time/`) — timezone auto-detection by racing three geo-IP services, SNTP against three servers, and a weekly scheduler that runs stored commands in time blocks.
- **Buttons** (`src/Modules/Hardware/Buttons/`) — bind any CLI command to a GPIO with pull-up/pull-down, `on_press` / `on_release` / `on_change` and a debounce time.
- **Web UI assets** (`src/Modules/Software/SmartHome/WebInterface/{static,templates}/`) — the schedule page was prototyped in Flask in [xewe-led-os-frontend](https://github.com/xewe-labs/xewe-led-os-frontend) and exported to `PROGMEM` headers.

### Usage

The CLI is the primary control interface. Commands use this format:

```text
$<module> <command> [param0] [param1] ... [paramN]
```

```text
$help                      # list modules and commands
$led help                  # list LED commands
$led set_brightness 128
$led set_rgb 255 0 0
$led set_mode 5            # 0 Solid, 1 Color Fade, 2 Color Fade Two Zone, 3 Brightness Fade, 4 Pulse, 5 Rainbow, 6 Christmas Lights
$wifi scan
$buttons add 9 "$led toggle_state" pullup on_press 50
$time set_zone GMT-08:00
```

Command rules: parameters are space-separated; most numeric parameters are `0-255`; commands are case-sensitive;
several commands can be sent one after another. After the device joins WiFi, open `http://<device-ip>` for the web UI
and `http://<device-ip>/schedule` for the weekly scheduler.

### Modules and commands

| Module | Prefix | Commands (besides `status`, `reset`) |
|---|---|---|
| System | `$system` | `restart`, `reboot`, `info`, `set_device_name`, `mac`, `uid` |
| LED strip | `$led` | `set_rgb`, `set_r/g/b`, `adj_rgb`, `adj_r/g/b`, `set_hsv`, `set_hue/sat/val`, `adj_hsv`, `adj_hue/sat/val`, `set_brightness`, `adj_brightness`, `set_state`, `toggle_state`, `turn_on`, `turn_off`, `set_mode`, `adj_mode`, `set_mode_param`, `adj_mode_param`, `get_mode_params`, `reset_current_mode`, `set_length`, `set_color_order` |
| WiFi | `$wifi` | `connect`, `disconnect`, `scan`, `enable`, `disable` |
| Web interface | `$web_interface` | `enable`, `disable` |
| Home Assistant | `$homeassistant` | `enable`, `disable` |
| HomeKit | `$homekit` | `enable`, `disable` |
| Alexa | `$alexa` | `enable`, `disable` |
| Time | `$time` | `set_zone`, `fetch`, `enable`, `disable` |
| Scheduler | `$schedule` | `add`, `remove` |
| Buttons | `$buttons` | `add`, `remove`, `enable`, `disable` |

Full reference with sample usage: [doc/modules.md](doc/modules.md). Resetting HomeKit or Alexa requires removing the device from the Home or Alexa app by hand.

### Configuration

- Modules can be enabled or disabled at runtime (`$wifi disable`, `$homekit enable`); a disabled module keeps its pointers valid for the others.
- Web interface: LAN-only; stateless clients receive the full state on connect; WebSocket push with a 1 s heartbeat and reconnect handling.
- Compile-time settings live in `Config.h`: `LED_PIN_DATA`, `LED_PIN_CLOCK`, `LED_STRIP_NUM_LEDS_MAX` (2,000). Per its comment, frame rate falls roughly as 1,000,000 / (LEDs × 30), so strips over 1,000 LEDs drop below 30 fps.
- Pins cannot be changed after upload: pick the image for your pin layout, or set the pins in `Config.h` and compile.

### Supported LED chips

46 FastLED chipsets (`doc/about_project/supported_led_chips.txt`). If your strip has only a DATA line and no CLOCK line, ignore the CLOCK pin when choosing the firmware.

- One-wire (32): APA104, APA106, GE8822, GS1903, GW6205, GW6205_400, LPD1886, LPD1886_8BIT, NEOPIXEL, PL9823, SK6812, SK6822, SM16703, SM16824E, TM1803, TM1804, TM1809, TM1812, TM1829, UCS1903, UCS1903B, UCS1904, UCS1912, UCS2903, WS2811, WS2811_400, WS2812, WS2812B, WS2813, WS2815, WS2816, WS2852
- Clocked / SPI (14): APA102, APA102HD, DOTSTAR, DOTSTARHD, HD107, HD107HD, LPD6803, LPD8806, P9813, SK9822, SK9822HD, SM16716, WS2801, WS2803

## Results

| Metric | Value | Baseline / note |
|---|---|---|
| Supported boards | ESP32-C3, ESP32-C6, ESP32-S3 | `build/release_matrix.csv` |
| Prebuilt images in release 2.3.0 | 43 (C3 7, C6 14, S3 22) | releases 1.0.1–2.1.0 shipped one C3 image each |
| Releases in `static/firmware/releases/` | 9 (1.0.1 → 2.3.0) | GitHub tag `v2.3.0`, 2026-08-31 |
| LED chipsets / colour orders / modes | 46 / 6 / 7 | `LedStrip.h`, `ModeRegistrar` ids 0–6 |
| Source size | 10,179 lines of C++ in 61 files + 1,870 lines of embedded web assets | `src/`, raw `wc -l` |
| Commits | 344 in this repository (2026-01-25 → 2026-09-14) | 437 more in xewe-led-os-draft (2025-05-19 → 2026-01-25) |

The project is firmware rather than a model, so its results are delivery measures: boards, images, chipsets and code size.
Release 2.3.0 compiled 43 images in 3,587 s of build time (56–96 s each, `meta.json`).

## Getting started

Supported hardware: ESP32-C3, ESP32-C6 or ESP32-S3 and an addressable LED strip (one of the chipsets above).

### Flash from the browser (easiest)

1. Open https://maxdokukin.com/projects/xewe-led-os and scroll to **Firmware Flasher**.
2. Pick your chip and pin layout, connect the board and click **Install**.
3. Open the serial console (115,200 baud), reset the board and answer the setup questions (pins, chip, voltage, LED count, colour order, WiFi, then each smart-home module).
4. Type `$help` to see all commands.

### Build with the scripts

```bash
git clone https://github.com/xewe-labs/xewe-led-os
cd xewe-led-os/build/scripts/mac          # or linux/ ; Windows: build/scripts/windows/*.ps1

./setup_build_environment.sh              # installs the toolchain and the libraries in build/libraries/required_libraries.txt

ls /dev/cu.*                              # find the port the ESP32 is connected to

# build: ./build.sh -c <target_chip> -p <port>
./build.sh -c c3                                # build
./build.sh -c c3 -p /dev/cu.usbmodem11143201    # build and upload
```

Other scripts in the same folder: `compile.sh`, `upload.sh`, `listen_serial.sh`, `format.sh`, `release.sh`.

### Build with the Arduino IDE

1. Set up the Arduino IDE for ESP32 development and open `xewe-led-os.ino`.
2. Install the libraries listed in [`build/libraries/required_libraries.txt`](build/libraries/required_libraries.txt): FastLED 3.10.3, ArduinoJson and PubSubClient, plus the patched Espalexa, HomeSpan and WebSockets builds the file points to; place them in `Arduino/libraries`.
3. Set `LED_PIN_DATA` / `LED_PIN_CLOCK` in `Config.h` if your layout is not prebuilt, then compile and upload.

### Smart-home prerequisites

- **Apple HomeKit** — an Apple TV or HomePod as a home hub and an Apple device for pairing; the setup QR is linked on the serial console.
- **Alexa** — an Alexa-enabled speaker on the same network; the strip is found by Alexa device discovery.
- **Home Assistant** — an MQTT broker (the Mosquitto app works) and the [xewe-led-os-homeassistant](https://github.com/xewe-labs/xewe-led-os-homeassistant) integration from HACS; or POST the broker details to `http://<device-ip>/provision`.

There is no automated test suite; changes are checked by compiling for all three chips and running on hardware.

## Documents

- [Module and command reference](doc/modules.md)
- [Adding a module](doc/adding_module/adding_module.md) · [Contributing](doc/adding_module/contributing.md) · [source templates](doc/adding_module/src_templates/)
- [Repository structure](doc/about_project/file_structure.md) · [Code quality matrix](doc/about_project/code_quality_matrix.md) (15 units human-reviewed, 8 mixed human/AI)
- [NVS and FlexData storage](src/Modules/Core/Nvs/README.md)
- [Supported LED chips](doc/about_project/supported_led_chips.txt) · [Release matrix](build/release_matrix.csv)
- Release notes: [2.3.0](static/firmware/releases/2.3.0/release_notes.txt) · [2.2.2](static/firmware/releases/2.2.2/release_notes.txt)
- Project page and web flasher: https://maxdokukin.com/projects/xewe-led-os
- License: [GPL-3.0](LICENSE.txt)

### Related repositories

| Repository | Role |
|---|---|
| [xewe-led-os-homeassistant](https://github.com/xewe-labs/xewe-led-os-homeassistant) | Home Assistant custom integration (HACS) that pairs the device and hands it MQTT credentials |
| [xewe-led-os-frontend](https://github.com/xewe-labs/xewe-led-os-frontend) | Flask prototype of the schedule UI and the exporter that turns it into firmware headers |
| [xewe-led-os-draft](https://github.com/xewe-labs/xewe-led-os-draft) | 2025 predecessor (437 commits) and the two 2024 legacy drafts |
| [xewe-os](https://github.com/xewe-labs/xewe-os) | The general-purpose ESP32 firmware this repository was forked from in January 2026 |

### XeWe LED builds

The one-device projects of the XeWe LED family, oldest first:

| Title | Portfolio page | GitHub repo |
|---|---|---|
| Hookah Decorative LED | [xewe-led-hookah-os](https://maxdokukin.com/projects/xewe-led-hookah-os) | [xewe-labs/xewe-led-hookah-os](https://github.com/xewe-labs/xewe-led-hookah-os) |
| XeWe LED - Buggy | [xewe-led-buggy](https://maxdokukin.com/projects/xewe-led-buggy) | [xewe-labs/xewe-led-buggy](https://github.com/xewe-labs/xewe-led-buggy) |
| Quantum LED Controller | [xewe-led-quantum-controller](https://maxdokukin.com/projects/xewe-led-quantum-controller) | [xewe-labs/xewe-led-quantum-controller](https://github.com/xewe-labs/xewe-led-quantum-controller) |
| XeWe LED - Bed | [xewe-led-bed](https://maxdokukin.com/projects/xewe-led-bed) | [xewe-labs/xewe-led-bed](https://github.com/xewe-labs/xewe-led-bed) |
| sUv Bot | [xewe-led-suv-bot](https://maxdokukin.com/projects/xewe-led-suv-bot) | [xewe-labs/xewe-led-suv-bot](https://github.com/xewe-labs/xewe-led-suv-bot) |
| PosterOS | [xewe-led-poster-os](https://maxdokukin.com/projects/xewe-led-poster-os) | [xewe-labs/xewe-led-poster-os](https://github.com/xewe-labs/xewe-led-poster-os) |
| XeWe LED - Desk | [xewe-led-desk](https://maxdokukin.com/projects/xewe-led-desk) | [xewe-labs/xewe-led-desk](https://github.com/xewe-labs/xewe-led-desk) |
| XeWe LED - Ceiling | [xewe-led-ceiling](https://maxdokukin.com/projects/xewe-led-ceiling) | [xewe-labs/xewe-led-ceiling](https://github.com/xewe-labs/xewe-led-ceiling) |
| XeWe LED - Bathroom | [xewe-led-bathroom](https://maxdokukin.com/projects/xewe-led-bathroom) | [xewe-labs/xewe-led-bathroom](https://github.com/xewe-labs/xewe-led-bathroom) |
| XeWe LED - Wardrobe | [xewe-led-wardrobe](https://maxdokukin.com/projects/xewe-led-wardrobe) | [xewe-labs/xewe-led-wardrobe](https://github.com/xewe-labs/xewe-led-wardrobe) |
| XeWe LED - Web | [xewe-led-web-draft](https://maxdokukin.com/projects/xewe-led-web-draft) | [xewe-labs/xewe-led-web-draft](https://github.com/xewe-labs/xewe-led-web-draft) |
| Retro Christmas Lights | [xewe-led-retro-christmas-lights](https://maxdokukin.com/projects/xewe-led-retro-christmas-lights) | [xewe-labs/xewe-led-retro-christmas-lights](https://github.com/xewe-labs/xewe-led-retro-christmas-lights) |

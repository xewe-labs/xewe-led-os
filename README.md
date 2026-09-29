# XeWe LED OS (draft) — the 2025 predecessor of XeWe LED OS

Personal project (XeWe Labs) · 2025-05-19 → 2026-01-25 (legacy drafts from 2024-07-22) · Solo: Max Dokukin · Status: Completed (archived; continued in [xewe-led-os](https://github.com/xewe-labs/xewe-led-os))

## Overview

The project distils the repetitive work between many one-off LED builds into one piece of software — an ESP32
"operating system" for addressable LED strips. This repository is the 2025 generation of
that idea: a serial command line to control the strip, WiFi with stored credentials, a local web server with WebSocket
state push, Amazon Alexa and Apple HomeKit control, physical buttons, and NVS storage, organised by a `SystemController`
that owns *modules* (system services) and *interfaces* (ways to control the LEDs). It went from a first serial CLI on
2025-05-19 to "release 2.0" on 2025-10-20 across 437 commits, and was continued on 2026-01-25 in
[xewe-led-os](https://github.com/xewe-labs/xewe-led-os). The `legacy/` folder keeps the two 2024 drafts it grew from.

## Highlights

- Control from the serial CLI, a web browser on the same network, Alexa (voice + app), Apple HomeKit/Siri (needs an Apple TV or HomePod hub) and physical buttons, on ESP32-C3, C6 and S3
- Releases in the history: 0.1 and 0.2 (2025-05-21), 1.0 (2025-05-28, web server + WebSockets), 1.1 (2025-06-03, Alexa; binary published on maxdokukin.com), 2.0 (2025-10-20, module/interface rewrite) (`git log`)
- `SystemController` with a common `Module` base and an `Interface` subclass for everything that must stay in sync with the LED state (`src/SystemController/`, `src/Interfaces/Interface/`)
- Migrated from Adafruit NeoPixel (2025-05-20) to FastLED (2025-05-22); a 2025-10-06 commit records Adafruit as "25% slower than fastled" (`git log`)
- 6,648 lines of C++ in `src/` at the final commit; snapshots in `doc/versions/line_counts_v1..v3.txt` (6,332 → 6,023 → 5,965 lines)

## How it works

```
xewe-led-os.ino → SystemController ─┬─ Modules:    System · SerialPort · CommandParser · Wifi · Buttons
                                    └─ Interfaces: LedStrip · Nvs · Web · Homekit · Alexa   (kept in sync with the LED state)
```

- **SystemController** (`src/SystemController/`) — creates every module and interface, runs their `begin` and `loop`, and propagates state changes to all interfaces.
- **Modules** (`src/Modules/`) — `SerialPort` (CLI I/O), `CommandParser` (`$<group> <command> <args>`), `System` (restart, status, reset), `Wifi` (join, store and reset credentials), `Buttons` (GPIO → command).
- **Interfaces** (`src/Interfaces/`) — `LedStrip` (FastLED output, `Brightness`, `AsyncTimer` transitions, modes `ColorSolid`, `ColorChanging`, `PerlinFade`), `Nvs` (persistent state), `Web` (web page + WebSockets), `Homekit` (HomeSpan), `Alexa` (Espalexa).
- **Templates** (`src_templates/`) — skeletons for a new module or interface.
- **Build scripts** (`build/scripts/`) — `setup_build_enviroment.sh`, `build.sh`, `compile.sh`, `upload.sh`, `listen_serial.sh`, `push_to_git.sh`; helpers in `scripts/` count and print source files.
- **Configuration** (`src/Config.h`, `src/ConfigDock.h`) — LED pin, strip type, colour order and maximum length (600 LEDs) are compile-time defines here; making them runtime choices is one of the things the successor changed.

### CLI

Commands follow `$<cmd_group> <cmd_name> <param_0> <param_1> ... <param_n>`; parameters are 0–255 and separated by spaces
(`$led set_brightness <0-255>`, `$led set_rgb <0-255> <0-255> <0-255>`). `$help` lists everything; `$system help`,
`$wifi help` and `$led help` list one group. Other devices can drive the LEDs by sending commands over the serial port.

Modules (WiFi, HomeKit, …) can be enabled or disabled at runtime (`$wifi disable`, `$<module> enable`); every module supports
`$<module> status` and `$<module> reset`, and toggleable ones `$<module> enable` / `$<module> disable`.

### Lineage

| Generation | Where | Dates | Commits |
|---|---|---|---|
| Draft 1 (`Arduino-XeWe-LED`: LedController, AsyncTimer, PerlinFade, SolidColor) | `legacy/xewe-led-os-draft-1.zip` | 2024-07-22 → 2024-08-08 | 28 (25 in 2024) |
| Draft 2 (`Arduino-XeWe-LED-New`: terminal interface, classes/config/functions layout) | `legacy/xewe-led-os-draft-2.zip` | 2024-10-13 → 2024-12-02 | 47 (43 in 2024) |
| This repository | `main` | 2025-05-19 → 2026-01-25 | 437 (444 on all branches) |
| XeWe LED OS | [xewe-led-os](https://github.com/xewe-labs/xewe-led-os) | 2026-01-25 → ongoing | 344 |

Each zip contains the full source and its own `.git` history. Besides `main`, the branches `ReliableDock-Release-1.5` (2025-07-27/28),
`ReliableDock-Release-2.0` (2025-10-21) and `binaries` (2026-01-17) carry a few release-related commits each.

## Results

| Metric | Value | Baseline / note |
|---|---|---|
| Releases | 0.1, 0.2, 1.0, 1.1, 2.0 (2025-05-21 → 2025-10-20) | commit messages |
| Control interfaces | 5: serial CLI, web, Alexa, HomeKit, buttons | `boot_log_v2.txt` banner |
| Source size | 6,648 lines of C++ (`src/`, final commit) | 6,332 / 6,023 / 5,965 in the v1 / v1.9 / v3 snapshots |
| Commits | 437 on `main` | 2025-05-19 → 2026-01-25 |

## Getting started

This repository is archived; new installs should use [XeWe LED OS](https://github.com/xewe-labs/xewe-led-os). The
original instructions are kept below.

### Easy way — flash from the website

Upload precompiled software from https://maxdokukin.com/projects/xewe-led-os
![Screenshot 2026-01-17 at 09.44.48.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2009.44.48.webp)
Select the port
![Screenshot 2026-01-17 at 09.49.05.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2009.49.05.webp)
Click install
![Screenshot 2026-01-17 at 09.52.39.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2009.52.39.webp)
After installation finishes, go to "Logs & Console"
![Screenshot 2026-01-17 at 09.53.53.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2009.53.53.webp)
Click "Reset Device", this will reboot the board
![Screenshot 2026-01-17 at 09.54.50.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2009.54.50.webp)
Finish by following the Serial Port instructions.
**Note: sometimes a line of text can go missing. If the next step makes no sense, hit "Enter".
To avoid this issue, use a more robust Serial Port monitor app at 115200 baud.**
![Screenshot 2026-01-17 at 09.55.34.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2009.55.34.webp)
You will see "Rebooting..." at the end of the setup
![Screenshot 2026-01-17 at 10.00.07.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2010.00.07.webp)
Done. Try `$help` to see all commands available
![Screenshot 2026-01-17 at 10.03.41.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2010.03.41.webp)

### Arduino IDE

- Set up the IDE for ESP32 development and upload a sample sketch to verify the environment.
- Install the libraries: FastLED (https://github.com/FastLED/FastLED), a modified build of Espalexa (per the original README: "modified, use Github"), HomeSpan (https://github.com/HomeSpan/HomeSpan) and WebSockets (https://github.com/Links2004/arduinoWebSockets).

**On a Mac with Apple Silicon you need the Intel edition of the Arduino IDE + Rosetta; otherwise the ESP32 sketches won't compile or will core dump.**

Make sure the board settings match, from "USB CDC on Boot" to "Zigbee Mode":
![Screenshot 2026-01-17 at 14.02.22.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2014.02.22.webp)

### Scripts (Mac/Linux)

```bash
cd build/scripts
./setup_build_enviroment.sh
./build.sh -t <chip> -p <serial_port>       # e.g. ./build.sh -t c3 -p /dev/cu.usbmodem11143201
```

## Documents

- Boot logs: [doc/initial_boot_log.txt](doc/initial_boot_log.txt), [doc/routine_boot_log.txt](doc/routine_boot_log.txt), [doc/versions/boot_log_v2.txt](doc/versions/boot_log_v2.txt)
- Plans: [doc/todo/big_picture.txt](doc/todo/big_picture.txt), [doc/todo/todo.txt](doc/todo/todo.txt)
- Size snapshots: [doc/versions/](doc/versions/)
- Hardware photo: [IMG_2737.webp](static/media/resources/readme/IMG_2737.webp)
- Legacy drafts: [legacy/](legacy/)
- Successor: [XeWe LED OS](https://github.com/xewe-labs/xewe-led-os) · project page https://maxdokukin.com/projects/xewe-led-os
- License: [PolyForm Noncommercial 1.0.0](LICENSE.md) with the [No-AI addendum](LICENSE-NO-AI.md)

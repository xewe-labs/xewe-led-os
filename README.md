# XeWe LED OS

Firmware for addressable LED strips on ESP32-C3, -C6 and -S3, for people who want one strip in
Apple Home, Alexa and Home Assistant at once, with a web page, schedules and buttons on the
device itself.

## Problem

- A strip that speaks one smart-home system needs a bridge or a hub to reach the others.
- Schedules and button bindings that live in a phone app or a hub stop when that hub is down.
- A different LED chip or data pin means editing source and rebuilding by hand.

## Solution

One firmware image carries every integration, and the device keeps its own settings, schedules
and buttons in flash. It is built on XeWe OS, and three ideas carry it:

- Every feature is a module with its own `$` console commands, and most modules can be enabled
  or disabled at run time. HomeKit, Alexa, Home Assistant and the schedule page are the four
  modules in `src/`. The LED engine, Wi-Fi, web interface, time, scheduler and buttons come from
  xewe-os-modules.
- Buttons and schedules run console commands, so anything you can type on the serial console you
  can bind to a button or a time of day.
- Each release builds one image per row of `release_matrix.csv`, 43 chip and pin combinations,
  so most boards need no build at all.

## See it

On first boot, after you answer "yes" to HomeKit, the serial console prints:

```text
Open the link with the Setup QR below and scan it
with your iPhone/iPad
https://github.com/xewe-labs/xewe-led-os/raw/main/doc/media/HomeKit_Connect_QR.png
If using Mac, go to the Home App and add device
using code 4663-7726

The setup process will continue automatically
after device is pared with HomeKit
TO ABORT PRESS (x):
```

Scan the code in the Home app and the strip appears there as a light.

## Why not WLED

| | WLED | XeWe LED OS |
|---|---|---|
| Effects | a large effect library | 7 animated modes |
| Apple Home | through a bridge such as Homebridge or Home Assistant | pairs directly through HomeSpan |
| Buttons and schedules | its own presets and timers | any console command |
| License | EUPL-1.2 | GPL-3.0-only |

## Install

Find your chip and LED pins in `firmware_map.csv` of the
[latest GitHub Release](https://github.com/xewe-labs/xewe-led-os/releases/latest). The columns
are `CHIP`, `XEWE_LED_PIN_DATA` and `XEWE_LED_PIN_CLOCK`, and `_BUILD_NOTES` names the board,
such as `Led Dock V2 pinout`. Download the `_ASSET` of that row and write it at 0x0. Writing it
erases the stored settings.

```sh
esptool.py --chip esp32c6 write_flash 0x0 c6_20_19_3.0.0-c6-xewe-led-os.bin   # esp32c3, esp32c6, esp32s3
```

For other pins, build from source. You need Python 3.11 or newer and git. Setup installs into
the project folder and `~/.xewe-os`, so it never needs sudo.

```sh
git clone https://github.com/xewe-labs/xewe-led-os && cd xewe-led-os
./setup.sh
build/tools/.venv/bin/python -m xewe build --chip c3
build/tools/.venv/bin/python -m xewe flash --chip c3
build/tools/.venv/bin/python -m xewe serial
```

The release asset layout is in [doc/about_project](doc/about_project/README.md). If you come
from 2.3, read [doc/UPGRADING_FROM_2_3.md](doc/UPGRADING_FROM_2_3.md) first, because settings
are not carried over.

## Usage

On first boot the serial console asks for the device name, Wi-Fi, each module's questions and
the LED strip (chipset, voltage, length, colour order). You answer them once, and the device
stores the answers in NVS.

To control the strip, type `$led` commands on the console, or open the device's address in a
browser for the LED page and the schedule editor at `/schedule`.

To connect a smart-home system, follow its module README:
[HomeKit](src/HomeKit/README.md), [Alexa](src/Alexa/README.md) or
[Home Assistant](src/HomeAssistant/README.md). The [schedule page](src/SchedulePage/README.md)
README lists its routes. The commands of the other modules are in
[xewe-os-modules](https://github.com/xewe-labs/xewe-os-modules).

## Status

Version 3.0.0 runs on ESP32-C3, -C6 and -S3 with 4 MB flash and is released as 43 images. It
behaves like 2.3.0, and [doc/UPGRADING_FROM_2_3.md](doc/UPGRADING_FROM_2_3.md) lists every
difference. Updates over the air are not supported. Arduino IDE builds are not supported either,
because `./setup.sh` generates the module list. The per-feature guides in
`doc/setup_and_troubleshooting/` are not written yet.

---

## Development

```sh
./setup.sh
build/tools/.venv/bin/python -m xewe test --unit-only
build/tools/.venv/bin/python -m xewe format
build/tools/.venv/bin/python -m xewe brand-lint .
```

Values are set in `Config.h`, and the LED pins of each release image in `release_matrix.csv`.
[doc/TESTING.md](doc/TESTING.md) covers the tests and CI, and [tests/MANUAL.md](tests/MANUAL.md)
the checks that need a phone, an Echo and a broker. The release procedure is in
[scripts/README.md](scripts/README.md). Every `xewe` command is in the
[tools command README](https://github.com/xewe-labs/xewe-os-tools/blob/main/src/xewe/README.md).

## Layout

- [src/](src/README.md) holds the four product modules.
- [tests/](tests/README.md) holds the unit tests and the board tests.
- [scripts/](scripts/README.md) holds the release script.
- [doc/](doc/README.md) holds the guides, the upgrade notes and the template's pages on the
  [XeWe OS ecosystem](doc/XEWE_OS_ECOSYSTEM.md) and [writing a module](doc/WRITING_A_MODULE.md).
- `xewe-led-os.ino` is the sketch, `Config.h` the configuration, `xewe.toml` the manifest and
  `setup.sh` the installer.
- XeWeCore lives in [xewe-os-core](https://github.com/xewe-labs/xewe-os-core) and the ready-made
  modules in [xewe-os-modules](https://github.com/xewe-labs/xewe-os-modules). `./setup.sh`
  fetches them into `build/`.

## License

GPL-3.0-only, see [LICENSE.txt](LICENSE.txt).

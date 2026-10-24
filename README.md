# XeWe LED OS

Firmware for addressable LED strips on ESP32-C3, -C6 and -S3, for people who want one strip in Apple Home, Alexa and Home Assistant at once.

<p align="center"><img src="doc/media/HERO.webp" alt="LED strip" width="360"></p>

## Problem

- A strip that speaks one smart-home system needs a bridge or a hub to reach the others.
- Schedules and button bindings that live in a phone app or a hub stop when that hub is down.
- A different LED chip or data pin means editing source and rebuilding by hand.

## Solution

One firmware image carries every integration, and the device keeps its settings, schedules and buttons in flash. Three ideas carry it:

- Every feature is a module with its own `$` console commands, and most modules can be enabled or disabled at run time.
- Buttons and schedules run console commands, so anything you can type on the console you can bind to a button or a time.
- Each release builds one image per row of `release_matrix.csv`, 43 chip and pin combinations, so most boards need no build.

## See it

On first boot, after you answer "yes" to HomeKit, the serial console prints:

```text
Open the link with the Setup QR below and scan it
with your iPhone/iPad
https://github.com/xewe-labs/xewe-led-os/raw/main/doc/media/HomeKit_Connect_QR.png
If using Mac, go to the Home App and add device
using code 4663-7726
```

Scan the code and the strip appears as a light.

## Why not WLED

| | WLED | XeWe LED OS |
|---|---|---|
| Effects | a large effect library | 7 animated modes |
| Apple Home | through a bridge such as Homebridge or Home Assistant | pairs directly through HomeSpan |
| Buttons and schedules | its own presets and timers | any console command |
| License | EUPL-1.2 | GPL-3.0-only |

## Install

Find your chip and pins in `firmware_map.csv` of the [latest release](https://github.com/xewe-labs/xewe-led-os/releases/latest), then write the `_ASSET` of that row at 0x0. This erases stored settings.

```sh
esptool.py --chip esp32c6 write_flash 0x0 c6_20_19_3.0.0-c6-xewe-led-os.bin
```

For other pins, build from source. You need Python 3.11 or newer and git, and setup never needs sudo.

```sh
git clone https://github.com/xewe-labs/xewe-led-os && cd xewe-led-os
./setup.sh
build/tools/.venv/bin/python -m xewe build --chip c3
build/tools/.venv/bin/python -m xewe flash --chip c3
```

The asset layout is in [doc/about_project](doc/about_project/README.md). Coming from 2.3, read [doc/UPGRADING_FROM_2_3.md](doc/UPGRADING_FROM_2_3.md) first, because settings are not carried over.

## Usage

On first boot the console asks for the device name, Wi-Fi, each module's questions and the LED strip, and the device stores your answers. Open it with `build/tools/.venv/bin/python -m xewe serial`.

To control the strip, type `$led` commands, or open the device's address in a browser for the LED page and the schedule editor at `/schedule`.

To connect a smart-home system, follow its module README: [HomeKit](src/HomeKit/README.md), [Alexa](src/Alexa/README.md) or [Home Assistant](src/HomeAssistant/README.md).

## Status

Version 3.0.0 runs on ESP32-C3, -C6 and -S3 with 4 MB flash. It behaves like 2.3.0 apart from the upgrade notes. Updates over the air and Arduino IDE builds are not supported. The per-feature guides come next.

---

## Development

```sh
./setup.sh
build/tools/.venv/bin/python -m xewe test --unit-only
build/tools/.venv/bin/python -m xewe format
build/tools/.venv/bin/python -m xewe brand-lint .
```

[doc/TESTING.md](doc/TESTING.md) covers the tests and CI. The release procedure is in [scripts/README.md](scripts/README.md).

## Layout

- [src/](src/README.md) holds the four product modules.
- [tests/](tests/README.md) holds the unit tests and the board tests.
- [scripts/](scripts/README.md) holds the release script.
- [doc/](doc/README.md) holds the guides and the upgrade notes.
- `xewe-led-os.ino` is the sketch, `Config.h` the configuration, `xewe.toml` the manifest.
- XeWeCore and the ready-made modules live in [xewe-os-core](https://github.com/xewe-labs/xewe-os-core) and [xewe-os-modules](https://github.com/xewe-labs/xewe-os-modules).

## License

GPL-3.0-only, see [LICENSE.txt](LICENSE.txt).

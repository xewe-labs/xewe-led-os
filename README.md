# XeWe LED OS

Firmware for addressable LED strips on ESP32-C3, -C6 and -S3, built on XeWe OS: serial console,
web interface with schedule editor, buttons, and Apple HomeKit, Alexa and Home Assistant control.

## Install a release

Pick your chip and LED pins in `firmware_map.csv` of the [latest GitHub
Release](https://github.com/xewe-labs/xewe-led-os/releases/latest) (`CHIP`, `XEWE_LED_PIN_DATA`,
`XEWE_LED_PIN_CLOCK`; `_BUILD_NOTES` names the board, such as `Led Dock V2 pinout`), download the
`_ASSET` of that row and write it at 0x0. It erases the stored settings.

```sh
esptool.py --chip esp32c6 write_flash 0x0 c6_20_19_3.1.0-c6-xewe-led-os.bin   # esp32c3, esp32c6, esp32s3
```

Other pins: build from source. The asset layout is in [doc/about_project](doc/about_project/README.md).

## Build from source

```sh
git clone https://github.com/xewe-labs/xewe-led-os && cd xewe-led-os
./setup.sh                                          # installs everything into build/
build/tools/.venv/bin/python -m xewe build --chip c3
build/tools/.venv/bin/python -m xewe flash --chip c3
build/tools/.venv/bin/python -m xewe serial
```

Needs Python >= 3.11 and git, no sudo.

## First boot

The serial console asks for the device name, Wi-Fi, each module's questions and the LED strip
questions (chipset, voltage, length, colour order). HomeKit prints a link to its setup QR image,
`doc/media/HomeKit_Connect_QR.png`. More: [doc/README.md](doc/README.md).

## Documentation

| What | Where |
|---|---|
| Setup and troubleshooting, per feature | [doc/setup_and_troubleshooting](doc/setup_and_troubleshooting/) (led_strip, wifi, buttons, time_and_scheduler, serial_port, smart_home) |
| Upgrading from 2.3 | [doc/upgrading-from-2.3.md](doc/upgrading-from-2.3.md) |
| Commands of the product modules | [HomeAssistant](src/HomeAssistant/README.md), [HomeKit](src/HomeKit/README.md), [Alexa](src/Alexa/README.md), [SchedulePage](src/SchedulePage/README.md); the other modules: [xewe-os-modules](https://github.com/xewe-labs/xewe-os-modules) |
| Configuration | `Config.h`, `release_matrix.csv` |
| Testing | [doc/testing.md](doc/testing.md), [tests/MANUAL.md](tests/MANUAL.md) |
| The template: ecosystem, writing a module | [doc/xewe_os_ecosystem.md](doc/xewe_os_ecosystem.md), [doc/writing_a_module.md](doc/writing_a_module.md) |
| Releasing | the header of [scripts/release.sh](scripts/release.sh) |
| Working here as a coding agent | [.agents/AGENTS.md](.agents/AGENTS.md) |
| Every `xewe` command | [tools commands](https://github.com/xewe-labs/xewe-os-tools/blob/main/src/xewe/README.md) |
| XeWeCore | [xewe-os-core](https://github.com/xewe-labs/xewe-os-core) |
| The modules | [xewe-os-modules](https://github.com/xewe-labs/xewe-os-modules) |

## License

GPL-3.0-only (see `LICENSE.txt`).

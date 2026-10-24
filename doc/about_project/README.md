# About the project

## Features and hardware

- Serial CLI for LED and system control: the 30 `$led` commands, 46 LED chipsets, 6 colour orders,
  7 animated modes drawn by a dedicated render task.
- Local web interface with the LED page and a schedule editor; Apple HomeKit and Siri; Alexa;
  Home Assistant over MQTT auto-discovery; GPIO buttons bound to CLI commands; weekly schedules
  that run CLI commands; settings in NVS; every module can be enabled or disabled at run time.
- Boards: ESP32-C3, ESP32-C6, ESP32-S3 with 4 MB flash, partition scheme `huge_app`
  (`[build] partition_scheme` in `xewe.toml`).
- LED pins are fixed at build time: `XEWE_LED_PIN_DATA` (default 4) and `XEWE_LED_PIN_CLOCK`
  (default 3, used by the 14 clocked chipsets only). The Led Dock V2 pair is c3 4/3, c6 20/19,
  s3 13/12. Up to 600 LEDs (`XEWE_LED_NUM_LEDS_MAX`).
- Home Assistant, Alexa and the schedule page need the web interface on port 80, the default.
- Arduino IDE builds are not supported: `./setup.sh` generates the module list.

## Configuration

`Config.h` lists every module value as a commented `#define`; uncommenting a line overrides the
default. The product sets `XEWE_LED_LISTENERS 8`. The override order is `xewe build --define`,
then the dotenv file (`INJECTED_FROM_DOTENV` lines), then the project's `Config.h`, then the
module default. HomeKit keeps its values in `src/HomeKit/Config.h`.

## Releases

A release has 43 images, one per row of `release_matrix.csv`
(`CHIP,XEWE_LED_PIN_DATA,XEWE_LED_PIN_CLOCK,_BUILD_NOTES`): c3 7, c6 14, s3 22. The rows are the
Led Dock V1 and V2 pinouts, SuperMini boards and plain pin combinations. CI builds every row and
publishes a GitHub Release; no image is committed. Its assets are flat, under
`https://github.com/xewe-labs/xewe-led-os/releases/download/<version>/`:

- per image `<chip>_<data>_<clock>_<version>-<chip>-xewe-led-os.bin` (a 4 MB merged image written
  at 0x0), `<chip>_<data>_<clock>_meta.json` and `<chip>_<data>_<clock>_manifest.json` (an ESP Web
  Tools manifest);
- `firmware_map.csv`: one row per image with the pin columns, then `_PATH`, `_ASSET`, `_SIZE`,
  `_SHA256`, `_META`, `_MANIFEST`, `_BUILD_NOTES`;
- `firmware-<version>.tar.gz` (the folders `<chip>/<data>/<clock>/`) and `release_card.md`, the
  test record.

`releases/latest/download/firmware_map.csv` names the newest release. The full layout is in the
release section of the tools'
[spec](https://github.com/xewe-labs/xewe-os-tools/blob/main/doc/spec.md), and the release steps
are in [scripts/README.md](../../scripts/README.md).

## Files

- [FILE_STRUCTURE.md](FILE_STRUCTURE.md): the repository tree.
- [CODE_QUALITY_MATRIX.md](CODE_QUALITY_MATRIX.md): review status of the source.
- [supported_led_chips.txt](supported_led_chips.txt): the LED chipsets by family.

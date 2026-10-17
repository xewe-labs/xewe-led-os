<!-- SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com) -->
<!-- SPDX-License-Identifier: GPL-3.0-only -->
# HomeKit

Project module of xewe-led-os: controls the LED strip from the Apple Home app through
[HomeSpan](https://github.com/xewe-labs/xewe-led-library-homespan) (the xewe-labs fork pinned in
`xewe.toml`). Requires an Apple Home hub (HomePod or Apple TV) for use away from the device.

| | |
|---|---|
| id / CLI group | `homekit` |
| name | `HomeKit` |
| requires | `wifi` |
| first boot | asks to enable, then waits for the pairing (blocking) |
| can be disabled | yes |
| commands | `$homekit status`, `reset`, `enable`, `disable` (the core's generic ones) |
| stored settings | none of its own; HomeSpan keeps pairings and characteristic values in its own NVS |

Declared in `xewe-led-os.ino` after the generated modules and after Home Assistant:

```cpp
#include "src/HomeKit/HomeKit.h"
HomeKit homekit(os, wifi, led);
```

## Accessory

- HomeSpan on HAP port 1201, its serial input disabled and its log off (level -1).
- Category Lighting, display name = the device name (`$system` device name).
- A bridge accessory, then one accessory named after the device with a LightBulb service:
  `On` (default off), `Hue` (0..360), `Saturation` (0..100), `Brightness` (default 100, range
  1..100 step 1). Each characteristic is stored by HomeSpan and restored at boot.
- Setup code and setup ID are HomeSpan's defaults: `466-37-726` and `HSPN`, the values the setup
  QR image encodes.

## First boot

After "yes" to the enable question the module prints

```

Open the link with the Setup QR below and scan it
with your iPhone/iPad
https://github.com/xewe-labs/xewe-led-os/raw/main/static/media/HomeKit_Connect_QR.png
If using Mac, go to the Home App and add device
using code 4663-7726

The setup process will continue automatically
after device is pared with HomeKit
TO ABORT PRESS (x): 
```

and polls HomeSpan until the device is paired. There is no timeout. A line starting with `x`
disables the module (which erases the pairings) and restarts; any other line prints `(x)?: `.
After the pairing it prints `Setting up HomeKit`, polls for 3 s with a dot every 200 ms, then
`Device successfully paired with HomeKit.` and the hub note. The QR image is
`static/media/HomeKit_Connect_QR.png` in this repository (Git LFS; the link serves it from main).
The link can be changed with `HOMEKIT_QR_URL` (Config.h or `--define`).

`xewe provision` cannot answer this wait; pair by hand on the first boot.

## Home app -> LED

On every write from the Home app (HomeSpan calls it from `homeSpan.poll()` in the main loop) the
module applies colour, brightness and state, in that order, whatever changed:

| HomeKit | LED byte |
|---|---|
| Hue `h` (0..360) | `(uint8_t) round((h / 360.0f) * 255.0f)` |
| Saturation `s` (0..100) | `(uint8_t) round((s / 100.0f) * 255.0f)` |
| Brightness `v` (1..100) | `(uint8_t) round((v / 100.0f) * 255.0f)` (3..255 for the Home app's 1..100) |

The colour is `hsv_to_rgb(hue, sat, 255)`; brightness is set separately. The arithmetic is float
on purpose: double rounds some half-way inputs one lower (e.g. hue 195.5294 gives 139 in float,
138 in double).

## LED -> Home app

The module is a `LedListener` (registered when it begins, so a disabled module receives nothing).
Changes it made itself are not sent back.

| LED event | HomeKit |
|---|---|
| colour (rgb as set) | Hue `round((h8 / 255.0f) * 360.0f)`, Saturation `round((s8 / 255.0f) * 100.0f)`; V ignored |
| brightness | Brightness `round((b / 255.0f) * 100.0f)`; 0 is passed through below the range minimum 1 |
| state | On |
| mode, length | nothing |
| `$led reset` | state, brightness, colour |

Nothing is pushed at boot: the characteristics start from the values HomeSpan stored.

## Commands

- `$homekit status`: when enabled, HomeSpan's status dump (`*** HomeSpan Status ***`) and info
  dump (`*** HomeSpan Info ***`, with every characteristic's value), then `HomeKit module enabled`.
  When disabled it prints nothing. `$system status` prints the dumps too.
- `$homekit reset`: prints `You also need to remove the device from the Home App manually`, erases
  the HomeKit pairings (HomeSpan `U`), resets the module and restarts; the next boot runs the
  pairing wait again. Wi-Fi credentials and the stored characteristic values are kept. The same
  erase runs on `$homekit disable` and `$system reset`. `$system reset` then erases the whole NVS,
  HomeSpan's data included.

## Known behaviour

- The pairing wait blocks the boot until the device is paired; the LED keeps rendering (it has
  its own task) but the console only accepts `x`.
- `$homekit reset` or `$system reset` while HomeKit is disabled (HomeSpan never started) erases only
  the stored pairing list.
- Out-of-range values written by a HAP controller (HomeSpan does not range-check writes; the Home
  app stays in range) are clamped before conversion.

## Tests

- `tests/unit/host_homekit_map.cpp` + `tests/unit/test_homekit.py`: the mapping for every byte and
  whole input, the float half-way inputs, and source checks of the HomeSpan setup, texts, wait,
  reset and status.
- `tests/unit/test_homekit.py` also compiles `HomeKit.cpp` on the host against recording stubs of
  HomeSpan, XeWeCore, Led and Wifi and runs every function: setup order, pairing wait (abort and
  paired), Home app writes, led events, reset before and after HomeSpan started, status.
- `tests/board/test_homekit.py`: build, status order, `$system status` dump, LED brightness, colour
  and state mirrored to the characteristics; the disable/pairing test (also `$homekit reset` while
  disabled) is gated by `XEWE_TEST_HOMEKIT_DISABLE=1`, and with `XEWE_TEST_HOMEKIT_PAIR=1` it
  re-pairs and checks that the stored characteristic values survived. Without a pairing the tests
  skip with `needs a paired Apple Home hub (…)` (HomeKit disabled on the board, or the gate unset).

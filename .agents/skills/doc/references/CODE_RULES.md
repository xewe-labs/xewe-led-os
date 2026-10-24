# Code rules

## Sketch and Config.h

`xewe-led-os.ino` includes `"Config.h"` first and `<XeWeCore.h>` second, which is the umbrella
and the only library include. It declares `XeWeOs os({...})` before any module. `src/Modules.h`
comes after that, then the four product modules in this order: `SchedulePage`,
`HomeAssistant`, `HomeKit`, `Alexa`. The comment above them in the sketch gives the reason for
each position, and `tests/unit/test_project.py` checks the order.

The project URL is the `.url` field on the `XeWeOs os({...})` line.

`Config.h` includes nothing and sets no build macros. `xewe build` puts the generated
`XeWeBuildInfo.h`, then `Config.h`, then the generated `XeWeOverrides.h` in front of every
source file with `-include`. `Config.h` holds the core block and one block per selected
module, each framed by start and end lines. Every generated line is a comment, so uncomment a
`#define` to override a value. The one active override is `XEWE_LED_LISTENERS 8`. First-boot
lines stay commented out, and no `INJECTED_FROM_DOTENV` line is active, because the release
script refuses one. The format is specified in `doc/CONFIG.md` of xewe-os-core.

The version is `[project] version` in `xewe.toml`. Never write it as a literal in code.

`release_matrix.csv` is the source of the LED pins per release image, one row per image, with
the header `CHIP,XEWE_LED_PIN_DATA,XEWE_LED_PIN_CLOCK,_BUILD_NOTES`. `tests/unit/test_project.py`
pins the row count per chip.

`doc/media/HomeKit_Connect_QR.png` stays at that path. `XEWE_HOMEKIT_QR_URL` in
`src/HomeKit/Config.h` links to it on `main`, and two tests read it.

## Product modules

The four modules in `src/` must compile against the current core. When the core's module API
changes, update them and their READMEs in the same change.

A project-local module keeps its values in `src/<Folder>/Config.h`, in the module form of the
core's `doc/CONFIG.md`: two sections, `#ifndef` groups, `/* */` comments, and `_PROMPT` only on
a value asked at first boot. Names are `XEWE_<ID>_*` with the module id in upper case, such as
`XEWE_HOMEKIT_*` in `src/HomeKit/Config.h`. The user edits the file in place, and
`xewe modules generate` never touches it.

The module contract applies to these modules too: the constructor parameter is named `host`,
lambdas capture `[this]`, nothing calls `cli(`, and ids are at most 15 characters. It lives in
`doc/CONTRACT.md` of xewe-os-modules.

## Comments and docs

A comment says what the code does or why. It never names an agent, a session, a date, a
decision or finding id, a run, or the code's past. Source files keep their SPDX and path
header lines, and the unit tests of each module check them.

A change to the first-boot flow, the commands, the layout or a product module updates
`README.md` and the module's README in the same change.

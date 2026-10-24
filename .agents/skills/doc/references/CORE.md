# XeWeCore

The core lives in `xewe-os-core`, not in this repo. These facts matter when a sketch or module here touches it.

## The sketch includes only the umbrella

A sketch must include `<XeWeCore.h>`. arduino-cli finds the library to compile by matching the sketch's includes against each library's top-level `src/` headers. A sketch whose only include is `<XeWeCore/Os/XeWeOs.h>` fails with "No such file", because that sub-header is not top-level and the library is never added (seen on arduino-cli 1.5.1). Sub-headers may be included after the umbrella.

The order in `xewe-led-os.ino` is `Config.h` first, so a plain Arduino IDE build reads the project's values (`xewe build` puts it in front of every source file anyway). Then `<XeWeCore.h>`, then the `XeWeOs os(...)` line, then `src/Modules.h` and the four product modules.

## Names shaped by Arduino core macros

- The serial type is `xewe::SerialPort`, because the Arduino core defines `Serial` as a macro.
- The member `os.cli` is brace-initialised and never followed by `(`, because the core defines `cli()`.
- A standalone `xewe::Cli` object is called `xewe_cli` by convention.

## Shape

- The facade owns `os.serial`, `os.cli`, `os.nvs` and `os.system`. Those components also work alone, without `XeWeOs`.
- Everything is in `namespace xewe`, and `XeWeOs` is a global alias.
- Include direction is Config.h, Utils, Serial, then Cli and Settings. FlexData comes before Nvs. Module depends on all of them and XeWeOs on Module. A new header must not point backwards.
- The `$test` group of the board tests exists only with `XEWE_TESTING` defined.
- Each component folder except `Testing/` has a README.
- The core's configuration values are specified in the core's `doc/CONFIG.md`.

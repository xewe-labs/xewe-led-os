# The XeWe OS ecosystem

How the repositories fit together, what `./setup.sh` does, what is committed and what is
generated, how `Config.h` works, and the everyday commands. For why it is built this way see
[.agents/doc/architecture.md](../.agents/doc/architecture.md).

## The five repositories

| Repository | What it is |
|---|---|
| [xewe-os-tools][tools] | The `xewe` command and the reusable CI workflows |
| [xewe-os-core][core] | XeWeCore, the one Arduino library: Os, console, CLI, NVS, `xewe::Module` |
| [xewe-os-modules][modules] | Ready-made modules (Wi-Fi, web interface, time, ...) |
| [xewe-os][template] | This template: sketch, `Config.h`, `xewe.toml`, `setup.sh`, two samples |
| Products | Your firmware: a copy of the template ("Use this template" or a clone) |

[tools]: https://github.com/xewe-labs/xewe-os-tools
[core]: https://github.com/xewe-labs/xewe-os-core
[modules]: https://github.com/xewe-labs/xewe-os-modules
[template]: https://github.com/xewe-labs/xewe-os

A product holds only its own code and `xewe.toml`. Everything else is fetched into `build/` at the
refs `xewe.toml` pins:

| Section | Pins | Ref forms |
|---|---|---|
| `[core]` | xewe-os-core | a tag `X.Y.Z` |
| `[modules]` | xewe-os-modules | module tag `<module>/X.Y.Z`, or set tag `X.Y.Z` |
| `[tools]` | xewe-os-tools | a tag `X.Y.Z` |

A tag freezes a ref; `latest` follows the default branch; `xewe manifest update` moves the refs to
the newest tags. The `@<tag>` of the tools workflows in `.github/workflows/ci.yml` names the same
tag as `[tools] ref`. Tags are `X.Y.Z` with no `v`. The firmware version is `[project] version`
in `xewe.toml`; builds never change it.

XeWeCore has three onboarding levels. Levels 1 and 2 need only the Arduino IDE (XeWeCore from the
Library Manager, then one module of your own in the sketch folder). Level 3 is this template and
the `xewe` tools: ready-made modules, multi-chip builds, board tests and releases. The modules
repository is not an Arduino library.

## What ./setup.sh does

- Installs the xewe tools into a venv in `build/tools/`.
- Installs XeWeCore and the Arduino libraries into `build/libraries/`.
- Generates the selected modules as one Arduino library (with their tests) in `build/modules/`,
  plus `src/Modules.h` and `run.sh`.
- Puts arduino-cli, the esp32 core and the modules repository into `~/.xewe-os/build-tools/`,
  once per machine (`XEWE_HOME` overrides `~/.xewe-os`). The first run on a machine downloads
  them (about 1.7 GB, about 8 GB on disk); later projects reuse them.
- Reads and writes nothing in `~/.arduino15` or `~/Arduino`. With an existing esp32 core use
  `./setup.sh --arduino-data DIR` (or `XEWE_ARDUINO_DATA=DIR`; `DIR` holds `packages/esp32/`).

The template ships with no modules selected (`[modules] selected = []`). On a terminal the first
`./setup.sh` shows a numbered menu (an empty answer means none); without a terminal it selects
nothing. Choose up front with `./setup.sh --modules wifi,web-interface` (or `all`, `none`;
dependencies are added). The choice is written to `xewe.toml`. Change it later with
`./setup.sh --modules LIST` or `xewe modules select LIST|all|none`; `xewe modules list` shows
what exists (`*` = selected). Zero modules is a valid firmware.

## Committed vs generated

- **Committed:** `xewe-os.ino`, `Config.h`, `xewe.toml`, `setup.sh`, `src/SimpleModule/`,
  `src/FullModule/`, `tests/`, `releases/<version>/` (`notes.txt` and the generated
  `release_card.md`), `scripts/release.sh`, `README.md`, `doc/`, `LICENSE.txt`,
  `.github/workflows/ci.yml`, `.agents/`.
- **Generated, ignored, safe to delete:** `build/`, `src/Modules.h`, `run.sh` (`./setup.sh`
  writes all three again); `releases/<version>/logs/` are local run logs.
- **Ignored, yours:** `.env`, the credentials `xewe provision` reads. It is optional; you create
  it, nothing writes it.
- **Shared, outside the project:** `~/.xewe-os/build-tools/`. Deleting it only means the next
  setup downloads it again.

No submodules. Never edit `src/Modules.h` or anything in `build/` by hand; setup regenerates
them. The full `build/` tree is in
[.agents/doc/architecture.md](../.agents/doc/architecture.md#build-layout).

## Config.h

`Config.h` is a catalogue of the core and of every selected module. It holds the core block and
one block per selected module, each framed by bars with a `start` and an `end` line, and each
block has two sections: default runtime values, and compile time defined constants. Every
generated line is a comment: uncomment a `#define` line and change the value to override the
default. The specification is the core's
[doc/config.md](https://github.com/xewe-labs/xewe-os-core/blob/main/doc/config.md).

`./setup.sh` and `xewe modules select` append the block of a module the first time it is selected
and never touch it again. `Config.h` includes nothing. `xewe build` puts three headers in front of
every source file, in this order: the generated `XeWeBuildInfo.h` (version, timestamp, chip), then
`Config.h`, then the generated `XeWeOverrides.h` (dotenv values and `--define`s).

For one name, the first of these that defines it wins:

1. `xewe build --define NAME=VALUE`; string values carry their own quotes, for example
   `--define 'XEWE_CORE_DEVICE_NAME="Bench"'`.
2. The dotenv value, where `Config.h` says `INJECTED_FROM_DOTENV`.
3. The project's `Config.h`.
4. The module's own `Config.h` default.

A plain Arduino IDE build uses the defaults. The project URL is the `.url` on the
`XeWeOs os({...})` line of `xewe-os.ino`.

**First boot.** `./run.sh` erases the whole flash, NVS included, so every run is a true first
boot (`--keep-nvs` keeps the stored values). The board then asks
`Name your device (Enter for "xewe-device"):`, and for each module in declaration order
`Would you like to enable <Name> module?` (for a module that can be disabled) followed by that
module's own setup. Uncommenting the line of a value the first boot asks for pre-answers its
question; setting it to `INJECTED_FROM_DOTENV` takes the answer from the dotenv file. Module
prompts are bounded, so they time out with their default when nobody answers. `xewe provision`
answers the prompts from flags, the environment or a dotenv file.

## Everyday commands

Run the tools as `build/tools/.venv/bin/python -m xewe <command>` (below: `xewe`), or `./run.sh`.
Avoid the bare `build/tools/.venv/bin/xewe` script: if the project folder changes location its
shebang is stale; re-run `./setup.sh` to fix it. Flags are in the
[tools command README](https://github.com/xewe-labs/xewe-os-tools/blob/main/src/xewe/README.md).

| Task | Command |
|---|---|
| Build | `xewe build --chip c3`, or `--all-chips` (c3, c6, s3) |
| Inspect a build | `xewe build --chip c3 --dry-run` prints the arduino-cli command |
| Flash | `xewe flash --chip c3` |
| Console | `xewe serial` (timestamped) |
| Build, erase, flash, console | `./run.sh --chip c3` (= `xewe run`) |
| Test | `xewe test --unit-only`, `xewe test [--module SLUG]` |
| List, check modules | `xewe modules list`, `xewe modules validate [PATH]` |
| Select modules | `xewe modules select LIST\|all\|none` (edits `xewe.toml`) |
| Look around | `xewe doctor`, `xewe manifest show`, `xewe boards --no-probe` |
| Format, brand check | `xewe format`, `xewe brand-lint .` |
| Everything | `xewe --help` |

`-v` works after the subcommand too (`xewe test -v`). Firmware lands in
`build/builds/<chip>/out/`: `<version>-<chip>-xewe-os.bin` (flash at 0x0), `manifest.json`,
`meta.json` (size, flash %) and `compile.log`.

## Without a board

`./run.sh`, `xewe flash` and `xewe test` compile, then print
`compiled, not run: no board attached (...)` and exit 0. Set `XEWE_NO_BOARD=1` for compile-only
sessions (CI, agents): no port is ever opened, even with a board plugged in. See
[testing.md](testing.md).

## Releasing

A release is two runs with one commit in between. The step-by-step reference is the header of
[scripts/release.sh](../scripts/release.sh).

1. `scripts/release.sh --prepare X.Y.Z --hardware c3,c6,s3` sets the version in `xewe.toml` and
   creates `releases/X.Y.Z/notes.txt`.
2. It stops with exit 2 until the notes are filled in; fill them and run it again.
3. It then runs setup, module validation, the unit tests, the board tests of every chip, the
   builds, format and brand lint once, and renders `releases/X.Y.Z/release_card.md`.
4. The card lists every test per chip and what was not covered; chips without a hardware run are
   marked compile only.
5. Commit and push what the script lists.
6. `scripts/release.sh X.Y.Z` (use `--dry-run` first) verifies the card against the tree and
   checks CI.
7. It pushes an annotated tag `X.Y.Z` whose message is `notes.txt`; the tag equals
   `[project] version`.
8. CI (`ci.yml`, release job) builds c3, c6 and s3 and publishes a GitHub Release.
9. The images are release assets only and are never committed; the run logs stay in
   `releases/X.Y.Z/logs/` on your machine.
10. Each asset has a direct URL:
    `https://github.com/xewe-labs/xewe-os/releases/download/<tag>/<asset>`.

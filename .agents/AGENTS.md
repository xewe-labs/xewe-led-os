# AGENTS.md — xewe-led-os

This is **WAX 1.3**, the WAX Agentic Workspace. The rules, preferences, and skills below belong
to this version. Human documentation lives at https://github.com/maxdokukin/wax_agents.

This folder is the agentic part of the project. It gives you boundaries, context, and tools,
in that order. Read it as described below before doing anything else in the repository.

## Read in this order

1. **`RULES.md` — root boundaries.** Absolute rules, the same in every project. Cite them by
   ID when you explain a decision or a refusal.
2. **`PREFERENCES.md` — user boundaries.** This project's chosen defaults (R-15). They bind
   like rules until a human changes them; cite them by ID too.
3. **`handoffs/HANDOFF.md` — context.** The head of the work record: where the last session
   stopped and what is open. Reach it only through the `pickup` procedure of the
   `wax_handoff` skill (R-04), because the directory has invariants that the skill checks
   before you rely on anything in it.
4. **`skills/` — tools.** One folder per skill, each with a `SKILL.md`. Discover them by
   reading the frontmatter of every `skills/*/SKILL.md` (R-10); a folder without a valid one
   is not a skill (R-11).

## Session shape

- **Pickup → work → handoff.** Start with `wax_handoff` pickup, do the work, end with
  `wax_handoff` handoff. A session that changed anything and did not end with a handoff is
  incomplete (R-06); say so rather than letting it pass.

## Never do these without being asked

- **Edit `RULES.md`, `PREFERENCES.md`, or this file** (R-15, P-05). Propose changes in the
  handoff instead (P-10).
- **Touch anything under `handoffs/` by hand** (R-04). The skill is the only door.
- **Add, rename, or remove a top-level item in `.agents/`** (R-13).
- **Commit, push, tag, release, publish, or delete what you did not create** (P-09).

## Precedence

- **Human instruction in this session > `RULES.md` > `PREFERENCES.md` > this file > a
  skill** (R-03). A project's own `.agents/` wins over any organization-level agent file. Record every
  human-instructed deviation in the handoff, quoting the instruction.

## Reporting back

- **Say what you actually ran** and label anything unverified as unverified (P-07).
- **Never report a skipped or failed step as done** (P-08). Failures come with their output.

## Project: xewe-led-os

XeWe LED OS: firmware for addressable LED strips on ESP32-C3, -C6 and -S3, built on XeWe OS 3
(the `xewe-os` template). The sketch `xewe-led-os.ino`, `Config.h`, `xewe.toml`,
`release_matrix.csv` (the pin images of a release) and the product's own modules in `src/<Name>/`
(Home Assistant, HomeKit, Alexa, the schedule page, the web fallback handler). The LED engine and its
web page are the `led` and `led-web` modules of xewe-os-modules; everything else (tools, XeWeCore,
modules, libraries) is fetched into `build/` by `./setup.sh`. Human documentation: `README.md` (use),
`doc/` (setup and troubleshooting per feature), `doc/architecture.md` (XeWe OS architecture and its
decisions log), `doc/naming.md`. Project rules are X-01 … X-10 at the end of `RULES.md` (written for
the template; they apply to this product where they fit). Organization rules:
`https://github.com/xewe-labs/.github` (`AGENTS.md` and `guidelines/`); they apply where this folder
is silent.

### Commands

Run everything from the project root, after `./setup.sh`, as
`build/tools/.venv/bin/python -m xewe <command>` (below: `xewe`). The bare `build/tools/.venv/bin/xewe`
script breaks when the project folder is moved; re-run `./setup.sh` then.

| Task | Command |
|---|---|
| Set up (fresh project, or after `xewe.toml` changed) | `./setup.sh` (`</dev/null` in a script: no module menu) |
| Inspect a build first | `xewe build --chip c3 --dry-run` prints the arduino-cli command |
| Build | `xewe build --chip c3`, or `--all-chips` (c3, c6, s3) before calling a change done |
| Test | `xewe test --unit-only`, `xewe test [--module SLUG]` |
| Modules | `xewe modules list`, `xewe modules validate [PATH]`; `xewe modules select LIST\|all\|none` only when asked (it edits `xewe.toml`) |
| Look around | `xewe doctor`, `xewe manifest show`, `xewe boards --no-probe` |
| Build, erase, flash, listen | `./run.sh` (= `xewe run`; `--keep-nvs` keeps the stored settings) — only when asked |
| Release (only when asked; the maintainer runs it) | `scripts/release.sh X.Y.Z --dry-run`; `--prepare X.Y.Z` sets the version and creates `releases/X.Y.Z/` |

Refs: core `3.0.0` is a tag; modules is the branch `led-module` (followed on every `./setup.sh`)
until the led modules are tagged. Tools: `xewe.toml` has `[build] partition_scheme = "huge_app"`,
which tools 3.0.1 rejects (`unknown key(s): build`). It needs the xewe-os-tools branch
`partition-scheme` (per-project partition scheme) until a tools release (3.1.0) contains it:
`[tools] ref` and the two `@` refs in `.github/workflows/ci.yml` name that branch, then the release
tag. While the branch is not pushed, run setup against a local clone:
`XEWE_TOOLS_SOURCE=<xewe-os-tools clone> ./setup.sh`. `ci.yml` calls the tools workflows at the
`[tools] ref` tag. Tags are `X.Y.Z` with no `v`. A tag makes CI build every row of
`release_matrix.csv` (43 images: chip and LED data/clock pin), publish a GitHub Release and commit `static/firmware/releases/X.Y.Z/` to `main` (`.bin` in Git
LFS); never write that folder by hand.

`-v` may go after the subcommand (`xewe test -v`). `--define` string values carry their own quotes:
`--define 'PROJECT_URL="https://..."'`.

### No board

No board is normal. `flash`, `run` and `test` compile, then print
`compiled, not run: no board attached (...)` and exit 0; grep for that line, it is not an error.
For a compile-only session `export XEWE_NO_BOARD=1` first: no port is listed, probed or opened even
if a board is plugged in; `test` still compiles, `flash`/`run`/`serial` exit 4. Never pass
`--require-board` or `--erase`, or flash a board, unless the user asks.

### Local sources and the harness pattern

`./setup.sh` normally fetches the `xewe.toml` refs. To build against working trees instead:

```sh
XEWE_TOOLS_SOURCE=../xewe-os-tools XEWE_CORE_SOURCE=../xewe-os-core \
XEWE_MODULES_SOURCE=../xewe-os-modules ./setup.sh </dev/null
```

(`--core-source DIR` and `--modules-source DIR` are the flag forms.) The core is copied into
`build/libraries/XeWeCore/`; the modules source is used where it is. `XEWE_HOME` moves the shared
toolchain (`~/.xewe-os/build-tools/`).

Core and module changes are compiled and tested in a **copy** of this template (a "harness"), set
up with the local sources above and the modules under test selected: never in the template itself,
and never by editing the tools' or a reference clone. The template is checked by itself: setup with
no modules, then `xewe build --chip c3`.

### Generated files: never edit

| File | Written by |
|---|---|
| `build/` (everything, including `build/modules/` and `build/libraries/`) | `./setup.sh`, `xewe build` |
| `src/Modules.h` | `xewe modules generate` (run by setup) |
| `<XeWeBuildInfo.h>` (`build/builds/<chip>/gen/`) | every `xewe build` |
| `build/config/build_config.toml` | `xewe setup` |

Fix the source (`xewe.toml`, the modules repository, the core) and regenerate. The shared toolchain
in `~/.xewe-os/build-tools/` is never deleted as a fix; `xewe clean` never touches it. Never run
arduino-cli directly and never touch `~/.arduino15` or `~/Arduino`.

### Code notes

- The sketch includes `<XeWeCore.h>` (the umbrella); `XeWeOs os({...})` is declared before any
  module; `src/Modules.h` is included after it, then the project-local modules.
- `Config.h` includes `<XeWeBuildInfo.h>` unconditionally (arduino-cli adds a library only when an
  include of it fails, so a `__has_include` guard silently drops it).
- The version is `[project] version` in `xewe.toml`, never a literal in code.
- The order of declarations in the sketch is the order in which the Os begins and loops the
  modules: the generated ones (`src/Modules.h`, order of `[modules] selected`) first, then the
  product's modules in this fixed order: `schedule_page`, `web_fallback`, `homeassistant`,
  `homekit`, `alexa`. WebFallback must come before Alexa (the server keeps the last not-found
  handler, so an enabled Alexa overrides the fallback); HomeAssistant must come before HomeKit (HA
  ends its first-boot mDNS before HomeSpan starts its own). The unit tests pin this order.
- Led listeners are registered by each module in its own begin routine (led_web in
  `begin_routines_common`, the three integrations in `begin_routines_required`), so the led notifies
  led_web, Home Assistant, HomeKit, Alexa in that order. Never register a listener in the sketch.
- Flash budget: the product builds with the esp32 `huge_app` partition scheme (3 MB app, no OTA,
  896 KB SPIFFS, NVS at the same offset as `no_ota`). With every module the images are c3
  2,089,527 B (66 %), c6 2,109,766 B (67 %), s3 1,892,189 B (60 %) of 3,145,728 B; under the old 2 MB
  `no_ota` partition c6 did not fit. `meta.json` records `partition_scheme` and
  `app_partition_size`. Measure all three chips after any change.
- The led pins are compile-time values: `XEWE_MODULE_LED_PIN_DATA` / `_PIN_CLOCK` (per-chip
  defaults in the led block of `Config.h`; `xewe build --define` or a `release_matrix.csv` row
  overrides them).
- Module code rules (constructor parameter `host`, `[this]` captures, never `cli(`, ids ≤ 15
  characters) are in the modules repository's `doc/contract.md` and `.agents/RULES.md`; they apply to
  project-local modules too.

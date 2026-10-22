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
   is not a skill (R-11). This project's skills are `wax_handoff`, `wax_init`,
   `xewe_os_ecosystem`, `writing_a_module` and `testing`.
5. **Project docs.** Read the one your task needs, with its skill. `README.md` and
   `doc/README.md` index them. The template's three docs in `doc/`: `xewe_os_ecosystem.md`
   (repositories, pins, `Config.h`, commands), `writing_a_module.md` (modules and the naming
   rules) and `testing.md` (unit tests, board tests, CI). Why the ecosystem is shaped this way:
   `.agents/doc/architecture.md` and `.agents/doc/naming.md`.

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

The firmware of XeWe LED OS for ESP32-C3, -C6 and -S3 (default chip `s3`): a project of the XeWe OS
template. It selects seven modules from xewe-os-modules at `led/3.1.2` (`led`, `wifi`,
`web-interface`, `led-web`, `time`, `scheduler`, `buttons`) on XeWeCore 3.1.2, and adds four
product modules in `src/`, each with its own `README.md`: `SchedulePage` (schedule page and JSON
routes), `HomeAssistant` (MQTT discovery), `HomeKit` (HomeSpan) and `Alexa` (Espalexa). The sketch
`xewe-led-os.ino` declares them after the generated `src/Modules.h`, in this order: SchedulePage,
HomeAssistant, HomeKit, Alexa; the reasons are in the sketch comment. Everything else (tools,
XeWeCore, modules, libraries) is fetched into `build/` by `./setup.sh`.

- `release_matrix.csv` is the per-board source of the LED pins: one row per release image
  (`CHIP,XEWE_LED_PIN_DATA,XEWE_LED_PIN_CLOCK,_BUILD_NOTES`).
- `Config.h` is the generated catalogue; its one active override is `XEWE_LED_LISTENERS 8`.
- Docs: `README.md` (index), `doc/README.md`, the feature guides in
  `doc/setup_and_troubleshooting/`, `doc/upgrading-from-2.3.md`, the template's `doc/testing.md`,
  `doc/writing_a_module.md` and `doc/xewe_os_ecosystem.md`, and `.agents/doc/` (the template's
  `architecture.md` and `naming.md`).

Project rules are X-01 … X-10 at the end of `RULES.md`. Organization rules:
`https://github.com/xewe-labs/.github` (`AGENTS.md` and `guidelines/`); they apply where this
folder is silent.

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
| Release (only when asked; the maintainer runs it) | `scripts/release.sh --prepare X.Y.Z --dry-run`, `scripts/release.sh X.Y.Z --dry-run`; a real `--prepare` only on a scratch copy |

Refs are pinned to tags (`xewe.toml`); `ci.yml` calls the tools workflows at the `[tools] ref` tag.
Tags are `X.Y.Z` with no `v`. `scripts/release.sh --prepare X.Y.Z [--hardware c3,c6,s3]` (no git
change) sets the version, stops with exit 2 until `releases/X.Y.Z/notes.txt` is filled, then records
one run through `xewe release-card` (setup, `xewe modules validate`, `xewe test --unit-only`, per
chip the build and the board tests: flashed and run on a `--hardware` chip, compiled, not run on the
others; format, brand-lint; logs in `releases/X.Y.Z/logs/`, gitignored) and renders the generated
`releases/X.Y.Z/release_card.md` (no hand-written test card). The tag run verifies the card, waits
for CI and pushes the tag; the tag's CI builds every image, checks the card and publishes the GitHub
Release with the images and the card as assets. Nothing is committed back: images are release assets only.

`-v` may go after the subcommand (`xewe test -v`). `--define` string values carry their own quotes:
`--define 'XEWE_CORE_DEVICE_NAME="Bench"'`. The project URL is `.url` on the sketch's `XeWeOs os({...})` line.

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

Core and module changes are compiled and tested in a **copy** of this project (a "harness"), set
up with the local sources above and the modules under test selected: never in this project itself,
and never by editing the tools' or a reference clone.

### Generated files: never edit

| File | Written by |
|---|---|
| `build/` (everything, including `build/modules/` and `build/libraries/`) | `./setup.sh`, `xewe build` |
| `src/Modules.h` | `xewe modules generate` (run by setup) |
| `XeWeBuildInfo.h`, `XeWeOverrides.h` (`build/builds/<chip>/gen/`) | every `xewe build` |
| `build/config/build_config.toml` | `xewe setup` |

Fix the source (`xewe.toml`, the modules repository, the core) and regenerate. The shared toolchain
in `~/.xewe-os/build-tools/` is never deleted as a fix; `xewe clean` never touches it. Never run
arduino-cli directly and never touch `~/.arduino15` or `~/Arduino`.

### Code notes

- The sketch `xewe-led-os.ino` includes `"Config.h"` first, then `<XeWeCore.h>` (the umbrella, its
  one library include); `XeWeOs os({...})` is declared before any module; `src/Modules.h` is
  included after it, then the project-local modules in the order above (`SchedulePage`,
  `HomeAssistant`, `HomeKit`, `Alexa`).
- `Config.h` includes nothing: `xewe build` puts the generated `XeWeBuildInfo.h`, then `Config.h`,
  then the generated `XeWeOverrides.h` in front of every source file (`-include`). It holds the core
  block and one block per selected module, framed by bars with start and end lines, two sections
  each; a catalogue where every generated line is a comment (uncomment a `#define` to override);
  spec: xewe-os-core `doc/config.md`.
- The version is `[project] version` in `xewe.toml`, never a literal in code.
- The four modules in `src/` must keep compiling against the current core. When the core's module
  API changes, update them and their READMEs together.
- A project-local module keeps its values in `src/<Folder>/Config.h`, in the module form of the
  core's `doc/config.md` (two sections, `#ifndef` groups, `/* */`
  comments, `_PROMPT` only on a value asked at first boot), named `XEWE_<ID>_*` with the module id
  in upper case (`XEWE_HOMEKIT_*` in `src/HomeKit/Config.h`). The user edits it in place; `xewe
  modules generate` never touches it.
- Module code rules (constructor parameter `host`, `[this]` captures, never `cli(`, ids ≤ 15
  characters) are in the modules repository's `doc/contract.md` and `.agents/RULES.md`; they apply to
  project-local modules too.

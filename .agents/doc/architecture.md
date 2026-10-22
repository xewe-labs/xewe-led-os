# Architecture

Why XeWe OS is shaped the way it is. The [README](../../README.md) says how to use it; this page says
why it is four repositories, why the sketch reaches the library through one include, why modules
carry an explicit declare line, what is decided versus still open, and where XeWe OS stands
against the rest of the ESP32 ecosystem (section 8).

## 1. Purpose and philosophy

- **A template you clone and vibecode.** `xewe-os` is not a firmware you configure; it is a
  starting point you own. `git clone` (or GitHub "Use this template"), `./setup.sh`, `./run.sh`,
  then write your device in `xewe-os.ino` and `Config.h`. The repository holds only your code and
  `xewe.toml`; everything else is fetched at pinned refs, so a project stays small and builds the
  same way later.
- **Modules are code figured out once.** WiFi reconnects, NTP and timezones, debounced buttons, an
  HTTP command endpoint: each took effort to get right on an ESP32. A module captures that once, in
  one repo, behind one contract, so the next project (or the next agent) selects it instead of
  solving it again.
- **The contract is the product.** Flashing, serial consoles, version pinning and Wi-Fi portals
  all exist elsewhere; XeWe OS wraps them. What it owns is the module contract: one id that is the
  CLI group and the NVS namespace, the same verbs on every module, typed settings, three tests a
  host runs over serial, and a device that can describe its own commands and settings. Section 8
  shows where this stands against the rest of the ecosystem.
- **Agents are first-class users.** Every action is a non-interactive command with stable output
  and exit codes (`xewe build`, `xewe test`, `xewe modules validate`). Rules that break silently
  are written down in each repo's agent guide (`.agents/AGENTS.md` and `.agents/RULES.md`) and
  enforced by validators where possible. A human and an agent run the same commands.
- **Hardware in the loop, and still useful with no board.** Tests flash a real board and talk to
  it over serial. Without a board every command still compiles, then reports
  `compiled, not run: no board attached (...)` and exits 0. Both paths are first class (D6, D22).

Target hardware is ESP32-C3, -C6 and -S3 only (D1), flashed over serial only (no OTA, D2), built
with arduino-cli (D5).

## 2. The four repos and their boundaries

| Repo | Job | Boundary |
|---|---|---|
| [`xewe-os`](https://github.com/xewe-labs/xewe-os) | the firmware template users clone | its own code, `xewe.toml` and `setup.sh`; its firmware images are GitHub Release assets; `run.sh` is generated |
| [`xewe-os-core`](https://github.com/xewe-labs/xewe-os-core) | `XeWeCore`, the one Arduino library | reusable code every firmware needs; publishable on its own; depends only on ArduinoJson |
| [`xewe-os-modules`](https://github.com/xewe-labs/xewe-os-modules) | every module, `modules/<slug>/` | features some firmware needs; the contract in `doc/contract.md`, the index in `doc/modules.md`; no toolchain of its own; built and tested through an `xewe-os` checkout (D21) |
| [`xewe-os-tools`](https://github.com/xewe-labs/xewe-os-tools) | `xewe`, one Python package (`src/xewe/`) | everything that runs on the host: setup, build, flash, serial, test, release, format (the one C++ style file ships inside the package), brand-lint, webui (a Flask web UI prototype exported as PROGMEM headers). The spec is `doc/spec.md` |

Every repo has the same top level: `README.md` and `LICENSE.txt` are the only files written for a
reader at the root, longer pages live in `doc/`, the agent guide in `.agents/`, CI in
`.github/workflows/`. No repo commits a `.clang-format` or a `run.sh` (D25).

`XeWeCore` is checked by its own CI (host tests, examples, `arduino-lint` against the rules in
core's [`doc/library-rules.md`](https://github.com/xewe-labs/xewe-os-core/blob/main/doc/library-rules.md))
and released by its own `scripts/release.sh` (D18).

```
                        xewe-os  (template)
                        xewe.toml pins one ref each
             ┌──────────────────┼──────────────────┐
             v                  v                  v
      xewe-os-core       xewe-os-modules      xewe-os-tools
      (XeWeCore lib) <── each module has      (xewe: setup, build,
             ^           requires_core         flash, serial, test)
             │                                     │
   scripts/release.sh                    fetches core and ArduinoJson into the
  (tag + release XeWeCore)               project's build/; arduino-cli, the esp32
                                         core and modules once per machine
```

### Build layout

A project after `./setup.sh --modules wifi,time`, and the toolchain it shares with every other
project on the machine:

```
~/.xewe-os/build-tools/      shared      once per machine; XEWE_HOME overrides ~/.xewe-os
├── arduino15/                           esp32 core(s), toolchains, esptool (several core versions coexist)
├── bin/arduino-cli-<ver>                one binary per pinned arduino-cli version
├── arduino-user/                        empty sketchbook (isolates ~/Arduino/libraries)
├── downloads/                           archives (XEWE_CACHE overrides)
├── sources/xewe-os-modules/<ref>/       the modules repo, one checkout per [modules] ref
└── .lock                                held while setup installs into the shared folder

my-fw/
├── xewe-os.ino              committed   #include "Config.h", <XeWeCore.h>; XeWeOs os({...}); your setup()/loop()
├── Config.h                 committed   the core block (the core's values) plus one appended block per
│                                        selected module (setup adds it once; every line a comment)
├── xewe.toml                committed   the manifest: [project] [core] [modules] [tools] [libraries]
├── setup.sh                 committed   bootstraps build/tools/.venv, then calls `python -m xewe setup`
├── src/SimpleModule/  src/FullModule/     committed   project-local sample modules, each with its Config.h
├── tests/board/  tests/unit/  committed  the project's tests (`xewe test`)
├── static/                    ignored        build output of `xewe release`; nothing in git, binaries are release assets (D26)
├── README.md  LICENSE.txt  .github/workflows/ci.yml   committed
├── doc/                     committed   xewe_os_ecosystem.md  writing_a_module.md  testing.md
├── .agents/                 committed   AGENTS.md  RULES.md  PREFERENCES.md  skills/  handoffs/
│   └── doc/                             architecture.md  naming.md
├── run.sh                   generated   written by every ./setup.sh; calls `xewe run`
├── .env                     local       credentials for `xewe provision`; optional, yours (setup never writes it)
├── src/Modules.h            generated   #include <XeWeModules.h> + the declare lines
└── build/                   generated   build/.gitignore is `*`
    ├── builds/<chip>/
    │   ├── gen/XeWeBuildInfo/           the generated XeWeBuildInfo.h and XeWeOverrides.h, per build
    │   ├── cache/                       arduino-cli build path
    │   └── out/                         <version>-<chip>-<name>.bin, manifest.json, meta.json, compile.log
    ├── config/                          build_config.toml (what setup installed), boards.toml (board scans)
    ├── libraries/                       XeWeCore/  ArduinoJson/  (and the libraries modules need)
    ├── modules/                         the generated Arduino library XeWeModules:
    │   ├── library.properties  modules.lock
    │   ├── src/                         XeWeModules.h, Wifi/, Time/, ...
    │   └── tests/<slug>/{board,unit}/   the selected modules' tests
    ├── tools/                           the xewe-os-tools checkout; tools/.venv
    └── tmp/                             only on demand: sketch mirror, staging, pytest cache
```

Nothing generated is committed and there are no submodules (D11). `build/`, `src/Modules.h` and
`run.sh` can be deleted at any time; `./setup.sh` rebuilds them from `xewe.toml`. Section 6 of the
tools' [`doc/spec.md`](https://github.com/xewe-labs/xewe-os-tools/blob/main/doc/spec.md) is the
authoritative description.

## 3. XeWeCore shape

One library (D8), utils inside it (D16), laid out as a facade plus standalone components (D17):

```
src/
  XeWeCore.h                   umbrella: the only top-level header
  XeWeCore/
    Config.h                   the core's configuration values (xewe-os-core doc/config.md)
    Os/XeWeOs.{h,cpp}          xewe::Os (global alias XeWeOs) and xewe::System
    Module/Module.{h,cpp}      xewe::Module, the base class of every module
    Settings/Settings.{h,cpp}  xewe::Settings: the settings table of a module
    Serial/Serial.{h,cpp}      xewe::SerialPort
    Cli/Cli.{h,cpp}            xewe::Cli: "$group command args", $help
    Nvs/Nvs.{h,cpp,tpp}        xewe::Nvs: typed key-value storage
    Nvs/FlexData.h             xewe::FlexData
    Utils/Utils.h + Utils/*.h  header-only: String, AsyncTimer, Span, Color, LockGuard, Pins, ...
    Testing/Testing.{h,cpp}    the `$test` group of the board tests, only with XEWE_TESTING
```

Each component folder except `Testing/` has a `README.md` that documents it.

```cpp
#include <XeWeCore.h>
XeWeOs os({.version = "0.1.0"});
void setup() { os.begin(); os.serial.println("hi"); }
void loop()  { os.loop(); }   // os.cli, os.nvs, os.system.restart()
```

The facade owns `os.serial`, `os.cli`, `os.nvs` and `os.system`. The components also work alone
(`xewe::SerialPort`, `xewe::Cli`, `xewe::Nvs`, `xewe::FlexData`, `xewe::str`) with no `XeWeOs`.
Everything is in `namespace xewe`; the include direction is Config.h ← Utils ← Serial ← Cli,
Settings; FlexData ← Nvs; all ← Module ← XeWeOs.

**Why `#include <XeWeCore.h>` is the sketch's one library include.** arduino-cli discovers which
library to compile by trying the sketch's includes against each library's top-level `src/` headers.
A sketch whose only include is `<XeWeCore/Os/XeWeOs.h>` fails with "No such file": the sub-header
is not a top-level header, so the library is never added (arduino-cli 1.5.1). Sub-headers may be
included *after* the umbrella. The sketch's other includes are its own files: `Config.h` comes
first, before the umbrella, so a plain Arduino IDE build reads the project's values too (`xewe
build` puts it in front of every source file anyway), and `src/Modules.h` and the project-local
modules come after the `XeWeOs os(...)` line.

Three names are shaped by the Arduino core's macros: the serial type is `xewe::SerialPort` (the
core defines `Serial` as a macro), the member `os.cli` is brace-initialised and never followed by
`(` (the core defines `cli()`), and a standalone `xewe::Cli` object is conventionally `xewe_cli`.

## 4. Module contract

The full contract is
[`xewe-os-modules/doc/contract.md`](https://github.com/xewe-labs/xewe-os-modules/blob/main/doc/contract.md).
In short: a module is `modules/<slug>/` with `module.properties`, `src/<Folder>/<Folder>.{h,cpp}`
(optionally `src/<Folder>/Config.h` with its compile-time values), `tests/board/test_<slug>.py`
(optionally `tests/unit/`) and a README. `id` (at most 15 characters) is both the CLI group and the
NVS namespace and never changes. The class derives from `xewe::Module`, lives in the global
namespace, includes `<XeWeCore.h>`, names its constructor's Os parameter `host` (a parameter named
`os` would hide the member), captures `[this]` only, and never writes `cli(`. Three tests are
required: `test_compiles`, `test_status` and one behaviour test. `tools/validate.py` must pass.

**Why the explicit declare line was kept (D13).** `declare=Time time_module(os, wifi);` is copied
verbatim into the generated `Modules.h`. The alternative, generating the declaration from
constructor signatures, would need a C++ parser in the tools and would hide the one line that
says how a module is wired. Explicit lines are readable in the generated file, let a module choose
its variable name and dependency arguments, and are cheap to check: the validator enforces unique
slug, id, folder, class and variable, that the type is the folder, that the first argument is `os`,
and that every other argument is a declared dependency. The validator does the review.

## 5. Tools

`xewe` (from `xewe-os-tools`, called as `build/tools/.venv/bin/python -m xewe` or through `./run.sh`).
Every command and flag is in the tools' `doc/spec.md`; the main ones:

| Command | Does |
|---|---|
| `setup` | arduino-cli, esp32 core and the modules repo into `~/.xewe-os/build-tools/` (once per machine); XeWeCore and libraries into `build/`; generates `build/modules/` (library, module tests), `src/Modules.h` and `run.sh` |
| `build [--chip C \| --all-chips] [--define K=V]` | compile into `build/builds/<chip>/out/`, prints flash % |
| `flash`, `run`, `serial` | write the merged image at 0x0 (NVS kept unless `--erase`); build, erase, flash, listen (`--keep-nvs` skips the erase); timestamped console |
| `test [--module SLUG] [--unit-only] [--all-chips]` | pytest over project and module tests (`tests/unit/`, `tests/board/`) |
| `boards`, `doctor`, `clean` | board discovery; environment check; delete generated output |
| `modules list\|select\|validate\|generate`, `manifest show\|update` | modules and refs |
| `provision` | answers the first-boot prompts of a flashed board (name, modules, Wi-Fi, timezone) |
| `release --version X.Y.Z` | release matrix into `build/release/` and the assets staged for the GitHub Release (run by CI on a tag; see §6) |
| `format [--check]`, `brand-lint [DIR...]` | C++ in the XeWe style shipped with the tools (no column alignment); banned brand spellings ([naming.md](naming.md)) |

**`xewe.toml`** is the project's manifest, its whole dependency state: `[project]` (name, version,
chip), `[core]`, `[modules]` (with `selected`) and `[tools]` (repo + ref each) and `[libraries]`
(ArduinoJson). Only `manifest update`, `modules select`, `setup --modules`, the first-setup menu and
`release` write it. The module selection is per project: the template ships `selected = []`, each
project picks its modules at first setup (menu on a terminal, or `--modules`) and commits that
choice; `setup --latest` tries the newest tags without editing it. The firmware version is
`[project] version`; builds never change it.

**No-board semantics (D22).** `flash`, `run` and `test` compile first, then report
`compiled, not run: no board attached (...)` and exit 0. `--require-board` turns that into exit 4
(1 for `test`). Agents grep for the line instead of treating it as an error.

**Test layers (D14).** Every firmware repo (core, each module, every project) splits its tests into
`tests/unit/` (developer machine) and `tests/board/` (the board, over serial), nothing else. Unit
tests run on the developer machine with no board and no build: C++ for pure logic (core's Arduino
shim in `xewe-os-core/tests/unit/`, a module's or project's pure headers built with g++ by a
`unit`-marked pytest driver) and pure-logic Python (`@pytest.mark.unit`, selected by
`xewe test --unit-only`). Board tests (`tests/board/*.py`, run by `xewe test`) flash the firmware
and assert on serial output; without a board they report "compiled, not run", except
`test_compiles`, which really builds. There are no on-device unit-test frameworks.

**One board at a time, compile cost (D15).** A build costs about a minute per chip, so `build` and
`test` compile only the selected chip; the c3/c6/s3 matrix runs with `--all-chips`. The shared
toolchain is installed once per machine, and `--arduino-data DIR` reuses an installed core
elsewhere. The tools never read or write `~/.arduino15` or `~/Arduino`.

## 6. Versioning and release policy (D12)

| Repo | Version | Tag | Released by |
|---|---|---|---|
| `xewe-os-core` | semver in `library.properties` | `X.Y.Z` (un-prefixed, Arduino registry rule) | `scripts/release.sh`; the Library Manager picks up the tag |
| `xewe-os-modules` | per module (`module.properties`) and the set | `<module>/X.Y.Z` per module, `X.Y.Z` for the set | `scripts/release.sh`; each module declares `requires_core`, for example `>=3.0.0,<4.0.0` |
| `xewe-os-tools` | `pyproject.toml` | `X.Y.Z` | `scripts/release.sh` |
| `xewe-os` | `xewe.toml [project] version`, independent | `X.Y.Z` | `scripts/release.sh` runs the tests once, renders `release_card.md` and tags; CI builds and publishes the GitHub Release with the images and the card as assets and the notes as the Release text (D26, D27) |

Tags carry no `v` from 3.0.0 on (older `v` tags keep their names).

**Refs: pinned to tags, 3.1.0 generation.** A ref in `xewe.toml` is a tag, a branch, a commit SHA or
`latest`, the newest commit of the repository's default branch. The template pins core `3.1.2`,
tools `3.1.5`, modules `wifi/3.1.2` and ArduinoJson `v7.4.3`; the
whole ecosystem releases 3.1.0 in dependency order (tools, core, modules, `xewe-os`, then the
product). A project moves its refs with `xewe manifest update` (newest tag), and a new core or
modules release reaches a project only when its manifest moves. `.github/workflows/ci.yml` calls
the tools workflows at the same tag as `[tools] ref`.

**Release flow.** `scripts/release.sh --prepare X.Y.Z` does one complete run (setup, module
validation, unit tests, the board tests of every chip, the builds, format and brand lint), keeps
its logs in `releases/X.Y.Z/logs/` (local, gitignored) and renders `releases/X.Y.Z/release_card.md`
from that run (D27). After a pushed prepare commit, `scripts/release.sh X.Y.Z` verifies the card
against the tree, checks main and CI, and pushes the annotated tag `X.Y.Z` with
`releases/X.Y.Z/notes.txt` as its message. The tag starts `ci.yml`: the build job (c3, c6, s3, host
tests, lint), then the release job (`xewe-release.yml`): one build per chip in parallel, then an
assembly job that checks the card against those runs and publishes the GitHub Release (the tag
message plus the resolved commits as notes). The assets are the images, each with its `meta.json`
and ESP Web Tools `manifest.json`, `firmware_map.csv`, `firmware-X.Y.Z.tar.gz` and
`release_card.md`. Nothing is committed back: the tag holds the sources and the card, the Release
holds the binaries (D26). Each image has a direct URL,
`https://github.com/xewe-labs/xewe-os/releases/download/<tag>/<asset>`. The old `binaries` branch
and the committed 3.0.0 folder are legacy; the CI lint `No committed firmware` keeps
`static/` holds nothing in git.

## 7. Decisions log

The decisions that shape the ecosystem. Numbers are stable; a retired decision keeps its number
unused.

| # | Decision | Notes |
|---|---|---|
| D1 | Target hardware is ESP32 **C3, C6, S3** only | Library metadata names these three only |
| D2 | No OTA | Keep `no_ota` partitions; flashing is serial only |
| D3 | Template UX is `git clone` → `./setup.sh` → `./run.sh` | Needs Python and git; `setup.sh` writes `run.sh` |
| D4 | Modules live in **one repo** (`xewe-os-modules`) | One contract, one validator, one index |
| D5 | Stay on **arduino-cli** | PlatformIO is not clearly better: its espressif32 platform still ships Arduino 2.0.17, so Arduino 3.x on C6 needs the community pioarduino fork; it has no lock file; and the test runner is custom code either way. arduino-cli `sketch.yaml` profiles (1.3 and later) pin core, libraries and FQBN, see §8 |
| D6 | Testing is hardware-in-the-loop with an attached board, and compile-only without one | Both paths are first class |
| D8 | **One library: `XeWeCore`** = utils + serial + cli + nvs + os | Avoids the `xewe-os` / `XeWeOS` name collision. ArduinoJson (header-only, the standard Arduino JSON library) is its one dependency |
| D9 | "Core" not "Kernel" | Kernel implies scheduling/memory management, which this does not do |
| D10 | **One tooling repo: `xewe-os-tools`** (setup + build + flash + serial + test) | Single Python package, shared port discovery and serial code |
| D11 | Template gets dependencies via **`setup.sh` + `xewe.toml`** | One ref each for core, modules, tools (`latest` until the 3.0.0 tag set, tags since); `--latest` tries the newest tags; nothing generated is committed; no submodules |
| D12 | Versioning: core is semver and published; modules repo tagged as a whole, each module declares `requires_core`; template pins both in `xewe.toml` | Template version is independent |
| D13 | Module contract keeps the explicit `declare` line in `module.properties` | An automated validator checks uniqueness of slug, id, folder and variable name instead of a human review |
| D14 | Tests are **Python, host-driven**: flash, send CLI commands over serial, assert on output | Two layers per repo: `tests/board/` (pytest against the board) and `tests/unit/` (no board: C++ with an Arduino shim or g++, and `@pytest.mark.unit` Python). No on-device Unity tests |
| D15 | One board attached at a time; compile per board costs ~1 min | Tester compiles only the attached/selected chip by default; the full C3/C6/S3 matrix runs only on an explicit flag |
| D16 | Utils is **inside** `XeWeCore` | Header-only; everything depends on it |
| D17 | `XeWeCore` layout: **`XeWeOs` facade + standalone component headers** | See section 3; `os.serial`, `os.cli`, `os.nvs`, `os.system` |
| D18 | **No separate publishing tool**: `XeWeCore` is released by its own `scripts/release.sh`; its Library Manager rules live in core's `doc/library-rules.md` and CI runs `arduino-lint` (strict) on it | One library is left to publish, so the multi-library publishing repo was retired |
| D19 | Repo names are **prefixed**: `xewe-os-core`, `xewe-os-modules`, `xewe-os-tools`, `xewe-os` | Everything reads as part of xewe-os |
| D20 | `xewe-os` is a **GitHub template repository** and also plain-cloneable | "Use this template" gives fresh history; clone keeps working |
| D21 | Modules are built and tested **through a `xewe-os` checkout** as the harness | The modules repo carries no toolchain; a harness is a copy of the template with the module selected. One build path, nothing to drift |
| D22 | **No board is a normal state.** Without a board, board tests compile only; the runner reports "compiled, not run" and exits 0 | `--require-board` makes a missing board an error; `XEWE_NO_BOARD=1` never opens a port |
| D23 | **Agent-driven development** | Coding agents do the hands-on work; a supervising session verifies and steers; issues are resolved and logged, only critical ones interrupt the maintainer |
| D24 | **Development mode until 3.0.0: refs were `latest`** | Manifests track `latest`; harnesses build from local working trees (`XEWE_*_SOURCE`); every repository was tagged `3.0.0` together, with CI/CD, and refs are pinned to tags since (3.1.0 for the next generation, §6) |
| D25 | **One repo layout** | `README.md` and `LICENSE.txt` are the only reader files at a root; pages live in `doc/`; the C++ style ships with the tools; `run.sh` is written by `setup.sh` and never committed |
| D26 | **Binaries are GitHub Release assets only, never committed** | A release publishes each image as an asset with a direct URL (`https://github.com/xewe-labs/xewe-os/releases/download/<tag>/<asset>`); nothing is committed back to `main`, so the tag is the source and the Release is the binary. Git LFS is for `static/media/**` only. `static/firmware/releases/` holds `.gitkeep`; the CI lint `No committed firmware` fails on anything else. Branch publishing (`publish_branch`, `releases_branch`) is deprecated and off |
| D27 | **Every release has a generated `release_card.md`** | `scripts/release.sh` produces it from one complete run (setup, validation, unit, board per chip, builds, lint) with `xewe release-card`; no hand-written cards. The card records what ran on which commit and tree, every test per chip, and what was not covered (chips without a hardware run are stated as compile only). It is a release asset. The run logs stay in `releases/X.Y.Z/logs/`, local and gitignored; the card lists their names, sizes and sha256 but links none. CI verifies the card against the tagged tree and fails on a mismatch |
| D28 | **Project-local modules in a product's `src/` are first class** | A module in `src/<Name>/` has the same `xewe::Module` API, contract, tests and NVS namespace rules as one from `xewe-os-modules`; it is declared in the sketch and needs no entry in the modules repo. Its values are in its own `src/<Folder>/Config.h` in the module form of the core's `doc/config.md`, prefixed `XEWE_<ID>_` (the module id in upper case), edited in place. `src/SimpleModule/` (smallest) and `src/FullModule/` (every hook) are the patterns. A module that other firmwares need moves to the modules repo (D4) |

Open and deferred:

| Item | State |
|---|---|
| CI strictness and release publishing | settled at 3.1.0: each repo calls the tools' reusable workflows at a tag; in the template lint blocks (`strict_lint: true`) and a release publishes a GitHub Release with the images and the card as assets and the notes as the Release text; nothing is committed to `main` (D26) |
| laptop-cooling-pad migration | on hold; first real consumer once the backbone is published |
| xewe-led-os migration | out of scope for now; after the backbone is proven |
| Self-describing device | proposed (§8): one command that dumps every module's commands and settings schema as machine-readable JSON; `$<id> schema` per module exists, the device-wide manifest does not |
| `xewe.toml` and `sketch.yaml` profiles | proposed (§8): generate one arduino-cli profile per chip from the manifest and let arduino-cli install and cache; keep `xewe.toml` for what profiles cannot pin (git refs, hashes, `--define` values, tool versions) |
| `xewe test` on pytest-embedded | proposed (§8): build the board runner on pytest-embedded's `arduino` and `serial-esp` services; xewe keeps the compile-only fallback, the USB re-enumeration wait and the provisioning fixture |
| `xewe::Cli` on arduino-esp32 `Console` | to evaluate (§8): keep `$group` dispatch, the uniform verbs and paste-safe queueing as a layer above the vendor console |
| Improv Serial | proposed (§8): speak Improv for the Wi-Fi and device-name part of first boot so ESP Web Tools hosts work; the dotenv helper keeps timezone and module settings |
| License of the library | open (§8): the gap the survey found is a *permissively licensed* includable library; the template and the modules are GPL-3.0-only. Either the core and modules move to a permissive license or the claim is narrowed to "includable, GPL" |
| Web UI builder | the exporter is `xewe webui` (tools `doc/webui.md`); later: prototype UIs under constraints that port straight to a web-interface module |
| Web flasher hosting | later: move the ESP Web Tools flasher into the org, fed by the Release asset URLs (D26) |
| OTA | out of scope (D2) |
| Windows | deferred; the tools keep paths portable |

## 8. Where XeWe OS stands

A survey of the ESP32 ecosystem as of 2026, capability by capability, against what this template
and its three repositories do. The short version: almost every mechanism exists somewhere, nobody
combines them the way the module contract does, and the more plumbing XeWe OS hands to existing
tools the more of its effort lands on the contract.

### 8.1 Host side: mostly re-wrapped

- **Pinning.** Since arduino-cli 1.3 a `sketch.yaml` profile pins the core platform and its index
  URL, exact Library-Index library versions with transitive dependencies, local `dir:` libraries
  and a full FQBN, installs into an isolated per-profile cache, and from 1.4 has `arduino-cli
  profile` subcommands ([sketch-project-file.md](https://github.com/arduino/arduino-cli/blob/master/docs/sketch-project-file.md)).
  Profiles cannot pin git refs, ZIP URLs, content hashes, `--build-property` values or tool
  versions other than the platform's, and each board needs its own profile. Those limits are the
  only reasons `xewe.toml` needs to exist beside a profile; pinning ArduinoJson by version is not
  one of them. PlatformIO has no lock file at all. ESP-IDF's component manager has a
  solver-generated `dependencies.lock` with a manifest hash and per-component hashes, which is the
  design to copy if `xewe.toml` grows hashes.
- **Flashing that keeps NVS** is not an invention, it is the default when nothing goes wrong:
  arduino-esp32's upload writes only bootloader, partition table, boot_app0 and app, and esptool
  erases only the sectors it writes. The hazard is the `.merged.bin`, padded with 0xFF to full
  flash size, which wipes NVS when written at 0x0. XeWe OS flashes exactly that image, so its real
  value here is making "update" (`flash`, NVS kept) and "fresh install" (`run`, erase first) two
  explicit modes, and applying `--after watchdog-reset` on chips with native USB-Serial/JTAG.
- **Serial console.** `arduino-cli monitor`, `pio device monitor` and `idf.py monitor` all exist.
  The timestamped `xewe serial` has no differentiation.
- **Board tests.** Espressif's pytest-embedded (MIT) has an `arduino` service that reads the
  arduino-cli build directory, flashes only changed sectors, selects ports by serial number, MAC
  or USB location, drives several boards with `--count` and offers `dut.expect()`; arduino-esp32
  itself tests with it on Wokwi, QEMU and hardware ([pytest-embedded](https://github.com/espressif/pytest-embedded)).
  It lacks exactly what `xewe test` emphasises: an automatic compile-only fallback when no board
  is attached (D22), a "wait for the native-USB port to re-enumerate and see the boot banner"
  primitive, and a provisioning step. labgrid, Jumpstarter and `pio test` lack them too. The
  harness is justified by its CLI-driven test semantics and its fallback, not by its serial
  plumbing.

### 8.2 Firmware side: every primitive exists, as loose parts or inside monoliths

- **`$group command args` CLI: partial.** arduino-esp32 3.3 ships a first-party `Console` library
  on the IDF console (argtable parsing, REPL with history, plain mode for scripting, UART and
  HWCDC) with a flat namespace ([Console](https://github.com/espressif/arduino-esp32/tree/master/libraries/Console)).
  ESP32SerialCtl (MIT) has a `group` per command, `help <group>` and an `OK`/`ERR` convention very
  close to `xewe::Cli`; AdvancedCLI (MIT) has two-level subcommands with typed arguments. Nothing
  enforces one verb set across modules, and nothing documents keeping back-to-back pasted lines
  while a command runs (AdvancedCLI's example drops them).
- **Uniform `status`/`enable`/`disable`/`reset` verbs: unique.** The nearest is Tasmota's
  convention that a command without parameters returns its settings as JSON.
- **Typed NVS settings: partial.** MycilaConfig (MIT) has declared keys, typed get/set over a
  `std::variant`, validators, change callbacks and an `_enable` suffix convention, but no
  per-module namespaces and no command layer; its validator and `_pwd` masking are worth reading
  before extending XeWe settings. Arduino `Preferences` silently returns the caller's default on a
  type mismatch, so strict typing on NVS is real value. Tasmota and ESPHome store opaque blobs and
  expose no per-key types.
- **Runtime-queryable settings schema: unique.** A schema a host can read over serial (name,
  type, default, range, secret flag) exists nowhere else. `$<id> schema` is this; a device-wide
  manifest is not written yet.
- **FlexData JSON layer: partial.** Precedent only in ESP32-sveltekit's stateful services and
  Tasmota's JSON responses, both on ArduinoJson. Valuable only if CLI, NVS and web all route
  through it.
- **Bounded prompts: unique, and contrarian.** Shellminator has timed `waitForKey` and
  `selectList` but an unbounded line `input()`; no library pairs yes/no, string and integer prompts
  with both a timeout and a retry count. Tasmota, ESPEasy, WLED and Meshtastic avoid prompts
  entirely and use one command grammar over every transport, which is what makes them scriptable.
  XeWe's prompts are defensible only because they are bounded and `xewe provision` answers them;
  an unbounded prompt would be a regression.
- **Wi-Fi, web, time, scheduler, buttons, pins modules: covered.** WiFiManager and
  MycilaESPConnect do portals, ESP32-sveltekit does web UI with NTP, provisioning and OTA; timers,
  rules, buttons and GPIO config are standard in Tasmota, ESPHome and ESPEasy. The modules justify
  themselves by conforming to the contract; the radio, HTTP and debounce logic underneath should be
  borrowed wherever the licence allows.

### 8.3 The contract as a library is the one real gap

Every large ESP32 platform picks modules at compile time and ships as a finished firmware: Tasmota
with `USE_*` defines and per-driver command tables, ESPEasy with `USES_P0xx` plugin sets, WLED with
`REGISTER_USERMOD()` and per-usermod `addToConfig`/`readFromConfig`, Meshtastic with
`MESHTASTIC_EXCLUDE_*` and `ProtobufModule<T>`. ESPHome comes closest to the contract (a `DOMAIN`
id, a config schema, a test file per component), but its tests are compile-only and its
includable library `esphome-core` was archived in 2021. Libraries in XeWe's shape are dormant
(Homie, Basecamp: last release 2020) or whole alternative frameworks (Sming, Mongoose OS, Toit).
Tasmota, Meshtastic and OpenMQTTGateway are GPL-3.0 and WLED is EUPL-1.2, so their module code
cannot be lifted into a permissively licensed library.

Behavioural tests per module, driven over the serial CLI on real hardware, have no reusable
precedent either: WLED states it has no C++ unit tests, and Tasmota and ESPEasy CI are build-only.
Real behavioural testing exists only as project-internal harnesses (Meshtastic's native Unity tests
plus a hardware tier, Marlin's simulators, Espressif's pytest-embedded runners).

First-boot provisioning from a dotenv file is unique. The closest neighbour is Improv Serial,
which carries Wi-Fi credentials, device info, scanning, hostname and device name, but no timezone
or custom settings, and has device and browser SDKs but no maintained Python host
([Improv serial](https://www.improv-wifi.com/serial/)).

The agent-oriented development model (D23) is standard practice now: ESPHome, WLED and Tasmota
ship agent guides, and Meshtastic ships an MCP server that lets agents flash, configure and test
real devices. The angle left is narrower and at the library level: deterministic CLI responses and
queryable command and settings manifests, so a device is agent-operable without a per-project
prompt.

### 8.4 Verdict per capability

| Capability | Closest existing tools | Verdict |
|---|---|---|
| `$group command args` CLI (`xewe::Cli`) | arduino-esp32 `Console`, ESP32SerialCtl, AdvancedCLI, IDF console | partial: grouping exists, per-module ownership does not |
| Uniform `status`/`enable`/`disable`/`reset` verbs | Tasmota naming convention | **unique**; small but real |
| Typed NVS settings (`xewe::Nvs`, settings tables) | Preferences (silent mismatch), MycilaConfig | partial: per-module namespace and strict mismatch errors are new |
| Runtime-queryable settings schema (`$<id> schema`) | none | **unique**; the highest-leverage gap for agents and host tools |
| `xewe::FlexData` | ArduinoJson, ESP32-sveltekit stateful services | partial |
| Bounded prompts (timeout and retries) | Shellminator (timeouts only) | **unique**, contrarian: product firmwares avoid prompts |
| `XeWeOs` facade and module contract (id, commands, NVS, three tests) | ESPHome, WLED usermods, Tasmota drivers | **unique as an includable library**; the core differentiator |
| wifi, web-interface, time, scheduler, buttons, pins | WiFiManager, MycilaESPConnect, ESP32-sveltekit, Tasmota, ESPHome | covered: keep thin, wrap |
| `xewe.toml` refs | arduino-cli `sketch.yaml` profiles, IDF `dependencies.lock` | mostly covered: justified for git refs, hashes, `--define` values, tool versions |
| Per-chip C3/C6/S3 builds | one profile per board, PlatformIO envs | covered; one chip matrix in one file is a convenience |
| Flash that keeps NVS (`xewe flash`) | arduino-cli upload, pytest-embedded fast-flash | covered; value is the explicit update vs fresh-install split |
| `xewe serial` | arduino-cli, pio, idf.py monitors | covered |
| `xewe test` board runner | pytest-embedded (arduino, wokwi, qemu, esp-emu) | mostly covered; unique in CLI semantics, compile-only fallback, re-enumeration handling |
| Automatic compile-only fallback (D22) | manual CI wiring elsewhere | **unique**, easy to copy |
| Dotenv first-boot provisioning (`xewe provision`) | Improv Serial, Tasmota `Backlog` | **unique**; should interoperate with Improv |
| Template with `setup.sh` and generated `run.sh` | ESP32-sveltekit template | unique, low moat |
| Agent-oriented model (D23) | ESPHome, WLED, Tasmota agent docs; Meshtastic MCP server | covered, and behind Meshtastic |

### 8.5 What follows

Candidates, not decisions; each is listed in the open items of section 7.

1. **Generate `sketch.yaml` profiles from `xewe.toml`**, one per chip, and let arduino-cli install
   and cache. `--dump-profile` bootstraps one. Keep `xewe.toml` for what profiles cannot express
   and model any hashes on IDF's `dependencies.lock`.
2. **Build `xewe test` on pytest-embedded's `arduino` and `serial-esp` services** rather than
   beside them: diff-flashing, port selection by MAC or serial number, several boards, and
   board-free backends (Wokwi; QEMU for S3 and C3 without Wi-Fi; Espressif's beta esp-emu with an
   emulated soft-AP) come for free. XeWe contributes the compile-only fallback, the re-enumeration
   wait and the provisioning fixture.
3. **Evaluate `xewe::Cli` on arduino-esp32's `Console`** (or the IDF console), keeping `$group`
   dispatch, the uniform verbs and paste-safe queueing as a layer above it. Gains line editing,
   history and HWCDC support from a vendor-maintained library; costs the IDF console's REPL task
   model.
4. **Wrap, do not reimplement, the wifi and web modules**: MycilaESPConnect or WiFiManager for
   connection and portal handling, ArduinoJson (already pinned) under `xewe::FlexData`.
5. **Speak Improv Serial for the Wi-Fi and device-name part of first boot**, so ESP Web Tools and
   other Improv hosts work out of the box; the dotenv helper keeps timezone and module settings.
   The ESP Web Tools `manifest.json` that `xewe release` already writes is the de facto
   distribution format.
6. **A device that describes itself**: one command that dumps every module's commands and settings
   schema as machine-readable JSON. It makes a XeWe device agent-operable without per-project
   prompts, nothing in the ecosystem offers it, and it is a better investment than further work on
   flashing, pinning or console tooling.

One point cuts across all of these. The gap the survey found is a *permissively licensed*
includable library. This template and the modules are GPL-3.0-only, so the gap is only partly
XeWe's to claim until the license question in section 7 is settled.

### 8.6 What the survey did not cover

The PlatformIO registry and the full Arduino index were not searched exhaustively for NVS-schema or
JSON-data-layer libraries, so a niche equivalent could exist. Whether pytest-embedded reopens a port
after USB re-enumeration was not confirmed. Serial pretty-printers were not assessed. The absence of
another `setup.sh`/`run.sh` template bundle is a negative search result, not proof.

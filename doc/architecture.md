# Architecture

Why XeWe OS is shaped the way it is. The [README](../README.md) says how to use it; this page says
why it is four repositories, why the sketch has exactly one include, why modules carry an explicit
declare line, and what is decided versus still open.

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
| [`xewe-os`](https://github.com/xewe-labs/xewe-os) | the firmware template users clone | its own code, `xewe.toml`, `setup.sh` and released firmware; `run.sh` is generated |
| [`xewe-os-core`](https://github.com/xewe-labs/xewe-os-core) | `XeWeCore`, the one Arduino library | reusable code every firmware needs; publishable on its own; depends only on ArduinoJson |
| [`xewe-os-modules`](https://github.com/xewe-labs/xewe-os-modules) | every module, `modules/<slug>/` | features some firmware needs; the contract in `doc/contract.md`, the index in `doc/modules.md`; no toolchain of its own; built and tested through an `xewe-os` checkout (D21) |
| [`xewe-os-tools`](https://github.com/xewe-labs/xewe-os-tools) | `xewe`, one Python package (`src/xewe/`) | everything that runs on the host: setup, build, flash, serial, test, release, format (the one C++ style file ships inside the package), brand-lint. The spec is `doc/spec.md` |

Every repo has the same top level: `README.md` and `LICENSE.txt` are the only files written for a
reader at the root, longer pages live in `doc/`, the agent guide in `.agents/`, CI in
`.github/workflows/`. No repo commits a `.clang-format` or a `run.sh` (D25).

[`publish-arduino-library`](https://github.com/xewe-labs/publish-arduino-library) stays a separate,
generic tool that checks and releases `XeWeCore` (D18).

```
                        xewe-os  (template)
                        xewe.toml pins one ref each
             ┌──────────────────┼──────────────────┐
             v                  v                  v
      xewe-os-core       xewe-os-modules      xewe-os-tools
      (XeWeCore lib) <── each module has      (xewe: setup, build,
             ^           requires_core         flash, serial, test)
             │                                     │
  publish-arduino-library                fetches core and ArduinoJson into the
  (check + release XeWeCore)             project's build/; arduino-cli, the esp32
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
├── xewe-os.ino              committed   #include <XeWeCore.h>; XeWeOs os({...}); your setup()/loop()
├── Config.h                 committed   your defaults plus one appended block of compile-time values per
│                                        selected module (setup adds it once; you edit it); includes <XeWeBuildInfo.h>
├── xewe.toml                committed   the manifest: [project] [core] [modules] [tools] [libraries]
├── setup.sh                 committed   bootstraps build/tools/.venv, then calls `python -m xewe setup`
├── src/YourModule/  src/YourModuleFull/   committed   project-local example modules
├── tests/board/  tests/unit/  committed  the project's tests (`xewe test`)
├── static/firmware/releases/  committed  written by `xewe release`
├── README.md  LICENSE.txt  doc/  .agents/  .github/workflows/ci.yml   committed
├── run.sh                   generated   written by every ./setup.sh; calls `xewe run`
├── .env                     local       credentials for `xewe provision`; optional, yours (setup never writes it)
├── src/Modules.h            generated   #include <XeWeModules.h> + the declare lines
└── build/                   generated   build/.gitignore is `*`
    ├── builds/<chip>/
    │   ├── gen/XeWeBuildInfo/           the generated header, per build
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
  XeWeCore.h                 umbrella: the only top-level header
  XeWeCore/
    XeWeOs.h/.cpp            xewe::Os (global alias XeWeOs) and xewe::System
    Module.h/.cpp            xewe::Module, the base class of every module
    Serial.h/.cpp            xewe::SerialPort
    Cli.h/.cpp               xewe::Cli: "$group command args", $help
    Nvs.h/.cpp/.tpp          xewe::Nvs: typed key-value storage
    FlexData.h               xewe::FlexData
    Utils.h + Utils/*.h      header-only: String, AsyncTimer, Span, Color, LockGuard, ...
```

```cpp
#include <XeWeCore.h>
XeWeOs os({.project_name = "my-device", .version = "0.1.0"});
void setup() { os.begin(); os.serial.println("hi"); }
void loop()  { os.loop(); }   // os.cli, os.nvs, os.system.restart()
```

The facade owns `os.serial`, `os.cli`, `os.nvs` and `os.system`. The components also work alone
(`xewe::SerialPort`, `xewe::Cli`, `xewe::Nvs`, `xewe::FlexData`, `xewe::str`) with no `XeWeOs`.
Everything is in `namespace xewe`; the include direction is Utils ← Serial ← Cli, FlexData ← Nvs,
all ← Module ← XeWeOs.

**Why `#include <XeWeCore.h>` is the only sketch include.** arduino-cli discovers which library to
compile by trying the sketch's includes against each library's top-level `src/` headers. A sketch
whose only include is `<XeWeCore/XeWeOs.h>` fails with "No such file": the sub-header is not a
top-level header, so the library is never added (arduino-cli 1.5.1). Sub-headers may be included
*after* the umbrella.

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
| `release --version X.Y.Z` | release matrix into `static/firmware/releases/` (run by CI on a tag; see §6) |
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
| `xewe-os-core` | semver in `library.properties` | `X.Y.Z` (un-prefixed, Arduino registry rule) | `publish-arduino-library` check + release |
| `xewe-os-modules` | per module (`module.properties`) and the set | `<module>/X.Y.Z` per module, `X.Y.Z` for the set | `scripts/release.sh`; each module declares `requires_core`, for example `>=3.0.0,<4.0.0` |
| `xewe-os-tools` | `pyproject.toml` | `X.Y.Z` | `scripts/release.sh` |
| `xewe-os` | `xewe.toml [project] version`, independent | `X.Y.Z` | `scripts/release.sh` tags; CI builds, publishes and commits the binaries under `static/firmware/releases/` |

Tags carry no `v` from 3.0.0 on (older `v` tags keep their names).

**Refs: frozen at 3.0.0.** A ref in `xewe.toml` is a tag, a branch, a commit SHA or `latest`, the
newest commit of the repository's default branch. The template pins core `3.0.0`, modules `3.0.0`,
tools `3.0.1` and ArduinoJson `v7.4.3`; the template itself is `3.0.0`. The repositories were
tagged 3.0.0 together, as one ecosystem generation (tools 3.0.1 adds the release commit to
`main`); a project moves its refs with `xewe manifest update` (newest tag), and a new core or
modules release reaches a project only when its manifest moves. `.github/workflows/ci.yml` calls
the tools workflows at the same tag as `[tools] ref`.

**Release flow.** `scripts/release.sh X.Y.Z` (after `--prepare X.Y.Z` and a pushed prepare commit)
checks main, the version locations and CI, runs `xewe test --no-board` and a build, and pushes the
annotated tag `X.Y.Z` with `releases/X.Y.Z/notes.txt` as its message. The tag starts `ci.yml`: the
build job (c3, c6, s3, host tests, lint), then the release job (`xewe-release.yml`): `./setup.sh`,
`xewe release` (the default matrix: one plain build per chip), a GitHub Release (the tag message
plus the resolved commits as notes; the `.bin` files and `firmware-X.Y.Z.tar.gz` as assets), then
one bot commit `release X.Y.Z (...) [skip ci]` on `main` that adds `static/firmware/releases/X.Y.Z/`
and `index.json` and keeps the rest of the tree. The `.bin` files are stored in Git LFS
(`.gitattributes`: `static/**/*.bin`, and png/webp/mp4/numbers). The tag does not contain its own
binaries; `main` after the bot commit does. The script waits for that run, attaches
`releases/X.Y.Z/testcard.md` and prints the assets and the commit. The layout is the pre-3.0
one: `X.Y.Z/{release_notes.txt,firmware_map.csv}` and
`X.Y.Z/<chip>/<matrix folders>/{X.Y.Z-<chip>-<name>.bin,manifest.json,meta.json,build_notes.txt}`.
The old `binaries` branch is legacy and untouched.

## 7. Decisions log

The decisions that shape the ecosystem. Numbers are stable; a retired decision keeps its number
unused.

| # | Decision | Notes |
|---|---|---|
| D1 | Target hardware is ESP32 **C3, C6, S3** only | Library metadata names these three only |
| D2 | No OTA | Keep `no_ota` partitions; flashing is serial only |
| D3 | Template UX is `git clone` → `./setup.sh` → `./run.sh` | Needs Python and git; `setup.sh` writes `run.sh` |
| D4 | Modules live in **one repo** (`xewe-os-modules`) | One contract, one validator, one index |
| D5 | Stay on **arduino-cli** | PlatformIO is not clearly better: C6 Arduino support only exists in the community pioarduino fork, and the test runner is custom code either way |
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
| D18 | `publish-arduino-library` **stays a separate repo** | Generic Arduino-library publishing tool; not folded into xewe-os-tools |
| D19 | Repo names are **prefixed**: `xewe-os-core`, `xewe-os-modules`, `xewe-os-tools`, `xewe-os` | Everything reads as part of xewe-os |
| D20 | `xewe-os` is a **GitHub template repository** and also plain-cloneable | "Use this template" gives fresh history; clone keeps working |
| D21 | Modules are built and tested **through a `xewe-os` checkout** as the harness | The modules repo carries no toolchain; a harness is a copy of the template with the module selected. One build path, nothing to drift |
| D22 | **No board is a normal state.** Without a board, board tests compile only; the runner reports "compiled, not run" and exits 0 | `--require-board` makes a missing board an error; `XEWE_NO_BOARD=1` never opens a port |
| D23 | **Agent-driven development** | Coding agents do the hands-on work; a supervising session verifies and steers; issues are resolved and logged, only critical ones interrupt the maintainer |
| D24 | **Development mode until 3.0.0: refs were `latest`** | Manifests track `latest`; harnesses build from local working trees (`XEWE_*_SOURCE`); every repository was tagged `3.0.0` together, with CI/CD, and refs are frozen since |
| D25 | **One repo layout** | `README.md` and `LICENSE.txt` are the only reader files at a root; pages live in `doc/`; the C++ style ships with the tools; `run.sh` is written by `setup.sh` and never committed |

Open and deferred:

| Item | State |
|---|---|
| CI strictness and release branch | settled at 3.0.0: each repo calls the tools' reusable workflows at a tag; in the template lint blocks (`strict_lint: true`) and a release publishes a GitHub Release and commits the binaries to `main` (`releases_branch: main`, Git LFS) |
| laptop-cooling-pad migration | on hold; first real consumer once the backbone is published |
| xewe-led-os migration | out of scope for now; after the backbone is proven |
| Frontend builder v2 | later: prototype UIs in Flask/FastAPI under constraints that port to a web-interface module |
| Web flasher hosting | later: move the ESP Web Tools flasher into the org, fed by `static/firmware/releases/` |
| OTA | out of scope (D2) |
| Windows | deferred; the tools keep paths portable |

# Repos

XeWe OS is four repositories under github.com/xewe-labs, and this product is a project made from the `xewe-os` template. `xewe.toml` holds the refs it uses.

## Where a change goes

| Change | Repo |
|---|---|
| The sketch, `Config.h`, `xewe.toml`, `release_matrix.csv`, the four product modules, the `REPO` and `BOARD_GATES` lines of `scripts/release.sh`, `ci.yml` | `xewe-led-os` (this repo) |
| `setup.sh`, the rest of `scripts/release.sh`, the template docs in `doc/` | `xewe-os`, the template this project copied them from |
| HomeSpan and Espalexa fixes | the forks `xewe-led-library-homespan` and `xewe-led-library-espalexa`, pinned by SHA in `xewe.toml` |
| Code every firmware needs, the `XeWeOs` facade, `xewe::Module`, Cli, Nvs, Settings, Utils | `xewe-os-core`, the one Arduino library `XeWeCore` |
| A feature only some firmware needs | `xewe-os-modules`, as `modules/<slug>/` |
| Anything that runs on the host (setup, build, flash, serial, test, release, format, brand-lint, webui) and the reusable CI workflows | `xewe-os-tools`, one Python package in `src/xewe/` |

A module that only one product needs stays in that product's `src/` (D28). The LED engine and LED web page are the `led` and `led-web` modules of `xewe-os-modules`, not code in this repo. The core depends only on ArduinoJson and must stay publishable on its own.

## Boundaries

- The modules repo has no toolchain of its own. A module is built and tested through an `xewe-os` checkout (D21).
- The core's CI runs host tests, the examples and `arduino-lint` against its `doc/LIBRARY_RULES.md`, and the core is released by its own `scripts/release.sh` (D18).
- The tools carry their own spec in `doc/spec.md` and the webui exporter in `doc/webui.md`. The spec is the authority for the `build/` layout (its section 6).
- The contract is in `doc/CONTRACT.md` of the modules repo and the module index in its `doc/modules.md`.
- Every repo has `README.md` and `LICENSE.txt` at the root, long pages in `doc/`, the agent guide in `.agents/` and CI in `.github/workflows/` (D25).
- Nothing generated is committed and there are no submodules (D11). `build/`, `src/Modules.h` and `run.sh` can be deleted at any time and `./setup.sh` rebuilds them from `xewe.toml`.

`doc/XEWE_OS_ECOSYSTEM.md` covers what `./setup.sh` installs and the committed versus generated split. This repo's `static/` is output of the release and nothing in it is tracked.

The organization guidelines in `https://github.com/xewe-labs/.github` (its `guidelines/`) apply where this skill is silent. Where the two differ, the guidelines win and this skill is corrected.

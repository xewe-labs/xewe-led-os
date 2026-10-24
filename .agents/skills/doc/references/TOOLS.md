# Tools

`xewe` comes from `xewe-os-tools`. Run it as `build/tools/.venv/bin/python -m xewe <command>` or through `./run.sh`. Every flag is in the tools' `doc/spec.md`, and the everyday commands are in `doc/XEWE_OS_ECOSYSTEM.md`.

## xewe.toml writes

Only `xewe manifest update`, `xewe modules select`, `setup --modules`, the first-setup menu and `release` write `xewe.toml`. This project's selection is its committed choice of seven modules. `setup --latest` tries the newest tags without editing the file. Builds never change `[project] version`.

## Test layers (D14)

Tests live in `tests/unit/` and `tests/board/` and nowhere else. Unit tests need no board and no build: C++ for pure logic (in the core, an Arduino shim; in a module or project, pure headers built with g++ by a `unit`-marked pytest driver) and `@pytest.mark.unit` Python, selected by `xewe test --unit-only`. Board tests flash the firmware and assert on serial output. Without a board they are reported as not run, except `test_compiles`, which really builds. See NO_BOARD.md for the no-board rules.

## Cost and isolation (D15)

A build costs about a minute per chip, so `build` and `test` compile only the selected chip. The full chip matrix runs with `--all-chips`. CI builds every chip, `scripts/release.sh` builds none.

The shared toolchain is installed once per machine in `~/.xewe-os/build-tools/` (`XEWE_HOME` moves it). The tools never read or write `~/.arduino15` or `~/Arduino`. `--arduino-data DIR` reuses an installed esp32 core.

## Flashing

`flash` writes the merged image at 0x0 and keeps NVS unless `--erase` is given. `run` builds, erases, flashes and listens, and `--keep-nvs` skips the erase. The merged image is padded with 0xFF, so writing it any other way wipes NVS.

## Not supported

Windows is deferred, and the tools keep paths portable. OTA is out of scope (D2).

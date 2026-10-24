# Commands

Run everything from the project root, after `./setup.sh`, as
`build/tools/.venv/bin/python -m xewe <command>` (written `xewe` below). The bare
`build/tools/.venv/bin/xewe` script breaks when the project folder moves. Re-run `./setup.sh`
if it does.

| Task | Command |
|---|---|
| Set up, on a fresh project or after `xewe.toml` changed | `./setup.sh`; add `</dev/null` in a script so no module menu appears |
| See the compiler command first | `xewe build --chip c3 --dry-run` prints the arduino-cli command |
| Build | `xewe build --chip c3`; run `xewe build --all-chips` before calling a change done |
| Test | `xewe test --unit-only`, or `xewe test --module SLUG` |
| Modules | `xewe modules list`, `xewe modules validate [PATH]` |
| Look around | `xewe doctor`, `xewe manifest show`, `xewe boards --no-probe` |
| Build, erase, flash, listen | `./run.sh`, the same as `xewe run`; `--keep-nvs` keeps stored settings |

The `xewe.toml` chip is `s3`. A command without `--chip` uses it.

Run `xewe modules select LIST|all|none` only when asked, because it edits `xewe.toml`. Run
`./run.sh` and anything else that flashes only when asked.

`-v` may follow the subcommand, as in `xewe test -v`. A `--define` string value carries its own
quotes: `--define 'XEWE_CORE_DEVICE_NAME="Bench"'`.

## Refs

`xewe.toml` pins the core, the modules and the tools to tags. `[tools] ref` and the `@<tag>`
of the two tools workflows in `.github/workflows/ci.yml` name the same tag. Move all of them
to a new tag with `xewe manifest update`, and never edit a commit SHA by hand. The HomeSpan and
Espalexa libraries are pinned to commit SHAs of the xewe-labs forks; change those only when
asked.

The module selection in `[modules] selected` is this product's committed choice. Only
`xewe modules select`, `./setup.sh --modules`, the first-setup menu, `xewe manifest update` and
the release write `xewe.toml`, and only when asked.

## Release

Never run `scripts/release.sh`. It commits, tags and pushes, and the maintainer runs it. Read
`scripts/README.md` before explaining it.

Tags are `X.Y.Z` with no `v`. The script starts only on a clean `main` that is level with
origin, and it refuses unless the one to three notes lines pass `xewe release-notes check`.
It records one test run into `build/release/X.Y.Z/` (gitignored) and renders
`release_card.md` there. With `--hardware c3` the board tests are flashed and run on that chip.
Without it they are compiled for the `xewe.toml` chip and not run. The script then commits the
version line as `release X.Y.Z`, tags it with the notes and the card as the tag message, and
pushes `main` before the tag. The tag's CI builds every row of `release_matrix.csv`, checks the
card and publishes the GitHub Release with the images and the card as assets. The script waits
for that run. Nothing is committed back, because images are release assets only.

The script is the template's, and this project changes only its two lines `REPO` and
`BOARD_GATES`. `BOARD_GATES` turns on the gated board tests `XEWE_TEST_SCHEDULE_PAGE_HTTP` and
`XEWE_TEST_ALEXA_HTTP`.

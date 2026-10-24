# Versioning

| Repo | Version lives in | Tag | Released by |
|---|---|---|---|
| `xewe-os-core` | `library.properties` | `X.Y.Z` | its `scripts/release.sh`, then the Library Manager picks up the tag |
| `xewe-os-modules` | `module.properties` per module, and the set | `<module>/X.Y.Z` per module, `X.Y.Z` for the set | its `scripts/release.sh` |
| `xewe-os-tools` | `pyproject.toml` | `X.Y.Z` | its `scripts/release.sh` |
| `xewe-os` | `xewe.toml` `[project] version`, independent of the others | `X.Y.Z` | `scripts/release.sh` |
| `xewe-led-os` (this repo) | `xewe.toml` `[project] version`, independent of the template | `X.Y.Z` | `scripts/release.sh` |

Tags carry no `v` from 3.0.0 on, and older `v` tags keep their names. Arduino's registry requires un-prefixed core tags.

## Refs and pins

A ref in `xewe.toml` is a tag, a branch, a commit SHA or `latest`, which is the newest commit of the default branch. A project moves its refs with `xewe manifest update`, which picks the newest tag. A new core or modules release reaches a project only when its manifest moves. Each module declares `requires_core`, for example `>=3.0.0,<4.0.0`.

`.github/workflows/ci.yml` calls the tools workflows at the same tag as `[tools] ref`, and `scripts/release.sh` refuses to run when they differ. A tools release therefore needs two edits in this repo.

A full ecosystem release goes in dependency order: tools, core, modules, `xewe-os`.

## Releasing this repo

Run `scripts/release.sh X.Y.Z --notes "line"`. `scripts/README.md` lists the stages, the refusals and `--resume`. Facts the script does not show:

- The tag message is the notes, a blank line and `release_card.md`. The release job in `xewe-release.yml` builds every row of `release_matrix.csv`, one job per chip with a 90 minute timeout (`timeout_minutes` in `ci.yml`), checks the card against its own runs and publishes the GitHub Release.
- A tag push starts only the release job of `ci.yml`. The build job runs on branches and pull requests.
- Nothing is committed back to `main` after the tag (D26). The run's logs stay in `build/release/X.Y.Z/`, which is gitignored.
- This project keeps its own copy of the template's `scripts/release.sh` and changes only `REPO` and `BOARD_GATES`.
- The 2.3.0 firmware is the `v2.3.0` tag, and `doc/UPGRADING_FROM_2_3.md` lists what 3.0 changed.

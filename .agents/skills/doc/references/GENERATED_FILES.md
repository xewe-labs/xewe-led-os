# Generated files

Never edit or commit these. Fix the source and regenerate.

| File | Written by |
|---|---|
| `build/`, including `build/modules/` and `build/libraries/` | `./setup.sh` and `xewe build` |
| `src/Modules.h` | `xewe modules generate`, which setup runs |
| `XeWeBuildInfo.h` and `XeWeOverrides.h` in `build/builds/<chip>/gen/` | every `xewe build` |
| `build/config/build_config.toml` | `xewe setup` |
| `run.sh` | `xewe setup`, on every run |

The sources are `xewe.toml`, the modules repository and the core. `./setup.sh` comes from the
template unchanged. It installs xewe-os-tools into `build/tools/.venv` and then runs
`xewe setup`.

Also keep out of git: release images (they are GitHub Release assets), venvs, caches, editor
files, dotenv files and the local notes file `doc/standards.md`. `.gitignore` lists them. Never open, print or copy a dotenv or key
file, because the tools read them.

The shared toolchain in `~/.xewe-os/build-tools/` is never deleted as a fix, and `xewe clean`
leaves it alone. Never run arduino-cli directly, and never touch `~/.arduino15` or `~/Arduino`.

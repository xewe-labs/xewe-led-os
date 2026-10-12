# Naming

One brand word, every form derived from it.

| Context | Form | Examples |
|---|---|---|
| Prose | **XeWe OS** (product), **XeWe** (brand) | "XeWe OS firmware", "the XeWe tools" |
| Repos, packages, folders, files, CLI, env home | lowercase `xewe`, kebab `xewe-os-*` | `xewe-os-core`, `xewe.toml`, `xewe`, `~/.xewe-os` |
| C++ namespace | `xewe::` | `xewe::Module`, `xewe::str::parse_hex_color` |
| C++ and Python type names | `XeWe` prefix | `XeWeCore`, `XeWeOs`, `XeWeModules`, `XeWeError`, `XeWeContext` |
| Macros, build defines, environment variables of the core and tools | `XEWE_*` | `XEWE_TESTING`, `XEWE_HOME`, `XEWE_DEVICE_NAME` |
| A module's compile-time values in its `Config.h` | `XEWE_MODULE_<SLUG>_<NAME>`, slug in upper case with `_` for `-` | `XEWE_MODULE_WIFI_DEBUG` |
| Module slug (folder in the modules repo, `--modules` names) | lowercase kebab, `^[a-z0-9][a-z0-9-]*$` | `wifi`, `web-interface` |
| Module id (CLI group and NVS namespace) | lowercase snake, `^[a-z][a-z0-9_]*$`, at most 15 characters, never branded | `$wifi`, `$your_mod_full` |
| Module folder and class | PascalCase, `^[A-Z][A-Za-z0-9]*$` | `Wifi`, `WebInterface` |
| Arduino library name | `XeWeCore` (registered; never changes) | |

Banned spellings: `Xewe` followed by a letter (`XeweError`, `XeweCore`), `xewe os` (lowercase with a
space), `Xewe OS`. `xewe brand-lint` (xewe-os-tools) fails on them; every repo's CI runs it.

`XeWeOS` is the old 1.x library name. It appears only where the tools and the modules validator
reject or ignore it, and in notes about the 1.x libraries.

Who checks what:

| Rule | Checked by |
|---|---|
| Banned spellings | `xewe brand-lint` |
| Slug, id, folder | `xewe modules validate` (the tools' module registry) |
| `XEWE_MODULE_<SLUG>_*` names in a module `Config.h` | the modules repo's `tools/validate.py`, rule `config` |
| Everything else in the table | review |

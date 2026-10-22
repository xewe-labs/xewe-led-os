# Writing a module

A module in this project's `src/` uses the same `xewe::Module` API and contract as one from the
modules repository: its own `$group` in the console, its own NVS namespace, typed settings and the
generic `status`, `enable`, `disable` and `reset` commands. It needs no entry anywhere else, and
setup never touches it. This page also holds the naming reference for the whole ecosystem.

## Start from a sample

- [src/SimpleModule/](../src/SimpleModule/README.md) is the one to copy. It prints
  `hello <name>` every `period_s` seconds; its settings are `name` (string, up to 15 characters,
  default `world`) and `period_s` (1 to 3600, default 10), both asked at first boot. It has no
  compile time constants.
- [src/FullModule/](../src/FullModule/README.md) is the one to read. It overrides every hook of
  `xewe::Module` and shows settings `active`, `label`, `pin` and `level` (the level is asked at
  first boot), listeners, a timed tick and a prompt timeout. Its compile time constants are
  `TICK_MS`, `PRINT_TICKS` and `PROMPT_TIMEOUT_MS`.

| | SimpleModule | FullModule |
|---|---|---|
| Folder, class | `src/SimpleModule`, `SimpleModule` | `src/FullModule`, `FullModule` |
| Id, console command | `simple_module`, `$simple_module` | `full_module`, `$full_module` |
| Define prefix | `XEWE_SIMPLE_MODULE_` | `XEWE_FULL_MODULE_` |
| Sketch line | `SimpleModule simple_module(os);` | `FullModule full_module(os);` |

To make your own: copy the folder, rename the folder, the files, the class, the id and the prefix
of its `Config.h` together, and change the sketch lines. Delete a sample by deleting its folder
and its lines in `xewe-os.ino`.

## Folder and names

One brand word, every form derived from it.

- **Prose:** **XeWe OS** (product), **XeWe** (brand): "XeWe OS firmware", "the XeWe tools".
- **Repos, packages, folders, files, CLI, env home:** lowercase `xewe`, kebab `xewe-os-*`
  (`xewe-os-core`, `xewe.toml`, `xewe`, `~/.xewe-os`).
- **C++ namespace:** `xewe::` (`xewe::Module`, `xewe::str::parse_hex_color`).
- **C++ and Python type names:** `XeWe` prefix (`XeWeCore`, `XeWeOs`, `XeWeModules`,
  `XeWeError`, `XeWeContext`).
- **Macros, build defines, environment variables of the core and tools:** `XEWE_*`
  (`XEWE_TESTING`, `XEWE_HOME`, `XEWE_CORE_DEVICE_NAME`).
- **The core's configuration values:** `XEWE_CORE_<PARAM>` (`XEWE_CORE_SERIAL_BAUD`).
- **A modules-repo module's compile time values:** `XEWE_<SLUG>_<PARAM>` (`XEWE_WIFI_DEBUG`).
- **Values of a project-local module** (`src/<Folder>/Config.h`): `XEWE_<ID>_<PARAM>`
  (`XEWE_SIMPLE_MODULE_PERIOD_S`, `XEWE_FULL_MODULE_LEVEL`).
- **Module slug** (modules repo folder, `--modules` names): lowercase kebab,
  `^[a-z0-9][a-z0-9-]*$` (`wifi`, `web-interface`).
- **Module id** (CLI group and NVS namespace): lowercase snake, `^[a-z][a-z0-9_]*$`, at most
  15 characters, never branded (`$wifi`, `$full_module`).
- **Module folder and class:** PascalCase, `^[A-Z][A-Za-z0-9]*$` (`Wifi`, `WebInterface`).
- **Arduino library name:** `XeWeCore` (registered; never changes).

Further rules:

- `<SLUG>` is the module slug in upper case with `-` written as `_` (`web-interface` becomes
  `WEB_INTERFACE`). The core's slug is `CORE`.
- For a project-local module `<ID>` is the module id in upper case. `<PARAM>` of a runtime default
  or first-boot value is its settings key in upper case: key `period_s` gives
  `XEWE_SIMPLE_MODULE_PERIOD_S`. `<PARAM>` of a compile time constant is any upper case name.
- The id is both the console group (`$simple_module`) and the NVS namespace, so it is at most
  15 characters. Settings keys and NVS keys are 1 to 15 characters and are never renamed once
  stored.
- `<PARAM>` never ends in `_PROMPT`; that suffix marks a value the first boot asks for.
- One module's prefix is never the start of another module's prefix.
- A command name is not empty and holds no whitespace.
- Banned spellings: `Xewe` followed by a letter, the product name written with a lowercase
  `w` and a lowercase `os`, and `OS` after a lowercase-`w` `Xewe`. `xewe brand-lint` fails on them
  and every repository's CI runs it.
- `XeWeOS` is the 1.x library name. It appears only where the tools and the modules validator
  reject or ignore it, and in notes about the 1.x libraries.

Who checks what:

| Rule | Checked by |
|---|---|
| Banned spellings | `xewe brand-lint` |
| `XEWE_<ID>_*` names in a project-local `Config.h` | review (the file is edited in place) |
| Slug, id, folder | `xewe modules validate` |
| `XEWE_<SLUG>_*` names in a modules-repo `Config.h` | the modules repo validator, rule `config` |
| Everything else in the table | review |

## The module class and hooks

The constructor is `Module(Os& os, id, name, description, requires_init_setup, can_be_disabled,
has_cli_commands)`. Name the constructor's Os parameter `host`, never `os`, so it does not hide
the member `os`; command handlers capture `[this]` only, never `[&]`. A module is neither
copyable nor movable: it is a global, never a vector element. Never write `cli(`, because the
Arduino core defines `cli` as a macro. `loop()` must not block, and a public function of a module
that can be disabled starts with `if (is_disabled()) return;`. Override only the hooks you need:

| Hook | What it is for |
|---|---|
| `begin_routines_required()` | every boot the module is enabled, first |
| `begin_routines_init()` | first boot only, until init setup completes (`requires_init_setup`) |
| `begin_routines_regular()` | every boot after init setup has completed |
| `begin_routines_common()` | every boot the module is enabled, last |
| `loop()` | called every iteration while enabled; must not block |
| `enable()`, `disable()`, `reset()` | change state; all three restart the device by default |
| `status()` | the status text; the core adds one line per settings row |
| `settings()` | the settings table; the core adds `set`, `get`, `schema` |
| `schema_extra()` | extra rows for the `schema` output |
| `on_setting_changed()` | called when a setting changes |
| `register_command()` | adds a command to the module's `$<id>` group |
| `add_requirement()` | declares that another module is needed |

A disabled module's `disable` wipes its NVS namespace, and its dependents'. The full API is the
core's Module README (`src/XeWeCore/Module/README.md` in xewe-os-core).

## Settings rows and first boot

A module declares plain persistent settings as one `static constexpr` table returned from
`settings()`. Each row binds a member of the module and has a key (1 to 15 characters), bounds or
a maximum length, a default and a doc string. The core loads table defaults, then NVS, and
provides `$<id> set`, `get` and `schema`. A row can be flagged `SECRET` (never printed) or
`RESTART` (takes effect after `$system restart`). Row defaults come from the module's `Config.h`.

A value the first boot asks for is a default runtime value with a `_PROMPT` marker. The module
asks only under `#ifdef <NAME>_PROMPT`, showing `<NAME>` as the suggested answer; without the
marker the value counts as pre-answered and is stored without a question. A definition earlier in
the build (a `--define`, a dotenv value, a line in the project's `Config.h`) leaves `_PROMPT`
undefined, so the question is not asked. Prompts must be bounded (a timeout and a retry count) so
an unattended device never waits forever.

## Config.h of a module

A project-local module keeps its values in `src/<Folder>/Config.h`, in the module form of the
core's [doc/config.md](https://github.com/xewe-labs/xewe-os-core/blob/main/doc/config.md). The
file is pure preprocessor: it includes nothing and declares no code. After the header (three
SPDX lines, the path, one multi-line block comment, `#pragma once`) come two sections in this
order, each between its start and end marker lines, exactly as written:

```c
/* ---- start default runtime values ---- */

/* Greeting name (simple_module/name), string literal, up to 15 characters; asked at first boot */
#ifndef XEWE_SIMPLE_MODULE_NAME
#define XEWE_SIMPLE_MODULE_NAME "world"
#define XEWE_SIMPLE_MODULE_NAME_PROMPT 1
#endif

/* ---- end default runtime values ---- */

/* ---- start compile time defined constants ---- */

/* ---- end compile time defined constants ---- */
```

- Every value is one single-line block comment (what it is, its unit and range) followed by an
  `#ifndef` group of its own name. The `#ifndef` makes it a fallback: a `--define` or a line in
  the project's `Config.h` wins.
- Text is written in `/* */` and commented-out code in `//`.
- A section may be empty, as above, but its markers stay and are never reworded.
- `<NAME>_PROMPT` is defined as `1` inside the `#ifndef` of a value the first boot asks for,
  and nowhere else.
- A module defines names with its prefix only in its `Config.h`.
- You edit the file in place. `xewe modules generate` never touches a project-local module, so
  its values get no block in the project's `Config.h`.

## Registering in the sketch

In `xewe-os.ino`, `Config.h` is included first, then `<XeWeCore.h>`, then `XeWeOs os({...});` is
declared before any module, then `src/Modules.h` (generated) is included. Add your module after
that:

```cpp
#include "src/SimpleModule/SimpleModule.h"
SimpleModule simple_module(os);
```

Declare modules in dependency order. A module that needs another takes it as a constructor
reference, calls `add_requirement`, and is declared after it.

## Listeners

`xewe::ListenerSet<Iface, N = 4>` is the core's module-to-module change notification. A module
that has something to announce defines a listener interface and owns a set; other modules, or the
sketch, call `add` and `remove`, and the owner calls `notify`. Every event carries a
`const void* origin`: whoever caused the change passes itself (`this`, the CLI passes `nullptr`),
and a listener that also sets the value skips events whose origin is itself. The set has fixed
capacity, no heap and no locking, so add and notify from the main loop only. FullModule is the
example: it owns a listener set for its level, and the sketch adds a level listener next to the
`FullModule full_module(os);` line.

## When a module belongs in the modules repo instead

Keep a module in `src/` while only this firmware needs it. A module that other firmwares need
moves to [xewe-os-modules](https://github.com/xewe-labs/xewe-os-modules), as `modules/<slug>/`,
where it follows that repository's
[contract](https://github.com/xewe-labs/xewe-os-modules/blob/main/doc/contract.md) and is built
and tested through an `xewe-os` checkout. There its values get a block in each project's
`Config.h` under the prefix `XEWE_<SLUG>_`, and `xewe modules validate` checks its slug, id and
folder.

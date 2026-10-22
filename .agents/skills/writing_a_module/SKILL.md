---
name: writing_a_module
description: >
  Write a project-local module in src/: copy SimpleModule, rename folder, class, id and define
  prefix, write its Config.h, register it in xewe-os.ino, test, format and build. Triggers: "new
  module", "my own module", "add a module", "copy SimpleModule", "module id", "settings row",
  "first boot prompt", "module Config.h", "listener".
---

# writing_a_module

Read `doc/writing_a_module.md` first; it also holds the naming rules.

## When to use

- You add, rename or change a module in this project's `src/`.
- A reusable module for other firmwares belongs in xewe-os-modules instead; say so and stop.

## Procedure

1. Read `doc/writing_a_module.md`, then `src/SimpleModule/README.md` (and `src/FullModule/README.md`
   if you need a hook it shows).
2. Pick names: folder and class PascalCase, id snake case of at most 15 characters, define
   prefix `XEWE_<ID>_`. Check them against the naming section of the doc.
3. Copy `src/SimpleModule/` to `src/<Name>/` and rename the files, the class, the id string and
   every `XEWE_SIMPLE_MODULE_` define together.
4. Write `src/<Name>/Config.h` in the module form: the two section markers (an empty section
   keeps its markers), one `#ifndef` group per value, `_PROMPT` only on a value asked at first
   boot.
5. Keep the constructor parameter named `host`, capture `[this]` only, never write `cli(`, keep
   `loop()` non-blocking.
6. Register it in `xewe-os.ino`: include `src/<Name>/<Name>.h` and declare `<Name> <id>(os);`
   after the generated modules, in dependency order.
7. Add tests under `tests/unit/` (source facts) and `tests/board/` (console).
8. Run:
   `build/tools/.venv/bin/python -m xewe format`
   `build/tools/.venv/bin/python -m xewe test --unit-only`
   `build/tools/.venv/bin/python -m xewe build --chip c3` (or `--all-chips`)

## Checks before reporting

- Format, unit tests and the build are green, with 0 warnings from the new code.
- `xewe brand-lint .` is clean.
- If the id, a command or a first-boot prompt changed, `doc/writing_a_module.md` and the
  sample README say the same (X-09).

## Never

- Edit `src/Modules.h` or `build/` (X-02); `xewe modules generate` never touches a
  project-local module.
- Use an id longer than 15 characters or a branded id.
- Run `xewe modules select` or flash a board unless asked.
- Commit or push without being asked (P-09).
- Read, print or copy a dotenv or key file (X-10).

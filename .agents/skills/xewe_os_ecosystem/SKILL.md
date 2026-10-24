---
name: xewe_os_ecosystem
description: >
  Orient in the XeWe OS repository set: which of tools, core, modules, template and product a
  change belongs to, how xewe.toml pins refs, what is committed and what is generated, how
  Config.h and the override ladder work. Triggers: "which repo", "where does this belong",
  "xewe.toml", "pin", "ref", "version", "tag", "Config.h", "generated file", "setup.sh",
  "release", "ecosystem".
---

# xewe_os_ecosystem

Read `doc/XEWE_OS_ECOSYSTEM.md` first. Paths are relative to the project root.

## When to use

- You must decide which repository a change belongs to.
- You touch `xewe.toml`, `Config.h`, `setup.sh`, versions, tags or a release.
- You are unsure whether a file is committed or generated.

## Procedure

1. Read `doc/XEWE_OS_ECOSYSTEM.md`: the five repositories, committed vs generated, Config.h.
2. Place the change: host tooling goes to xewe-os-tools, library code every firmware needs to
   xewe-os-core, a feature some firmwares need to xewe-os-modules, this firmware's own code to
   this repository (`src/`, `xewe-led-os.ino`, `Config.h`, `release_matrix.csv`).
3. Look at the pins without changing anything:
   `build/tools/.venv/bin/python -m xewe manifest show` (`!` marks drift from `xewe.toml`).
4. For a value, find its rung: `--define`, then dotenv `INJECTED_FROM_DOTENV`, then the
   project's `Config.h`, then the module's own default. Change the lowest rung that is enough.
5. For a generated file (`build/`, `src/Modules.h`, `run.sh`, `release_card.md`), change its
   source (`xewe.toml`, the modules repository, the core) and regenerate with `./setup.sh`.
6. To build against working trees of other repositories, use a copy of the template (a
   harness) with `XEWE_CORE_SOURCE` and `XEWE_MODULES_SOURCE` set, never this tree.

## Checks before reporting

- `build/tools/.venv/bin/python -m xewe build --chip c3` passes after a source change.
- `xewe brand-lint .` is clean when you wrote prose.
- Say what you ran and label anything unverified (R-06).

## Never

- Commit, push, tag or release (R-10); the owner runs releases.
- Edit or commit generated files (`.agents/skills/doc/references/GENERATED_FILES.md`).
- Move refs by hand-editing a SHA; refs are pinned to tags (`.agents/skills/doc/references/COMMANDS.md`). Use `xewe manifest update` when asked.
- Read, print or copy a dotenv or key file (R-04).
- Change `[modules] selected` unless asked; it is this product's committed choice (`.agents/skills/doc/references/COMMANDS.md`).

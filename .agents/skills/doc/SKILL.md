---
name: doc
description: >
  What an agent needs to know about xewe-led-os that the code does not show: the commands to
  run and the ones never to run (build, test, flash, release), working with no board attached
  (XEWE_NO_BOARD, "compiled, not run"), building against local core, modules and tools
  checkouts through a harness copy, generated files that are never edited or committed
  (build/, src/Modules.h, run.sh), code rules for the sketch, Config.h, release_matrix.csv and
  the four product modules (SchedulePage, HomeAssistant, HomeKit, Alexa) and their order,
  naming and brand spelling, the repositories and where a change belongs, the XeWeCore
  pitfalls, the module contract, the tools, versioning and tags, competing ESP32 tools, and
  the decisions log. Use it before a build, test, flash or release, before touching Config.h,
  xewe.toml, release_matrix.csv, setup.sh or a product module, before naming anything, and
  when asking why something is the way it is.
---

# doc

This is the index of the project's agent knowledge. Open only the file whose row matches the
task. Each row names one file under `references/`, and every file there has a row.

| File | Read it when |
|---|---|
| `references/COMMANDS.md` | building, testing, flashing, bumping refs, or asked about a release |
| `references/NO_BOARD.md` | no board is attached, or a command prints `compiled, not run` |
| `references/LOCAL_SOURCES.md` | building against local core, modules or tools checkouts, or setting up a harness |
| `references/GENERATED_FILES.md` | about to edit or commit anything under `build/`, `src/Modules.h`, `run.sh`, or a dotenv file |
| `references/CODE_RULES.md` | editing the sketch, `Config.h`, `release_matrix.csv`, a product module, a comment, or the docs |
| `references/NAMING.md` | naming a macro, module, type, repo or file, or writing XeWe in prose |
| `references/REPOS.md` | deciding which repository a change belongs in |
| `references/CORE.md` | a sketch or module here touches XeWeCore, or a build fails on includes |
| `references/MODULE_CONTRACT.md` | writing or reviewing a module |
| `references/TOOLS.md` | using `xewe` subcommands or changing what writes `xewe.toml` |
| `references/VERSIONING.md` | choosing a version, tag or release path in any of the repositories |
| `references/LANDSCAPE.md` | adding a feature another ESP32 tool may already have, or claiming something is unique |
| `references/DECISIONS.md` | changing anything a past decision covers |

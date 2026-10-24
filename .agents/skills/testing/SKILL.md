---
name: testing
description: >
  Run and read the project's tests: unit tests, board tests with and without a board, the
  compiled-not-run result, dotenv keys by name, and CI. Triggers: "run the tests", "xewe test",
  "unit tests", "board tests", "no board", "compiled, not run", "XEWE_NO_BOARD", "CI failed",
  "pytest".
---

# testing

Read `doc/TESTING.md` first. Run the tools as `build/tools/.venv/bin/python -m xewe` (below:
`xewe`) from the project root, after `./setup.sh`.

## When to use

- You changed code and must show it still works.
- A test, a CI job or a release card step failed and you must read why.

## Procedure

1. Read `doc/TESTING.md`.
2. For a compile-only session run `export XEWE_NO_BOARD=1` first: no port is opened.
3. Unit tests, no board needed: `xewe test --unit-only` (add `-v`, or `-- -k NAME` for pytest).
4. Board tests: `xewe test --chip c3 -- -m board`. With no board the line
   `compiled, not run: no board attached (...)` and exit 0 are the expected result.
5. To prove the board tests compile without touching a board:
   `xewe test --chip c3 --no-board -- -m board`.
6. Use `--require-board` only when a board is attached and the user asked; a missing board is
   then exit 4.
7. For a CI failure, read the failing job of `ci.yml` (it calls the tools workflows) and rerun
   the same command locally: `xewe test --unit-only`, then `xewe build --all-chips`.

## Checks before reporting

- State exactly which commands you ran and their exit codes.
- Report "compiled, not run" as exactly that, never as passed on hardware.
- Name any test you skipped or could not run.

## Never

- Pass `--erase`, or flash a board, unless the user asks.
- Open, print or copy a dotenv or key file; the tools read it, and the docs name its keys only.
- Report a skipped or failed step as done (R-06).
- Edit generated files (`.agents/skills/doc/references/GENERATED_FILES.md`) or commit anything (R-10).

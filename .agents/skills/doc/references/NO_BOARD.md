# No board

Working without a board is normal. `xewe flash`, `xewe run` and `xewe test` compile, then
print `compiled, not run: no board attached (...)` and exit 0. Search the output for that line,
because it is not an error.

For a compile-only session run `export XEWE_NO_BOARD=1` first. With it set, no port is
listed, probed or opened, even when a board is plugged in. `xewe test` still compiles, and
`xewe flash`, `xewe run` and `xewe serial` exit 4. `xewe test --no-board` sets the same switch
for one run.

Never pass `--require-board` or `--erase`, and never flash a board, unless the user asks.
`--require-board` turns the compile-only result into exit 4, and `--erase` wipes stored
settings.

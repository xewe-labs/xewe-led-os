# tests/

`unit/` holds pytest files that check source facts on the host, with no board and no build. The `host_*.cpp` and `test_schedule_page.cpp` files there are compiled with g++ and run the product modules' pure headers. `board/` holds pytest files that run against a flashed board over serial. `xewe test` runs both.

Each product module has one file in each folder, named after it, such as `unit/test_homekit.py` and `board/test_homekit.py`.

```sh
build/tools/.venv/bin/python -m xewe test --unit-only
build/tools/.venv/bin/python -m xewe test --chip c3 -- -m board
```

Without a board, the board tests are reported as `compiled, not run` and the command exits 0. Tests that need network access to the board, a broker, a phone or an Echo are skipped unless their `XEWE_TEST_*` key is set, and each test file's docstring names its keys. [doc/TESTING.md](../doc/TESTING.md) has the flags and the dotenv keys.

[MANUAL.md](MANUAL.md) is the checklist for what no script can test: pairing with the Home app, voice commands to an Echo, a real Home Assistant, and power cuts.

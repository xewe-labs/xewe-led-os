# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/tests/board/test_firmware.py
"""Board tests of the whole firmware. Run from the project: `build/tools/.venv/bin/python -m xewe test`.

Hardware preconditions: a provisioned board (first boot done). The modules' own board tests
(build/modules/tests/<slug>/board/) cover each module's commands.
"""

import re


def test_compiles(compiled):
    # the project firmware (every selected module plus the product's modules) for the session chip
    assert compiled.is_file()


def test_system_lists_modules(serial):
    serial.send("$system status")
    lines = serial.collect(limit=10)
    text = "\n".join(lines)
    for name in ("Led", "Led Web", "Wifi", "Scheduler", "Buttons"):
        assert re.search(rf"\b{re.escape(name)}\b", text), (
            f"{name} missing from $system status"
        )


def test_led_groups_answer(serial):
    for group in ("led", "led_web"):
        serial.command(
            f"${group} status", expect=r"module (enabled|disabled)", timeout=5
        )

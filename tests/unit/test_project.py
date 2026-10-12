# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/tests/unit/test_project.py
"""Unit tests of the project files: the sketch, Config.h, xewe.toml and release_matrix.csv.
Source facts only, no board and no build."""

import re
import tomllib

import pytest

GENERATED_MARK = "(xewe modules generate) ----"
MATRIX_HEADER = "CHIP,XEWE_MODULE_LED_PIN_DATA,XEWE_MODULE_LED_PIN_CLOCK,_BUILD_NOTES"
# The Led Dock rows of the pin matrix: (chip, data, clock).
LED_DOCK = {
    ("c3", "0", "9"),
    ("c6", "14", "2"),
    ("s3", "9", "3"),
    ("c3", "4", "3"),
    ("c6", "20", "19"),
    ("s3", "13", "12"),
}


@pytest.mark.unit
def test_sketch_declares_os_before_modules(project_root):
    ino = (project_root / "xewe-led-os.ino").read_text()
    os_at = ino.index("XeWeOs os(")
    modules_at = ino.index('#include "src/Modules.h"')
    assert os_at < modules_at
    assert "#include <XeWeCore.h>" in ino
    assert '#include "Config.h"' in ino


@pytest.mark.unit
def test_config_product_values_come_before_generated_blocks(project_root):
    text = (project_root / "Config.h").read_text()
    assert "#include <XeWeBuildInfo.h>" in text
    first_block = text.find(GENERATED_MARK)
    for macro, value in (
        ("XEWE_MODULE_LED_LISTENERS", "8"),
        ("XEWE_MODULE_LED_NUM_LEDS_MAX", "600"),
    ):
        m = re.search(rf"^#define {macro} (\S+)$", text, re.MULTILINE)
        assert m, f"{macro} not defined"
        assert m.group(1) == value
        if first_block >= 0:
            assert m.start() < first_block, (
                f"{macro} must come before the generated module blocks"
            )


@pytest.mark.unit
def test_manifest_selects_the_led_modules(project_root):
    lock = tomllib.loads((project_root / "xewe.toml").read_text())
    assert lock["project"]["name"] == "xewe-led-os"
    selected = lock["modules"]["selected"]
    for slug in (
        "led",
        "led-web",
        "wifi",
        "web-interface",
        "time",
        "scheduler",
        "buttons",
    ):
        assert slug in selected
    for name in ("core", "modules", "tools"):
        assert lock[name]["ref"] != "latest"
    # FastLED and WebSockets come from the modules' libraries.toml, at the refs the modules pin
    assert "FastLED" not in lock["libraries"]
    assert "WebSockets" not in lock["libraries"]


@pytest.mark.unit
def test_release_matrix(project_root):
    lines = (project_root / "release_matrix.csv").read_text().splitlines()
    assert lines[0] == MATRIX_HEADER
    rows = [line.split(",") for line in lines[1:] if line]
    assert len(rows) == 43
    assert all(len(r) == 4 for r in rows)
    assert {r[0] for r in rows} == {"c3", "c6", "s3"}
    assert all(r[1].isdigit() and r[2].isdigit() for r in rows)
    keys = [tuple(r[:3]) for r in rows]
    assert len(set(keys)) == len(keys), "two rows build the same chip and pins"
    assert LED_DOCK <= set(keys)


@pytest.mark.unit
def test_release_matrix_parses_with_the_tools(project_root):
    release = pytest.importorskip("xewe.build.release")
    matrix = release.parse_matrix((project_root / "release_matrix.csv").read_text())
    assert matrix.map_header == [
        "CHIP",
        "XEWE_MODULE_LED_PIN_DATA",
        "XEWE_MODULE_LED_PIN_CLOCK",
    ]
    assert len(matrix.rows) == 43
    for row in matrix.rows:
        assert set(row.defines) == {
            "XEWE_MODULE_LED_PIN_DATA",
            "XEWE_MODULE_LED_PIN_CLOCK",
        }
        assert row.folders == [
            row.chip,
            row.defines["XEWE_MODULE_LED_PIN_DATA"],
            row.defines["XEWE_MODULE_LED_PIN_CLOCK"],
        ]

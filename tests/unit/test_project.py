# SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
# SPDX-License-Identifier: GPL-3.0-only
# xewe-led-os/tests/unit/test_project.py
"""Unit tests of the project files: the sketch, Config.h, xewe.toml and release_matrix.csv.
Source facts only, no board and no build."""

import re
import tomllib

import pytest

MATRIX_HEADER = "CHIP,XEWE_LED_PIN_DATA,XEWE_LED_PIN_CLOCK,_BUILD_NOTES"
# The Led Dock rows of the pin matrix: (chip, data, clock).
LED_DOCK = {
    ("c3", "0", "9"),
    ("c6", "14", "2"),
    ("s3", "9", "3"),
    ("c3", "4", "3"),
    ("c6", "20", "19"),
    ("s3", "13", "12"),
}
# The SuperMini rows: the board's built-in addressable LED (WS2812B) on the data pin.
SUPERMINI = {("c6", "8", "21"), ("s3", "48", "12")}
# The 3.0.0 release builds one image per row: 43 images, per chip as below.
MATRIX_ROWS = 43
MATRIX_ROWS_PER_CHIP = {"c3": 7, "c6": 14, "s3": 22}


@pytest.mark.unit
def test_sketch_declares_os_before_modules(project_root):
    ino = (project_root / "xewe-led-os.ino").read_text()
    os_at = ino.index("XeWeOs os(")
    modules_at = ino.index('#include "src/Modules.h"')
    assert os_at < modules_at
    assert "#include <XeWeCore.h>" in ino
    assert '#include "Config.h"' in ino


# The product's modules (src/), declared after the generated ones in this order. The web
# interface's catch-all needs no module of its own (web-interface 3.1.0 installs it).
PRODUCT_MODULES = (
    ("SchedulePage", "schedule_page"),
    ("HomeAssistant", "homeassistant"),
    ("HomeKit", "homekit"),
    ("Alexa", "alexa"),
)


@pytest.mark.unit
def test_sketch_declares_the_product_modules_in_order(project_root):
    ino = (project_root / "xewe-led-os.ino").read_text()
    code = re.sub(r"//[^\n]*", "", ino)
    modules_at = code.index('#include "src/Modules.h"')
    found = re.findall(
        r'^#include "src/(\w+)/(\w+)\.h"\n(\w+) (\w+)\(', code, re.MULTILINE
    )
    assert [(cls, name) for _, _, cls, name in found] == list(PRODUCT_MODULES)
    for folder, header, cls, _ in found:
        assert folder == header == cls
        assert (project_root / "src" / folder / f"{header}.h").is_file()
    first = code.index(f'#include "src/{PRODUCT_MODULES[0][0]}/')
    assert modules_at < first
    product_dirs = sorted(
        p.name for p in (project_root / "src").iterdir() if p.is_dir()
    )
    assert product_dirs == sorted(cls for cls, _ in PRODUCT_MODULES)


@pytest.mark.unit
def test_config_has_no_include_and_the_led_override(project_root):
    text = (project_root / "Config.h").read_text()
    lines = text.splitlines()
    assert not [l for l in lines if re.match(r"\s*#\s*include\b", l)]
    hits = [i for i, l in enumerate(lines) if l == "#define XEWE_LED_LISTENERS 8"]
    assert len(hits) == 1, "XEWE_LED_LISTENERS 8 must be defined exactly once"
    assert (
        lines.index("/* ---- start led ---- */")
        < hits[0]
        < lines.index("/* ---- end led ---- */")
    )
    assert "XEWE_MODULE_" not in text


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
    assert all(lines[1:]), "blank line in the matrix"
    rows = [line.split(",") for line in lines[1:]]
    assert len(rows) == MATRIX_ROWS
    assert all(len(r) == 4 for r in rows)
    per_chip = {chip: sum(r[0] == chip for r in rows) for chip in MATRIX_ROWS_PER_CHIP}
    assert per_chip == MATRIX_ROWS_PER_CHIP
    assert sum(per_chip.values()) == MATRIX_ROWS
    assert all(r[1].isdigit() and r[2].isdigit() for r in rows)
    keys = [tuple(r[:3]) for r in rows]
    assert len(set(keys)) == len(keys), "two rows build the same chip and pins"
    notes = {tuple(r[:3]): r[3] for r in rows}
    assert {k for k, v in notes.items() if v.startswith("Led Dock")} == LED_DOCK
    assert {k for k, v in notes.items() if v.startswith("SuperMini")} == SUPERMINI
    assert all(v == "" for k, v in notes.items() if k not in LED_DOCK | SUPERMINI)


@pytest.mark.unit
def test_release_matrix_parses_with_the_tools(project_root):
    release = pytest.importorskip("xewe.build.release")
    matrix = release.parse_matrix((project_root / "release_matrix.csv").read_text())
    assert matrix.map_header == [
        "CHIP",
        "XEWE_LED_PIN_DATA",
        "XEWE_LED_PIN_CLOCK",
    ]
    assert len(matrix.rows) == MATRIX_ROWS
    for row in matrix.rows:
        assert set(row.defines) == {
            "XEWE_LED_PIN_DATA",
            "XEWE_LED_PIN_CLOCK",
        }
        assert row.folders == [
            row.chip,
            row.defines["XEWE_LED_PIN_DATA"],
            row.defines["XEWE_LED_PIN_CLOCK"],
        ]

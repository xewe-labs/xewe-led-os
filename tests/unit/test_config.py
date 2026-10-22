"""Unit tests of the project's Config.h (the core's doc/config.md): source facts only, no board and
no build. The tools append module blocks to this file and never rewrite it, so the template ships it
in the shape they expect."""

import re
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]  # the project: tests/unit/ -> tests/ -> root
START = re.compile(r"^/\* ---- start ([a-z0-9-]+) ---- \*/$")
SECTIONS = (
    "/* ---- start default runtime values ---- */",
    "/* ---- end default runtime values ---- */",
    "/* ---- start compile time defined constants ---- */",
    "/* ---- end compile time defined constants ---- */",
)
OVERRIDE = re.compile(r"^#define [A-Z0-9_]+ \S.*$")
SENTINEL = re.compile(r"^\s*#\s*define\s+\w+\s+INJECTED_FROM_DOTENV", re.M)


def _text():
    return (ROOT / "Config.h").read_text()


def _bar(marker):
    return "/*" + "*" * (len(marker) - 4) + "*/"


def _blocks(lines):
    """(slug, first line, last line) of every block, from its opening bar to its closing bar."""
    blocks = []
    for i, line in enumerate(lines):
        match = START.match(line)
        if match:
            slug = match.group(1)
            blocks.append((slug, i - 1, lines.index(f"/* ---- end {slug} ---- */") + 1))
    return blocks


@pytest.mark.unit
def test_pragma_once_and_no_include():
    lines = _text().splitlines()
    assert "#pragma once" in lines
    assert not [l for l in lines if re.match(r"\s*#\s*include\b", l)]


@pytest.mark.unit
def test_core_block_first_and_blocks_framed():
    lines = _text().splitlines()
    blocks = _blocks(lines)
    assert blocks and blocks[0][0] == "core"
    assert len({slug for slug, _, _ in blocks}) == len(blocks)
    for slug, first, last in blocks:
        start, end = f"/* ---- start {slug} ---- */", f"/* ---- end {slug} ---- */"
        block = lines[first : last + 1]
        assert block[:3] == [_bar(start), start, _bar(start)]
        assert block[-3:] == [_bar(end), end, _bar(end)]
        assert [l for l in block if l in SECTIONS] == list(SECTIONS)


@pytest.mark.unit
def test_every_generated_line_is_a_comment_or_an_override():
    lines = _text().splitlines()
    for _, first, last in _blocks(lines):
        for line in lines[first : last + 1]:
            assert (
                not line.strip()
                or (line.startswith("/*") and line.endswith("*/"))
                or line.startswith("// ")
                or OVERRIDE.match(line)  # an activated override
            ), line


@pytest.mark.unit
def test_active_lines_are_pragma_once_and_overrides_inside_blocks():
    lines = _text().splitlines()
    blocks = _blocks(lines)
    directives = [l for l in lines if re.match(r"\s*#", l)]
    assert [l for l in directives if not l.startswith("#define ")] == ["#pragma once"]
    for i, line in enumerate(lines):
        if not line.startswith("#define "):
            continue
        inside = [
            slug
            for slug, _, _ in blocks
            if lines.index(f"/* ---- start {slug} ---- */")
            < i
            < lines.index(f"/* ---- end {slug} ---- */")
        ]
        assert len(inside) == 1, f"override outside a block: {line}"
        prefix = "XEWE_" + inside[0].upper().replace("-", "_") + "_"
        assert line.split()[1].startswith(prefix), line


@pytest.mark.unit
def test_project_overrides():
    defines = {}
    for line in _text().splitlines():
        match = re.match(r"^#define ([A-Z0-9_]+) (\S.*)$", line)
        if match:
            defines[match.group(1)] = match.group(2)
    assert defines == {"XEWE_LED_LISTENERS": "8"}


@pytest.mark.unit
def test_no_prompt_build_macro_or_sentinel():
    text = _text()
    # only a module's own Config.h defines <NAME>_PROMPT
    assert "_PROMPT" not in text
    # build macros come from the generated XeWeBuildInfo.h, never from this file
    assert not re.search(r"\b(BUILD|PROJECT)_[A-Z0-9_]+", text)
    # a release image takes no value from a dotenv file
    assert not SENTINEL.search(text)

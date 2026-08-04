#!/usr/bin/env python3
"""modeconfig_layout.py — enforce a shallow, fixed layout for ModeConfig(...) tables.

At ColumnLimit:0 clang-format aligns a nested braced init list to its opening
'{' column, so a ModeConfig param table nested inside `: Mode(ModeConfig(id,
"name", {...}), params)` gets pushed ~40 columns to the right and `params)`
breaks onto its own line. That is ugly and unstable.

Rather than fight clang-format with config, this tool owns the table's layout
directly. It runs AFTER clang-format and rewrites each ModeConfig table into a
canonical shallow form:

    : Mode(ModeConfig(6, "Christmas Lights", {
        {"density", "Density", 1, 10, 1, 1, 'b'},
        {"speed", "Flicker", 0, 20, 5, 1, 'a'},
    }), params)

The head line (up to and including the table's '{') is left exactly as
clang-format produced it. Each row is placed one indent level (4 spaces) past
the head line's indent, the closing '}' aligns with the head line's indent, and
everything from '}' through the enclosing Mode(...) closing ')' is joined onto
the close line. Row *contents* are preserved byte-for-byte from clang-format —
only whitespace/indentation between rows is rewritten.

    modeconfig_layout.py --check [path ...]   # exit 1 if any table is off-layout
    modeconfig_layout.py --fix   [path ...]   # rewrite tables to canonical form

The scanner masks comments and string/char literals to spaces so bracket
scanning ignores punctuation inside them (e.g. the "ModeConfig():" debug
strings in Mode.cpp are not mistaken for calls).
"""

import re

from fmtlib import IDENT, mask

INDENT = "    "  # one indent level: rows sit at (head indent + INDENT)


def match_close(s, open_idx, open_ch, close_ch):
    depth = 0
    for i in range(open_idx, len(s)):
        if s[i] == open_ch:
            depth += 1
        elif s[i] == close_ch:
            depth -= 1
            if depth == 0:
                return i
    return -1


def find_tables(m):
    """Yield (mc_idx, popen, pclose, topen, tclose) for each ModeConfig call
    whose braced param table has at least one `{...}` row. `m` is masked text."""
    n = len(m)
    needle = "ModeConfig"
    start = 0
    while True:
        idx = m.find(needle, start)
        if idx == -1:
            return
        start = idx + len(needle)
        # Must be a standalone identifier followed by '('.
        before = m[idx - 1] if idx > 0 else " "
        if before in IDENT:
            continue
        p = idx + len(needle)
        while p < n and m[p] in " \t\r\n":
            p += 1
        if p >= n or m[p] != "(":
            continue
        pclose = match_close(m, p, "(", ")")
        if pclose == -1:
            continue
        topen = _first_toplevel_brace(m, p + 1, pclose)
        if topen == -1:
            continue
        tclose = match_close(m, topen, "{", "}")
        if tclose == -1 or tclose > pclose:
            continue
        if not _has_row(m, topen + 1, tclose):
            continue
        yield idx, p, pclose, topen, tclose


def _first_toplevel_brace(m, lo, hi):
    """Index of the first '{' at paren/bracket/angle-depth 0 in [lo, hi)."""
    depth = 0
    for i in range(lo, hi):
        c = m[i]
        if c in "([<":
            depth += 1
        elif c in ")]>":
            if depth > 0:
                depth -= 1
        elif c == "{" and depth == 0:
            return i
    return -1


def _has_row(m, lo, hi):
    """True if a '{' (a row) exists at brace-depth 1 within the table body."""
    depth = 0
    for i in range(lo, hi):
        if m[i] == "{":
            if depth == 0:
                return True
            depth += 1
        elif m[i] == "}":
            if depth > 0:
                depth -= 1
    return False


def _rows(text, m, topen, tclose):
    """Original text of each depth-1 `{...}` row in the table body, stripped."""
    rows = []
    depth = 0
    start = -1
    for i in range(topen + 1, tclose):
        c = m[i]
        if c == "{":
            if depth == 0:
                start = i
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0 and start != -1:
                rows.append(text[start:i + 1].strip())
                start = -1
    return rows


def _mode_close(m, mc_idx, pclose):
    """The ')' closing the Mode(...) that wraps this ModeConfig, or pclose if the
    call is not wrapped (so `params` etc. is joined onto the close line)."""
    j = mc_idx - 1
    while j >= 0 and m[j] in " \t\r\n":
        j -= 1
    if j < 0 or m[j] != "(":
        return pclose
    close = match_close(m, j, "(", ")")
    return close if close != -1 else pclose


def _head_indent(text, topen):
    """Leading whitespace of the line containing the table's opening '{'."""
    ls = text.rfind("\n", 0, topen) + 1
    k = ls
    while k < topen and text[k] in " \t":
        k += 1
    return text[ls:k]


def canonical(text):
    """Return `text` with every ModeConfig table rewritten to the shallow form."""
    m = mask(text)
    nl = "\r\n" if "\r\n" in text else "\n"
    edits = []
    for mc_idx, popen, pclose, topen, tclose in find_tables(m):
        base = _head_indent(text, topen)
        rows = _rows(text, m, topen, tclose)
        close_b = _mode_close(m, mc_idx, pclose)
        tail = re.sub(r"\s+", " ", text[tclose + 1:close_b + 1]).strip()
        new = "{" + nl
        for r in rows:
            new += base + INDENT + r + "," + nl
        new += base + "}" + tail
        edits.append((topen, close_b + 1, new))
    for start, end, rep in sorted(edits, reverse=True):
        text = text[:start] + rep + text[end:]
    return text


def process(text, path=None):
    return canonical(text)

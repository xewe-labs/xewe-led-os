#!/usr/bin/env python3
"""Enforce the canonical top-of-file layout for .h and .cpp sources.

Header (.h) order (see doc/standards.md sec.13):

    // SPDX-FileCopyrightText ...    # canonical GPL header, replacing any other
    // SPDX-License-Identifier ...   # license block (e.g. old PolyForm banner)
    // <path from repo root>        # rewritten to the file's real path
    #pragma once
                                    # 1 blank
    #include <third_party>          # angle-bracket includes, original order
    #include <...>
                                    # 1 blank (only if both groups exist)
    #include "project"              # quoted includes, original order
                                    # 2 blanks
    <first definition>

Source (.cpp) order:

    // SPDX-FileCopyrightText ...    # same canonical GPL header
    // SPDX-License-Identifier ...
    // <path from repo root>        # rewritten to the file's real path
                                    # 1 blank (no #pragma once in a .cpp)
    #include "Self.h"               # the matching header first
    #include "other project"        # remaining quoted project includes
                                    # 2 blanks
    <first definition>

    A .cpp must not carry third-party <...> includes: those belong in the
    matching .h. Any found are relocated to the bottom of the sibling header's
    angle-include group (deduped) and dropped from the .cpp. If no sibling .h
    exists the includes are left in place and a warning is emitted.

Runs AFTER clang-format (whose MaxEmptyLinesToKeep:1 would otherwise collapse the
two blank lines before the first definition). Idempotent.
"""

import os
import re

SPDX_LINES = [
    "// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)",
    "// SPDX-License-Identifier: GPL-3.0-only",
]

ANGLE_RE = re.compile(r"#include\s*<")
QUOTE_RE = re.compile(r'#include\s*"')
TARGET_RE = re.compile(r'#include\s*[<"]([^>"]+)[>"]')


def is_angle(inc):
    return bool(ANGLE_RE.search(inc))


def is_quote(inc):
    return bool(QUOTE_RE.search(inc))


def include_basename(inc):
    m = TARGET_RE.search(inc)
    return os.path.basename(m.group(1)) if m else ""


def norm(inc):
    return re.sub(r"\s+", "", inc)


def dedup(seq):
    seen, out = set(), []
    for x in seq:
        k = norm(x)
        if k in seen:
            continue
        seen.add(k)
        out.append(x)
    return out


def find_root(path):
    d = os.path.dirname(os.path.abspath(path))
    while True:
        if os.path.exists(os.path.join(d, ".git")) or \
           os.path.exists(os.path.join(d, ".clang-format")):
            return d
        parent = os.path.dirname(d)
        if parent == d:
            return None
        d = parent


def parse_preamble(lines):
    """Return (includes, body_start).

    Consumes the leading comment block (license + stale path comment),
    #pragma once, and #include lines. Everything from body_start on is left
    untouched. includes are returned in original order.
    """
    n, i = len(lines), 0

    # Phase 1: leading comment block (before any #pragma/#include/code).
    while i < n:
        s = lines[i].strip()
        if s == "":
            i += 1
            continue
        if s.startswith("//"):
            i += 1
            continue
        if s.startswith("/*"):
            if "*/" not in s:
                i += 1
                while i < n and "*/" not in lines[i]:
                    i += 1
                if i < n:
                    i += 1
            else:
                i += 1
            continue
        break

    # Phase 2: #pragma once + #include lines + interleaved blanks.
    includes = []
    while i < n:
        s = lines[i].strip()
        if s == "":
            i += 1
            continue
        if s == "#pragma once":
            i += 1
            continue
        if s.startswith("#include"):
            includes.append(s)
            i += 1
            continue
        break

    return includes, i


def process_header(text, rel_path):
    lines = text.split("\n")
    includes, body_start = parse_preamble(lines)

    # Any leading comment block (SPDX lines, an old PolyForm banner, a stale
    # path comment) is discarded and replaced by the canonical GPL header.
    angle = dedup([inc for inc in includes if is_angle(inc)])
    quote = dedup([inc for inc in includes if is_quote(inc)])

    out = list(SPDX_LINES)
    out.append("// " + rel_path)
    out.append("#pragma once")
    if angle or quote:
        out.append("")
        out.extend(angle)
        if angle and quote:
            out.append("")
        out.extend(quote)
    out.append("")
    out.append("")
    out.extend(lines[body_start:])

    return "\n".join(out)


def process_source(text, rel_path, drop_angle):
    lines = text.split("\n")
    includes, body_start = parse_preamble(lines)

    angle = dedup([inc for inc in includes if is_angle(inc)])
    quote = dedup([inc for inc in includes if is_quote(inc)])

    stem = os.path.splitext(os.path.basename(rel_path))[0]
    relh = stem + ".h"
    related = [q for q in quote if include_basename(q) == relh]
    others = [q for q in quote if include_basename(q) != relh]

    ordered = related + others
    if not drop_angle:
        # No sibling .h to relocate into: leave the angle includes in place
        # rather than silently dropping code (a warning is emitted in main).
        ordered = ordered + angle

    out = list(SPDX_LINES)
    out.append("// " + rel_path)
    if ordered:
        out.append("")
        out.extend(ordered)
    out.append("")
    out.append("")
    out.extend(lines[body_start:])

    return "\n".join(out)


def sibling_header(cpp_path):
    stem = os.path.splitext(os.path.basename(cpp_path))[0]
    h = os.path.join(os.path.dirname(os.path.abspath(cpp_path)), stem + ".h")
    return h if os.path.exists(h) else None


def rel_of(path, root, emit_path):
    if emit_path:
        return emit_path
    r = root or find_root(path)
    if not r:
        return None
    return os.path.relpath(os.path.abspath(path), r).replace(os.sep, "/")


def inject(h_text, angles, hrel):
    """Return `h_text` with third-party angle includes moved out of a sibling
    .cpp appended to its angle-include group (deduped) and the whole header
    re-rendered canonically. Pure: no file I/O."""
    lines = h_text.split("\n")
    includes, body_start = parse_preamble(lines)
    existing = {norm(i) for i in includes}
    to_add = [a for a in dedup(angles) if norm(a) not in existing]
    if to_add:
        merged = lines[:body_start] + to_add + lines[body_start:]
        return process_header("\n".join(merged), hrel)
    return process_header(h_text, hrel)

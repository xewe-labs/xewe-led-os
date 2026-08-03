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
import sys

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


def inject_into_header(header_path, angles, root, check, changed):
    """Append third-party angle includes moved out of a .cpp into its sibling
    .h (deduped), re-render the header canonically, and write it. Returns True
    if the header content changed."""
    with open(header_path, encoding="utf-8") as f:
        src = f.read()
    lines = src.split("\n")
    includes, body_start = parse_preamble(lines)
    existing = {norm(i) for i in includes}
    to_add = [a for a in dedup(angles) if norm(a) not in existing]

    hrel = rel_of(header_path, root, None)
    if hrel is None:
        print(f"cannot locate repo root for {header_path}", file=sys.stderr)
        return False

    if to_add:
        merged = lines[:body_start] + to_add + lines[body_start:]
        rendered = process_header("\n".join(merged), hrel)
    else:
        rendered = process_header(src, hrel)

    if rendered != src:
        if header_path not in changed:
            changed.append(header_path)
        if not check:
            with open(header_path, "w", encoding="utf-8", newline="\n") as f:
                f.write(rendered)
            print(f"header {header_path}")
        return True
    return False


def main(argv):
    args = argv[1:]
    root = None
    check = False
    emit_path = None
    files = []
    k = 0
    while k < len(args):
        a = args[k]
        if a == "--root":
            root = args[k + 1]
            k += 2
        elif a == "--emit-path":
            emit_path = args[k + 1]
            k += 2
        elif a in ("--check", "-c"):
            check = True
            k += 1
        else:
            files.append(a)
            k += 1

    if not files:
        print("usage: header_layout.py [--root DIR] [--emit-path REL] "
              "[--check] <file.h|file.cpp> ...", file=sys.stderr)
        return 2

    # Process .cpp before .h: a .cpp may relocate includes into its sibling
    # header, and we want that header re-normalized afterward.
    files.sort(key=lambda p: 0 if p.endswith(".cpp") else 1)

    changed = []
    for path in files:
        rel = rel_of(path, root, emit_path)
        if rel is None:
            print(f"cannot locate repo root for {path}", file=sys.stderr)
            return 2
        with open(path, encoding="utf-8") as f:
            src = f.read()

        if path.endswith(".cpp"):
            includes, _ = parse_preamble(src.split("\n"))
            angle = [inc for inc in includes if is_angle(inc)]
            drop = False
            if angle:
                sib = sibling_header(path)
                if sib:
                    inject_into_header(sib, angle, root, check, changed)
                    drop = True
                else:
                    print(f"warning: {path}: third-party <...> include(s) but "
                          f"no sibling .h to relocate them into; left in place",
                          file=sys.stderr)
            new = process_source(src, rel, drop_angle=drop)
        else:
            new = process_header(src, rel)

        if new != src:
            if path not in changed:
                changed.append(path)
            if not check:
                with open(path, "w", encoding="utf-8", newline="\n") as f:
                    f.write(new)
                print(f"header {path}")

    if check and changed:
        for p in changed:
            print(p)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

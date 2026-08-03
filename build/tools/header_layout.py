#!/usr/bin/env python3
"""Enforce the canonical top-of-file layout for .h headers.

Canonical order (see doc/standards.md sec.13):

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


def process(text, rel_path):
    lines = text.split("\n")
    includes, body_start = parse_preamble(lines)

    # Any leading comment block (SPDX lines, an old PolyForm banner, a stale
    # path comment) is discarded and replaced by the canonical GPL header.
    angle = [inc for inc in includes if re.search(r"#include\s*<", inc)]
    quote = [inc for inc in includes if re.search(r'#include\s*"', inc)]

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
              "[--check] <file.h> ...", file=sys.stderr)
        return 2

    changed = []
    for path in files:
        if emit_path:
            rel = emit_path
        else:
            r = root or find_root(path)
            if not r:
                print(f"cannot locate repo root for {path}", file=sys.stderr)
                return 2
            rel = os.path.relpath(os.path.abspath(path), r).replace(os.sep, "/")
        with open(path, encoding="utf-8") as f:
            src = f.read()
        new = process(src, rel)
        if new != src:
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

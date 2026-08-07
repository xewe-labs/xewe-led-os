#!/usr/bin/env python3
"""format.py — the format-pipeline orchestrator.

    format.py [--check] [--only STAGE[,STAGE...]] [path ...]

Loads every .h/.cpp under the given paths (default: <repo>/src) into an in-memory
{abspath: text} map, '\\n'-normalized, then runs the ordered STAGES over it. Each
stage mutates the map. clang-format runs on map text over stdin (fmtlib).

  * write mode (default): after all stages, flush every file whose text changed
    and print its path.
  * --check: identical run on the same in-memory map, but nothing is written;
    the changed paths are reported and the exit code is 1 if any changed. Check
    is exactly "write-to-memory + diff", so the two modes can never drift.

  * --only STAGE,...: run just those stages (single-stage debugging). A stage
    that needs a sibling file degrades gracefully when the sibling is not loaded.

Pipeline order (must match the historical bash pipeline):

    1 inline_move       headers   pre-clang    pair (h -> h + sibling cpp)
    2 method_order      sources   pre-clang    pair (cpp reads sibling h)
    3 clang_format      all       format       text
    4 header_layout     sources   post-clang   ctx  (cpp may inject into h)
    5 align_decls       headers   post-clang   text
    6 header_layout     headers   post-clang   ctx
    7 param_split       all       post-clang   text
    8 ctor_brace        all       post-clang   text
    9 open_break        all       post-clang   text
   10 dangling_close    all       post-clang   text
   11 modeconfig_layout sources   post-clang   text
"""

import os
import sys

import fmtlib
import align_decls
import body_layout
import header_layout
import inline_move
import method_order
import modeconfig_layout

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
STYLE = os.path.join(TOOLS_DIR, ".clang-format")


def sibling(path, ext):
    return os.path.splitext(path)[0] + ext


def rel_of(path, root):
    return os.path.relpath(path, root).replace(os.sep, "/")


# --------------------------------------------------------------------------- #
# Stage handlers. Each takes (files, scope, root) and mutates `files` in place.
# `scope` is the list of abspaths this stage applies to (already filtered).
# --------------------------------------------------------------------------- #
def _text_stage(fn):
    def run(files, scope, root):
        for p in scope:
            files[p] = fn(files[p], p)
    return run


def _clang(files, scope, root):
    for p in scope:
        files[p] = fmtlib.run_clang_format(files[p], p, STYLE)


def _inline_move(files, scope, root):
    for h in scope:  # headers
        cpp = sibling(h, ".cpp")
        if cpp in files:
            files[h], files[cpp] = inline_move.move(files[h], files[cpp])


def _method_order(files, scope, root):
    for cpp in scope:  # sources
        files[cpp] = method_order.reorder(files[cpp], files.get(sibling(cpp, ".h")))


def _header_sources(files, scope, root):
    for cpp in scope:  # sources
        src = files[cpp]
        includes, _ = header_layout.parse_preamble(src.split("\n"))
        angle = [inc for inc in includes if header_layout.is_angle(inc)]
        drop = False
        if angle:
            h = sibling(cpp, ".h")
            if h in files:
                files[h] = header_layout.inject(files[h], angle, rel_of(h, root))
                drop = True
        files[cpp] = header_layout.process_source(src, rel_of(cpp, root), drop)


def _header_headers(files, scope, root):
    for h in scope:  # headers
        files[h] = header_layout.process_header(files[h], rel_of(h, root))


# name -> (scope, handler). scope: "h" | "cpp" | "all".
STAGES = [
    ("inline_move",       "h",   _inline_move),
    ("method_order",      "cpp", _method_order),
    ("clang_format",      "all", _clang),
    ("header_sources",    "cpp", _header_sources),
    ("align_decls",       "h",   _text_stage(lambda t, p: align_decls.process(t))),
    ("header_headers",    "h",   _header_headers),
    ("param_split",       "all", _text_stage(body_layout.param_split)),
    ("ctor_brace",        "all", _text_stage(body_layout.ctor_brace)),
    ("open_break",        "all", _text_stage(body_layout.open_break)),
    ("dangling_close",    "all", _text_stage(body_layout.dangling_close)),
    ("modeconfig_layout", "cpp", _text_stage(modeconfig_layout.process)),
]


def _scope(files, kind):
    if kind == "h":
        return [p for p in files if p.endswith(".h")]
    if kind == "cpp":
        return [p for p in files if p.endswith(".cpp")]
    return list(files)


def run_pipeline(files, only=None):
    for name, kind, handler in STAGES:
        if only and name not in only:
            continue
        handler(files, sorted(_scope(files, kind)), _root_of(files))


def _root_of(files):
    """Repo root shared by the loaded files (the dir holding .git/.clang-format).
    All targets live under one repo, so any file resolves the same root."""
    any_path = next(iter(files))
    return header_layout.find_root(any_path) or os.path.dirname(any_path)


def main(argv):
    check = False
    only = None
    paths = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a in ("--check", "-c"):
            check = True
        elif a == "--only":
            only = set(argv[i + 1].split(","))
            i += 1
        elif a.startswith("--only="):
            only = set(a.split("=", 1)[1].split(","))
        else:
            paths.append(a)
        i += 1

    if not paths:
        repo = header_layout.find_root(TOOLS_DIR) or os.path.dirname(TOOLS_DIR)
        paths = [os.path.join(repo, "src")]

    targets = fmtlib.gather_targets(paths)
    if not targets:
        print("No files to format.")
        return 0

    original = {p: fmtlib.read(p) for p in targets}
    files = dict(original)

    run_pipeline(files, only)

    changed = [p for p in targets if files[p] != original[p]]

    if check:
        for p in changed:
            print(p)
        if changed:
            print("Would reformat %d file(s)." % len(changed))
            return 1
        print("All files already formatted.")
        return 0

    for p in changed:
        fmtlib.write(p, files[p])
        print(p)
    print("Formatted %d file(s)." % len(changed) if changed
          else "All files already formatted.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

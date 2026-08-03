#!/usr/bin/env python3
"""ctor_brace.py — put a constructor's opening body '{' on its own line,
separated from the member-initializer list.

    ctor_brace.py --check [path ...]   # report attached ctor braces, exit 1
    ctor_brace.py --fix   [path ...]   # move the brace to its own line

clang-format (BreakBeforeBraces: Attach) glues the body '{' onto the last member
initializer:

    Foo::Foo(int x)
        : a_(x)
        , b_(0) {          ->      : a_(x)
        ...                        , b_(0)
                                   {
                                       ...

Only out-of-line DEFINITIONS whose signature carries a member-init list (a
top-level ':' between the parameter ')' and the body '{') are treated as
constructors; every other function keeps clang-format's attached brace. The
brace is moved to the indent of the definition's first line. Regular function
bodies, calls, and aggregate initializers are untouched.

Runs AFTER clang-format and is idempotent. Text-based (no libclang): comments
and string/char literals are masked to spaces.
"""

import os
import sys

IDENT = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"


def mask(text):
    """Blank comment/literal *contents* (newlines preserved)."""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            j = i
            while j < n and text[j] != "\n":
                out[j] = " "
                j += 1
            i = j
        elif c == "/" and i + 1 < n and text[i + 1] == "*":
            out[i] = out[i + 1] = " "
            j = i + 2
            while j < n and not (text[j] == "*" and j + 1 < n and text[j + 1] == "/"):
                if text[j] != "\n":
                    out[j] = " "
                j += 1
            if j < n:
                out[j] = " "
                if j + 1 < n:
                    out[j + 1] = " "
                j += 2
            i = j
        elif c == '"' or c == "'":
            quote = c
            j = i + 1
            while j < n:
                if text[j] == "\\":
                    if j + 1 < n and text[j + 1] != "\n":
                        out[j + 1] = " "
                    out[j] = " "
                    j += 2
                    continue
                if text[j] == quote:
                    break
                if text[j] != "\n":
                    out[j] = " "
                j += 1
            i = j + 1
        else:
            i += 1
    return "".join(out)


def match_paren(s, open_idx):
    depth = 0
    for i in range(open_idx, len(s)):
        if s[i] == "(":
            depth += 1
        elif s[i] == ")":
            depth -= 1
            if depth == 0:
                return i
    return -1


def match_brace(s, open_idx):
    depth = 0
    for i in range(open_idx, len(s)):
        if s[i] == "{":
            depth += 1
        elif s[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    return -1


def find_body_open(m, start):
    """First '{' at paren/bracket/angle-depth 0; -1 if a ';' comes first."""
    depth = 0
    for i in range(start, len(m)):
        c = m[i]
        if c in "([<":
            depth += 1
        elif c in ")]>":
            if depth > 0:
                depth -= 1
        elif c == "{" and depth == 0:
            return i
        elif c == ";" and depth == 0:
            return -1
    return -1


def has_init_list(m, lo, hi):
    """True if a top-level ':' (member-init list, not '::') appears in [lo, hi)."""
    depth = 0
    for i in range(lo, hi):
        c = m[i]
        if c in "([{<":
            depth += 1
        elif c in ")]}>":
            if depth > 0:
                depth -= 1
        elif c == ":" and depth == 0:
            prev = m[i - 1] if i > 0 else " "
            nxt = m[i + 1] if i + 1 < len(m) else " "
            if prev != ":" and nxt != ":":
                return True
    return False


def ident_before(m, idx):
    j = idx
    while j > 0 and m[j - 1] in " \t\r\n":
        j -= 1
    end = j
    while j > 0 and m[j - 1] in IDENT:
        j -= 1
    return m[j:end]


def line_start(text, idx):
    return text.rfind("\n", 0, idx) + 1


def is_preproc_line(text, idx):
    ls = line_start(text, idx)
    j = ls
    while j < len(text) and text[j] in " \t":
        j += 1
    return j < len(text) and text[j] == "#"


def indent_of_line(text, idx):
    ls = line_start(text, idx)
    j = ls
    while j < len(text) and text[j] in " \t":
        j += 1
    return text[ls:j]


def compute_edits(text):
    """Return [(cut_start, brace_idx, replacement)] moving attached ctor braces."""
    m = mask(text)
    n = len(m)
    nl = "\r\n" if "\r\n" in text else "\n"
    edits = []
    i = depth = 0
    while i < n:
        c = m[i]
        if c == "{":
            depth += 1
            i += 1
            continue
        if c == "}":
            if depth > 0:
                depth -= 1
            i += 1
            continue
        if depth == 0 and c == "(":
            if not ident_before(m, i) or is_preproc_line(text, i):
                i += 1
                continue
            pclose = match_paren(m, i)
            if pclose == -1:
                i += 1
                continue
            bopen = find_body_open(m, pclose + 1)
            if bopen == -1:
                i = pclose + 1
                continue
            if has_init_list(m, pclose + 1, bopen):
                ls = line_start(text, bopen)
                if text[ls:bopen].strip() != "":  # brace attached to init line
                    cut = bopen
                    while cut > 0 and text[cut - 1] in " \t":
                        cut -= 1
                    indent = indent_of_line(text, i)
                    edits.append((cut, bopen, nl + indent))
            bclose = match_brace(m, bopen)
            i = (bclose + 1) if bclose != -1 else (bopen + 1)
            continue
        i += 1
    return edits


def transform(text):
    edits = compute_edits(text)
    for start, end, rep in sorted(edits, reverse=True):
        text = text[:start] + rep + text[end:]
    return text, edits


def line_of(text, idx):
    return text.count("\n", 0, idx) + 1


def process_file(path, fix):
    if not (path.endswith(".cpp") or path.endswith(".h")):
        return False, []
    with open(path, "r", encoding="utf-8", newline="") as f:
        text = f.read()
    new, edits = transform(text)
    if not edits:
        return False, []
    if fix:
        with open(path, "w", encoding="utf-8", newline="") as f:
            f.write(new)
        return True, ["ctor_brace: %s" % path]
    lines = sorted({line_of(text, b) for _, b, _ in edits})
    return True, ["%s: constructor brace not on its own line at line(s) %s"
                  % (path, ", ".join(str(x) for x in lines))]


def gather_targets(paths):
    targets = []
    for p in paths:
        if os.path.isfile(p):
            if p.endswith(".cpp") or p.endswith(".h"):
                targets.append(p)
        elif os.path.isdir(p):
            for root, _, files in os.walk(p):
                for fn in files:
                    if fn.endswith(".cpp") or fn.endswith(".h"):
                        targets.append(os.path.join(root, fn))
    return targets


def main(argv):
    mode = "check"
    paths = []
    for a in argv:
        if a in ("--check", "-c"):
            mode = "check"
        elif a in ("--fix", "-f"):
            mode = "fix"
        else:
            paths.append(a)
    if not paths:
        print("usage: ctor_brace.py [--check|--fix] <path ...>", file=sys.stderr)
        return 2

    fix = mode == "fix"
    any_flag = False
    for t in gather_targets(paths):
        flagged, msgs = process_file(t, fix)
        any_flag = any_flag or (flagged and not fix)
        for line in msgs:
            print(line)

    if fix:
        return 0
    if any_flag:
        print("\nconstructor braces not on their own line (run ctor_brace.py --fix)")
        return 1
    print("ctor_brace: all constructor braces already on their own line.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

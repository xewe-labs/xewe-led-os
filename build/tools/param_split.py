#!/usr/bin/env python3
"""param_split.py — put each parameter of a multi-parameter function DEFINITION
on its own line, continuation lines aligned under the '(' column.

    param_split.py --check [path ...]   # report signatures that would change
    param_split.py --fix   [path ...]   # rewrite in place

Only out-of-line / file-scope DEFINITIONS are touched: a signature whose
parameter '(' sits at brace-depth 0 and whose ')' is followed (past any
qualifiers / constructor member-init list) by a '{' body. This covers every
.cpp method definition, free functions, and out-of-class template definitions
in headers. It never touches:

  * in-class member DECLARATIONS (they end in ';', owned by align_decls.py);
  * in-class inline bodies (brace-depth > 0);
  * variable definitions like `static Foo x(1);` (followed by ';', not '{');
  * constructor member-initializer lists and call arguments (only the
    signature's own parameter list is rewritten).

Signatures with 0 or 1 parameter keep their parameter list on one line, but every
definition (any arity) also has the name-column padding clang-format emits under
AlignConsecutiveDeclarations squeezed out in two places, so definitions are NOT
aligned by name in sources: the return-type/name region before '(' is collapsed
to single spaces, and each parameter's internal whitespace is collapsed likewise.
The closing ')' is left glued to the last parameter; dangling_close.py then moves
it onto its own line for .cpp.

Runs AFTER clang-format (ColumnLimit:0 never wraps parameters itself) and is
idempotent. Text-based (no libclang): comments and string/char literals are
masked to spaces so bracket scanning ignores punctuation inside them.
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


def ident_before(m, idx):
    """Rightmost identifier ending just before idx (skipping whitespace)."""
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


def split_param_spans(m, lo, hi):
    """(start, end) spans of each top-level parameter in masked [lo, hi)."""
    spans, depth, start = [], 0, lo
    for i in range(lo, hi):
        c = m[i]
        if c in "([{<":
            depth += 1
        elif c in ")]}>":
            if depth > 0:
                depth -= 1
        elif c == "," and depth == 0:
            spans.append((start, i))
            start = i + 1
    spans.append((start, hi))
    return spans


def compute_edits(text):
    """Return sorted [(popen, pclose, replacement)] for signatures to rewrite."""
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
            name = ident_before(m, i)
            if not name or is_preproc_line(text, i):
                i += 1
                continue
            pclose = match_paren(m, i)
            if pclose == -1:
                i += 1
                continue
            bopen = find_body_open(m, pclose + 1)
            if bopen == -1:
                # Declaration / variable init, not a definition body.
                i = pclose + 1
                continue
            # Function definition. First collapse any name-column padding in the
            # return-type/name region before '(': clang-format's
            # AlignConsecutiveDeclarations pads adjacent file-scope definitions
            # so their names line up (e.g. `void      Foo::bar(` next to a longer
            # return type). Sources are not name-aligned, so squeeze it out.
            ls = line_start(text, i)
            j = ls
            while j < i and text[j] in " \t":
                j += 1
            indent = text[ls:j]
            pre = text[j:i]
            collapsed = " ".join(pre.split())
            if collapsed != pre:
                edits.append((j, i, collapsed))
            # Rewrite the parameter list if multi-param, aligning continuation
            # lines under the '(' at its post-collapse column.
            col = len(indent) + len(collapsed)
            spans = split_param_spans(m, i + 1, pclose)
            params = [text[s:e].strip() for s, e in spans]
            params = [" ".join(p.split()) for p in params if p != "" or len(spans) > 1]
            real = [p for p in params if p]
            if len(real) >= 2:
                cont = " " * (col + 1)
                body = ("(" + real[0] + "," + nl
                        + "".join(cont + p + "," + nl for p in real[1:-1])
                        + cont + real[-1] + ")")
                if body != text[i:pclose + 1]:
                    edits.append((i, pclose + 1, body))
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
        return True, ["param_split: %s" % path]
    lines = sorted({line_of(text, s) for s, _, _ in edits})
    return True, ["%s: multi-param signature(s) not one-per-line at line(s) %s"
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
        print("usage: param_split.py [--check|--fix] <path ...>", file=sys.stderr)
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
        print("\nmulti-param signatures not one-per-line (run param_split.py --fix)")
        return 1
    print("param_split: all multi-param signatures already one-per-line.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

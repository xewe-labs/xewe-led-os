#!/usr/bin/env python3
"""open_break.py — break the first element of a MULTI-LINE braced initializer
list off the opener line so it starts its own block-indented line.

    open_break.py --check [path ...]   # report lines that would change, exit 1
    open_break.py --fix   [path ...]   # rewrite in place

Rule: for a '{' whose matching '}' is on a LATER line (the list spans more than
one line) and that still has content glued after it on the opener line, move that
content down to a new line indented one level (IndentWidth 4) past the opener
line's indent:

    hsv_to_rgb({a,          hsv_to_rgb({
               b,     ->         a,
               c})                   b,
                                     c})

Only braced lists '{' are broken. clang-format under AlignAfterOpenBracket:
DontAlign block-indents a broken list's continuation lines at base+IndentWidth —
exactly where this pass places the moved first element — so the output is a
clang-format fixpoint. Call/subscript '(' '[' argument lists are re-indented by
clang-format to its own alignment-based continuation column, which a flat
block-indent post-pass cannot match; breaking those would never settle, so they
are left glued.

When several openers share the opener's line (e.g. the '(' and '{' of
`push_back(Command{`), only the INNERMOST candidate '{' with no later multi-line
opener to its right breaks, so nested content stays correctly one level deep.

Groups already broken (nothing but whitespace after the '{' on its line) and
single-line groups are left untouched, so the pass is idempotent. This is a
COSMETIC pass run AFTER clang-format (which, under ColumnLimit:0 +
AlignAfterOpenBracket:DontAlign, block-indents the continuation lines but still
glues the first element to the opener). dangling_close then dangles the closer.

Eligibility is restricted to bracket groups whose opener lies INSIDE a function
DEFINITION body (see find_body_ranges): declaration signature param lists in a
header are owned by align_decls.py and left glued. The restriction is applied on
both .h and .cpp (a .cpp declaration signature — e.g. a forward-declared
function — should likewise be left to align_decls' domain).

The scanner is text-based (no libclang): comments and string/char literals are
masked to spaces so bracket scanning ignores punctuation inside them.
"""

import bisect
import os
import sys

OPENERS = "([{"
CLOSERS = ")]}"
IDENT = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"
INDENT_WIDTH = 4


def mask(text):
    """Blank the *contents* of comments and string/char literals (newlines
    preserved) so bracket scanning ignores punctuation inside them."""
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


def build_match_map(m):
    """({close_pos: open_pos}, {open_pos: close_pos}) over () [] {}."""
    open_of = {}
    close_of = {}
    stack = []
    for i, ch in enumerate(m):
        if ch in OPENERS:
            stack.append(i)
        elif ch in CLOSERS:
            if stack:
                op = stack.pop()
                open_of[i] = op
                close_of[op] = i
    return open_of, close_of


def ident_before(m, idx):
    """Rightmost identifier ending just before idx (skipping whitespace)."""
    j = idx
    while j > 0 and m[j - 1] in " \t\r\n":
        j -= 1
    end = j
    while j > 0 and m[j - 1] in IDENT:
        j -= 1
    return m[j:end]


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


def find_body_ranges(m, close_of):
    """(body_open, body_close) spans of every function DEFINITION body: a '('
    with an identifier before it, a matching ')', then a '{' before any ';'."""
    ranges = []
    n = len(m)
    i = 0
    while i < n:
        if m[i] == "(" and ident_before(m, i):
            pclose = close_of.get(i)
            if pclose is None:
                i += 1
                continue
            bopen = find_body_open(m, pclose + 1)
            if bopen == -1:
                i = pclose + 1
                continue
            bclose = close_of.get(bopen)
            if bclose is not None:
                ranges.append((bopen, bclose))
                i = bclose + 1
                continue
            i = bopen + 1
            continue
        i += 1
    return ranges


def in_any_range(pos, ranges):
    return any(a < pos < b for a, b in ranges)


def line_starts_of(text):
    starts = [0]
    for i, ch in enumerate(text):
        if ch == "\n":
            starts.append(i + 1)
    return starts


def line_no(starts, pos):
    return bisect.bisect_right(starts, pos) - 1


def indent_at(text, starts, pos):
    ls = starts[line_no(starts, pos)]
    j = ls
    while j < len(text) and text[j] in " \t":
        j += 1
    return text[ls:j]


def macro_lines(text, starts):
    """Set of line indices inside a preprocessor directive, including '\\'-
    continuation lines. Offset edits there are unsafe (line continuations)."""
    lines = set()
    n = len(text)
    in_macro = False
    for li in range(len(starts)):
        s = starts[li]
        e = starts[li + 1] - 1 if li + 1 < len(starts) else n
        j = s
        while j < e and text[j] in " \t":
            j += 1
        if not in_macro and j < e and text[j] == "#":
            in_macro = True
        if in_macro:
            lines.add(li)
            # Continues only if this line ends with a backslash (before EOL ws).
            k = e - 1
            while k >= s and text[k] in " \t\r":
                k -= 1
            in_macro = k >= s and text[k] == "\\"
    return lines


def compute_opens(text):
    """Return sorted list of (start, end, replacement) edits that replace the
    whitespace run right after a multi-line braced-list opener '{' with a newline
    + block indent, moving the first glued element onto its own line. Restricted
    to '{' groups whose opener lies inside a function definition body."""
    m = mask(text)
    open_of, close_of = build_match_map(m)
    body_ranges = find_body_ranges(m, close_of)
    starts = line_starts_of(text)
    macros = macro_lines(text, starts)
    n = len(text)
    nl = "\r\n" if "\r\n" in text else "\n"

    # Multi-line opener positions grouped by their line. When several openers on
    # one line each span multiple lines (e.g. the '(' and '{' of
    # `push_back(Command{`), only the INNERMOST (rightmost) one may break: its
    # first element lands at base+IndentWidth, matching where clang-format's
    # DontAlign already put the sibling continuation lines. Breaking an outer
    # opener would push nested content one level too shallow (this pass runs
    # after clang-format and cannot re-indent the nested block).
    multiline_by_line = {}
    for o, c in close_of.items():
        lo = line_no(starts, o)
        if lo != line_no(starts, c):
            multiline_by_line.setdefault(lo, []).append(o)

    edits = []
    for op in sorted(close_of):
        if m[op] != "{":
            # Braced initializer lists only. clang-format under
            # AlignAfterOpenBracket:DontAlign block-indents a broken '{' list's
            # continuation lines at base+IndentWidth — exactly where this pass
            # puts the moved first element, so the result is a clang-format
            # fixpoint. Call/subscript '(' '[' argument lists are re-indented by
            # clang-format to its own (alignment-based) continuation column, which
            # a flat block-indent post-pass cannot match, so breaking those would
            # never settle. They are left glued.
            continue
        cp = close_of[op]
        ol = line_no(starts, op)
        if ol == line_no(starts, cp):
            continue  # single-line group
        if ol in macros:
            continue  # inside a #define — offset edits unsafe
        if not in_any_range(op, body_ranges):
            continue  # declaration signature param list — align_decls owns it
        if any(o2 > op for o2 in multiline_by_line[ol]):
            continue  # a nested multi-line opener to the right owns the break

        # First real token after the opener on the SAME line, scanning MASKED
        # text so a trailing comment or string reads as whitespace ('\r' too, so
        # a CRLF opener line stays idempotent).
        line_end = starts[ol + 1] - 1 if ol + 1 < len(starts) else n
        j = op + 1
        while j < line_end and m[j] in " \t\r":
            j += 1
        if j >= line_end:
            continue  # only whitespace/comment after '{' (already broken)
        if m[j] in CLOSERS:
            continue  # immediate closer on same line handled as single-line
        if text[op + 1:j].strip() != "":
            continue  # an inline comment sits before the first token — leave it

        target = indent_at(text, starts, op) + " " * INDENT_WIDTH
        edits.append((op + 1, j, nl + target))

    edits.sort()
    return edits


def apply_edits(text, edits):
    for s, e, rep in sorted(edits, reverse=True):
        text = text[:s] + rep + text[e:]
    return text


def process_file(path, fix):
    if not (path.endswith(".cpp") or path.endswith(".h")):
        return False, []
    with open(path, "r", encoding="utf-8", newline="") as f:
        text = f.read()

    edits = compute_opens(text)
    if not edits:
        return False, []

    if not fix:
        starts = line_starts_of(text)
        lines = sorted({line_no(starts, s) + 1 for s, _, _ in edits})
        msgs = ["%s: first element glued to opener at line(s) %s"
                % (path, ", ".join(str(x) for x in lines))]
        return True, msgs

    new_text = apply_edits(text, edits)
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(new_text)
    return True, ["open_break: %s" % path]


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
        print("usage: open_break.py [--check|--fix] <path ...>", file=sys.stderr)
        return 2

    targets = gather_targets(paths)
    fix = mode == "fix"
    any_flag = False
    for t in targets:
        flagged, msgs = process_file(t, fix)
        any_flag = any_flag or (flagged and not fix)
        for line in msgs:
            print(line)

    if fix:
        return 0
    if any_flag:
        print("\nfirst element glued to a multi-line opener (run open_break.py --fix)")
        return 1
    print("open_break: all multi-line openers already broken.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

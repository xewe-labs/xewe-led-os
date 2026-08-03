#!/usr/bin/env python3
"""dangling_close.py — put the closing bracket of a MULTI-LINE bracket group on
its own line.

    dangling_close.py --check [path ...]   # report lines that would change, exit 1
    dangling_close.py --fix   [path ...]   # rewrite in place

Rule: for any '(' / '{' / '[' whose matching close is on a LATER line (i.e. the
group spans more than one line), the closing bracket must begin its own line.
Closers whose OPENING brackets sit on the same source line stack together:

    foo(               foo(
        a,        ->       a,
        b);                b
                       );

    push_back(T{            push_back(T{
        a,                      a,
        [](){                   [](){
            g();       ->           g();
        }});                    }
                            });   # T{ and push_back( opened together -> "});"

Single-line groups (open and close on the same line) are left untouched, so
`compute(a, b);` and already-canonical `});` lines are stable (idempotent).

This is a COSMETIC pass: it runs AFTER clang-format (which, under ColumnLimit:0,
glues the closer onto the last argument line).

On .cpp sources every multi-line group is eligible. On .h headers only groups
whose opener sits INSIDE an inline function body are eligible: member-declaration
signature param lists (e.g. `add(a,\n b,\n c);`) keep their glued ')' because
that column layout is owned by align_decls.py. Detection of function bodies is
text-based (a '(' with an identifier before it, a matching ')', then a '{' body
before any ';'), so a call/initializer inside an inline method like
`return std::make_tuple(\n ...,\n last));` gets its ')' dangled while the class's
declaration signatures are left alone.

The scanner is text-based (no libclang): comments and string/char literals are
masked to spaces so bracket scanning ignores punctuation inside them.
"""

import bisect
import os
import sys

OPENERS = "([{"
CLOSERS = ")]}"
WS = " \t\r"
IDENT = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"


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


def compute_breaks(text, header=False):
    """Return sorted list of (insert_pos, indent) where a newline+indent should
    be inserted before a closing bracket. In header mode only closers whose
    opener lies inside an inline function body are eligible."""
    m = mask(text)
    open_of, close_of = build_match_map(m)
    body_ranges = find_body_ranges(m, close_of) if header else None
    starts = line_starts_of(text)
    n = len(text)
    breaks = []

    for li in range(len(starts)):
        s = starts[li]
        e = starts[li + 1] - 1 if li + 1 < len(starts) else n  # excludes '\n'

        # Skip preprocessor lines (line continuations make offset edits unsafe).
        j = s
        while j < e and text[j] in " \t":
            j += 1
        if j < e and text[j] == "#":
            continue

        # Trailing run of closers, ignoring trailing ws and one ';' or ','.
        k = e - 1
        while k >= s and m[k] in WS:
            k -= 1
        if k >= s and m[k] in ";,":
            k -= 1
            while k >= s and m[k] in WS:
                k -= 1
        run_end = k + 1
        while k >= s and m[k] in CLOSERS:
            k -= 1
        run_start = k + 1
        if run_start >= run_end:
            continue  # no trailing closers on this line

        content_before = m[s:run_start].strip()

        prev_open_line = None
        for idx, cp in enumerate(range(run_start, run_end)):
            op = open_of.get(cp)
            if op is None:
                prev_open_line = None
                continue
            ol = line_no(starts, op)
            if ol == li:
                # Single-line group: closer stays glued to its opener's content.
                prev_open_line = ol
                continue
            if body_ranges is not None and not in_any_range(op, body_ranges):
                # Header declaration signature (not an inline body): align_decls
                # owns its glued ')'. Leave it.
                prev_open_line = ol
                continue
            # Multi-line group: closer should begin a line.
            if idx == 0:
                if content_before:
                    breaks.append((cp, indent_at(text, starts, op)))
                # else: already at line start -> leave it.
            elif ol != prev_open_line:
                breaks.append((cp, indent_at(text, starts, op)))
            prev_open_line = ol

    breaks.sort()
    return breaks


def apply_breaks(text, breaks, nl):
    for cp, indent in sorted(breaks, reverse=True):
        text = text[:cp] + nl + indent + text[cp:]
    return text


def process_file(path, fix):
    if not (path.endswith(".cpp") or path.endswith(".h")):
        return False, []
    with open(path, "r", encoding="utf-8", newline="") as f:
        text = f.read()

    breaks = compute_breaks(text, header=path.endswith(".h"))
    if not breaks:
        return False, []

    if not fix:
        starts = line_starts_of(text)
        lines = sorted({line_no(starts, cp) + 1 for cp, _ in breaks})
        msgs = ["%s: closer(s) not on own line at line(s) %s"
                % (path, ", ".join(str(x) for x in lines))]
        return True, msgs

    nl = "\r\n" if "\r\n" in text else "\n"
    new_text = apply_breaks(text, breaks, nl)
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(new_text)
    return True, ["dangling_close: %s" % path]


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
        print("usage: dangling_close.py [--check|--fix] <path ...>", file=sys.stderr)
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
        print("\nmulti-line closers not on their own line (run dangling_close.py --fix)")
        return 1
    print("dangling_close: all multi-line closers already on their own line.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

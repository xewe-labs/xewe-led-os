#!/usr/bin/env python3
"""body_layout.py — the four cosmetic body-layout passes, merged.

Each runs AFTER clang-format (which under ColumnLimit:0 does none of this) and is
idempotent. They are applied in this order by format.py:

    param_split      one parameter per line in multi-param DEFINITIONS
    ctor_brace       constructor body '{' on its own line, off the init list
    open_break       first element of a multi-line braced list off the opener
    dangling_close   multi-line bracket-group closer on its own line

Each is a pure (text, path) -> text transform. See the per-function docstrings
for the exact rule. Text-based scanning via fmtlib.mask (comments/literals
blanked).
"""

from fmtlib import (
    mask, match_paren, match_brace, find_body_open, find_body_ranges,
    build_match_map, ident_before, line_start, line_starts_of, line_no,
    indent_at, in_any_range, is_preproc_line, CLOSERS,
)

INDENT_WIDTH = 4


# --------------------------------------------------------------------------- #
# param_split
# --------------------------------------------------------------------------- #
def _split_param_spans(m, lo, hi):
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


def _param_split_edits(text):
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
                i = pclose + 1
                continue
            ls = line_start(text, i)
            j = ls
            while j < i and text[j] in " \t":
                j += 1
            indent = text[ls:j]
            pre = text[j:i]
            collapsed = " ".join(pre.split())
            if collapsed != pre:
                edits.append((j, i, collapsed))
            col = len(indent) + len(collapsed)
            spans = _split_param_spans(m, i + 1, pclose)
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


def param_split(text, path=None):
    edits = _param_split_edits(text)
    for start, end, rep in sorted(edits, reverse=True):
        text = text[:start] + rep + text[end:]
    return text


# --------------------------------------------------------------------------- #
# ctor_brace
# --------------------------------------------------------------------------- #
def _has_init_list(m, lo, hi):
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


def _indent_of_line(text, idx):
    ls = line_start(text, idx)
    j = ls
    while j < len(text) and text[j] in " \t":
        j += 1
    return text[ls:j]


def _ctor_brace_edits(text):
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
            if _has_init_list(m, pclose + 1, bopen):
                ls = line_start(text, bopen)
                if text[ls:bopen].strip() != "":  # brace attached to init line
                    cut = bopen
                    while cut > 0 and text[cut - 1] in " \t":
                        cut -= 1
                    indent = _indent_of_line(text, i)
                    edits.append((cut, bopen, nl + indent))
            bclose = match_brace(m, bopen)
            i = (bclose + 1) if bclose != -1 else (bopen + 1)
            continue
        i += 1
    return edits


def ctor_brace(text, path=None):
    edits = _ctor_brace_edits(text)
    for start, end, rep in sorted(edits, reverse=True):
        text = text[:start] + rep + text[end:]
    return text


# --------------------------------------------------------------------------- #
# open_break
# --------------------------------------------------------------------------- #
def _macro_lines(text, starts):
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
            k = e - 1
            while k >= s and text[k] in " \t\r":
                k -= 1
            in_macro = k >= s and text[k] == "\\"
    return lines


def _open_break_edits(text):
    m = mask(text)
    open_of, close_of = build_match_map(m)
    body_ranges = find_body_ranges(m, close_of)
    starts = line_starts_of(text)
    macros = _macro_lines(text, starts)
    n = len(text)
    nl = "\r\n" if "\r\n" in text else "\n"

    multiline_by_line = {}
    for o, c in close_of.items():
        lo = line_no(starts, o)
        if lo != line_no(starts, c):
            multiline_by_line.setdefault(lo, []).append(o)

    edits = []
    for op in sorted(close_of):
        if m[op] != "{":
            continue  # braced initializer lists only
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


def open_break(text, path=None):
    edits = _open_break_edits(text)
    for s, e, rep in sorted(edits, reverse=True):
        text = text[:s] + rep + text[e:]
    return text


# --------------------------------------------------------------------------- #
# dangling_close
# --------------------------------------------------------------------------- #
WS = " \t\r"


def _dangling_breaks(text, header):
    m = mask(text)
    open_of, close_of = build_match_map(m)
    body_ranges = find_body_ranges(m, close_of) if header else None
    starts = line_starts_of(text)
    n = len(text)
    breaks = []

    for li in range(len(starts)):
        s = starts[li]
        e = starts[li + 1] - 1 if li + 1 < len(starts) else n  # excludes '\n'

        j = s
        while j < e and text[j] in " \t":
            j += 1
        if j < e and text[j] == "#":
            continue  # preprocessor line

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
                prev_open_line = ol
                continue  # single-line group
            if body_ranges is not None and not in_any_range(op, body_ranges):
                prev_open_line = ol
                continue  # header decl signature — align_decls owns its ')'
            if idx == 0:
                if content_before:
                    breaks.append((cp, indent_at(text, starts, op)))
            elif ol != prev_open_line:
                breaks.append((cp, indent_at(text, starts, op)))
            prev_open_line = ol

    breaks.sort()
    return breaks


def dangling_close(text, path=None):
    header = bool(path) and path.endswith(".h")
    breaks = _dangling_breaks(text, header)
    nl = "\r\n" if "\r\n" in text else "\n"
    for cp, indent in sorted(breaks, reverse=True):
        text = text[:cp] + nl + indent + text[cp:]
    return text

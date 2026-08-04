#!/usr/bin/env python3
"""fmtlib.py — shared library for the format pipeline (see format.py).

Single home for the text-scanning helpers the individual stage modules used to
each carry their own copy of, plus discovery / file-I/O / clang-format glue.

All scanning is text-based (no libclang): comments and string/char literals are
blanked to spaces by mask() so bracket scanning ignores punctuation inside them.
"""

import bisect
import os
import subprocess

IDENT = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"
OPENERS = "([{"
CLOSERS = ")]}"


# --------------------------------------------------------------------------- #
# Masking
# --------------------------------------------------------------------------- #
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


# --------------------------------------------------------------------------- #
# Bracket matching
# --------------------------------------------------------------------------- #
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


def build_match_map(m):
    """({close_pos: open_pos}, {open_pos: close_pos}) over () [] {}."""
    open_of, close_of, stack = {}, {}, []
    for i, ch in enumerate(m):
        if ch in OPENERS:
            stack.append(i)
        elif ch in CLOSERS:
            if stack:
                op = stack.pop()
                open_of[i] = op
                close_of[op] = i
    return open_of, close_of


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


# --------------------------------------------------------------------------- #
# Identifier scanning
# --------------------------------------------------------------------------- #
def ident_before(m, idx):
    """Rightmost identifier ending just before idx (skipping whitespace)."""
    j = idx
    while j > 0 and m[j - 1] in " \t\r\n":
        j -= 1
    end = j
    while j > 0 and m[j - 1] in IDENT:
        j -= 1
    return m[j:end]


def trailing_ident(s):
    """Rightmost identifier (optionally ~-prefixed) at the end of s."""
    j = len(s)
    while j > 0 and s[j - 1] in " \t\r\n":
        j -= 1
    end = j
    while j > 0 and s[j - 1] in IDENT:
        j -= 1
    name = s[j:end]
    if not name:
        return ""
    if j > 0 and s[j - 1] == "~":
        name = "~" + name
    return name


def first_toplevel_paren(stmt_masked):
    """Index of the first '(' at bracket-depth 0 (ignoring []{}<> nesting)."""
    depth = 0
    for i, ch in enumerate(stmt_masked):
        if ch in "[{<":
            depth += 1
        elif ch in "]}>":
            if depth > 0:
                depth -= 1
        elif ch == "(" and depth == 0:
            return i
    return -1


def top_level_index(s, ch):
    depth = 0
    for i, c in enumerate(s):
        if depth == 0 and c == ch:
            return i
        if c in "<([{":
            depth += 1
        elif c in ">)]}":
            depth -= 1
    return -1


def split_params(s):
    s = s.strip()
    if not s:
        return []
    parts, depth, start = [], 0, 0
    for i, c in enumerate(s):
        if c in "<([{":
            depth += 1
        elif c in ">)]}":
            depth -= 1
        elif c == "," and depth == 0:
            parts.append(s[start:i].strip())
            start = i + 1
    parts.append(s[start:].strip())
    return parts


def strip_comment(line):
    """Split off a trailing // comment. Returns (code, comment-or-empty)."""
    idx = line.find("//")
    if idx == -1:
        return line, ""
    return line[:idx].rstrip(), line[idx:].strip()


# --------------------------------------------------------------------------- #
# Line geometry
# --------------------------------------------------------------------------- #
def line_start(text, idx):
    return text.rfind("\n", 0, idx) + 1


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


def is_preproc_line(text, idx):
    ls = line_start(text, idx)
    j = ls
    while j < len(text) and text[j] in " \t":
        j += 1
    return j < len(text) and text[j] == "#"


# --------------------------------------------------------------------------- #
# clang-format
# --------------------------------------------------------------------------- #
def run_clang_format(text, path, style):
    """Format `text` as if it were `path`, using the .clang-format at `style`,
    over stdin->stdout (no temp files, no in-place edits)."""
    style = os.path.abspath(style).replace(os.sep, "/")
    proc = subprocess.run(
        ["clang-format", "--assume-filename=" + path, "--style=file:" + style],
        input=text, capture_output=True, text=True,
    )
    if proc.returncode != 0:
        raise RuntimeError("clang-format failed for %s:\n%s" % (path, proc.stderr))
    return proc.stdout


# --------------------------------------------------------------------------- #
# Discovery + I/O
# --------------------------------------------------------------------------- #
def gather_targets(paths):
    """All .h/.cpp under the given files/directories, as absolute paths."""
    targets = []
    for p in paths:
        if os.path.isfile(p):
            if p.endswith((".h", ".cpp")):
                targets.append(os.path.abspath(p))
        elif os.path.isdir(p):
            for root, _, files in os.walk(p):
                for fn in files:
                    if fn.endswith((".h", ".cpp")):
                        targets.append(os.path.abspath(os.path.join(root, fn)))
    return targets


def read(path):
    """Read a file and normalize to '\\n' line endings."""
    with open(path, encoding="utf-8", newline="") as f:
        return f.read().replace("\r\n", "\n").replace("\r", "\n")


def write(path, text):
    """Write with '\\n' line endings (utf-8)."""
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)

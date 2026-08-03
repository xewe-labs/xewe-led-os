#!/usr/bin/env python3
"""method_order.py — enforce that a .cpp's out-of-line method DEFINITION order
matches the DECLARATION order in its sibling .h.

    method_order.py --check [path ...]   # report divergences, exit 1 if any
    method_order.py --fix   [path ...]   # reorder .cpp blocks to match the .h

For a given Foo.cpp the sibling header is Foo.h in the same directory. Only
methods that are BOTH declared in the .h and defined in the .cpp are compared:
declarations with no definition (inline bodies / undefined overloads) are
skipped, and any .cpp definition with no matching declaration is left anchored
in its current slot. Overloads are disambiguated by (name, arity).

The scanner is text-based (no libclang): comments and string/char literals are
masked to spaces so brace/paren/angle scanning is safe, then definitions are
found at brace-depth 0 as `Class::method(` blocks.
"""

import os
import sys


# --------------------------------------------------------------------------- #
# Lexing helpers
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


def match_paren(s, open_idx):
    """Index of the ')' matching the '(' at open_idx (masked text)."""
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
    """Index of the '}' matching the '{' at open_idx (masked text)."""
    depth = 0
    for i in range(open_idx, len(s)):
        if s[i] == "{":
            depth += 1
        elif s[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    return -1


def count_arity(params_masked):
    """Number of parameters given the masked text between the outer parens."""
    s = params_masked.strip()
    if not s:
        return 0
    depth = 0
    commas = 0
    for ch in s:
        if ch in "([{<":
            depth += 1
        elif ch in ")]}>":
            if depth > 0:
                depth -= 1
        elif ch == "," and depth == 0:
            commas += 1
    return commas + 1


IDENT = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"


def trailing_ident(s):
    """Rightmost identifier (optionally ~-prefixed) at the end of s."""
    j = len(s)
    while j > 0 and s[j - 1] in " \t\n":
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


def line_start(text, idx):
    p = text.rfind("\n", 0, idx)
    return p + 1


def line_end_incl(text, idx):
    p = text.find("\n", idx)
    return len(text) if p == -1 else p + 1


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


# --------------------------------------------------------------------------- #
# Header: declaration order per class
# --------------------------------------------------------------------------- #
def extract_header_classes(text):
    """{class_name: [(name, arity), ...]} in declaration order."""
    m = mask(text)
    result = {}
    i, n = 0, len(m)
    while True:
        k = _find_class_keyword(m, i)
        if k is None:
            break
        kw_end, cname = k
        i = kw_end
        # Body opens at the first '{' before any ';' (skip forward decls).
        brace = m.find("{", kw_end)
        semi = m.find(";", kw_end)
        if brace == -1 or (semi != -1 and semi < brace):
            continue
        close = match_brace(m, brace)
        if close == -1:
            continue
        methods = _decls_in_body(m, brace + 1, close)
        if cname not in result:
            result[cname] = methods
        i = brace + 1
    return result


def _find_class_keyword(m, start):
    for kw in ("class", "struct"):
        pass
    # Scan for either keyword, whichever comes first at/after start.
    best = None
    for kw in ("class", "struct"):
        p = start
        while True:
            idx = m.find(kw, p)
            if idx == -1:
                break
            before = m[idx - 1] if idx > 0 else " "
            after = m[idx + len(kw)] if idx + len(kw) < len(m) else " "
            if before in IDENT or after in IDENT:
                p = idx + len(kw)
                continue
            j = idx + len(kw)
            while j < len(m) and m[j] in " \t\n":
                j += 1
            e = j
            while e < len(m) and m[e] in IDENT:
                e += 1
            name = m[j:e]
            if name:
                cand = (idx, e, name)
                if best is None or cand[0] < best[0]:
                    best = cand
            break
    if best is None:
        return None
    return best[1], best[2]


def _decls_in_body(m, lo, hi):
    """Ordered [(name, arity)] for method declarations directly in a class body
    (brace-depth 0 relative to the body)."""
    methods = []
    depth = 0
    stmt_start = lo
    i = lo
    while i < hi:
        c = m[i]
        if c == "{":
            depth += 1
        elif c == "}":
            if depth > 0:
                depth -= 1
        elif c == ";" and depth == 0:
            stmt = m[stmt_start:i]
            md = _decl_from_stmt(stmt)
            if md:
                methods.append(md)
            stmt_start = i + 1
        i += 1
    return methods


def _decl_from_stmt(stmt_masked):
    p = first_toplevel_paren(stmt_masked)
    if p == -1:
        return None
    name = trailing_ident(stmt_masked[:p])
    if not name:
        return None
    close = match_paren(stmt_masked, p)
    if close == -1:
        return None
    return (name, count_arity(stmt_masked[p + 1:close]))


# --------------------------------------------------------------------------- #
# Source: definition order + block spans
# --------------------------------------------------------------------------- #
class Block:
    __slots__ = ("cls", "name", "arity", "start", "end")

    def __init__(self, cls, name, arity, start, end):
        self.cls = cls
        self.name = name
        self.arity = arity
        self.start = start  # char offset, incl. attached leading comments
        self.end = end      # char offset, one past the trailing newline


def extract_source_blocks(text):
    """Ordered list of Block for out-of-line definitions at brace-depth 0."""
    m = mask(text)
    n = len(m)
    blocks = []
    depth = 0
    i = 0
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
        if depth == 0 and (m[i - 1] not in IDENT if i > 0 else True):
            hit = _match_def_here(m, i)
            if hit:
                cls, name, paren = hit
                pclose = match_paren(m, paren)
                arity = count_arity(m[paren + 1:pclose])
                bopen = _find_body_open(m, pclose + 1)
                if bopen != -1:
                    bclose = match_brace(m, bopen)
                    if bclose != -1:
                        start = _attach_leading(text, line_start(text, i))
                        end = line_end_incl(text, bclose)
                        blocks.append(Block(cls, name, arity, start, end))
                        i = bclose + 1
                        continue
        i += 1
    return blocks


def _match_def_here(m, i):
    """If a `Class::method(` starts exactly at i, return (cls, name, paren_idx)."""
    j = i
    while j < len(m) and m[j] in IDENT:
        j += 1
    cls = m[i:j]
    if not cls:
        return None
    if m[j:j + 2] != "::":
        return None
    j += 2
    tilde = ""
    if j < len(m) and m[j] == "~":
        tilde = "~"
        j += 1
    k = j
    while k < len(m) and m[k] in IDENT:
        k += 1
    name = m[j:k]
    if not name:
        return None
    p = k
    while p < len(m) and m[p] in " \t\n":
        p += 1
    if p >= len(m) or m[p] != "(":
        return None
    return cls, tilde + name, p


def _find_body_open(m, start):
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


def _attach_leading(text, block_start):
    start = block_start
    while start > 0:
        prev_ls = line_start(text, start - 1)
        line = text[prev_ls:start - 1]
        if line.strip().startswith("//"):
            start = prev_ls
        else:
            break
    return start


# --------------------------------------------------------------------------- #
# Comparison
# --------------------------------------------------------------------------- #
def compare(header_classes, blocks):
    """Return (ok, messages). Compares, per class, the relative order of the
    .cpp definitions against the .h declaration order."""
    msgs = []
    ok = True
    by_class = {}
    for b in blocks:
        by_class.setdefault(b.cls, []).append(b)

    for cls, defs in by_class.items():
        decls = header_classes.get(cls)
        if decls is None:
            continue  # class not declared in this header; skip silently
        decl_index = {}
        for idx, key in enumerate(decls):
            decl_index.setdefault(key, idx)  # first decl wins for dup (name,arity)

        matched = []  # (block, decl_idx) for defs that have a declaration
        for b in defs:
            key = (b.name, b.arity)
            if key in decl_index:
                matched.append((b, decl_index[key]))

        expected = sorted(matched, key=lambda t: t[1])
        actual = matched
        if [t[1] for t in actual] != [t[1] for t in expected]:
            ok = False
            msgs.append("  class %s: definition order does not match %s.h" % (cls, cls))
            msgs.append("    expected:")
            for b, _ in expected:
                msgs.append("      %s/%d" % (b.name, b.arity))
            msgs.append("    actual:")
            for b, _ in actual:
                msgs.append("      %s/%d" % (b.name, b.arity))
    return ok, msgs


# --------------------------------------------------------------------------- #
# Reorder (--fix)
# --------------------------------------------------------------------------- #
def reorder_text(text, header_classes, blocks):
    """Return reordered text, or None if nothing to do. Gaps between blocks are
    kept in their positional slots; only whole definition blocks are permuted.
    Aborts (returns None) if the byte-preservation invariant would break."""
    if not blocks:
        return None

    # Slot layout: gap, block, gap, block, ..., gap.
    gaps = []
    prev = blocks[0].start
    gaps.append(text[:prev])
    for idx in range(len(blocks)):
        b = blocks[idx]
        nxt = blocks[idx + 1].start if idx + 1 < len(blocks) else len(text)
        gaps.append(text[b.end:nxt])
    head = text[: blocks[0].start]

    # Desired order: matched blocks in .h order; unmatched blocks anchored.
    order_key = {}
    for i, b in enumerate(blocks):
        decls = header_classes.get(b.cls)
        di = None
        if decls is not None:
            for idx, key in enumerate(decls):
                if key == (b.name, b.arity):
                    di = idx
                    break
        order_key[i] = di

    matched_idx = [i for i in range(len(blocks)) if order_key[i] is not None]
    want_seq = sorted(matched_idx, key=lambda i: order_key[i])

    new_order = list(range(len(blocks)))
    it = iter(want_seq)
    for slot in range(len(blocks)):
        if order_key[slot] is not None:
            new_order[slot] = next(it)

    if new_order == list(range(len(blocks))):
        return None

    # Rebuild: head + block[perm[0]] + gap[1] + block[perm[1]] + gap[2] + ...
    parts = [head]
    for slot in range(len(blocks)):
        b = blocks[new_order[slot]]
        parts.append(text[b.start:b.end])
        parts.append(gaps[slot + 1])
    result = "".join(parts)

    # Byte-preservation invariant: multiset of block texts + all gap text
    # unchanged; only ordering differs.
    before = sorted(text[b.start:b.end] for b in blocks) + sorted(gaps)
    after_blocks = sorted(text[b.start:b.end] for b in blocks)
    after_gaps = sorted(gaps)
    if sorted(after_blocks + after_gaps) != sorted(before):
        return None
    if len(result) != len(text):
        return None
    return result


# --------------------------------------------------------------------------- #
# Driver
# --------------------------------------------------------------------------- #
def sibling_header(cpp_path):
    stem, _ = os.path.splitext(cpp_path)
    h = stem + ".h"
    return h if os.path.isfile(h) else None


def process_file(cpp_path, fix):
    """Returns (changed_or_divergent, message_lines)."""
    if not cpp_path.endswith(".cpp"):
        return False, []
    header = sibling_header(cpp_path)
    if header is None:
        return False, ["%s: no sibling .h, skipped" % cpp_path]

    with open(cpp_path, "r", encoding="utf-8", newline="") as f:
        cpp_text = f.read()
    with open(header, "r", encoding="utf-8", newline="") as f:
        h_text = f.read()

    header_classes = extract_header_classes(h_text)
    blocks = extract_source_blocks(cpp_text)

    if fix:
        new_text = reorder_text(cpp_text, header_classes, blocks)
        if new_text is None:
            return False, []
        with open(cpp_path, "w", encoding="utf-8", newline="") as f:
            f.write(new_text)
        return True, ["reordered: %s" % cpp_path]

    ok, msgs = compare(header_classes, blocks)
    if ok:
        return False, []
    return True, ["%s:" % cpp_path] + msgs


def gather_targets(paths):
    targets = []
    for p in paths:
        if os.path.isfile(p):
            if p.endswith(".cpp"):
                targets.append(p)
        elif os.path.isdir(p):
            for root, _, files in os.walk(p):
                for fn in files:
                    if fn.endswith(".cpp"):
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
        print("usage: method_order.py [--check|--fix] <path ...>", file=sys.stderr)
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
        print("\nmethod order: .cpp definitions diverge from .h (run method_order.py --fix)")
        return 1
    print("method order: all files match .h declaration order.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

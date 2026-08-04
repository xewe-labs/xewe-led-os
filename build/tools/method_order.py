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

from fmtlib import (IDENT, find_body_open, first_toplevel_paren, line_start,
                    mask, match_brace, match_paren, trailing_ident)


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


def line_end_incl(text, idx):
    p = text.find("\n", idx)
    return len(text) if p == -1 else p + 1


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
                bopen = find_body_open(m, pclose + 1)
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
# Pair transform
# --------------------------------------------------------------------------- #
def reorder(cpp_text, h_text):
    """Reorder `cpp_text`'s out-of-line definitions to match declaration order in
    its sibling `h_text`. Returns the (possibly unchanged) source. Pure."""
    if h_text is None:
        return cpp_text  # header-only / no sibling in scope
    header_classes = extract_header_classes(h_text)
    blocks = extract_source_blocks(cpp_text)
    new = reorder_text(cpp_text, header_classes, blocks)
    return cpp_text if new is None else new

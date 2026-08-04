#!/usr/bin/env python3
"""inline_move.py — move plain inline member-function DEFINITIONS out of a .h
class body into its sibling .cpp as out-of-line definitions.

    inline_move.py --check [path ...]   # report movable inline defs, exit 1 if any
    inline_move.py --fix   [path ...]   # move bodies .h -> sibling .cpp

Motivation: align_decls.py column-aligns class declarations but skips any line
containing a brace, so an inline-defined member never joins the column grid. This
pass converts a qualifying inline definition into a brace-free declaration in the
.h (which align_decls can then align) and appends the body to the sibling .cpp.

v1 is deliberately conservative — when in doubt, leave the member inline:
  * only PLAIN member functions move (non-empty return type). Constructors,
    destructors, and operator/conversion functions stay inline.
  * ALWAYS skipped: templates, and any member marked static/constexpr/consteval/
    friend/inline. `= default` / `= delete` / pure-virtual `= 0` end in ';', so
    they are declarations (never candidates) and are untouched.
  * only TOP-LEVEL classes are processed. Classes nested in another class or
    inside a namespace are skipped (their brace body is jumped over), avoiding
    qualified-name complexity.
  * a .h with no sibling .cpp (header-only) is skipped silently.

The scanner is text-based (no libclang): comments and string/char literals are
masked to spaces so brace/paren/angle scanning is safe.
"""

import re

from fmtlib import (IDENT, first_toplevel_paren, mask, match_brace,
                    match_paren)

WS = " \t\r\n"


def word_at(m, i, word):
    if m[i:i + len(word)] != word:
        return False
    before = m[i - 1] if i > 0 else " "
    after = m[i + len(word)] if i + len(word) < len(m) else " "
    return before not in IDENT and after not in IDENT


# --------------------------------------------------------------------------- #
# Top-level class discovery
# --------------------------------------------------------------------------- #
def _is_template_before(m, kw_idx):
    """True if the class keyword at kw_idx is preceded by a `template<...>`
    (or bare `template`) prefix."""
    j = kw_idx - 1
    while j >= 0 and m[j] in WS:
        j -= 1
    if j >= 0 and m[j] == ">":
        depth = 0
        k = j
        while k >= 0:
            if m[k] == ">":
                depth += 1
            elif m[k] == "<":
                depth -= 1
                if depth == 0:
                    break
            k -= 1
        if k < 0:
            return False
        j = k - 1
        while j >= 0 and m[j] in WS:
            j -= 1
    end = j + 1
    start = end
    while start > 0 and m[start - 1] in IDENT:
        start -= 1
    return m[start:end] == "template"


def _class_body_open(m, name_end):
    """First '{' opening the class body at paren/bracket/angle-depth 0, or -1 if
    a ';' (forward declaration) is hit first."""
    depth = 0
    for i in range(name_end, len(m)):
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


def find_toplevel_classes(m):
    """Yield (name, body_open, body_close) for named class/struct at brace-depth
    0. Skips forward declarations, templates, and unnamed aggregates. Nested and
    namespaced classes are never reached (their brace body is jumped over)."""
    results = []
    depth = 0
    i, n = 0, len(m)
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
        if depth == 0 and (word_at(m, i, "class") or word_at(m, i, "struct")):
            kw = "class" if m[i] == "c" else "struct"
            j = i + len(kw)
            while j < n and m[j] in WS:
                j += 1
            e = j
            while e < n and m[e] in IDENT:
                e += 1
            name = m[j:e]
            if name:
                body_open = _class_body_open(m, e)
                if body_open != -1 and not _is_template_before(m, i):
                    body_close = match_brace(m, body_open)
                    if body_close != -1:
                        results.append((name, body_open, body_close))
                        i = body_close + 1
                        continue
        i += 1
    return results


# --------------------------------------------------------------------------- #
# Inline-body discovery within a class body
# --------------------------------------------------------------------------- #
def find_inline_bodies(m, lo, hi):
    """Yield (unit_start, brace_open, brace_close) for each '{' at the class
    body's top level (relative depth 0). Nested braces are jumped over."""
    bodies = []
    unit_start = lo
    i = lo
    while i < hi:
        c = m[i]
        if c == "{":
            bclose = match_brace(m, i)
            if bclose == -1 or bclose >= hi:
                break
            bodies.append((unit_start, i, bclose))
            unit_start = bclose + 1
            i = bclose + 1
            continue
        if c == ";":
            unit_start = i + 1
        i += 1
    return bodies


FORBIDDEN = ("template", "static", "constexpr", "consteval",
             "friend", "inline", "operator", "using", "typedef")

ACCESS_LABEL = re.compile(r"^\s*(?:public|private|protected)\s*:\s*")


def _quals_ok(quals_masked):
    """True if the region between ')' and '{' is only const/noexcept/
    noexcept(...)/override/final/&/&& and whitespace."""
    s = re.sub(r"noexcept\s*\([^()]*\)", " ", quals_masked)
    for w in ("const", "noexcept", "override", "final"):
        s = re.sub(r"\b" + w + r"\b", " ", s)
    s = s.replace("&&", " ").replace("&", " ")
    return s.strip() == ""


class Candidate:
    __slots__ = ("cls", "name", "ret", "params", "quals", "body",
                 "brace_open", "brace_close")


def evaluate(text, m, cls, unit_start, brace_open, brace_close):
    """Return a Candidate if the inline body is a movable plain member function,
    else None."""
    sig_m = m[unit_start:brace_open]

    p_rel = first_toplevel_paren(sig_m)
    if p_rel == -1:
        return None
    p = unit_start + p_rel
    pclose = match_paren(m, p)
    if pclose == -1 or pclose >= brace_open:
        return None

    # Name and its byte span (rightmost identifier before the paren).
    namep = p
    while namep > unit_start and m[namep - 1] in WS:
        namep -= 1
    name_end = namep
    name_start = name_end
    while name_start > unit_start and m[name_start - 1] in IDENT:
        name_start -= 1
    name = text[name_start:name_end]
    if not name or name in ("operator",):
        return None
    if name_start > unit_start and m[name_start - 1] == "~":
        return None  # destructor

    # Return type is everything between the unit start and the name. Skip the
    # leading run of comments/whitespace (comments are blanked to spaces in the
    # mask) and any access-specifier label so section banners like "// Getters"
    # stay in the .h and never leak into the emitted .cpp definition.
    k = 0
    span_m = m[unit_start:name_start]
    while k < len(span_m) and span_m[k] in WS:
        k += 1
    ret = text[unit_start + k:name_start]
    ret_m = span_m[k:]
    lbl = ACCESS_LABEL.match(ret)
    if lbl:
        ret = ret[lbl.end():]
        ret_m = ret_m[lbl.end():]
    ret = ret.strip()
    if not ret:
        return None  # constructor (no return type)
    # A stray non-'::' colon or leftover brace/semicolon signals a mis-parse.
    if any(ch in ret for ch in "{};") or ":" in ret.replace("::", ""):
        return None

    for w in FORBIDDEN:
        if re.search(r"\b" + w + r"\b", ret_m):
            return None

    quals_m = m[pclose + 1:brace_open]
    if "->" in quals_m:            # trailing return type — leave inline
        return None
    if not _quals_ok(quals_m):
        return None

    c = Candidate()
    c.cls = cls
    c.name = name
    c.ret = re.sub(r"^\s*virtual\b\s*", "", ret).strip()
    c.params = _strip_param_defaults(text[p + 1:pclose], m[p + 1:pclose])
    c.quals = _clean_quals(text[pclose + 1:brace_open])
    c.body = text[brace_open:brace_close + 1]
    c.brace_open = brace_open
    c.brace_close = brace_close
    return c


def _strip_param_defaults(orig, m_slice):
    """Drop top-level ` = default` from each parameter (defaults stay in the .h
    declaration). orig and m_slice are the same length; m_slice is masked."""
    if not m_slice.strip():
        return ""
    segs, depth, start = [], 0, 0
    for i, ch in enumerate(m_slice):
        if ch in "([{<":
            depth += 1
        elif ch in ")]}>":
            if depth > 0:
                depth -= 1
        elif ch == "," and depth == 0:
            segs.append((start, i))
            start = i + 1
    segs.append((start, len(m_slice)))

    out = []
    for s, e in segs:
        seg_o, seg_m = orig[s:e], m_slice[s:e]
        d, eq = 0, -1
        for k, ch in enumerate(seg_m):
            if ch in "([{<":
                d += 1
            elif ch in ")]}>":
                if d > 0:
                    d -= 1
            elif ch == "=" and d == 0:
                eq = k
                break
        if eq != -1:
            seg_o = seg_o[:eq]
        out.append(seg_o.strip())
    return ", ".join(out)


def _clean_quals(q):
    q = re.sub(r"\boverride\b", " ", q)
    q = re.sub(r"\bfinal\b", " ", q)
    return re.sub(r"\s+", " ", q).strip()


# --------------------------------------------------------------------------- #
# File processing
# --------------------------------------------------------------------------- #
def collect_candidates(text):
    m = mask(text)
    out = []
    for cls, bo, bc in find_toplevel_classes(m):
        for unit_start, brace_open, brace_close in find_inline_bodies(m, bo + 1, bc):
            cand = evaluate(text, m, cls, unit_start, brace_open, brace_close)
            if cand:
                out.append(cand)
    return out


def rewrite_header(text, cands):
    """Replace each inline body span '{'..'}' with ';' (applied right-to-left so
    earlier offsets stay valid)."""
    result = text
    for c in sorted(cands, key=lambda c: c.brace_open, reverse=True):
        result = result[:c.brace_open] + ";" + result[c.brace_close + 1:]
    return result


def emit_definitions(cands, nl):
    """Out-of-line definitions for the sibling .cpp, in .h source order.
    method_order --fix reorders these into declaration order afterward."""
    defs = []
    for c in cands:
        head = "%s %s::%s(%s)" % (c.ret, c.cls, c.name, c.params)
        if c.quals:
            head += " " + c.quals
        body = c.body.replace("\r\n", "\n").replace("\r", "\n")
        if nl != "\n":
            body = body.replace("\n", nl)
        defs.append(head + " " + body)
    return defs


def move(h_text, cpp_text):
    """Move qualifying inline member definitions out of `h_text` into `cpp_text`.
    Returns (new_h, new_cpp). Pure: no file I/O. Text is '\\n'-normalized."""
    cands = collect_candidates(h_text)
    if not cands:
        return h_text, cpp_text
    new_h = rewrite_header(h_text, cands)
    defs = emit_definitions(cands, "\n")
    if not cpp_text.endswith("\n"):
        cpp_text += "\n"
    return new_h, cpp_text + "\n" + "\n".join(defs) + "\n"

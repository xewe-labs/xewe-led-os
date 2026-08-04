#!/usr/bin/env python3
"""Post-clang-format pass that aligns class/struct declarations into columns.

Reproduces the project header style. Four columns, whose positions are computed
across the whole file so every declaration lines up:

    <type> .... <name> .... (<params>) .... <trailing>;      # method
    col1        col2        col3            col4
    <type> .... <name> .... = <value>;                       # member w/ default
    <type> .... <name> .... [<dim>];                         # array member
    <type> .... <name>;                                      # plain member

Rules:
  * Column positions = longest content in that column across the file + GAP.
  * Methods with more than one parameter are split one-parameter-per-line,
    continuation lines aligned under the '(' column; trailing tags land in col4.

Runs AFTER clang-format. Only touches lines whose enclosing scope is a
class/struct (function bodies, enums, namespaces, aggregate initializers are
left alone). Idempotent.
"""

import re

from fmtlib import split_params, strip_comment, top_level_index

GAP = 1  # spaces between one column's longest content and the next column
PARAM_OUTLIER_GAP = 16  # a param wider than the rest by more than this opts out
                        # of the type/name/= columns (renders single-spaced)

SKIP_PREFIXES = ("using", "typedef", "friend", "static_assert", "template",
                 "public:", "private:", "protected:", "//", "/*", "*", "#",
                 "return", "}")  # '}' is a scope closer, never a decl start —
                                 # its unmatched brace would otherwise make the
                                 # brace-aware collector over-consume.


def pad_to(s: str, col: int) -> str:
    return s + (" " * (col - len(s)) if len(s) < col else " ")


def top_level_assign(s: str) -> int:
    """Index of a top-level standalone '=' — an initializer or defaulted-decl
    marker — skipping the '=' inside operator=, ==, +=, etc. Bodies are
    whitespace-normalized, so a real assignment reads ' = ' (space on both sides),
    while operator names have no surrounding space. Angle brackets are not tracked
    (a '=' never sits inside template args here), which also spares operator</>
    from skewing the depth count."""
    depth = 0
    for i, c in enumerate(s):
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        elif (depth == 0 and c == "=" and 0 < i < len(s) - 1
              and s[i - 1] == " " and s[i + 1] == " "):
            return i
    return -1


def split_type_name(decl: str):
    toks = decl.split()
    return " ".join(toks[:-1]), toks[-1]


def complete_decl(buf: str) -> bool:
    """A collected buffer is a complete unit once its parens AND braces balance
    and it ends with ';' (a declaration) or '}' (an inline body: a constructor
    with a member-init list, or an inline function). Brace-awareness is what
    stops an empty-bodied ctor (`) {}`, no internal ';') from running past its
    own body into the next declaration."""
    b = buf.rstrip()
    return (buf.count("(") == buf.count(")")
            and buf.count("{") == buf.count("}")
            and b.endswith((";", "}")))


def brace_kind(line: str) -> str:
    if re.search(r"\benum\b", line):  # 'enum class' must not match 'class'
        return "enum"
    if re.search(r"\bclass\b", line):
        return "class"
    if re.search(r"\bstruct\b", line):
        return "struct"
    if re.search(r"\bnamespace\b", line):
        return "namespace"
    return "block"


def parse_decl(indent: str, body: str):
    """body: single-line, no indent, ends with ';'."""
    body = body[:-1].strip()  # drop ';'
    lp = top_level_index(body, "(")
    eq = top_level_assign(body)
    # A top-level '=' before the first top-level '(' marks a member initializer
    # whose value contains a call/cast, e.g. `uint8_t t = static_cast<uint8_t>(x)`
    # — not a function declaration. (top_level_assign ignores operator=, so
    # `operator=(...)` still parses as a method.) Handle members here.
    if lp == -1 or (eq != -1 and eq < lp):
        if eq != -1:
            left, init = body[:eq].strip(), body[eq + 1:].strip()
            typ, name = split_type_name(left)
            return {"kind": "member", "indent": indent, "type": typ,
                    "name": name, "col3": ("=", init)}
        lb = top_level_index(body, "[")
        lc = top_level_index(body, "{")
        if lb != -1 and (lc == -1 or lb < lc):
            # array member, possibly with a trailing brace-init: `result[16]{}`
            left, rest = body[:lb].strip(), body[lb:].strip()
            typ, name = split_type_name(left)
            return {"kind": "member", "indent": indent, "type": typ,
                    "name": name, "col3": ("[", rest)}
        if lc != -1:
            # brace-init member: `abort{false}`
            left, init = body[:lc].strip(), body[lc:].strip()
            typ, name = split_type_name(left)
            return {"kind": "member", "indent": indent, "type": typ,
                    "name": name, "col3": ("{", init)}
        if len(body.split()) < 2:
            return None
        typ, name = split_type_name(body)
        return {"kind": "member", "indent": indent, "type": typ,
                "name": name, "col3": None}

    depth, rp = 0, -1
    for i, c in enumerate(body):
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                rp = i
                break
    if rp == -1:
        return None
    before = body[:lp].strip()
    trailing = body[rp + 1:].strip()
    toks = before.split()
    if not toks:
        return None
    if len(toks) < 2:
        # No return type before the name: a constructor/destructor. Reject if
        # another paren group follows the first (e.g. a function-pointer member
        # `void (*cb)(int)`, whose leading token is a lone type, not a ctor).
        if "(" in trailing:
            return None
        typ, name = "", before
    else:
        typ, name = split_type_name(before)
    return {"kind": "method", "indent": indent, "type": typ, "name": name,
            "params": split_params(body[lp + 1:rp]),
            "trailing": trailing}


def collect(lines):
    """Return list of (start, end, parsed-decl) for class/struct-body lines.

    Each record carries a 'scope' id identifying its enclosing class/struct body,
    so member-initializer columns can be computed per scope (see assign_init_cols).
    """
    records, stack, scope_stack, scope_id, i = [], [], [], 0, 0
    while i < len(lines):
        line = lines[i]
        code, comment = strip_comment(line)
        stripped = code.strip()
        top = stack[-1] if stack else "global"
        consumed = 1

        starts_decl = stripped.endswith(";") or (
            "(" in code and code.count("(") > code.count(")"))
        # Braces on a line are tolerated for: function declarations with a brace
        # default argument (`set_mode(... = {})`, has '(') and brace-init members
        # (`abort{false};`, `result[16]{};`, no '(' and no scope keyword). Inline
        # enums/unions/classes are still excluded via the keyword test; inline
        # function bodies are filtered by starts_decl (they end in '}' with
        # balanced parens, so starts_decl is False).
        has_brace = "{" in code or "}" in code
        no_scope_kw = not re.search(r"\b(enum|class|struct|union|namespace)\b", code)
        brace_ok = ("(" in code) or (not has_brace) or no_scope_kw
        if (top in ("class", "struct") and stripped
                and brace_ok
                and not stripped.startswith(SKIP_PREFIXES)
                and starts_decl):
            buf, last_comment, j = code, comment, i
            while not complete_decl(buf) and j + 1 < len(lines):
                j += 1
                nxt_code, nxt_comment = strip_comment(lines[j])
                buf += " " + nxt_code.strip()
                last_comment = nxt_comment or last_comment
            if complete_decl(buf) and buf.rstrip().endswith("}"):
                # Inline-body construct (ctor with a member-init list, or an
                # inline function). align_decls leaves function bodies alone, so
                # record nothing — but consume the whole span. Otherwise a member-
                # init continuation line (': Base(...)', unbalanced paren) would be
                # re-collected as a standalone decl and merged with the next one.
                consumed = j - i + 1
            elif complete_decl(buf):
                # Indent is derived from class-nesting depth, not the line's
                # leading whitespace: a constructor/destructor renders with an
                # empty type, so its whole leading run up to the name column is
                # padding that would otherwise be recaptured as indent and grow
                # name_col by GAP every run. clang-format (IndentWidth 4,
                # NamespaceIndentation None) puts members at 4*depth.
                depth_cls = sum(1 for k in stack if k in ("class", "struct"))
                indent = "    " * depth_cls
                parsed = parse_decl(indent, " ".join(buf.split()))
                if parsed:
                    parsed["comment"] = last_comment
                    parsed["scope"] = next(
                        (s for s in reversed(scope_stack) if s is not None), None)
                    records.append((i, j, parsed))
                    consumed = j - i + 1

        kind = brace_kind(line)
        for c in line:
            if c == "{":
                stack.append(kind)
                if kind in ("class", "struct"):
                    scope_id += 1
                    scope_stack.append(scope_id)
                else:
                    scope_stack.append(None)
            elif c == "}" and stack:
                stack.pop()
                if scope_stack:
                    scope_stack.pop()
        i += consumed
    return records


def split_param(p):
    """(type, name, default-or-None) for one parameter. Unnamed params and the
    variadic `...` come back with an empty name."""
    eq = top_level_index(p, "=")
    if eq != -1:
        lhs, default = p[:eq].strip(), p[eq + 1:].strip()
    else:
        lhs, default = p.strip(), None
    toks = lhs.split()
    if len(toks) < 2:
        return lhs, "", default
    return " ".join(toks[:-1]), toks[-1], default


def outlier_column(widths):
    """Largest width, dropping high outliers: a value separated from the next
    smaller one by more than PARAM_OUTLIER_GAP opts out (and every value above
    it), so a single giant param does not drag the whole column right."""
    ws = sorted(widths)
    i = len(ws) - 1
    while i >= 1 and ws[i] - ws[i - 1] > PARAM_OUTLIER_GAP:
        i -= 1
    return ws[i]


def align_param_defaults(params):
    """Within one method's parameter list, align three sub-columns: the parameter
    type, the parameter name, and the ` = default`. Each column ignores lone
    giant outliers (see outlier_column), which render single-spaced and overflow
    past the column. Params arrive whitespace-normalized (collect joins with
    single spaces), so re-parsing is idempotent."""
    parsed = [split_param(p) for p in params]
    named_types = [len(t) for t, n, _ in parsed if n]
    if not named_types:
        return params
    type_col = outlier_column(named_types)

    def name_start(t):
        return type_col if len(t) <= type_col else len(t)

    name_ends = [name_start(t) + 1 + len(n) if n else len(t)
                 for t, n, _ in parsed]
    eq_src = [name_ends[k] for k, (t, n, d) in enumerate(parsed)
              if d is not None and n]
    eq_col = outlier_column(eq_src) if eq_src else 0

    out = []
    for (t, n, d), end in zip(parsed, name_ends):
        if not n:
            out.append(t if d is None else t + " = " + d)
            continue
        s = t.ljust(name_start(t)) + " " + n
        if d is not None:
            s = (s.ljust(eq_col) if end <= eq_col else s) + " = " + d
        out.append(s)
    return out


def compute_columns(records):
    name_col = max(len(d["indent"] + d["type"]) for _, _, d in records) + GAP
    paren_col = name_col + max(len(d["name"]) for _, _, d in records) + GAP

    end = paren_col
    for _, _, d in records:
        if d["kind"] != "method" or not d["trailing"]:
            continue
        params = d["params"]
        if len(params) <= 1:
            inner = params[0] if params else ""
            e = paren_col + len("(" + inner + ")")
        else:
            e = paren_col + 1 + len(params[-1]) + 1
        end = max(end, e)
    trail_col = end + GAP
    return name_col, paren_col, trail_col


def assign_init_cols(records, name_col, paren_col):
    """Choose each member's `=`/`{`/`[` initializer column, per enclosing scope.

    A scope that also declares methods keeps its members' initializers on the
    global paren_col, so a member `= value` lines up with the method `(` column
    (the unified class look). A data-only scope (e.g. a nested POD struct) has no
    method paren column to unify with, so its initializers get a LOCAL column just
    past the scope's longest initialized member name — avoiding the wide gap a
    long-method-named outer class would otherwise force onto a short-named struct.
    """
    scopes = {}
    for _, _, d in records:
        scopes.setdefault(d.get("scope"), []).append(d)
    for members in scopes.values():
        has_method = any(d["kind"] == "method" for d in members)
        col3 = [d for d in members if d["kind"] == "member" and d.get("col3")]
        if not col3:
            continue
        col = paren_col if has_method \
            else name_col + max(len(d["name"]) for d in col3) + GAP
        for d in col3:
            d["init_col"] = col


def render(d, name_col, paren_col, trail_col):
    comment = d.get("comment", "")

    def finish(s):
        return s + " " + comment if comment else s

    if d["kind"] == "member":
        line = pad_to(d["indent"] + d["type"], name_col) + d["name"]
        if d["col3"]:
            k, payload = d["col3"]
            col = d.get("init_col", paren_col)
            line = pad_to(line, col) + ("= " + payload if k == "=" else payload)
        return finish(line + ";")

    base = pad_to(d["indent"] + d["type"], name_col) + d["name"]
    params, trailing = d["params"], d["trailing"]
    if len(params) <= 1:
        inner = params[0] if params else ""
        line = pad_to(base, paren_col) + "(" + inner + ")"
        if trailing:
            line = pad_to(line, trail_col) + trailing
        return finish(line + ";")

    cont = " " * (paren_col + 1)
    out = [pad_to(base, paren_col) + "(" + params[0] + ","]
    out += [cont + p + "," for p in params[1:-1]]
    last = cont + params[-1] + ")"
    if trailing:
        last = pad_to(last, trail_col) + trailing
    out.append(finish(last + ";"))
    return "\n".join(out)


NUM_RE = re.compile(r"-?\d+$")


def align_tables(lines):
    """Column-align rows of aggregate-initializer tables:

        <decl> = {
            {c0, c1, c2},
            ...
        };

    Numeric columns are right-aligned, others left-aligned; the last column
    is not padded. Runs before declaration alignment; row lines contain '{'
    so the declaration collector ignores them.
    """
    out, i = [], 0
    while i < len(lines):
        line = lines[i]
        if line.rstrip().endswith("= {"):
            j = i + 1
            rows, ok = [], True
            while j < len(lines):
                stripped = lines[j].strip()
                if stripped.startswith("};") or stripped == "}":
                    break
                code, comment = strip_comment(lines[j])
                body = code.strip()
                if not (body.startswith("{") and body.rstrip(",").endswith("}")):
                    ok = False
                    break
                indent = re.match(r"\s*", lines[j]).group()
                inner = body.rstrip(",").strip()[1:-1]
                rows.append((indent, split_params(inner), comment))
                j += 1
            if ok and rows and j < len(lines):
                ncol = max(len(r[1]) for r in rows)
                widths, numeric = [], []
                for c in range(ncol):
                    vals = [r[1][c] for r in rows if c < len(r[1])]
                    widths.append(max(len(v) for v in vals))
                    numeric.append(all(NUM_RE.fullmatch(v) for v in vals))
                out.append(line)
                for indent, fields, comment in rows:
                    cells = []
                    for c, v in enumerate(fields):
                        last = c == len(fields) - 1
                        if last:
                            cells.append(v)
                        elif numeric[c]:
                            cells.append(v.rjust(widths[c]))
                        else:
                            cells.append(v.ljust(widths[c]))
                    row = indent + "{" + ", ".join(cells) + "},"
                    out.append(row + " " + comment if comment else row)
                i = j
                continue
        out.append(line)
        i += 1
    return out


def process(text: str) -> str:
    lines = align_tables(text.split("\n"))
    records = collect(lines)
    if not records:
        return "\n".join(lines)
    for _, _, d in records:
        if d["kind"] == "method" and len(d["params"]) > 1:
            d["params"] = align_param_defaults(d["params"])
    name_col, paren_col, trail_col = compute_columns(records)
    assign_init_cols(records, name_col, paren_col)

    out, i, ri = [], 0, 0
    while i < len(lines):
        if ri < len(records) and records[ri][0] == i:
            start, endl, d = records[ri]
            out.append(render(d, name_col, paren_col, trail_col))
            i = endl + 1
            ri += 1
        else:
            out.append(lines[i])
            i += 1
    return "\n".join(out)

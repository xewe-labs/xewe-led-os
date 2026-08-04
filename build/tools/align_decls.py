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
import sys

GAP = 1  # spaces between one column's longest content and the next column
PARAM_OUTLIER_GAP = 16  # a param wider than the rest by more than this opts out
                        # of the type/name/= columns (renders single-spaced)

SKIP_PREFIXES = ("using", "typedef", "friend", "static_assert", "template",
                 "public:", "private:", "protected:", "//", "/*", "*", "#",
                 "return")


def pad_to(s: str, col: int) -> str:
    return s + (" " * (col - len(s)) if len(s) < col else " ")


def top_level_index(s: str, ch: str) -> int:
    depth = 0
    for i, c in enumerate(s):
        if depth == 0 and c == ch:
            return i
        if c in "<([{":
            depth += 1
        elif c in ">)]}":
            depth -= 1
    return -1


def split_type_name(decl: str):
    toks = decl.split()
    return " ".join(toks[:-1]), toks[-1]


def split_params(s: str):
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


def strip_comment(line: str):
    """Split off a trailing // comment. Returns (code, comment-or-empty)."""
    idx = line.find("//")
    if idx == -1:
        return line, ""
    return line[:idx].rstrip(), line[idx:].strip()


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
    if lp == -1:
        eq = top_level_index(body, "=")
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
    """Return list of (start, end, parsed-decl) for class/struct-body lines."""
    records, stack, i = [], [], 0
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
            while not (buf.rstrip().endswith(";")
                       and buf.count("(") == buf.count(")")) and j + 1 < len(lines):
                j += 1
                nxt_code, nxt_comment = strip_comment(lines[j])
                buf += " " + nxt_code.strip()
                last_comment = nxt_comment or last_comment
            if buf.rstrip().endswith(";") and buf.count("(") == buf.count(")"):
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
                    records.append((i, j, parsed))
                    consumed = j - i + 1

        kind = brace_kind(line)
        for c in line:
            if c == "{":
                stack.append(kind)
            elif c == "}" and stack:
                stack.pop()
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


def assign_init_cols(records, lines, name_col):
    """Give each member's `=`/`{`/`[` initializer a LOCAL column, per contiguous
    run of member declarations, instead of the global method paren_col. A run is a
    maximal sequence of member records at the same nesting indent with no method
    record and no scope brace ({ or }) between them. Within a run the initializer
    column sits just past the longest name among that run's initialized members,
    so a nested struct (short names) doesn't inherit the wide paren gap forced by
    a long-method-named outer class."""
    n, i = len(records), 0
    while i < n:
        d = records[i][2]
        if d["kind"] != "member":
            i += 1
            continue
        indent, group, prev_end = d["indent"], [i], records[i][1]
        j = i + 1
        while j < n:
            sj, ej, dj = records[j]
            if dj["kind"] != "member" or dj["indent"] != indent:
                break
            if any(("{" in lines[k] or "}" in lines[k])
                   for k in range(prev_end + 1, sj)):
                break
            group.append(j)
            prev_end = ej
            j += 1
        widths = [len(records[g][2]["name"]) for g in group
                  if records[g][2].get("col3")]
        if widths:
            init_col = name_col + max(widths) + GAP
            for g in group:
                records[g][2]["init_col"] = init_col
        i = j


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
    assign_init_cols(records, lines, name_col)

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


def main(argv):
    if len(argv) < 2:
        print("usage: align_decls.py <file.h> [more.h ...]", file=sys.stderr)
        return 2
    for path in argv[1:]:
        with open(path, encoding="utf-8") as f:
            src = f.read()
        new = process(src)
        if new != src:
            with open(path, "w", encoding="utf-8", newline="\n") as f:
                f.write(new)
            print(f"aligned {path}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

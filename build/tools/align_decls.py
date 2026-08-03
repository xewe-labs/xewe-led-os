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
        if c in "<([":
            depth += 1
        elif c in ">)]":
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
        if c in "<([":
            depth += 1
        elif c in ">)]":
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
        br = body.find("[")
        if br != -1:
            left, dim = body[:br].strip(), body[br:].strip()
            typ, name = split_type_name(left)
            return {"kind": "member", "indent": indent, "type": typ,
                    "name": name, "col3": ("[", dim)}
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
    if len(before.split()) < 2:  # need type + name
        return None
    typ, name = split_type_name(before)
    return {"kind": "method", "indent": indent, "type": typ, "name": name,
            "params": split_params(body[lp + 1:rp]),
            "trailing": body[rp + 1:].strip()}


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
        if (top in ("class", "struct") and stripped
                and "{" not in code and "}" not in code
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
                indent = re.match(r"\s*", line).group()
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


def render(d, name_col, paren_col, trail_col):
    comment = d.get("comment", "")

    def finish(s):
        return s + " " + comment if comment else s

    if d["kind"] == "member":
        line = pad_to(d["indent"] + d["type"], name_col) + d["name"]
        if d["col3"]:
            k, payload = d["col3"]
            line = pad_to(line, paren_col) + ("= " + payload if k == "=" else payload)
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
    name_col, paren_col, trail_col = compute_columns(records)

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

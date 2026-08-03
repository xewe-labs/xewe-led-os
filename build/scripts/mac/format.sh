#!/usr/bin/env bash
set -euo pipefail

# format.sh — run clang-format then the align_decls.py / header_layout.py
# post-passes over sources.
#   ./format.sh [--check] [path ...]
# With no paths, formats <project>/src. --check exits 1 if anything would change.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
ALIGN_SCRIPT="${BUILD_ROOT}/tools/align_decls.py"
HEADER_SCRIPT="${BUILD_ROOT}/tools/header_layout.py"
METHOD_SCRIPT="${BUILD_ROOT}/tools/method_order.py"
INLINE_SCRIPT="${BUILD_ROOT}/tools/inline_move.py"
PARAM_SCRIPT="${BUILD_ROOT}/tools/param_split.py"
CTOR_SCRIPT="${BUILD_ROOT}/tools/ctor_brace.py"
DANGLE_SCRIPT="${BUILD_ROOT}/tools/dangling_close.py"
MODECFG_SCRIPT="${BUILD_ROOT}/tools/modeconfig_layout.py"
STYLE_FILE="${BUILD_ROOT}/tools/.clang-format"

CHECK=0
PATHS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --check|-c) CHECK=1; shift ;;
    *) PATHS+=("$1"); shift ;;
  esac
done

if [[ ${#PATHS[@]} -eq 0 ]]; then
  PATHS=("${PROJECT_ROOT}/src")
fi

if command -v clang-format >/dev/null 2>&1; then
  CLANG_FORMAT="clang-format"
else
  echo "clang-format not found on PATH. Install it (brew install clang-format)." >&2
  exit 1
fi

if command -v python3 >/dev/null 2>&1; then
  PY="python3"
elif command -v python >/dev/null 2>&1; then
  PY="python"
else
  echo "python not found on PATH." >&2
  exit 1
fi

TARGETS=()
for p in "${PATHS[@]}"; do
  if [[ -f "$p" ]]; then
    TARGETS+=("$p")
  elif [[ -d "$p" ]]; then
    while IFS= read -r f; do TARGETS+=("$f"); done \
      < <(find "$p" -type f \( -name '*.h' -o -name '*.cpp' \))
  else
    echo "Path not found: $p" >&2
  fi
done

if [[ ${#TARGETS[@]} -eq 0 ]]; then
  echo "No files to format."
  exit 0
fi

HEADERS=()
SOURCES=()
for f in "${TARGETS[@]}"; do
  case "$f" in
    *.h)   HEADERS+=("$f") ;;
    *.cpp) SOURCES+=("$f") ;;
  esac
done

rel_of() {
  "$PY" -c "import os,sys;print(os.path.relpath(os.path.abspath(sys.argv[1]),sys.argv[2]).replace(os.sep,'/'))" "$1" "$PROJECT_ROOT"
}

if [[ $CHECK -eq 1 ]]; then
  changed=()
  for f in "${TARGETS[@]}"; do
    # Temp keeps the source's extension so clang-format detects C++; the config
    # is passed explicitly via --style=file:<path>.
    tmp="$(dirname "$f")/.fmtcheck.$$.$(basename "$f")"
    cp "$f" "$tmp"
    "${CLANG_FORMAT}" -i --style="file:${STYLE_FILE}" "$tmp"
    rel="$(rel_of "$f")"
    if [[ "$f" == *.h ]]; then
      "$PY" "$ALIGN_SCRIPT" "$tmp" >/dev/null
      "$PY" "$HEADER_SCRIPT" --emit-path "$rel" "$tmp" >/dev/null
    elif [[ "$f" == *.cpp ]]; then
      # No sibling .h beside the mangled temp, so the cross-file include move is
      # skipped here; check still catches license/path/ordering drift.
      "$PY" "$HEADER_SCRIPT" --emit-path "$rel" "$tmp" >/dev/null 2>&1
    fi
    # Split multi-param definition signatures one-per-line and put ctor body
    # braces on their own line so a canonical file compares equal (clang-format
    # under ColumnLimit:0 does neither).
    "$PY" "$PARAM_SCRIPT" --fix "$tmp" >/dev/null 2>&1
    "$PY" "$CTOR_SCRIPT" --fix "$tmp" >/dev/null 2>&1
    # Dangle multi-line closers so a canonical file compares equal (clang-format
    # under ColumnLimit:0 glues them to the last arg line). On .h only inline
    # function bodies are affected.
    "$PY" "$DANGLE_SCRIPT" --fix "$tmp" >/dev/null 2>&1
    # Reproduce the ModeConfig shallow relayout on the temp so a correctly
    # formatted source compares equal (clang-format alone deep-indents tables).
    "$PY" "$MODECFG_SCRIPT" --fix "$tmp" >/dev/null 2>&1
    diff -q "$f" "$tmp" >/dev/null 2>&1 || changed+=("$f")
    rm -f "$tmp"
  done
  # Structural checks run on the real files (they read across the .h/.cpp pair,
  # which the per-file mangled temp copies above cannot represent). inline_move
  # --check catches un-moved inline .h bodies (clang-format leaves them and
  # align_decls skips the braced line, so the temp diff cannot see them).
  lint_fail=0
  if [[ ${#HEADERS[@]} -gt 0 ]]; then
    "$PY" "$INLINE_SCRIPT" --check "${HEADERS[@]}" || lint_fail=1
  fi
  if [[ ${#SOURCES[@]} -gt 0 ]]; then
    "$PY" "$METHOD_SCRIPT" --check "${SOURCES[@]}" || lint_fail=1
  fi
  if [[ ${#changed[@]} -gt 0 || $lint_fail -eq 1 ]]; then
    if [[ ${#changed[@]} -gt 0 ]]; then
      echo "Would reformat:"
      for f in "${changed[@]}"; do echo "  $f"; done
    fi
    exit 1
  fi
  echo "All files already formatted."
  exit 0
fi

# --- 1. Structural passes (run BEFORE clang-format) ---------------------------
# Move qualifying inline member-function bodies out of each .h into its sibling
# .cpp. This must precede method_order: an inline body has no depth-0 ';', so the
# member is invisible to method_order's header parser until it becomes a plain
# declaration here.
if [[ ${#HEADERS[@]} -gt 0 ]]; then
  echo "inline_move: ${#HEADERS[@]} header(s)..."
  "$PY" "$INLINE_SCRIPT" --fix "${HEADERS[@]}"
fi

# Reorder each .cpp's out-of-line definitions to match its sibling .h (now that
# the moved members participate in the .h declaration order).
if [[ ${#SOURCES[@]} -gt 0 ]]; then
  echo "method_order: ${#SOURCES[@]} source(s)..."
  "$PY" "$METHOD_SCRIPT" --fix "${SOURCES[@]}"
fi

# --- 2. Normalize whitespace/braces everywhere --------------------------------
echo "clang-format: ${#TARGETS[@]} file(s)..."
"${CLANG_FORMAT}" -i --style="file:${STYLE_FILE}" "${TARGETS[@]}"

# --- 3. Cosmetic passes (depend on clang-format's output) ---------------------
# .cpp first: a source may relocate third-party <...> includes into its sibling
# header, which the header pass below then re-normalizes.
if [[ ${#SOURCES[@]} -gt 0 ]]; then
  echo "header_layout: ${#SOURCES[@]} source(s)..."
  "$PY" "$HEADER_SCRIPT" --root "$PROJECT_ROOT" "${SOURCES[@]}"
fi

if [[ ${#HEADERS[@]} -gt 0 ]]; then
  echo "align_decls: ${#HEADERS[@]} header(s)..."
  "$PY" "$ALIGN_SCRIPT" "${HEADERS[@]}"
  echo "header_layout: ${#HEADERS[@]} header(s)..."
  "$PY" "$HEADER_SCRIPT" --root "$PROJECT_ROOT" "${HEADERS[@]}"
fi

# Split every multi-param function DEFINITION one parameter per line (clang-format
# under ColumnLimit:0 never wraps) and collapse the name-column padding, then put
# each constructor's body '{' on its own line (clang-format attaches it to the
# last member initializer). Both run on headers and sources.
echo "param_split: ${#TARGETS[@]} file(s)..."
"$PY" "$PARAM_SCRIPT" --fix "${TARGETS[@]}"
echo "ctor_brace: ${#TARGETS[@]} file(s)..."
"$PY" "$CTOR_SCRIPT" --fix "${TARGETS[@]}"

# Put multi-line bracket-group closers on their own line (clang-format under
# ColumnLimit:0 glues them onto the last argument line). Runs on headers too, but
# there only inside inline function bodies (declaration signatures stay glued —
# align_decls owns those). Before modeconfig_layout, which owns the ModeConfig
# table's own close line.
echo "dangling_close: ${#TARGETS[@]} file(s)..."
"$PY" "$DANGLE_SCRIPT" --fix "${TARGETS[@]}"

# Last: rewrite each ModeConfig table into the shallow layout clang-format
# would otherwise deep-indent.
if [[ ${#SOURCES[@]} -gt 0 ]]; then
  echo "modeconfig_layout: ${#SOURCES[@]} source(s)..."
  "$PY" "$MODECFG_SCRIPT" --fix "${SOURCES[@]}"
fi

echo "Done."

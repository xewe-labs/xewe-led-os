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
    diff -q "$f" "$tmp" >/dev/null 2>&1 || changed+=("$f")
    rm -f "$tmp"
  done
  # Method-order check runs on the real .cpp files (it reads each sibling .h,
  # so the mangled temp copies above can't be used here).
  method_fail=0
  if [[ ${#SOURCES[@]} -gt 0 ]]; then
    "$PY" "$METHOD_SCRIPT" --check "${SOURCES[@]}" || method_fail=1
  fi
  if [[ ${#changed[@]} -gt 0 || $method_fail -eq 1 ]]; then
    if [[ ${#changed[@]} -gt 0 ]]; then
      echo "Would reformat:"
      for f in "${changed[@]}"; do echo "  $f"; done
    fi
    exit 1
  fi
  echo "All files already formatted."
  exit 0
fi

echo "clang-format: ${#TARGETS[@]} file(s)..."
"${CLANG_FORMAT}" -i --style="file:${STYLE_FILE}" "${TARGETS[@]}"

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

# Reorder each .cpp's out-of-line definitions to match its sibling .h.
if [[ ${#SOURCES[@]} -gt 0 ]]; then
  echo "method_order: ${#SOURCES[@]} source(s)..."
  "$PY" "$METHOD_SCRIPT" --fix "${SOURCES[@]}"
fi

echo "Done."

#!/usr/bin/env bash
# add_license.sh — Append or prepend license lines to all .cpp and .h files recursively.
# Usage:
#   ./add_license.sh --dir /path/to/code --license /path/to/header.txt [--mode append|prepend] [--marker "SPDX-License-Identifier"] [--exts ".cpp,.h"] [--dry-run] [--backup-suffix .bak]
#
# Notes:
# - The license/header file should already be comment-formatted for C++ (e.g., lines starting with // or /* ... */).
# - By default this script APPENDS the header to each file. Use --mode prepend to put it at the top.
# - Files already containing the marker (default: "SPDX-License-Identifier") are skipped.
# - Requires bash.
set -euo pipefail

DIR=""
LICENSE_FILE=""
MODE="append"            # default to append based on user request
MARKER="SPDX-License-Identifier"
EXTS=".cpp,.h"
DRYRUN=0
BACKUP_SUFFIX=""

usage() {
  sed -n '1,30p' "$0"
}

# Parse args
while [[ $# -gt 0 ]]; do
  case "$1" in
    --dir) DIR="${2:-}"; shift 2;;
    --license) LICENSE_FILE="${2:-}"; shift 2;;
    --mode) MODE="${2:-}"; shift 2;;
    --marker) MARKER="${2:-}"; shift 2;;
    --exts) EXTS="${2:-}"; shift 2;;
    --dry-run) DRYRUN=1; shift;;
    --backup-suffix) BACKUP_SUFFIX="${2:-}"; shift 2;;
    -h|--help) usage; exit 0;;
    *) echo "[ERROR] Unknown option: $1" >&2; usage; exit 1;;
  esac
done

# Validate
if [[ -z "$DIR" || -z "$LICENSE_FILE" ]]; then
  echo "[ERROR] --dir and --license are required." >&2
  usage
  exit 1
fi
if [[ ! -d "$DIR" ]]; then
  echo "[ERROR] Directory not found: $DIR" >&2
  exit 1
fi
if [[ ! -f "$LICENSE_FILE" ]]; then
  echo "[ERROR] License/header file not found: $LICENSE_FILE" >&2
  exit 1
fi
if [[ "$MODE" != "append" && "$MODE" != "prepend" ]]; then
  echo "[ERROR] --mode must be 'append' or 'prepend'." >&2
  exit 1
fi

# Build find predicates for extensions
IFS=',' read -r -a _exts <<< "$EXTS"
FIND_EXPR=()
first=1
for ext in "${_exts[@]}"; do
  ext="${ext//[[:space:]]/}"
  [[ -z "$ext" ]] && continue
  [[ "${ext:0:1}" != "." ]] && ext=".$ext"
  if [[ $first -eq 1 ]]; then
    FIND_EXPR+=(-name "*${ext}")
    first=0
  else
    FIND_EXPR+=(-o -name "*${ext}")
  fi
done
if [[ ${#FIND_EXPR[@]} -eq 0 ]]; then
  echo "[ERROR] No valid extensions parsed from --exts." >&2
  exit 1
fi

# Read header first line for optional extra marker match
HEADER_FIRST_LINE="$(head -n 1 "$LICENSE_FILE" | tr -d '\r')"

processed=0
skipped=0
errors=0

# Export vars for subshells if needed
export LC_ALL=C

while IFS= read -r -d '' file; do
  # Skip non-text files (best-effort)
  if ! grep -Iq . "$file"; then
    echo "[SKIP] Binary-like file: $file"
    ((skipped++)) || true
    continue
  fi

  # Marker detection
  if [[ -n "$MARKER" ]]; then
    if grep -Fq -- "$MARKER" "$file"; then
      echo "[SKIP] Marker found in: $file"
      ((skipped++)) || true
      continue
    fi
  fi
  # Also skip if header first line already present
  if [[ -n "$HEADER_FIRST_LINE" ]] && grep -Fq -- "$HEADER_FIRST_LINE" "$file"; then
    echo "[SKIP] Header already present: $file"
    ((skipped++)) || true
    continue
  fi

  if [[ $DRYRUN -eq 1 ]]; then
    echo "[DRY-RUN] Would ${MODE} header in: $file"
    continue
  fi

  # Backup if requested
  if [[ -n "$BACKUP_SUFFIX" ]]; then
    cp -p -- "$file" "${file}${BACKUP_SUFFIX}" || true
  fi

  tmp="$(mktemp)"
  if [[ "$MODE" == "prepend" ]]; then
    # Prepend header + one newline, then original content
    { cat -- "$LICENSE_FILE"; printf "\n"; cat -- "$file"; } > "$tmp"
  else
    # Append header, ensuring a single blank line separation
    { cat -- "$file"; printf "\n"; cat -- "$LICENSE_FILE"; printf "\n"; } > "$tmp"
  fi

  # Overwrite atomically
  if mv -- "$tmp" "$file"; then
    echo "[OK] Modified: $file"
    ((processed++)) || true
  else
    echo "[ERROR] Could not write: $file" >&2
    rm -f -- "$tmp" || true
    ((errors++)) || true
  fi
done < <(find "$DIR" -type f \( "${FIND_EXPR[@]}" \) -print0)

echo
echo "Done. Modified: $processed, Skipped: $skipped, Errors: $errors"
exit 0

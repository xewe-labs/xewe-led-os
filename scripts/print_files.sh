#!/usr/bin/env bash
#
# dump.sh — print every path (relative to START_DIR) and the contents of files

# ─── Configuration ────────────────────────────────────────────────────────────

# Change this to whatever default directory you like:
DEFAULT_START_DIR="."

# ─── Parse arguments ──────────────────────────────────────────────────────────

# If you pass a directory as the first argument, use that; otherwise use default.
START_DIR="${1:-$DEFAULT_START_DIR}"

# ─── Enter the start directory ─────────────────────────────────────────────────

if ! cd "$START_DIR"; then
  echo "Error: cannot cd into '$START_DIR'" >&2
  exit 1
fi

# ─── Traverse and dump ────────────────────────────────────────────────────────

# Use NUL-delimited output so we handle all possible filenames safely
find . -print0 | while IFS= read -r -d '' path; do
  # strip leading "./" so paths read nicely
  rel="${path#./}"

  # print the (relative) path
  echo "$rel"

  # if it’s a regular file, dump its contents
  if [ -f "$path" ]; then
    cat "$path"
    echo    # extra blank line between files
  fi
done

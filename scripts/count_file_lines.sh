#!/bin/sh
# count_lines.sh — per-file line counts + grand total
# Usage: ./count_lines.sh [DIRECTORY]
# If no directory is given, it defaults to the current directory.

DIR="${1:-.}"

if [ ! -d "$DIR" ]; then
  echo "Error: '$DIR' is not a directory." >&2
  echo "Usage: $0 [DIRECTORY]" >&2
  exit 1
fi

# For each regular file, run `wc -l` individually, then sum the first column.
# This handles spaces in filenames and always prints a total (even for one file).
# Note: `wc -l` counts newline characters; a last line without a trailing newline
# may not be included in the count.
find "$DIR" -type f -exec wc -l {} \; 2>/dev/null | awk '
  { total += $1; print }
  END { printf "%8d total\n", total }
'

#!/bin/sh
# compare_dirs_by_name.sh
# Usage: ./compare_dirs_by_name.sh /path/to/dir1 /path/to/dir2 [output.csv]
# Outputs CSV columns: fname,path_1_lc,path_2_lc,difference
# - difference = path_1_lc - path_2_lc
# - Files are matched by *filename only* (basename). If multiple files with the
#   same name exist in a tree, their line counts are SUMMED for that name.
# - If a file exists only in one directory, the other count and difference stay empty.

set -eu

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
  echo "Usage: $0 DIR1 DIR2 [OUTPUT.csv]" >&2
  exit 1
fi

DIR1=$1
DIR2=$2
OUT=${3:-}

[ -d "$DIR1" ] || { echo "Not a directory: $DIR1" >&2; exit 1; }
[ -d "$DIR2" ] || { echo "Not a directory: $DIR2" >&2; exit 1; }

# mktemp fallback for systems without plain mktemp(1)
mktemp_wrap() {
  mktemp 2>/dev/null || mktemp -t compare_dirs_by_name.XXXXXX
}

TAB="$(printf '\t')"
T1="$(mktemp_wrap)"
T2="$(mktemp_wrap)"
BODY_TSV="$(mktemp_wrap)"
BODY_CSV="$(mktemp_wrap)"

cleanup() {
  rm -f "$T1" "$T2" "$BODY_TSV" "$BODY_CSV"
}
trap cleanup EXIT INT HUP TERM

# Collect: produce "name<TAB>sum_of_line_counts" for a directory
collect_dir() {
  DIR="$1"
  OUTFILE="$2"
  # Use find -print0 to safely handle spaces/newlines in paths
  find "$DIR" -type f -print0 \
  | while IFS= read -r -d '' f; do
      name=${f##*/}                         # basename only
      # wc -l prints just the number when used with input redirection
      lc=$(wc -l < "$f" 2>/dev/null | awk '{print $1}')
      [ -n "${lc:-}" ] || lc=0
      # We separate fields with TAB in intermediates
      printf '%s\t%s\n' "$name" "$lc"
    done \
  | awk -F '\t' '{sum[$1]+=$2} END{for (k in sum) printf "%s\t%d\n", k, sum[k]}' \
  | LC_ALL=C sort -t "$TAB" -k1,1 > "$OUTFILE"
}

collect_dir "$DIR1" "$T1"
collect_dir "$DIR2" "$T2"

# Merge by filename (tab-delimited), compute difference when both present
awk -F '\t' -v OFS='\t' '
  FNR==NR { a[$1]=$2; keys[$1]=1; next }
           { b[$1]=$2; keys[$1]=1 }
  END {
    for (k in keys) {
      p1 = (k in a) ? a[k] : ""
      p2 = (k in b) ? b[k] : ""
      diff = (p1 != "" && p2 != "") ? (p1 - p2) : ""
      printf "%s\t%s\t%s\t%s\n", k, p1, p2, diff
    }
  }
' "$T1" "$T2" \
| LC_ALL=C sort -t "$TAB" -k1,1 > "$BODY_TSV"

# Convert TSV to CSV with proper quoting of the filename column
awk -F '\t' '
  BEGIN { OFS="," }
  function csvq(s){ gsub(/"/, "\"\"", s); return "\"" s "\"" }
  { print csvq($1), $2, $3, $4 }
' "$BODY_TSV" > "$BODY_CSV"

# Output with header
if [ -n "$OUT" ]; then
  {
    echo "fname,path_1_lc,path_2_lc,difference"
    cat "$BODY_CSV"
  } > "$OUT"
  echo "Wrote CSV to: $OUT"
else
  echo "fname,path_1_lc,path_2_lc,difference"
  cat "$BODY_CSV"
fi

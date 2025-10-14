#!/bin/sh
# copy-structure.sh
# Replicates the directory structure of SRC into DST, with empty files.

set -eu

usage() {
  echo "Usage: $0 <SRC> <DST>" >&2
  exit 1
}

[ "${1:-}" ] && [ "${2:-}" ] || usage

SRC=$1
DST=$2

# Normalize to remove trailing slashes (cosmetic).
case "$SRC" in
  */) SRC=${SRC%/} ;;
esac
case "$DST" in
  */) DST=${DST%/} ;;
esac

# Basic checks
[ -d "$SRC" ] || { echo "Source is not a directory: $SRC" >&2; exit 2; }
# Create destination root
mkdir -p "$DST"

# 1) Create all directories
# For each directory under SRC, create the corresponding directory under DST.
find "$SRC" -type d -exec sh -c '
  src=$1; dst=$2; p=$3
  if [ "$p" = "$src" ]; then
    # top-level: already ensured
    :
  else
    rel=${p#"$src"/}
    mkdir -p "$dst/$rel"
  fi
' sh "$SRC" "$DST" {} \;

# 2) Create empty files
# For each file under SRC, create an empty counterpart under DST.
find "$SRC" -type f -exec sh -c '
  src=$1; dst=$2; p=$3
  rel=${p#"$src"/}
  d=$(dirname "$dst/$rel")
  mkdir -p "$d"
  : > "$dst/$rel"
' sh "$SRC" "$DST" {} \;

# (Optional) 3) Recreate symlinks (commented out; uncomment if desired)
# This requires readlink to be available.
# find "$SRC" -type l -exec sh -c '
#   src=$1; dst=$2; p=$3
#   rel=${p#"$src"/}
#   target=$(readlink "$p") || exit 0
#   d=$(dirname "$dst/$rel")
#   mkdir -p "$d"
#   ln -snf "$target" "$dst/$rel"
# ' sh "$SRC" "$DST" {} \;

echo "Structure copied from \"$SRC\" to \"$DST\" with empty files."

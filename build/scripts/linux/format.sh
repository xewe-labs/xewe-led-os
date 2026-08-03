#!/usr/bin/env bash
# Forwarding stub: the shared format script lives in ../mac.
# cd into that dir first so its relative paths (../../tools/align_decls.py) resolve.
cd "$(dirname "${BASH_SOURCE[0]}")/../mac" && exec ./format.sh "$@"

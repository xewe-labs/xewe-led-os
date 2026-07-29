#!/usr/bin/env bash
# Forwarding stub: the shared build scripts live in ../mac.
# cd into that dir first so their CWD-relative paths (../../build_config, ./compile.sh) resolve.
cd "$(dirname "${BASH_SOURCE[0]}")/../mac" && exec ./compile.sh "$@"

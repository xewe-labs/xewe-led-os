#!/usr/bin/env bash
# Forwarding stub: the shared format script lives in ../mac.
# cd into that dir first so its relative path to the orchestrator
# (../../tools/code_formatter/format.py) resolves.
cd "$(dirname "${BASH_SOURCE[0]}")/../mac" && exec ./format.sh "$@"

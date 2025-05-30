#!/usr/bin/env bash
set -euo pipefail

# -----------------------------------------------------------------------------
# Upload script for ESP32-C3: tries esptool first, then arduino-cli if needed.
#
# Usage:
#   ./scripts/upload.sh <serial-port>
#
# Example:
#   ./scripts/upload.sh /dev/cu.usbmodem1101
# -----------------------------------------------------------------------------

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <serial-port>"
  exit 1
fi

PORT="$1"
BAUD=460800

# 1) Locate an esptool command:
if command -v esptool.py >/dev/null 2>&1; then
  UPLOAD_TOOL="esptool.py"
elif command -v esptool >/dev/null 2>&1; then
  UPLOAD_TOOL="esptool"
elif python3 -c 'import esptool' >/dev/null 2>&1; then
  UPLOAD_TOOL="python3 -m esptool"
else
  UPLOAD_TOOL=""
fi

# Resolve project root and build dir
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/.."
BUILD_DIR="$PROJECT_ROOT/build"

# Paths to the three binaries
BOOTLOADER_BIN="$BUILD_DIR/XeWe-LedOS.ino.bootloader.bin"
PARTITIONS_BIN="$BUILD_DIR/XeWe-LedOS.ino.partitions.bin"
APP_BIN="$BUILD_DIR/XeWe-LedOS.ino.bin"

# If we have esptool, do a three-region flash:
if [ -n "$UPLOAD_TOOL" ]; then
  # verify files exist
  for f in "$BOOTLOADER_BIN" "$PARTITIONS_BIN" "$APP_BIN"; do
    if [ ! -f "$f" ]; then
      echo "Error: Missing file $f"
      exit 1
    fi
  done

  echo "🔌 Flashing ESP32-C3 on $PORT at ${BAUD}bps using $UPLOAD_TOOL…"
  "$UPLOAD_TOOL" \
    --chip esp32c3 \
    --port "$PORT" \
    --baud "$BAUD" \
    write_flash \
      0x1000  "$BOOTLOADER_BIN" \
      0x8000  "$PARTITIONS_BIN" \
      0x10000 "$APP_BIN"
  echo "✅ Flash complete!"
  exit 0
fi

# 2) Fallback: use Arduino CLI’s upload
if command -v arduino-cli >/dev/null 2>&1; then
  echo "ℹ️  esptool not found, falling back to Arduino CLI."
  echo "🔨 Compiling and uploading with arduino-cli…"
  arduino-cli upload \
    --fqbn esp32:esp32:esp32c3 \
    --port "$PORT" \
    "$PROJECT_ROOT"
  echo "✅ Upload via Arduino CLI complete!"
  exit 0
fi

# 3) Nothing found: bail out
echo "Error: neither esptool nor Arduino CLI found."
echo " • To install esptool:  pip3 install esptool"
echo " • To install Arduino CLI: brew install arduino-cli  (or see https://arduino.github.io/arduino-cli/installation/)"
exit 1

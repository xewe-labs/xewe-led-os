#!/usr/bin/env bash
set -euo pipefail

# ————————————————————————————————
# 1) Determine which serial port to use
# ————————————————————————————————
if [ -n "${1:-}" ]; then
  ESPPORT="$1"
elif [ -n "${ESPPORT:-}" ]; then
  ESPPORT="$ESPPORT"
else
  echo "🔌 Skipping upload: you must specify a serial port"
  echo "   Usage: $0 <serial-port>   (e.g. $0 /dev/cu.usbmodem1101)"
  echo "   Or:   export ESPPORT=/dev/cu.usbmodem1101  and run without args"
  exit 1
fi

# ————————————————————————————————
# 2) Locate OUTPUT_DIR and set VENV_DIR to the absolute path
# ————————————————————————————————
# (Adjust this if your project ever moves, but this is the exact path you pasted.)
VENV_DIR="/Users/xewe/Documents/Programming/Arduino/XeWe-LedOS/tools/venv"
OUTPUT_DIR="/Users/xewe/Documents/Programming/Arduino/XeWe-LedOS/binary/latest"
FIRMWARE_BIN="${OUTPUT_DIR}/firmware.bin"

# ————————————————————————————————
# 3) Activate Python virtualenv
# ————————————————————————————————
if [ ! -f "${VENV_DIR}/bin/activate" ]; then
  echo "❌ Virtualenv not found at ${VENV_DIR}"
  echo "   Did you run the build script first to create/upgrade the venv?"
  exit 1
fi
# shellcheck source=/dev/null
source "${VENV_DIR}/bin/activate"

# ————————————————————————————————
# 4) Verify firmware.bin exists
# ————————————————————————————————
if [[ ! -f "${FIRMWARE_BIN}" ]]; then
  echo "❌ Cannot find firmware to upload: ${FIRMWARE_BIN}" >&2
  echo "   Did you run the build step and merge process?"
  exit 1
fi

# ————————————————————————————————
# 5) Flash with esptool
# ————————————————————————————————
echo "🚀 Uploading ${FIRMWARE_BIN} to ESP32-C3 on ${ESPPORT}…"
# Adjust baud (e.g. 460800) if you prefer faster flashing
python -m esptool \
  --chip esp32c3 \
  --port "${ESPPORT}" \
  --baud 921600 \
  write_flash \
    0x0 "${FIRMWARE_BIN}"

echo "✅ Flash complete!"

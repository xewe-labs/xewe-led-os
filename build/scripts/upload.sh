#!/usr/bin/env bash
set -euo pipefail

# upload.sh — Flash a single merged image (firmware.bin at 0x0) from a specific build dir.
# All paths are supplied by build.sh.
#
# Required:
#   -c, --chip        c3|c6|s3
#   -p, --port        Serial port
#
# Optional:
#       --build-dir   Absolute path to the build folder (…/builds/<ts>-<ver>-<chip>-<proj>)
#       --baud        Baud (default: 921600)

BUILD_CONFIG_FILE="../build_config"
source "${BUILD_CONFIG_FILE}"

BUILD_DIR="$(get_cfg builds_latest_dir)"
PYTHON_BIN="$(get_cfg venv_python_bin)"

ESP_CHIP=""
ESP_PORT=""
ESP_BAUD="921600"

while [[ $# -gt 0 ]]; do
  case "$1" in
    -c|--chip)          ESP_CHIP="$2"; shift 2 ;;
    -p|--port)          ESP_PORT="$2"; shift 2 ;;
    --build-dir)        BUILD_DIR="$2"; shift 2 ;;
    --baud)             ESP_BAUD="$2"; shift 2 ;;
    *) echo "❌ Unknown arg: $1"; exit 1 ;;
  esac
done

[[ "$ESP_CHIP" =~ ^(c3|c6|s3)$ ]] || { echo "❌ Missing/Invalid chip (c3|c6|s3)"; exit 1; }
[[ -z "${ESP_PORT}" ]] && { echo "❌ Missing port (-p)."; exit 1; }

# Map short chip name to esptool chip ID
case "${ESP_CHIP}" in
  c3) ESPID="esp32c3" ;;
  c6) ESPID="esp32c6" ;;
  s3) ESPID="esp32s3" ;;
esac

ESPTOOL_CMD="${PYTHON_BIN} -m esptool"

shopt -s nullglob
BIN_FILES=("${BUILD_DIR}/binary/"*.bin)
shopt -u nullglob

if [[ ${#BIN_FILES[@]} -eq 0 ]]; then
  echo "❌ Error: No .bin file found in ${BUILD_DIR}/binary/"
  exit 1
elif [[ ${#BIN_FILES[@]} -gt 1 ]]; then
  echo "⚠️  Warning: Multiple .bin files found. Using the first one: ${BIN_FILES[0]}"
fi

FIRMWARE_BIN="${BIN_FILES[0]}"

echo "📦 Build       : ${BUILD_DIR}"
echo "📄 Image       : ${FIRMWARE_BIN}"
echo "🔌 Port        : ${ESP_PORT}"
echo "⚡ Baud        : ${ESP_BAUD}"
echo "🔧 Chip        : ${ESPID}"
echo "🧰 Esptool     : ${ESPTOOL_CMD}"
echo "🚀 Flashing merged image at 0x00000000 …"

# shellcheck disable=SC2086
${ESPTOOL_CMD} \
  --chip "${ESPID}" \
  --port "${ESP_PORT}" \
  --baud "${ESP_BAUD}" \
  write_flash 0x0 "${FIRMWARE_BIN}"

echo "✅ Upload complete."
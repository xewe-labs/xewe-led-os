#!/usr/bin/env bash
set -euo pipefail

# upload.sh — Flash a single merged image (firmware.bin at 0x0) from a specific build dir.
# All paths are supplied by build.sh.
#
# Required:
#   -t, --type        c3|c6|s3
#   -p, --port        Serial port
#       --build-dir   Absolute path to the build folder (…/builds/<ts>-<ver>-<chip>-<proj>)
#
# Optional:
#   -b, --baud        Baud (default: 921600)
#       --venv        Python venv to prefer for esptool (default: none)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

ESP_CHIP=""
ESP_PORT=""
ESP_BAUD="921600"
BUILD_DIR=""
VENV_DIR=""

usage() {
  cat <<'EOF'
Usage: upload.sh -t <c3|c6|s3> -p <serial-port> --build-dir <dir> [-b <baud>] [--venv <path>]
EOF
  exit 1
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -t|--type)      ESP_CHIP="${2:-}"; shift 2 ;;
    -p|--port)      ESP_PORT="${2:-}"; shift 2 ;;
    -b|--baud)      ESP_BAUD="${2:-}"; shift 2 ;;
    --build-dir)    BUILD_DIR="${2:-}"; shift 2 ;;
    --venv)         VENV_DIR="${2:-}"; shift 2 ;;
    -h|--help)      usage ;;
    *) echo "Unknown arg: $1"; usage ;;
  esac
done

[[ -z "${ESP_CHIP}"  ]] && usage
[[ -z "${ESP_PORT}"  ]] && usage
[[ -z "${BUILD_DIR}" ]] && usage
[[ -d "${BUILD_DIR}" ]] || { echo "❌ Build dir not found: ${BUILD_DIR}"; exit 1; }

# ---------- Require venv (same folder as this script) ----------
DEFAULT_VENV="${SCRIPT_DIR}/.venv"
[[ -z "${VENV_DIR}" ]] && [[ -d "${DEFAULT_VENV}" ]] && VENV_DIR="${DEFAULT_VENV}"
if [[ -z "${VENV_DIR}" || ! -x "${VENV_DIR}/bin/python" ]]; then
  echo "❌ Could not find venv at: ${DEFAULT_VENV}"
  echo "   Run setup script: ${SCRIPT_DIR}/setup_build_enviroment.sh"
  exit 1
fi

FW=$(find "${BUILD_DIR}/binary" -name '*.bin' | head -n 1)
[[ -f "${FW}" ]] || { echo "❌ Firmware not found: ${FW}"; exit 1; }

chip_to_esptool_id() {
  case "$1" in
    c3) echo "esp32c3" ;;
    c6) echo "esp32c6" ;;
    s3) echo "esp32s3" ;;
    *)  echo "esp32c3" ;;
  esac
}

# Choose esptool (prefer venv)
pick_esptool() {
  if [[ -n "${VENV_DIR}" && -x "${VENV_DIR}/bin/python3" ]]; then
    echo "${VENV_DIR}/bin/python3 -m esptool"; return 0
  fi
  if [[ -n "${VENV_DIR}" && -x "${VENV_DIR}/bin/python" ]]; then
    echo "${VENV_DIR}/bin/python -m esptool"; return 0
  fi
  if command -v esptool.py >/dev/null 2>&1; then
    echo "esptool.py"; return 0
  fi
  if command -v python3 >/dev/null 2>&1; then
    echo "python3 -m esptool"; return 0
  fi
  if command -v python >/dev/null 2>&1; then
    echo "python -m esptool"; return 0
  fi
  return 1
}

ESPID="$(chip_to_esptool_id "${ESP_CHIP}")"
if ! ESPTOOL_CMD=$(pick_esptool); then
  echo "❌ esptool not available. Install via brew/pip or provide --venv."
  exit 1
fi

echo "📦 Build       : ${BUILD_DIR}"
echo "📄 Image       : ${FW}"
echo "🔌 Port        : ${ESP_PORT}"
echo "⚡ Baud        : ${ESP_BAUD}"
echo "🔧 Chip        : ${ESPID}"
echo "🧰 Esptool     : ${ESPTOOL_CMD}"
echo "🚀 Flashing merged image @ 0x00000000 …"

# shellcheck disable=SC2086
${ESPTOOL_CMD} \
  --chip "${ESPID}" \
  --port "${ESP_PORT}" \
  --baud "${ESP_BAUD}" \
  write_flash 0x0 "${FW}"

echo "✅ Upload complete."

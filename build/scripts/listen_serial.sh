#!/usr/bin/env bash
set -euo pipefail

# listen_serial.sh — Open a serial monitor to the device.
#
# Usage:
#   ./listen_serial.sh -p <serial-port> [-b <baud>]
#
# Prefers Arduino CLI monitor; falls back to Python's miniterm, then to screen.

BUILD_CONFIG_FILE="../build_config"
source "${BUILD_CONFIG_FILE}"

PYTHON_BIN="$(get_cfg venv_python_bin)"

ESP_PORT=""
ESP_BAUD="115200"   # adjust to your sketch default if needed

usage() {
  cat <<'EOF'
Usage: listen_serial.sh -p <serial-port> [-b <baud>]
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -p|--port) ESP_PORT="${2:-}"; shift 2 ;;
    -b|--baud) ESP_BAUD="${2:-}"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "❌ Unknown arg: $1"; usage; exit 1 ;;
  esac
done

[[ -z "${ESP_PORT}" ]] && { echo "❌ Missing -p|--port"; exit 1; }

if command -v arduino-cli >/dev/null 2>&1; then
  echo "🖥️  arduino-cli monitor ${ESP_PORT} @ ${ESP_BAUD} (Ctrl-C to exit)…"
  exec arduino-cli monitor -p "${ESP_PORT}" -c "${ESP_BAUD}"
fi

if "${PYTHON_BIN}" - <<'PYCHK' >/dev/null 2>&1
import importlib.util, sys
sys.exit(0 if importlib.util.find_spec("serial.tools.miniterm") else 1)
PYCHK
then
  echo "🖥️  Python miniterm ${ESP_PORT} @ ${ESP_BAUD} (Ctrl-] then q to quit)…"
  exec "${PYTHON_BIN}" -m serial.tools.miniterm "${ESP_PORT}" "${ESP_BAUD}"
fi

if command -v screen >/dev/null 2>&1; then
  echo "🖥️  screen ${ESP_PORT} @ ${ESP_BAUD} (Ctrl-A then K to quit)…"
  exec screen "${ESP_PORT}" "${ESP_BAUD}"
fi

echo "❌ No suitable serial monitor found (arduino-cli, pyserial, or screen)."
exit 1

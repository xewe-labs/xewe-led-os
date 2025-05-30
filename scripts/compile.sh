#!/usr/bin/env bash
set -euo pipefail

# ————————————————————————————————
# Configuration
# ————————————————————————————————
SKETCH="../XeWe-LedOS.ino"
BOARD_FQBN="esp32:esp32:esp32c3"

# Standard Arduino sketchbook & libraries paths
SKETCHBOOK_BASE="${HOME}/Documents/Arduino"
LIB_DIR="${SKETCHBOOK_BASE}/libraries"

# Build & output
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/../build"
OUTPUT_DIR="${SCRIPT_DIR}/../binary/latest"

# Which Git repos to sync into ~/Documents/Arduino/libraries
LIBS=(
  "FastLED:https://github.com/FastLED/FastLED.git"
  "AsyncTCP:https://github.com/me-no-dev/AsyncTCP.git"
  "ESPAsyncWebServer:https://github.com/me-no-dev/ESPAsyncWebServer.git"
)

# ————————————————————————————————
# Prep directories
# ————————————————————————————————
mkdir -p "${LIB_DIR}" "${BUILD_DIR}" "${OUTPUT_DIR}"

# ————————————————————————————————
# Arduino CLI bootstrap
# ————————————————————————————————
CLI_CFG="${SCRIPT_DIR}/arduino-cli.yaml"
CLI_CMD="arduino-cli"

# Install or update arduino-cli if missing
if ! command -v arduino-cli &> /dev/null; then
  case "$(uname)" in
    Darwin)
      brew install arduino-cli
      ;;
    Linux)
      mkdir -p "${SCRIPT_DIR}/../tools"
      curl -sL https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Linux_64bit.tar.gz \
        | tar -xz -C "${SCRIPT_DIR}/../tools"
      chmod +x "${SCRIPT_DIR}/../tools/arduino-cli"
      CLI_CMD="${SCRIPT_DIR}/../tools/arduino-cli"
      ;;
    *)
      echo "❗ Unsupported OS" >&2
      exit 1
      ;;
  esac
fi

# Initialize & point CLI at your sketchbook (so it finds <user>/libraries)
if [ ! -f "${CLI_CFG}" ]; then
  "${CLI_CMD}" config init --dest-file "${CLI_CFG}" --overwrite
fi
"${CLI_CMD}" config set directories.user "${SKETCHBOOK_BASE}" --config-file "${CLI_CFG}"
CLI_OPTS=(--config-file "${CLI_CFG}")

# Ensure ESP32 core is installed
"${CLI_CMD}" "${CLI_OPTS[@]}" core update-index
"${CLI_CMD}" "${CLI_OPTS[@]}" core install esp32:esp32

# ————————————————————————————————
# Sync libraries into ~/Documents/Arduino/libraries
# ————————————————————————————————
echo "🔄 Syncing libraries to ${LIB_DIR}…"
for entry in "${LIBS[@]}"; do
  name="${entry%%:*}"
  url="${entry#*:}"
  target="${LIB_DIR}/${name}"

  if [ -d "${target}/.git" ]; then
    echo "→ Updating ${name}"
    git -C "${target}" pull --ff-only
  else
    echo "→ Cloning ${name}"
    rm -rf "${target}"
    git clone --depth 1 "${url}" "${target}"
  fi
done

# ————————————————————————————————
# Compile
# ————————————————————————————————
echo "🔧 Compiling ${SKETCH} for ${BOARD_FQBN}…"
"${CLI_CMD}" "${CLI_OPTS[@]}" compile \
  --fqbn "${BOARD_FQBN}" \
  --build-path "${BUILD_DIR}" \
  "${SCRIPT_DIR}/${SKETCH}"

# ————————————————————————————————
# Copy firmware
# ————————————————————————————————
BIN=$(find "${BUILD_DIR}" -maxdepth 1 -name '*.bin' | head -n1)
if [ -z "${BIN}" ]; then
  echo "❌ Build failed: no .bin found" >&2
  exit 1
fi

cp "${BIN}" "${OUTPUT_DIR}/firmware.bin"
echo "✅ Firmware ready at ${OUTPUT_DIR}/firmware.bin"

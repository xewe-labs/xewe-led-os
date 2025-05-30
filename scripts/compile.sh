#!/usr/bin/env bash
set -euo pipefail

# ————————————————————————————————
# Configuration
# ————————————————————————————————
SKETCH="../XeWe-LedOS.ino"
FQBN="esp32:esp32:esp32c3"

# Project-local libraries
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIB_DIR="${SCRIPT_DIR}/../lib"
BUILD_DIR="${SCRIPT_DIR}/../build"
OUTPUT_DIR="${SCRIPT_DIR}/../binary/latest"

# Git repos to pull into ../lib (name:git-url)
LIBS=(
  "FastLED:https://github.com/FastLED/FastLED.git"
  "AsyncTCP:https://github.com/me-no-dev/AsyncTCP.git"
  "ESPAsyncWebServer:https://github.com/me-no-dev/ESPAsyncWebServer.git"
)

# ————————————————————————————————
# Prep folders
# ————————————————————————————————
mkdir -p "${LIB_DIR}" "${BUILD_DIR}" "${OUTPUT_DIR}"

# ————————————————————————————————
# Arduino CLI bootstrap (if needed)
# ————————————————————————————————
CLI_CMD="arduino-cli"
if ! command -v "${CLI_CMD}" &> /dev/null; then
  case "$(uname)" in
    Darwin)
      echo "📦 Installing arduino-cli via Homebrew…"
      brew install arduino-cli
      ;;
    Linux)
      echo "📦 Installing arduino-cli locally…"
      mkdir -p "${SCRIPT_DIR}/../tools"
      curl -sL https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Linux_64bit.tar.gz \
        | tar -xz -C "${SCRIPT_DIR}/../tools"
      chmod +x "${SCRIPT_DIR}/../tools/arduino-cli"
      export PATH="${SCRIPT_DIR}/../tools:$PATH"
      ;;
    *)
      echo "❗ Unsupported OS: $(uname)" >&2
      exit 1
      ;;
  esac
fi

# — Print Arduino CLI info —
echo "⚙️ Using Arduino CLI at: $(command -v ${CLI_CMD})"
echo "⚙️ Arduino CLI version: $(${CLI_CMD} version)"

# ————————————————————————————————
# Ensure ESP32 core is installed
# ————————————————————————————————
"${CLI_CMD}" core update-index
"${CLI_CMD}" core install esp32:esp32

# ————————————————————————————————
# Clone or update each library under ../lib
# ————————————————————————————————
echo "🔄 Syncing project-local libraries into ${LIB_DIR}"
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

  # — Remove .github folder to avoid accidental pushes —
  if [ -d "${target}/.github" ]; then
    echo "🗑️ Removing ${name}/.github directory"
    rm -rf "${target}/.github"
  fi
done

# ————————————————————————————————
# Build: pass all lib dirs to --libraries
# ————————————————————————————————
LIB_PATHS="$(printf "%s," "${LIB_DIR}/"{FastLED,AsyncTCP,ESPAsyncWebServer})"
LIB_PATHS="${LIB_PATHS%,}"  # strip trailing comma

echo "🔧 Compiling ${SKETCH} for ${FQBN}"
"${CLI_CMD}" compile \
  --fqbn "${FQBN}" \
  --build-path "${BUILD_DIR}" \
  --libraries "${LIB_PATHS}" \
  "${SCRIPT_DIR}/${SKETCH}"
# ————————————————————————————————
# Copy firmware to ../binary/latest
# ————————————————————————————————
BIN=$(find "${BUILD_DIR}" -maxdepth 1 -name '*.bin' | head -n1)
if [ -z "${BIN}" ]; then
  echo "❌ Build failed: no .bin found in ${BUILD_DIR}" >&2
  exit 1
fi

cp "${BIN}" "${OUTPUT_DIR}/firmware.bin"
echo "✅ Firmware ready at ${OUTPUT_DIR}/firmware.bin"

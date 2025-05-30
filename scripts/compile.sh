#!/usr/bin/env bash
set -euo pipefail

# ————————————————————————————————
# Configuration
# ————————————————————————————————
SKETCH="../XeWe-LedOS.ino"
FQBN="esp32:esp32:esp32c3"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIB_DIR="${SCRIPT_DIR}/../lib"
BUILD_DIR="${SCRIPT_DIR}/../build"
OUTPUT_DIR="${SCRIPT_DIR}/../binary/latest"
VENV_DIR="${SCRIPT_DIR}/../tools/venv"

# Git repos to pull into ../lib (name:git-url)
LIBS=(
  "FastLED:https://github.com/FastLED/FastLED.git"
  "AsyncTCP:https://github.com/me-no-dev/AsyncTCP.git"
  "ESPAsyncWebServer:https://github.com/me-no-dev/ESPAsyncWebServer.git"
)

# ————————————————————————————————
# Prep folders
# ————————————————————————————————
mkdir -p "${LIB_DIR}" "${BUILD_DIR}" "${OUTPUT_DIR}" "${VENV_DIR}"

# ————————————————————————————————
# Python venv & esptool installation
# ————————————————————————————————
if [ ! -f "${VENV_DIR}/bin/activate" ]; then
  echo "🐍 Creating Python virtualenv in ${VENV_DIR}"
  python3 -m venv "${VENV_DIR}"
fi
# shellcheck source=/dev/null
source "${VENV_DIR}/bin/activate"

echo "🐍 Installing esptool in venv"
pip install --upgrade pip setuptools
pip install esptool

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

  # Remove .github to avoid accidental pushes
  rm -rf "${target}/.github"
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
# Locate build artifacts
# ————————————————————————————————
BOOTLOADER_BIN=$(find "${BUILD_DIR}" -maxdepth 1 -name '*bootloader*.bin' | head -n1)
PARTITION_BIN=$(find "${BUILD_DIR}" -maxdepth 1 -name '*partitions*.bin' | head -n1)
SKETCH_NAME=$(basename "${SKETCH}" .ino)
APP_BIN=$(find "${BUILD_DIR}" -maxdepth 1 -name "${SKETCH_NAME}.ino.bin" | head -n1)

if [[ ! -f "$BOOTLOADER_BIN" || ! -f "$PARTITION_BIN" || ! -f "$APP_BIN" ]]; then
  echo "❌ Missing one of the required binaries:" >&2
  echo "   BOOTLOADER:   $BOOTLOADER_BIN" >&2
  echo "   PARTITIONS:   $PARTITION_BIN" >&2
  echo "   APPLICATION:  $APP_BIN" >&2
  exit 1
fi

# ————————————————————————————————
# Merge into single flat firmware.bin
# ————————————————————————————————
echo "🔀 Merging into single firmware.bin…"
python -m esptool --chip esp32c3 merge_bin -o "${OUTPUT_DIR}/firmware.bin" \
  0x0       "${BOOTLOADER_BIN}"  \
  0x8000    "${PARTITION_BIN}"   \
  0x10000   "${APP_BIN}"

echo "✅ Merged firmware ready at ${OUTPUT_DIR}/firmware.bin"

# ————————————————————————————————
# Generate manifest.json for ESP Web Tools
# ————————————————————————————————
MANIFEST_PATH="${OUTPUT_DIR}/manifest.json"
echo "📝 Writing manifest to ${MANIFEST_PATH}"
cat > "${MANIFEST_PATH}" <<EOF
{
  "name": "XeWe-LedOS",
  "version": "latest",
  "products": [
    {
      "platform": "ESP32C3",
      "flash_size": "4MB",
      "build": "firmware.bin",
      "download": "firmware.bin"
    }
  ]
}
EOF

echo "✅ Manifest written to ${MANIFEST_PATH}"

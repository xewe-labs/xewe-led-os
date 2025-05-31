#!/usr/bin/env bash
set -euo pipefail

# ————————————————————————————————
# Configuration
# ————————————————————————————————

# (1) Base board identifier:
FQBN_BASE="esp32:esp32:esp32c3"

# (2) All of the “Tools → Menu” options from your screenshot,
#     listed as key=value pairs, comma-separated. These have been
#     verified against Arduino CLI’s board‐options for esp32:esp32:esp32c3.
FQBN_OPTS="\
CDCOnBoot=cdc,\
CPUFreq=160,\
DebugLevel=none,\
EraseFlash=all,\
FlashFreq=80,\
FlashMode=qio,\
FlashSize=4M,\
JTAGAdapter=default,\
PartitionScheme=no_ota,\
UploadSpeed=921600\
"

# (3) Final FQBN string that Arduino CLI will consume:
FQBN="${FQBN_BASE}:${FQBN_OPTS}"

# Path to your sketch (relative to this script):
SKETCH="../XeWe-LedOS.ino"

# Build / output directories:
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIB_DIR="${SCRIPT_DIR}/../lib"
BUILD_DIR="${SCRIPT_DIR}/../build"
OUTPUT_DIR="${SCRIPT_DIR}/../binary/latest"
VENV_DIR="${SCRIPT_DIR}/../tools/venv"

# List of Git-based libraries to clone/pull under ../lib
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
#echo "🔄 Syncing project-local libraries into ${LIB_DIR}"
#for entry in "${LIBS[@]}"; do
#  name="${entry%%:*}"
#  url="${entry#*:}"
#  target="${LIB_DIR}/${name}"
#
#  if [ -d "${target}/.git" ]; then
#    echo "→ Updating ${name}"
#    git -C "${target}" pull --ff-only
#  else
#    echo "→ Cloning ${name}"
#    rm -rf "${target}"
#    git clone --depth 1 "${url}" "${target}"
#  fi
#
#  # Remove .github to avoid accidental pushes
#  rm -rf "${target}/.github"
#done
#
## ————————————————————————————————
## Build: pass all lib dirs to --libraries
## ————————————————————————————————
#LIB_PATHS="$(printf "%s," "${LIB_DIR}/"{FastLED,AsyncTCP,ESPAsyncWebServer})"
#LIB_PATHS="${LIB_PATHS%,}"  # remove trailing comma

echo
echo "🔧 Compiling ${SKETCH} for ${FQBN}"
echo "   → Arduino CLI will see these menu options: ${FQBN_OPTS}"
"${CLI_CMD}" compile \
  --fqbn "${FQBN}" \
  --build-path "${BUILD_DIR}" \
#  --libraries "${LIB_PATHS}" \
  "${SCRIPT_DIR}/${SKETCH}"

# ————————————————————————————————
# Locate the merged .ino.merged.bin (produced by Arduino CLI)
# ————————————————————————————————
SKETCH_NAME=$(basename "${SKETCH}" .ino)
MERGED_BIN=$(find "${BUILD_DIR}" -maxdepth 1 -name "${SKETCH_NAME}.ino.merged.bin" | head -n1)

if [[ ! -f "$MERGED_BIN" ]]; then
  echo "❌ ERROR: Could not find ${SKETCH_NAME}.ino.merged.bin in ${BUILD_DIR}" >&2
  exit 1
fi

echo "🔍 Found merged image: $MERGED_BIN"
cp "$MERGED_BIN" "${OUTPUT_DIR}/firmware.bin"
echo "✅ Copied merged firmware → ${OUTPUT_DIR}/firmware.bin"

# —————————————————————————————
# Generate ESP-Web-Tools v10 manifest.json
# —————————————————————————————
MANIFEST_PATH="${OUTPUT_DIR}/manifest.json"
cat > "${MANIFEST_PATH}" <<EOF
{
  "name": "XeWe-LedOS",
  "version": "latest",
  "new_install_improv_wait_time": 0,
  "builds": [
    {
      "chipFamily": "ESP32-C3",
      "parts": [
        { "path": "firmware.bin", "offset": 0 }
      ]
    }
  ]
}
EOF
echo "✅ v10 manifest written to ${MANIFEST_PATH}"

# (Optional) — if you want to _upload_ immediately, uncomment below.
# echo
# echo "📲 Uploading to ESP32-C3 on /dev/ttyUSB0 at 921600 baud…"
# "${CLI_CMD}" upload \
#   --fqbn "${FQBN}" \
#   --port /dev/ttyUSB0 \
#   --input-dir "${BUILD_DIR}" \
#   --upload-speed 921600

echo
echo "🏁 Build complete. Flash “${OUTPUT_DIR}/firmware.bin” at 0x0."

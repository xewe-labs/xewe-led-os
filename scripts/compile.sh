#!/usr/bin/env bash
set -euo pipefail

# ————————————————————————————————
# Configuration
# ————————————————————————————————

# (1) Base board identifier:
FQBN_BASE="esp32:esp32:esp32c3"

# (2) All of the “Tools → Menu” options, listed as key=value pairs.
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

# List of libraries that must exist under ../lib
# This list is now used only to construct the --libraries path.
LIBS=(
  "FastLED"
  "Espalexa"
  "HomeSpan"
  "WebSockets"
)

# ————————————————————————————————
# Pre-flight Dependency Checks (Offline-First)
# ————————————————————————————————
echo "🔎 Performing pre-flight dependency checks…"

# 1. Check for arduino-cli
if ! command -v arduino-cli &> /dev/null; then
  echo "❌ ERROR: 'arduino-cli' is not installed or not in your PATH." >&2
  echo "   Please install it first. See: https://arduino.github.io/arduino-cli/latest/installation/" >&2
  exit 1
fi

# 2. Check for ESP32 core
if ! arduino-cli core list | grep -q 'esp32:esp32'; then
  echo "❌ ERROR: The 'esp32:esp32' core is not installed." >&2
  echo "   Please install it by running: arduino-cli core install esp32:esp32" >&2
  exit 1
fi

echo "✅ Core dependencies are present."
echo "⚙️ Using Arduino CLI at: $(command -v arduino-cli)"
echo "⚙️ Arduino CLI version: $(arduino-cli version)"

# ————————————————————————————————
# Prep folders
# ————————————————————————————————
mkdir -p "${LIB_DIR}" "${BUILD_DIR}" "${OUTPUT_DIR}"

# ————————————————————————————————
# Build: pass all lib dirs to --libraries
# ————————————————————————————————
# Dynamically generate LIB_PATHS from the LIBS array
LIB_PATHS_ARRAY=()
if [ ${#LIBS[@]} -gt 0 ]; then
  for lib_name in "${LIBS[@]}"; do
    LIB_PATHS_ARRAY+=("${LIB_DIR}/${lib_name}")
  done
fi

# Join array elements with a comma
# Source for join_by: https://stackoverflow.com/a/17841619/1291435
function join_by {
  local d=${1-} f=${2-}
  if shift 2; then
    printf %s "$f" "${@/#/$d}"
  fi
}

# Ensure LIB_PATHS is empty if LIB_PATHS_ARRAY is empty, otherwise join.
if [ ${#LIB_PATHS_ARRAY[@]} -gt 0 ]; then
  LIB_PATHS=$(join_by , "${LIB_PATHS_ARRAY[@]}")
else
  LIB_PATHS=""
fi

echo
echo "🔧 Compiling ${SKETCH} for ${FQBN}"
echo "   → Arduino CLI will see these menu options: ${FQBN_OPTS}"

# Construct the compile command
COMPILE_CMD_ARGS=(
  compile
  --fqbn "${FQBN}"
  --build-path "${BUILD_DIR}"
)

# Add --libraries argument only if LIB_PATHS is not empty
if [ -n "${LIB_PATHS}" ]; then
  COMPILE_CMD_ARGS+=(--libraries "${LIB_PATHS}")
  echo "   → Using local libraries from: ${LIB_PATHS}"
fi

COMPILE_CMD_ARGS+=("${SCRIPT_DIR}/${SKETCH}")

# Execute the compile command
arduino-cli "${COMPILE_CMD_ARGS[@]}"

# ————————————————————————————————
# Locate the merged .ino.merged.bin (produced by Arduino CLI)
# ————————————————————————————————
SKETCH_NAME=$(basename "${SKETCH}" .ino)
MERGED_BIN=$(find "${BUILD_DIR}" -maxdepth 1 -name "${SKETCH_NAME}.ino.merged.bin" -print -quit)

if [[ -z "$MERGED_BIN" || ! -f "$MERGED_BIN" ]]; then
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
# Note: Ensure the --input-dir points to your pre-compiled binaries.
# echo
# echo "📲 Uploading to ESP32-C3 on /dev/ttyUSB0 at 921600 baud…"
# arduino-cli upload \
#   --fqbn "${FQBN}" \
#   --port /dev/ttyUSB0 \
#   --input-dir "${BUILD_DIR}"

echo
echo "🏁 Build complete. Flash “${OUTPUT_DIR}/firmware.bin” at 0x0."
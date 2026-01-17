#!/usr/bin/env bash
set -euo pipefail

# compile.sh — Build a versioned ESP32 firmware using Arduino CLI.
# All paths/options are supplied by build.sh.
#
# Required flags:
#   -t, --type           c3|c6|s3
#       --project-root   <abs path>
#       --builds-dir     <abs path>
#       --work-dir       <abs path>    (Arduino --build-path)
#       --target-dir     <abs path>    (final build folder)
#       --project-name   <name>
#       --version        <X.Y.ZZZ>
#       --timestamp      <ISO-8601 UTC>
#
# Optional:
#       --manifest-name  <product name> (default: PROJECT_NAME)
#       --libs           <path[:path...]>  (each becomes --libraries)
#       --fqbn-extra     <comma-separated FQBN opts>
#       --venv           <path> (used only if merge is needed)
#
# Output layout (under --target-dir):
#   src/  lib/ (if any)  version.txt
#   output/  binary/{firmware.bin, <ver>-<chip>-<project>.bin, manifest.json}
#
# Notes:
# - No version state is modified here.
# - We upload a merged image; if the core doesn't produce it, we merge via esptool.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ---------- Parse args ----------
ESP_CHIP=""
PROJECT_ROOT=""
BUILDS_DIR=""
WORK_DIR=""
TARGET_DIR=""
PROJECT_NAME=""
VERSION_NEXT=""
TS_ISO=""
MANIFEST_NAME=""
LIBS_LIST=""
FQBN_EXTRA_OPTS=""
VENV_DIR=""

usage_fail() { echo "❌ $1"; exit 1; }

while [[ $# -gt 0 ]]; do
  case "$1" in
    -t|--type)         ESP_CHIP="${2:-}"; shift 2 ;;
    --project-root)    PROJECT_ROOT="${2:-}"; shift 2 ;;
    --builds-dir)      BUILDS_DIR="${2:-}"; shift 2 ;;
    --work-dir)        WORK_DIR="${2:-}"; shift 2 ;;
    --target-dir)      TARGET_DIR="${2:-}"; shift 2 ;;
    --project-name)    PROJECT_NAME="${2:-}"; shift 2 ;;
    --version)         VERSION_NEXT="${2:-}"; shift 2 ;;
    --timestamp)       TS_ISO="${2:-}"; shift 2 ;;
    --manifest-name)   MANIFEST_NAME="${2:-}"; shift 2 ;;
    --libs)            LIBS_LIST="${2:-}"; shift 2 ;;
    --fqbn-extra)      FQBN_EXTRA_OPTS="${2:-}"; shift 2 ;;
    --venv)            VENV_DIR="${2:-}"; shift 2 ;;
    -h|--help)         sed -n '1,120p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) usage_fail "Unknown arg: $1" ;;
  esac
done

[[ -z "${ESP_CHIP}"      ]] && usage_fail "Missing --type"
[[ -z "${PROJECT_ROOT}"  ]] && usage_fail "Missing --project-root"
[[ -z "${BUILDS_DIR}"    ]] && usage_fail "Missing --builds-dir"
[[ -z "${WORK_DIR}"      ]] && usage_fail "Missing --work-dir"
[[ -z "${TARGET_DIR}"    ]] && usage_fail "Missing --target-dir"
[[ -z "${PROJECT_NAME}"  ]] && usage_fail "Missing --project-name"
[[ -z "${VERSION_NEXT}"  ]] && usage_fail "Missing --version"
[[ -z "${TS_ISO}"        ]] && usage_fail "Missing --timestamp"
[[ -z "${MANIFEST_NAME}" ]] && MANIFEST_NAME="${PROJECT_NAME}"

# ---------- Require venv (same folder as this script) ----------
DEFAULT_VENV="${SCRIPT_DIR}/.venv"
[[ -z "${VENV_DIR}" ]] && [[ -d "${DEFAULT_VENV}" ]] && VENV_DIR="${DEFAULT_VENV}"
if [[ -z "${VENV_DIR}" || ! -x "${VENV_DIR}/bin/python" ]]; then
  echo "❌ Could not find venv at: ${DEFAULT_VENV}"
  echo "   Run setup script: ${SCRIPT_DIR}/setup_build_enviroment.sh"
  exit 1
fi

# ---------- Tooling checks ----------
if ! command -v arduino-cli >/dev/null 2>&1; then
  usage_fail "'arduino-cli' not found in PATH"
fi
if ! arduino-cli core list | grep -q 'esp32:esp32'; then
  usage_fail "Espressif core not installed. Run: arduino-cli core install esp32:esp32"
fi

# ---------- Chip mapping ----------
chip_to_fqbn_board() {
  case "$1" in
    c3) echo "esp32c3" ;;
    c6) echo "esp32c6" ;;
    s3) echo "esp32s3" ;;
    *)  echo "esp32c3" ;;
  esac
}
chip_to_family_str() {
  case "$1" in
    c3) echo "ESP32-C3" ;;
    c6) echo "ESP32-C6" ;;
    s3) echo "ESP32-S3" ;;
  esac
}
FQBN_BOARD="$(chip_to_fqbn_board "${ESP_CHIP}")"
CHIP_FAMILY="$(chip_to_family_str "${ESP_CHIP}")"
FQBN_BASE="esp32:esp32:${FQBN_BOARD}"
FQBN_OPTS_DEFAULT="\
CDCOnBoot=cdc,\
CPUFreq=160,\
DebugLevel=none,\
EraseFlash=all,\
FlashMode=qio,\
FlashSize=4M,\
JTAGAdapter=default,\
PartitionScheme=no_ota,\
UploadSpeed=921600\
"
FQBN_OPTS="${FQBN_OPTS_DEFAULT}${FQBN_EXTRA_OPTS:+,${FQBN_EXTRA_OPTS}}"
FQBN="${FQBN_BASE}:${FQBN_OPTS}"

# ---------- Detect sketch ----------
detect_sketch() {
  local candidate
  if [[ -f "${PROJECT_ROOT}/${PROJECT_NAME}.ino" ]]; then
    echo "${PROJECT_ROOT}/${PROJECT_NAME}.ino"; return
  fi
  candidate="$(find "${PROJECT_ROOT}" -maxdepth 1 -name '*.ino' | head -n 1 || true)"
  [[ -n "${candidate}" ]] && { echo "${candidate}"; return; }
  candidate="$(find "${PROJECT_ROOT}/src" -maxdepth 1 -name '*.ino' 2>/dev/null | head -n 1 || true)"
  [[ -n "${candidate}" ]] && { echo "${candidate}"; return; }
  echo ""
}
SKETCH_PATH="$(detect_sketch)"
[[ -n "${SKETCH_PATH}" ]] || usage_fail "Could not locate .ino in project root or src/"
SKETCH_NAME="$(basename "${SKETCH_PATH}" .ino)"

# ---------- Prep directories ----------
OUTPUT_DIR="${TARGET_DIR}/output"
BINARY_DIR="${TARGET_DIR}/binary"
mkdir -p "${WORK_DIR}" "${TARGET_DIR}" "${OUTPUT_DIR}" "${BINARY_DIR}"

echo "🔧 Arduino FQBN: ${FQBN}"
echo "📄 Sketch: ${SKETCH_PATH}"
echo "🧰 Work path: ${WORK_DIR}"
echo "📁 Target dir: ${TARGET_DIR}"

# ---------- Build arguments ----------
COMPILE_ARGS=( compile --fqbn "${FQBN}" --build-path "${WORK_DIR}" --warnings default )

# Libraries (split colon-separated)
declare -a LIB_FLAGS=()
if [[ -n "${LIBS_LIST}" ]]; then
  IFS=':' read -r -a _libarr <<< "${LIBS_LIST}"
  for lp in "${_libarr[@]}"; do
    if [[ -d "${lp}" ]]; then
      LIB_FLAGS+=( --libraries "${lp}" )
      echo "📚 Using libs: ${lp}"
    else
      echo "⚠️  Skipping non-existent libs dir: ${lp}"
    fi
  done
fi

# ---------- Compile ----------
set +e
if ((${#LIB_FLAGS[@]})); then
  arduino-cli "${COMPILE_ARGS[@]}" "${LIB_FLAGS[@]}" "${SKETCH_PATH}" | tee "${TARGET_DIR}/compile.log"
else
  arduino-cli "${COMPILE_ARGS[@]}" "${SKETCH_PATH}" | tee "${TARGET_DIR}/compile.log"
fi
BUILD_RC="${PIPESTATUS[0]}"
set -e
[[ "${BUILD_RC}" -eq 0 ]] || { echo "❌ Compile failed (see ${TARGET_DIR}/compile.log)"; exit "${BUILD_RC}"; }

# ---------- Find or create merged binary ----------
# Prefer core-produced merged file
MERGED_BIN="$(find "${WORK_DIR}" -maxdepth 1 -name "${SKETCH_NAME}.ino.merged.bin" -print -quit || true)"
if [[ -z "${MERGED_BIN}" ]]; then
  echo "ℹ️  No *.merged.bin found; attempting merge via esptool…"

  # choose esptool (venv first)
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
  if ! ESPTOOL_CMD=$(pick_esptool); then
    echo "❌ esptool not found; cannot merge binaries. Install via brew/pip or provide --venv."
    exit 1
  fi

  APP_BIN="$(find "${WORK_DIR}" -maxdepth 1 -name "${SKETCH_NAME}.ino.bin" -print -quit || true)"
  BOOT_BIN="$(find "${WORK_DIR}" -type f -name 'bootloader*.bin' -print -quit || true)"
  PART_BIN="$(find "${WORK_DIR}" -type f -name '*partitions*.bin' -print -quit || true)"
  [[ -f "${APP_BIN}" && -f "${BOOT_BIN}" && -f "${PART_BIN}" ]] || { echo "❌ Missing components to merge"; exit 1; }

  MERGED_BIN="${WORK_DIR}/${SKETCH_NAME}.ino.merged.bin"
  echo "🔗 Merging → ${MERGED_BIN}"
  # shellcheck disable=SC2086
  ${ESPTOOL_CMD} merge_bin -o "${MERGED_BIN}" \
    0x0 "${BOOT_BIN}" \
    0x8000 "${PART_BIN}" \
    0x10000 "${APP_BIN}"
  echo "✅ Created merged image."
else
  echo "✅ Found merged image: ${MERGED_BIN}"
fi

# ---------- Snapshot sources & libs ----------
if [[ -d "${PROJECT_ROOT}/src" ]]; then
  cp -a "${PROJECT_ROOT}/src" "${TARGET_DIR}/src"
else
  mkdir -p "${TARGET_DIR}/src"
  cp -a "${SKETCH_PATH}" "${TARGET_DIR}/src/"
fi

if [[ -n "${LIBS_LIST}" ]]; then
  mkdir -p "${TARGET_DIR}/lib"
  IFS=':' read -r -a _libarr2 <<< "${LIBS_LIST}"
  for lp in "${_libarr2[@]}"; do
    [[ -d "${lp}" ]] && cp -a "${lp}" "${TARGET_DIR}/lib/$(basename "${lp}")"
  done
fi

# ---------- version.txt ----------
echo "${VERSION_NEXT}" > "${TARGET_DIR}/version.txt"

# ---------- Collect outputs ----------
# Copy key artifacts from work → output
find "${WORK_DIR}" -maxdepth 1 -type f \( -name "*.bin" -o -name "*.elf" -o -name "*.map" \) -exec cp -a {} "${OUTPUT_DIR}/" \;

# Place final merged image
MERGED_BIN_FILENAME="${VERSION_NEXT}-${CHIP_FAMILY}-${PROJECT_NAME}.bin"
cp -a "${MERGED_BIN}" "${BINARY_DIR}/${MERGED_BIN_FILENAME}"

# ---------- Manifest (ESP Web Tools v10) ----------
cat > "${BINARY_DIR}/manifest.json" <<EOF
{
  "name": "${MANIFEST_NAME}",
  "version": "${VERSION_NEXT}",
  "new_install_improv_wait_time": 0,
  "builds": [
    {
      "chipFamily": "${CHIP_FAMILY}",
      "parts": [
        { "path": "${MERGED_BIN_FILENAME}", "offset": 0 }
      ]
    }
  ]
}
EOF
echo "📝 Wrote manifest → ${BINARY_DIR}/manifest.json"

echo
echo "🎉 Build complete."
echo "   ➤ Final dir : ${TARGET_DIR}"
echo "   ➤ Firmware  : ${BINARY_DIR}/firmware.bin"
echo "   ➤ Version   : ${VERSION_NEXT}"

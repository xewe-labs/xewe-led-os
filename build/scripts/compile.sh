#!/usr/bin/env bash
set -euo pipefail

# compile.sh - Build a versioned ESP32 firmware using Arduino CLI.
# All paths/options are supplied by build.sh.
#
# Required flags:
# --chip c3|c6|s3
# --version <X.Y.ZZZ>
# --timestamp <ISO-8601 UTC>
# Optional flags:
# --fqbn-extra <comma-separated FQBN opts>
# --config_json <JSON string> (additional params written into meta.json)
#
# Output layout (under --target-dir):
# output/ binary/{firmware.bin, <ver>-<chip>-<project>.bin, manifest.json, meta.json}

#-------------------------#
#--- GET THE VARIABLES ---#
#-------------------------#
COMPILE_START_EPOCH="$(date -u +%s)"

CONFIG_FILE="../build_config"
source "${CONFIG_FILE}"

PROJECT_ROOT="$(get_cfg project_root)"
BUILDS_DIR="$(get_cfg builds_dir)"
WORK_DIR_BASE="$(get_cfg builds_cache_dir)"
PROJECT_NAME="$(get_cfg project_name)"
LIBS_DIR="$(get_cfg libraries_dir)"
PYTHON_BIN="$(get_cfg venv_python_bin)"

ESP_CHIP="" VERSION="" TS_ISO="" FQBN_EXTRA_OPTS="" CONFIG_JSON_RAW=""

usage_fail() { echo "❌ $1" >&2; exit 1; }

while [[ $# -gt 0 ]]; do
  case "$1" in
    --chip) ESP_CHIP="${2:?missing value}"; shift 2 ;;
    --version) VERSION="${2:?missing value}"; shift 2 ;;
    --timestamp) TS_ISO="${2:?missing value}"; shift 2 ;;
    --fqbn-extra) FQBN_EXTRA_OPTS="${2:?missing value}"; shift 2 ;;
    --config_json) CONFIG_JSON_RAW="${2:?missing value}"; shift 2 ;;
    *) usage_fail "Unknown arg: $1" ;;
  esac
done

[[ -n "${ESP_CHIP}" && -n "${VERSION}" && -n "${TS_ISO}" ]] || usage_fail "Missing required flag(s)"

case "${ESP_CHIP}" in
  c3) FQBN_BOARD="esp32c3"; CHIP_FAMILY="ESP32-C3" ;;
  c6) FQBN_BOARD="esp32c6"; CHIP_FAMILY="ESP32-C6" ;;
  s3) FQBN_BOARD="esp32s3"; CHIP_FAMILY="ESP32-S3" ;;
  *) usage_fail "Invalid --chip: ${ESP_CHIP} (expected c3, c6, or s3)" ;;
esac

WORK_DIR_PARENT="$(dirname "${WORK_DIR_BASE}")"
WORK_DIR="${WORK_DIR_PARENT}/cache/${ESP_CHIP}"
mkdir -p "${WORK_DIR}"

CONFIG_JSON_VALIDATED='""'
if [[ -n "${CONFIG_JSON_RAW//[[:space:]]/}" ]]; then
  CONFIG_JSON_VALIDATED=$("${PYTHON_BIN}" -c 'import json,sys; print(json.dumps(json.loads(sys.argv[1]), separators=(",",":"), sort_keys=True))' "${CONFIG_JSON_RAW}" 2>/dev/null) || usage_fail "Invalid --config_json (must be valid JSON): ${CONFIG_JSON_RAW}"
fi

FQBN_OPTS="CDCOnBoot=cdc,CPUFreq=160,DebugLevel=none,EraseFlash=all,FlashMode=qio,FlashSize=4M,JTAGAdapter=default,PartitionScheme=no_ota,UploadSpeed=921600${FQBN_EXTRA_OPTS:+,${FQBN_EXTRA_OPTS}}"
FQBN="esp32:esp32:${FQBN_BOARD}:${FQBN_OPTS}"
SKETCH_PATH="$(get_cfg project_ino_file)"

TARGET_DIR="${BUILDS_DIR}/${TS_ISO}-${VERSION}-${ESP_CHIP}-${PROJECT_NAME}"
OUTPUT_DIR="${TARGET_DIR}/output"
BINARY_DIR="${TARGET_DIR}/binary"
mkdir -p "${OUTPUT_DIR}" "${BINARY_DIR}"

echo -e "🔧 Arduino FQBN: ${FQBN}\n📄 Sketch: ${SKETCH_PATH}\n📚 Using libs: ${LIBS_DIR}\n📁 Target dir: ${TARGET_DIR}\n🧰 Work path: ${WORK_DIR}"

COMPILE_ARGS=(compile --fqbn "${FQBN}" --build-path "${WORK_DIR}" --warnings default)
for libdir in "${LIBS_DIR}"/*; do
  [[ -d "${libdir}" ]] || continue
  COMPILE_ARGS+=(--library "${libdir}")
done
COMPILE_ARGS+=("${SKETCH_PATH}")

#--------------------------#
#--- /GET THE VARIABLES ---#
#--------------------------#
#-------------------#
#--- PRE COMPILE ---#
#-------------------#
cp -a "${PROJECT_ROOT}/src" "${TARGET_DIR}/src"
cp -a "${LIBS_DIR}" "${TARGET_DIR}/libs"

#--------------------#
#--- /PRE COMPILE ---#
#--------------------#
#--------------#
#--- COMPILE---#
#--------------#
arduino-cli "${COMPILE_ARGS[@]}" || {
  echo "❌ Compile failed (see ${TARGET_DIR}/compile.log)"
  exit 1
}

#---------------#
#--- /COMPILE---#
#---------------#
#-------------------------#
#--- PROCESS ARTIFACTS ---#
#-------------------------#
MERGED_BIN="$(find "${WORK_DIR}" -maxdepth 1 -name "${PROJECT_NAME}.ino.merged.bin" -print -quit || true)"
MERGED_BIN_FILENAME="${VERSION}-${ESP_CHIP}-${PROJECT_NAME}.bin"
cp -a "${MERGED_BIN}" "${BINARY_DIR}/${MERGED_BIN_FILENAME}"

cat > "${BINARY_DIR}/manifest.json" <<EOF
{
  "name": "${PROJECT_NAME}",
  "version": "${VERSION}",
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

ln -sfn "$(basename "${TARGET_DIR}")" "$(get_cfg builds_latest_dir)"

COMPILE_TIME_SEC=$(( $(date -u +%s) - COMPILE_START_EPOCH ))

json_escape() {
  local s="${1-}"
  s="${s//\\/\\\\}"; s="${s//\"/\\\"}"; s="${s//$'\n'/\\n}"; s="${s//$'\r'/\\r}"; s="${s//$'\t'/\\t}"
  echo -n "$s"
}

TARGET_DIR_REL="${PROJECT_NAME}${TARGET_DIR#*${PROJECT_NAME}}"

cat > "${BINARY_DIR}/meta.json" <<EOF
{
  "type": "$(json_escape "${ESP_CHIP}")",
  "chip_family": "$(json_escape "${CHIP_FAMILY}")",
  "project_name": "$(json_escape "${PROJECT_NAME}")",
  "version": "$(json_escape "${VERSION}")",
  "timestamp_param": "$(json_escape "${TS_ISO}")",
  "config": ${CONFIG_JSON_VALIDATED},
  "fqbn": "$(json_escape "${FQBN}")",
  "fqbn_extra": "$(json_escape "${FQBN_EXTRA_OPTS}")",
  "compile_time_sec": ${COMPILE_TIME_SEC},
  "artifacts": {
    "binary_filename": "$(json_escape "${MERGED_BIN_FILENAME}")",
    "path_rel_binary": "$(json_escape "${TARGET_DIR_REL}/binary/${MERGED_BIN_FILENAME}")",
    "path_rel_manifest_json": "$(json_escape "${TARGET_DIR_REL}/binary/manifest.json")",
    "path_rel_meta_json": "$(json_escape "${TARGET_DIR_REL}/binary/meta.json")",
    "path_abs_binary": "$(json_escape "${BINARY_DIR}/${MERGED_BIN_FILENAME}")",
    "path_abs_manifest_json": "$(json_escape "${BINARY_DIR}/manifest.json")",
    "path_abs_meta_json": "$(json_escape "${BINARY_DIR}/meta.json")"
  }
}
EOF

echo -e "🎉 Build complete.\n⏱️ Total compile time: $((COMPILE_TIME_SEC / 60))m $((COMPILE_TIME_SEC % 60))s\n ➤ Final dir : ${TARGET_DIR}\n ➤ Firmware  : ${BINARY_DIR}/firmware.bin\n ➤ Version   : ${VERSION}"
#--------------------------#
#--- /PROCESS ARTIFACTS ---#
#--------------------------#
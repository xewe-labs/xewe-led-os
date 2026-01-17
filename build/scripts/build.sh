#!/usr/bin/env bash
set -euo pipefail

# build.sh — Orchestrates compile → optional upload → optional serial monitor for ESP32.
#
# Key guarantees:
#  - ALL paths and config are computed here and passed to sub-scripts
#  - Port is optional; if omitted, we only compile
#  - Version is incremented (state committed) ONLY if upload succeeds
#  - build_info.h is generated BEFORE compilation so sources can embed it
#
# Usage examples:
#   ./build.sh -t c3
#   ./build.sh -t s3 -p /dev/cu.usbmodem11143201 -b 921600 -l ../lib
#   ./build.sh -t c6 --venv ../tools/venv --no-monitor
#
# Flags:
#   -t, --type         c3|c6|s3            (required)
#   -p, --port         Serial port (optional; if omitted -> compile only)
#   -l, --libs         Path to extra libraries folder (optional)
#   -b, --baud         Upload baud (default: 921600)
#       --venv         Path to python venv for esptool (default: build/tools/venv if exists)
#       --name         Project name override (default: repo folder name)
#       --manifest     Manifest product name (default: XeWe-LedOS)
#       --fqbn-extra   Extra Arduino FQBN opts, e.g. "FlashMode=qio,FlashFreq=80"
#       --no-monitor   Do not open serial monitor after upload
#       --compile-only Compile only (alias for omitting -p)
#   -h, --help

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

usage() {
  sed -n '1,70p' "$0" | sed -n '1,60p' | sed 's/^# \{0,1\}//'
  exit 0
}

# ---------- Parse args ----------
ESP_CHIP=""
ESP_PORT=""
LIBS_DIR=""
ESP_BAUD="921600"
SERIAL_BAUD="9600"
VENV_DIR=""
PROJECT_NAME=""
MANIFEST_NAME=""
FQBN_EXTRA_OPTS=""
DO_MONITOR=1
COMPILE_ONLY=0
BUILD_NOTES=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    -t|--type)       ESP_CHIP="${2:-}"; shift 2 ;;
    -p|--port)       ESP_PORT="${2:-}"; shift 2 ;;
    -l|--libs)       LIBS_DIR="${2:-}"; shift 2 ;;
    -b|--baud)       ESP_BAUD="${2:-}"; shift 2 ;;
    --venv)          VENV_DIR="${2:-}"; shift 2 ;;
    --name)          PROJECT_NAME="${2:-}"; shift 2 ;;
    --manifest)      MANIFEST_NAME="${2:-}"; shift 2 ;;
    --fqbn-extra)    FQBN_EXTRA_OPTS="${2:-}"; shift 2 ;;
    --no-monitor)    DO_MONITOR=0; shift ;;
    --compile-only)  COMPILE_ONLY=1; shift ;;
    -h|--help)       usage ;;
    *) echo "Unknown arg: $1"; usage ;;
  esac
done

[[ -z "${ESP_CHIP}" ]] && { echo "❌ Missing -t|--type (c3|c6|s3)"; exit 1; }
[[ "${ESP_CHIP}" =~ ^(c3|c6|s3)$ ]] || { echo "❌ Invalid chip type: ${ESP_CHIP}"; exit 1; }

# ---------- Paths (all owned here) ----------
BUILD_ROOT="${PROJECT_ROOT}/build"
BUILDS_DIR="${BUILD_ROOT}/builds"
WORK_DIR="${BUILDS_DIR}/cache"         # shared incremental work dir
STATE_FILE="${BUILD_ROOT}/builds/.version_state"
DEFAULT_VENV="${SCRIPT_DIR}/.venv"
[[ -z "${VENV_DIR}" ]] && [[ -d "${DEFAULT_VENV}" ]] && VENV_DIR="${DEFAULT_VENV}"

mkdir -p "${BUILDS_DIR}" "${WORK_DIR}"

# ---------- Require venv (same folder as this script) ----------
if [[ -z "${VENV_DIR}" || ! -x "${VENV_DIR}/bin/python" ]]; then
  echo "❌ Could not find venv at: ${DEFAULT_VENV}"
  echo "   Run setup script: ${SCRIPT_DIR}/setup_build_enviroment.sh"
  exit 1
fi

# ---------- Helpers ----------
chip_to_family_str() {
  case "$1" in
    c3) echo "ESP32-C3" ;;
    c6) echo "ESP32-C6" ;;
    s3) echo "ESP32-S3" ;;
  esac
}
read_kv() { grep -E "^$1=" "$2" | cut -d'=' -f2- || true; }
write_kv() {
  local file="$1" k="$2" v="$3"
  if grep -qE "^${k}=" "${file}"; then
    sed -i.bak -E "s|^${k}=.*|${k}=${v}|" "${file}" && rm -f "${file}.bak"
  else
    echo "${k}=${v}" >> "${file}"
  fi
}

# ---------- Detect project name & manifest name ----------
[[ -z "${PROJECT_NAME}" ]] && PROJECT_NAME="$(basename "${PROJECT_ROOT}")"
[[ -z "${MANIFEST_NAME}" ]] && MANIFEST_NAME="XeWe-LedOS"

# ---------- Initialize version state if needed ----------
if [[ ! -f "${STATE_FILE}" ]]; then
  cat > "${STATE_FILE}" <<EOF
MAJOR=0
MINOR=0
PATCH=0
BUILD_ID=0
LAST_BUILD_TS=
PROJECT=${PROJECT_NAME}
EOF
fi
MAJOR="$(read_kv MAJOR "${STATE_FILE}")"
MINOR="$(read_kv MINOR "${STATE_FILE}")"
PATCH="$(read_kv PATCH "${STATE_FILE}")"
BUILD_ID="$(read_kv BUILD_ID "${STATE_FILE}")"

# ---------- Compute candidate version + timestamps ----------
PATCH_NEXT=$(( 10#$PATCH + 1 ))                            # safe with leading zeros
VERSION_NEXT="$(printf "%d.%d.%03d" "${MAJOR}" "${MINOR}" "${PATCH_NEXT}")"
TS_SHORT="$(date +"%Y%m%d-%H%M%S")"
TS_ISO="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
CHIP_FAMILY="$(chip_to_family_str "${ESP_CHIP}")"
TARGET_DIR_NAME="${TS_SHORT}-${VERSION_NEXT}-${CHIP_FAMILY}-${PROJECT_NAME}"
TARGET_DIR="${BUILDS_DIR}/${TARGET_DIR_NAME}"

# ---------- Prompt for build notes (before build begins) ----------
echo -n "✍️  Build notes (optional, press Enter to skip): "
IFS= read -r BUILD_NOTES || true

# ---------- Generate src/build_info.h BEFORE compile ----------
BUILD_INFO_H="${PROJECT_ROOT}/src/build_info.h"
mkdir -p "$(dirname "${BUILD_INFO_H}")"
cat > "${BUILD_INFO_H}" <<EOF
#pragma once
// Auto-generated by build/scripts/build.sh — DO NOT EDIT.
#define BUILD_VERSION   ${VERSION_NEXT}
#define BUILD_TIMESTAMP ${TS_ISO}
EOF
echo "📝 Wrote ${BUILD_INFO_H}"

# ---------- Decide phases (port optional) ----------
DO_UPLOAD=0
DO_MONITOR_FINAL=0
if [[ -n "${ESP_PORT}" && "${COMPILE_ONLY}" -eq 0 ]]; then
  DO_UPLOAD=1
  DO_MONITOR_FINAL=${DO_MONITOR}
fi

# ---------- Compose library search paths (all passed to compile.sh) ----------
LIBS_LIST=""
if [[ -n "${LIBS_DIR}" ]]; then
  if [[ -d "${LIBS_DIR}" ]]; then
    LIBS_LIST="${LIBS_DIR}"
  else
    echo "⚠️  Provided libs path doesn't exist: ${LIBS_DIR}"
  fi
fi
# Also include project lib/ if present
if [[ -d "${PROJECT_ROOT}/lib" ]]; then
  if [[ -z "${LIBS_LIST}" ]]; then
    LIBS_LIST="${PROJECT_ROOT}/lib"
  else
    LIBS_LIST="${LIBS_LIST}:${PROJECT_ROOT}/lib"
  fi
fi

# ---------- Compile ----------
echo "🧱 Compile → ${TARGET_DIR_NAME}"
./compile.sh \
  -t "${ESP_CHIP}" \
  --project-root "${PROJECT_ROOT}" \
  --builds-dir "${BUILDS_DIR}" \
  --work-dir "${WORK_DIR}" \
  --target-dir "${TARGET_DIR}" \
  --project-name "${PROJECT_NAME}" \
  --manifest-name "${MANIFEST_NAME}" \
  --version "${VERSION_NEXT}" \
  --timestamp "${TS_ISO}" \
  ${LIBS_LIST:+--libs "${LIBS_LIST}"} \
  ${FQBN_EXTRA_OPTS:+--fqbn-extra "${FQBN_EXTRA_OPTS}"}

# Maintain 'latest' symlink under builds/ (points to this compile result)
ln -sfn "${TARGET_DIR}" "${BUILDS_DIR}/latest"

# ---------- Persist build notes ----------
if [[ -n "${BUILD_NOTES//[[:space:]]/}" ]]; then
  printf "%s\n" "${BUILD_NOTES}" > "${TARGET_DIR}/build_notes.txt"
  echo "📝 Wrote ${TARGET_DIR}/build_notes.txt"
fi

# ---------- Optional upload ----------
if [[ "${DO_UPLOAD}" -eq 1 ]]; then
  echo "📤 Upload → ${ESP_PORT} @ ${ESP_BAUD}"
  ./upload.sh \
    -t "${ESP_CHIP}" \
    -p "${ESP_PORT}" \
    -b "${ESP_BAUD}" \
    --build-dir "${TARGET_DIR}" \
    ${VENV_DIR:+--venv "${VENV_DIR}"} || { echo "❌ Upload failed"; exit 1; }

  # Commit version ONLY on successful upload
  write_kv "${STATE_FILE}" PATCH "$(printf "%03d" "${PATCH_NEXT}")"
  write_kv "${STATE_FILE}" BUILD_ID "$(( 10#${BUILD_ID} + 1 ))"
  write_kv "${STATE_FILE}" LAST_BUILD_TS "${TS_SHORT}"
  write_kv "${STATE_FILE}" PROJECT "${PROJECT_NAME}"
  echo "✅ Version committed → ${VERSION_NEXT}"

  # Publish only the two artifacts. No new files are created in the repo.
  "${SCRIPT_DIR}/push_to_git.sh" \
    --project-root "${PROJECT_ROOT}" \
    --target-dir   "${TARGET_DIR}" \
    --version      "${VERSION_NEXT}"
else
  echo "ℹ️  No port provided (or --compile-only). Skipping upload; version NOT incremented."
fi

# ---------- Optional serial monitor ----------
if [[ "${DO_MONITOR_FINAL}" -eq 1 ]]; then
  echo "🖥️  Serial monitor (Ctrl-C to exit)…"
  ./listen_serial.sh -p "${ESP_PORT}" -b "${SERIAL_BAUD}"
fi

echo
echo "✅ Done."

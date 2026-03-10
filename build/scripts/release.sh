#!/usr/bin/env bash
set -euo pipefail

# release.sh — Orchestrates a multi-chip release with CSV Build Matrix support.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# ---------- Config ----------
BUILD_ROOT="${PROJECT_ROOT}/build"
BUILDS_DIR="${BUILD_ROOT}/builds"
WORK_DIR="${BUILDS_DIR}/cache"
STATE_FILE="${BUILD_ROOT}/builds/.version_state"
CONFIG_FILE="${PROJECT_ROOT}/Config.h"
DEFAULT_VENV="${SCRIPT_DIR}/.venv"

# Static Directory Config
STATIC_RELEASES_ROOT="${PROJECT_ROOT}/static/firmware/releases"

REMOTE="origin"
BRANCH="binaries"

# ---------- 1. Argument Parsing ----------
MATRIX_FILE=""
LIBS_DIR=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --matrix) MATRIX_FILE="$2"; shift 2 ;;
    -l|--libs) LIBS_DIR="$2"; shift 2 ;;
    -*) echo "❌ Unknown option: $1"; exit 1 ;;
    *)  echo "❌ Positional chips are deprecated. Usage: $0 --matrix <build_matrix.csv> [-l path/to/libs]"; exit 1 ;;
  esac
done

if [[ -z "${MATRIX_FILE}" ]]; then
  echo "❌ Usage: $0 --matrix <build_matrix.csv> [-l path/to/libs]"
  exit 1
fi

if [[ ! -f "${MATRIX_FILE}" ]]; then
  echo "❌ Matrix file not found: ${MATRIX_FILE}"
  exit 1
fi

if [[ ! -f "${CONFIG_FILE}" ]]; then
  echo "❌ Config file not found at: ${CONFIG_FILE}"
  exit 1
fi

# ---------- Default Libs Check ----------
if [[ -z "${LIBS_DIR}" ]]; then
  DEFAULT_LIBS="${SCRIPT_DIR}/../libraries"
  if [[ -d "${DEFAULT_LIBS}" ]]; then
    LIBS_DIR="${DEFAULT_LIBS}"
    echo "ℹ️  Using default libraries at: ${LIBS_DIR}"
  fi
fi

# ---------- Compose library search paths ----------
LIBS_LIST=""
if [[ -n "${LIBS_DIR}" ]]; then
  if [[ -d "${LIBS_DIR}" ]]; then
    LIBS_LIST="${LIBS_DIR}"
  else
    echo "⚠️  Provided libs path doesn't exist: ${LIBS_DIR}"
  fi
fi

if [[ -d "${PROJECT_ROOT}/lib" ]]; then
  if [[ -z "${LIBS_LIST}" ]]; then
    LIBS_LIST="${PROJECT_ROOT}/lib"
  else
    LIBS_LIST="${LIBS_LIST}:${PROJECT_ROOT}/lib"
  fi
fi

# ---------- Helpers ----------
read_kv() { grep -E "^$1=" "$2" | cut -d'=' -f2- || true; }
write_kv() {
  local file="$1" k="$2" v="$3"
  if grep -qE "^${k}=" "${file}"; then
    sed -i.bak -E "s|^${k}=.*|${k}=${v}|" "${file}" && rm -f "${file}.bak"
  else
    echo "${k}=${v}" >> "${file}"
  fi
}

update_matrix_config() {
    local pin="$1" type="$2" max="$3" order="$4" file="$5"
    # Replace the exact define lines regardless of current value/spacing
    sed -i.bak -E "s/^#define PIN_LED_STRIP.*/#define PIN_LED_STRIP               ${pin}/" "${file}"
    sed -i.bak -E "s/^#define LED_STRIP_TYPE.*/#define LED_STRIP_TYPE              ${type}/" "${file}"
    sed -i.bak -E "s/^#define LED_STRIP_NUM_LEDS_MAX.*/#define LED_STRIP_NUM_LEDS_MAX      ${max}/" "${file}"
    sed -i.bak -E "s/^#define LED_STRIP_COLOR_ORDER.*/#define LED_STRIP_COLOR_ORDER       ${order}/" "${file}"
    rm -f "${file}.bak"
}

chip_to_family() { case "$1" in c3) echo "ESP32-C3";; c6) echo "ESP32-C6";; s3) echo "ESP32-S3";; esac; }

write_meta_json() {
  # build_dir, TS_SHORT, TS_ISO, VER, FAMILY, PROJ, PIN, TYPE, ORDER
  local build_dir="$1" ts_short="$2" ts_iso="$3" ver="$4" family="$5" proj="$6" pin="$7" type="$8" order="$9"

  cat > "${build_dir}/meta.json" <<EOF
{
  "timestamp": "${ts_short}",
  "timestamp_iso": "${ts_iso}",
  "version": "${ver}",
  "chip_family": "${family}",
  "project_name": "${proj}",
  "pin": "${pin}",
  "led_strip_type": "${type}",
  "color_order": "${order}"
}
EOF
}

# ---------- 2. Read & Validate Version ----------
[[ ! -f "${STATE_FILE}" ]] && { echo "❌ Missing ${STATE_FILE}"; exit 1; }

CUR_MAJOR="$(read_kv MAJOR "${STATE_FILE}")"
CUR_MINOR="$(read_kv MINOR "${STATE_FILE}")"
CUR_PATCH="$(read_kv PATCH "${STATE_FILE}")"
PROJECT_NAME_ORIG="$(read_kv PROJECT "${STATE_FILE}")"
[[ -z "${PROJECT_NAME_ORIG}" ]] && PROJECT_NAME_ORIG="$(basename "${PROJECT_ROOT}")"

echo "ℹ️  Current State: ${CUR_MAJOR}.${CUR_MINOR}.${CUR_PATCH}"
echo -n "🎯 Enter Release Version (X.Y.Z): "
read -r INPUT_VER

[[ "${INPUT_VER}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "❌ Invalid format"; exit 1; }
IFS='.' read -r IN_MAJOR IN_MINOR IN_PATCH <<< "${INPUT_VER}"

VER_OK=0
if [[ $((10#$IN_MAJOR)) -gt $((10#$CUR_MAJOR)) ]]; then VER_OK=1;
elif [[ $((10#$IN_MAJOR)) -eq $((10#$CUR_MAJOR)) ]]; then
  if [[ $((10#$IN_MINOR)) -gt $((10#$CUR_MINOR)) ]]; then VER_OK=1;
  elif [[ $((10#$IN_MINOR)) -eq $((10#$CUR_MINOR)) ]]; then
    if [[ $((10#$IN_PATCH)) -ge $((10#$CUR_PATCH)) ]]; then VER_OK=1; fi
  fi
fi
[[ $VER_OK -eq 0 ]] && { echo "❌ Error: Version must be >= Current"; exit 1; }

echo "🚀 Starting Release: ${INPUT_VER}"

# ---------- 2.5. Feature: Release Notes ----------
echo "📝 Opening VI to capture release comments..."
sleep 1
NOTES_TMP="$(mktemp)"
trap 'rm -f "${NOTES_TMP}"' EXIT

{
  echo "Release Version: ${INPUT_VER}"
  echo "Project: ${PROJECT_NAME_ORIG}"
  echo "----------------------------------------"
  echo ""
} > "${NOTES_TMP}"

vi "${NOTES_TMP}"
echo "✅ Release notes captured."

# ---------- 3. Set State to RELEASE Version ----------
TS_SHORT="$(date +"%Y%m%d-%H%M%S")"
TS_ISO="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"

write_kv "${STATE_FILE}" MAJOR "${IN_MAJOR}"
write_kv "${STATE_FILE}" MINOR "${IN_MINOR}"
write_kv "${STATE_FILE}" PATCH "${IN_PATCH}"
write_kv "${STATE_FILE}" LAST_BUILD_TS "${TS_SHORT}"
echo "💾 [1/2] State set to RELEASE: ${INPUT_VER}"

# ---------- 4. Build, Copy & Push Loop ----------
BUILD_INFO_H="${PROJECT_ROOT}/src/build_info.h"
mkdir -p "$(dirname "${BUILD_INFO_H}")"
cat > "${BUILD_INFO_H}" <<EOF
#pragma once
#define BUILD_VERSION   ${INPUT_VER}
#define BUILD_TIMESTAMP ${TS_ISO}
EOF

# Process matrix file line by line (process substitution avoids subshell scoping issues)
while IFS=, read -r CHIP PIN_LED_STRIP LED_STRIP_TYPE LED_STRIP_NUM_LEDS_MAX COLOR_ORDER || [[ -n "$CHIP" ]]; do
    # Clean up variables (removes \r from Windows CSVs and trims whitespace)
    CHIP=$(echo "$CHIP" | tr -d '\r' | xargs)
    PIN_LED_STRIP=$(echo "$PIN_LED_STRIP" | tr -d '\r' | xargs)
    LED_STRIP_TYPE=$(echo "$LED_STRIP_TYPE" | tr -d '\r' | xargs)
    LED_STRIP_NUM_LEDS_MAX=$(echo "$LED_STRIP_NUM_LEDS_MAX" | tr -d '\r' | xargs)
    COLOR_ORDER=$(echo "$COLOR_ORDER" | tr -d '\r' | xargs)

    # Skip empty lines
    [[ -z "$CHIP" ]] && continue

    # Validate chip
    if [[ ! "$CHIP" =~ ^(c3|c6|s3)$ ]]; then
        echo "⚠️  [SKIP] Invalid chip '${CHIP}' in matrix. Continuing..."
        continue
    fi

    CHIP_FAMILY="$(chip_to_family "${CHIP}")"
    echo "🧱 [${CHIP_FAMILY}] Processing Matrix Row: Pin ${PIN_LED_STRIP} | ${LED_STRIP_TYPE} | ${COLOR_ORDER} | Max ${LED_STRIP_NUM_LEDS_MAX}"

    # Overwrite the config file
    update_matrix_config "${PIN_LED_STRIP}" "${LED_STRIP_TYPE}" "${LED_STRIP_NUM_LEDS_MAX}" "${COLOR_ORDER}" "${CONFIG_FILE}"

    # Ensure binary names are unique for this matrix row so they don't overwrite each other
    CURRENT_PROJECT_NAME="${PROJECT_NAME_ORIG}-${CHIP}-pin${PIN_LED_STRIP}-${LED_STRIP_TYPE}-${COLOR_ORDER}"

    # NEW folder schema (underscore-delimited, 7 parts):
    # TS_VER_FAMILY_PROJ_pinX_TYPE_ORDER
    BUILD_DIR_NAME="${TS_SHORT}_${INPUT_VER}_${CHIP_FAMILY}_${PROJECT_NAME_ORIG}-${CHIP}_pin${PIN_LED_STRIP}_${LED_STRIP_TYPE}_${COLOR_ORDER}"
    TARGET_DIR="${BUILDS_DIR}/${BUILD_DIR_NAME}"

    echo "      Compile → ${BUILD_DIR_NAME}"

    # --- TRY / CATCH START ---
    if ! "${SCRIPT_DIR}/compile.sh" \
        -t "${CHIP}" \
        --project-root "${PROJECT_ROOT}" \
        --builds-dir "${BUILDS_DIR}" \
        --work-dir "${WORK_DIR}" \
        --target-dir "${TARGET_DIR}" \
        --project-name "${CURRENT_PROJECT_NAME}" \
        --version "${INPUT_VER}" \
        --timestamp "${TS_ISO}" \
        --venv "${DEFAULT_VENV}" \
        ${LIBS_LIST:+--libs "${LIBS_LIST}"} > /dev/null; then

        echo "⚠️  [SKIP] Compilation failed for ${CHIP} Pin ${PIN_LED_STRIP}. Continuing..."
        continue
    fi
    # --- TRY / CATCH END ---

    # --- Add Release Notes ---
    NOTES_DEST_DIR="${TARGET_DIR}/binary"
    mkdir -p "${NOTES_DEST_DIR}"
    cp "${NOTES_TMP}" "${NOTES_DEST_DIR}/release_notes.txt"

    # --- NEW: meta.json in the BUILD ROOT (keeps new V2 logic) ---
    write_meta_json "${TARGET_DIR}" "${TS_SHORT}" "${TS_ISO}" "${INPUT_VER}" "${CHIP_FAMILY}" "${PROJECT_NAME_ORIG}-${CHIP}" "${PIN_LED_STRIP}" "${LED_STRIP_TYPE}" "${COLOR_ORDER}"
    echo "      ✅ meta.json -> ${BUILD_DIR_NAME}/meta.json"

    # --- Copy 'binary' + meta.json to Static ---
    STATIC_DEST="${STATIC_RELEASES_ROOT}/${INPUT_VER}/${BUILD_DIR_NAME}"
    echo "      📂 Copying artifacts to: ${STATIC_DEST}"
    mkdir -p "${STATIC_DEST}"
    cp -r "${TARGET_DIR}/binary" "${STATIC_DEST}/"
    cp "${TARGET_DIR}/meta.json" "${STATIC_DEST}/"

    echo "      📤 Pushing to '${BRANCH}'..."
    "${SCRIPT_DIR}/push_to_git.sh" --project-root "${PROJECT_ROOT}" --target-dir "${TARGET_DIR}" --version "${INPUT_VER}"

done < <(tail -n +2 "${MATRIX_FILE}")

echo "✅ Release Complete."
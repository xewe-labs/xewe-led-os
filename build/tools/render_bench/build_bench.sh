#!/usr/bin/env bash
set -euo pipefail

# build_bench.sh — build the LED render benchmark firmware matrix: sync baseline vs async render task.
#
# Sources are staged into a work dir and patched there (the source trees are not modified):
#   Config.h   LED_STRIP_NUM_LEDS_MAX, LED_PIN_DATA, LED_PIN_CLOCK
#   Debug.h    DEBUG_RenderPerf 1      (enables the [BENCH] report line)
#   LedStrip.h frame_delay             (20 = shipped 50 fps cap, 1 = uncapped)
#
# Usage:
#   ./build_bench.sh --sync-root ../../../../xewe-led-os-sync
#   ./build_bench.sh --sync-root <path> --chip s3 --leds-max 2000 --frame-ms "20 1" --data-pin 48 --clock-pin 12
#
# Output: <out>/bench-<chip>-<variant>-f<frame_ms>.bin (merged image, flash at 0x0)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ASYNC_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
LIBS_DIR="${ASYNC_ROOT}/build/libraries"

SYNC_ROOT="" CHIP="s3" LEDS_MAX="2000" FRAME_MS_LIST="20 1" DATA_PIN="48" CLOCK_PIN="12"
OUT_DIR="${ASYNC_ROOT}/build/builds/bench"
WORK_DIR="${ASYNC_ROOT}/build/builds/bench/work"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --sync-root) SYNC_ROOT="$(cd "${2:?}" && pwd)"; shift 2 ;;
    --chip)      CHIP="${2:?}"; shift 2 ;;
    --leds-max)  LEDS_MAX="${2:?}"; shift 2 ;;
    --frame-ms)  FRAME_MS_LIST="${2:?}"; shift 2 ;;
    --data-pin)  DATA_PIN="${2:?}"; shift 2 ;;
    --clock-pin) CLOCK_PIN="${2:?}"; shift 2 ;;
    --out)       OUT_DIR="${2:?}"; shift 2 ;;
    *) echo "❌ Unknown arg: $1" >&2; exit 1 ;;
  esac
done

[[ -n "${SYNC_ROOT}" ]] || { echo "❌ --sync-root is required" >&2; exit 1; }

case "${CHIP}" in
  c3) FQBN_BOARD="esp32c3" ;;
  c6) FQBN_BOARD="esp32c6" ;;
  s3) FQBN_BOARD="esp32s3" ;;
  *) echo "❌ Invalid --chip: ${CHIP}" >&2; exit 1 ;;
esac

# same board options as build/scripts/mac/compile.sh
FQBN="esp32:esp32:${FQBN_BOARD}:CDCOnBoot=cdc,CPUFreq=160,DebugLevel=none,EraseFlash=all,FlashMode=qio,FlashSize=4M,JTAGAdapter=default,PartitionScheme=no_ota,UploadSpeed=921600"

LIB_ARGS=()
for libdir in "${LIBS_DIR}"/*; do
  [[ -d "${libdir}" ]] && LIB_ARGS+=(--library "${libdir}")
done
[[ ${#LIB_ARGS[@]} -gt 0 ]] || { echo "❌ No libraries in ${LIBS_DIR} (run setup_build_environment.sh)" >&2; exit 1; }

# replace_or_fail <file> <sed expression> <grep pattern that must match afterwards>
replace_or_fail() {
  sed -i.bak -E "$2" "$1" && rm -f "$1.bak"
  grep -Eq "$3" "$1" || { echo "❌ Patch failed in $1: $2" >&2; exit 1; }
}

mkdir -p "${OUT_DIR}" "${WORK_DIR}"

for VARIANT in sync async; do
  SRC_ROOT="${ASYNC_ROOT}"; [[ "${VARIANT}" == "sync" ]] && SRC_ROOT="${SYNC_ROOT}"

  for FRAME_MS in ${FRAME_MS_LIST}; do
    NAME="bench-${CHIP}-${VARIANT}-f${FRAME_MS}"
    SKETCH="${WORK_DIR}/${NAME}"

    echo "=== ${NAME} (from ${SRC_ROOT})"
    rm -rf "${SKETCH}"
    mkdir -p "${SKETCH}"
    cp -a "${SRC_ROOT}/src" "${SRC_ROOT}/Config.h" "${SKETCH}/"
    cp "$(ls "${SRC_ROOT}"/*.ino | head -n1)" "${SKETCH}/${NAME}.ino"

    replace_or_fail "${SKETCH}/Config.h" \
      "s/^#define LED_STRIP_NUM_LEDS_MAX .*/#define LED_STRIP_NUM_LEDS_MAX      ${LEDS_MAX}/" \
      "^#define LED_STRIP_NUM_LEDS_MAX +${LEDS_MAX}$"
    replace_or_fail "${SKETCH}/Config.h" \
      "s/^#define LED_PIN_DATA .*/#define LED_PIN_DATA                ${DATA_PIN}/" \
      "^#define LED_PIN_DATA +${DATA_PIN}$"
    replace_or_fail "${SKETCH}/Config.h" \
      "s/^#define LED_PIN_CLOCK .*/#define LED_PIN_CLOCK               ${CLOCK_PIN}/" \
      "^#define LED_PIN_CLOCK +${CLOCK_PIN}$"
    replace_or_fail "${SKETCH}/src/Utils/Debug.h" \
      "s/^(#define DEBUG_RenderPerf +)0/\11/" \
      "^#define DEBUG_RenderPerf +1"
    replace_or_fail "${SKETCH}/src/Modules/Hardware/LedStrip/LedStrip.h" \
      "s/(frame_delay +=) [0-9]+;/\1 ${FRAME_MS};/" \
      "frame_delay += ${FRAME_MS};"

    arduino-cli compile --fqbn "${FQBN}" --build-path "${SKETCH}/.build" --warnings default \
      "${LIB_ARGS[@]}" "${SKETCH}" > "${SKETCH}/compile.log" 2>&1 || {
        tail -n 40 "${SKETCH}/compile.log"
        echo "❌ Compile failed: ${SKETCH}/compile.log" >&2
        exit 1
      }

    cp "${SKETCH}/.build/${NAME}.ino.merged.bin" "${OUT_DIR}/${NAME}.bin"
    echo "✅ ${OUT_DIR}/${NAME}.bin"
  done
done

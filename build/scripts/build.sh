#!/usr/bin/env bash
set -euo pipefail

# build.sh — Orchestrates compile → optional upload → optional serial monitor for ESP32.
#
# Usage examples:
#   ./build.sh -c c3
#   ./build.sh -c s3 -p /dev/cu.usbmodem11143201
#   ./build.sh -c c6 --config_json '{"WIFI_SSID": "\"MyNet\"", "DEBUG_LEVEL": "2"}'
#   ./build.sh -c s3 --build_notes "Fixed Wi-Fi stability"
#   ./build.sh -c s3 --build_notes "" # Skips prompt and skips writing notes file
#
# Flags:
#   -c, --chip         c3|c6|s3            (required)
#   -p, --port         Serial port (optional; if omitted -> compile only)
# Optional flags:
#       --config_json <JSON string> (additional params replaced in Config.h)
#   -n  --build_notes <string>      (Pass build notes directly, skipping the prompt)

BUILD_CONFIG_FILE="../build_config"
source "${BUILD_CONFIG_FILE}"

PROJECT_ROOT="$(get_cfg project_root)"
PROJECT_NAME="$(get_cfg project_name)"
STATE_FILE="$(get_cfg build_state_file)"
BUILDS_DIR="$(get_cfg builds_dir)"
CONFIG_FILE="$(get_cfg project_config_h_file)"
PYTHON_BIN="$(get_cfg venv_python_bin)"

TS_ISO="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"

ESP_CHIP="" ESP_PORT="" CONFIG_JSON_RAW=""
ESP_BAUD="921600" SERIAL_BAUD="115200"

BUILD_NOTES=""
BUILD_NOTES_PROVIDED=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    -t|--type|-c|--chip) ESP_CHIP="$2"; shift 2 ;;
    -p|--port)           ESP_PORT="$2"; shift 2 ;;
    --config_json)       CONFIG_JSON_RAW="$2"; shift 2 ;;
    -n | --build_notes)       BUILD_NOTES="$2"; BUILD_NOTES_PROVIDED=1; shift 2 ;;
    *) echo "❌ Unknown arg: $1"; exit 1 ;;
  esac
done

[[ "$ESP_CHIP" =~ ^(c3|c6|s3)$ ]] || { echo "❌ Missing/Invalid chip (c3|c6|s3)"; exit 1; }

# ---------- Update Config.h #defines ----------
if [[ -n "${CONFIG_JSON_RAW//[[:space:]]/}" ]]; then
  echo "⚙️  Injecting JSON configs into ${CONFIG_FILE}..."
  "${PYTHON_BIN}" -c 'import json,sys,re
try:
    d=json.loads(sys.argv[1]); f=sys.argv[2]
    with open(f,"r") as file: c=file.read()
    for k,v in d.items():
        # \g<1> captures "#define " and \g<2> captures the original spacing before the value
        c, n = re.subn(r"(?m)^(#define\s+)"+re.escape(k)+r"(\s+).*$", r"\g<1>"+k+r"\g<2>"+str(v), c)
        if n == 0:
            sys.exit(f"❌ Error: Key \"{k}\" not found in {f}")
    with open(f,"w") as file: file.write(c)
except Exception as e:
    sys.exit(f"❌ JSON parse/write error: {e}")' "${CONFIG_JSON_RAW}" "${CONFIG_FILE}" || exit 1
fi

# ---------- VC Helpers ----------
get_version() {
  if [[ -f "$STATE_FILE" ]]; then
    source "$STATE_FILE"
    echo "${MAJOR:-0}.${MINOR:-0}.${PATCH:-0}"
  else
    echo "0.0.0"
  fi
}

bump_patch() {
  if [[ -f "$STATE_FILE" ]]; then
    source "$STATE_FILE"
    MAJOR="${MAJOR:-0}"
    MINOR="${MINOR:-0}"
    PATCH="${PATCH:-0}"
    BUILD_ID="${BUILD_ID:-0}"

    PATCH=$((PATCH + 1))
    BUILD_ID=$((BUILD_ID + 1))
  else
    # Initialize if missing
    MAJOR=0
    MINOR=0
    PATCH=1
    BUILD_ID=1
  fi

  LAST_BUILD_TS="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"

  cat <<EOF > "$STATE_FILE"
MAJOR=$MAJOR
MINOR=$MINOR
PATCH=$PATCH
BUILD_ID=$BUILD_ID
LAST_BUILD_TS=$LAST_BUILD_TS
EOF
}

VERSION_NEXT="$(get_version)"

# ---------- Inject Build Params into Config.h ----------
echo "⚙️  Injecting build params (Project, Version, Timestamp) into ${CONFIG_FILE}..."
"${PYTHON_BIN}" -c 'import sys,re
try:
    d={"PROJECT_NAME": f"\"{sys.argv[1]}\"", "BUILD_VERSION": f"\"{sys.argv[2]}\"", "BUILD_TIMESTAMP": f"\"{sys.argv[3]}\""}; f=sys.argv[4]
    with open(f,"r") as file: c=file.read()
    if c and not c.endswith("\n"): c += "\n"
    for k,v in d.items():
        pattern = r"(?m)^(#define\s+)"+re.escape(k)+r"(\s+).*$"
        if re.search(pattern, c):
            c = re.sub(pattern, r"\g<1>"+k+r"\g<2>"+v, c)
        else:
            # Append if it doesn'\''t exist
            c += f"#define {k} {v}\n"
    with open(f,"w") as file: file.write(c)
except Exception as e:
    sys.exit(f"❌ Build param injection error: {e}")' "$PROJECT_NAME" "$VERSION_NEXT" "$TS_ISO" "$CONFIG_FILE" || exit 1


if [[ "$BUILD_NOTES_PROVIDED" -eq 0 ]]; then
  read -rp "✍️  Build notes (Enter to skip): " BUILD_NOTES || true
fi

./compile.sh --chip "$ESP_CHIP" --version "$VERSION_NEXT" --timestamp "$TS_ISO" ${FQBN_EXTRA_OPTS:+--fqbn-extra "$FQBN_EXTRA_OPTS"} ${CONFIG_JSON_RAW:+--config_json "$CONFIG_JSON_RAW"}

[[ -n "${BUILD_NOTES//[[:space:]]/}" ]] && echo "$BUILD_NOTES" > "$(get_cfg builds_latest_dir)/build_notes.txt"

# ---------- Upload Execution ----------
if [[ -n "$ESP_PORT" ]]; then
  echo "📤 Upload → $ESP_PORT @ $ESP_BAUD"
  ./upload.sh -c "$ESP_CHIP" -p "$ESP_PORT"

  bump_patch

  echo -e "✅ Version bumped → $(get_version)\n🖥️  Serial monitor..."
  ./listen_serial.sh -p "$ESP_PORT" -b "$SERIAL_BAUD"
else
  echo "ℹ️  No port provided. Upload skipped."
fi

echo -e "\n✅ Done."

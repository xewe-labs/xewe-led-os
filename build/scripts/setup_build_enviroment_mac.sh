#!/usr/bin/env bash
set -euo pipefail

# setup_build_environment.sh — one-time environment setup for this repo.
# - Checks Homebrew; offers to install if missing
# - Checks Arduino CLI; offers to install via brew if missing
# - Checks ESP32 Core; offers to install if missing
# - Checks Python; offers to install (macOS via brew) if missing
# - Creates .venv next to this script
# - Checks esptool; offers to install into .venv if missing
# - Clones Arduino libraries from required_libraries.txt into libraries
# - Creates build_config for reuse by build/upload scripts

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

VENV_DIR="${BUILD_ROOT}/.venv"
LIBRARIES_DIR="${BUILD_ROOT}/libraries"
REQUIREMENTS_FILE="${LIBRARIES_DIR}/required_libraries.txt"
STATE_FILE="${BUILD_ROOT}/version_state"
RELEASE_MATRIX_FILE="${BUILD_ROOT}/release_matrix.csv"
BUILD_CONFIG_FILE="${BUILD_ROOT}/build_config"

ESP32_CORE_FQBN="esp32:esp32"
ESP32_BOARD_MANAGER_URL="https://espressif.github.io/arduino-esp32/package_esp32_index.json"

confirm() {
  local prompt="${1:-Continue?}"
  read -r -p "${prompt} [y/N]: " ans
  case "${ans}" in
    y|Y|yes|YES) return 0 ;;
    *) return 1 ;;
  esac
}

have_cmd() { command -v "$1" >/dev/null 2>&1; }

is_macos() { [[ "$(uname -s)" == "Darwin" ]]; }

ensure_brew_shellenv() {
  if [[ -x /opt/homebrew/bin/brew ]]; then
    eval "$(/opt/homebrew/bin/brew shellenv)"
  elif [[ -x /usr/local/bin/brew ]]; then
    eval "$(/usr/local/bin/brew shellenv)"
  fi
}

install_brew() {
  if ! is_macos; then
    echo "❌ Homebrew auto-install is only implemented for macOS in this script." >&2
    echo "   Install brew manually: https://brew.sh" >&2
    exit 1
  fi
  echo "➡️  Installing Homebrew..." >&2
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  ensure_brew_shellenv
}

ensure_brew() {
  ensure_brew_shellenv
  if have_cmd brew; then
    echo "✅ Homebrew found: $(command -v brew)" >&2
    return 0
  fi

  echo "⚠️  Homebrew not found." >&2
  if confirm "Install Homebrew now?"; then
    install_brew
    have_cmd brew || { echo "❌ Homebrew install did not result in 'brew' on PATH." >&2; exit 1; }
    echo "✅ Homebrew installed: $(command -v brew)" >&2
  else
    echo "❌ Cannot proceed without Homebrew for automated installs." >&2
    exit 1
  fi
}

ensure_arduino_cli() {
  if have_cmd arduino-cli; then
    echo "✅ arduino-cli found: $(arduino-cli version 2>/dev/null || command -v arduino-cli)" >&2
    return 0
  fi

  echo "⚠️  arduino-cli not found." >&2
  if confirm "Install Arduino CLI via Homebrew now?"; then
    brew update
    brew install arduino-cli
    have_cmd arduino-cli || { echo "❌ arduino-cli still not found after install." >&2; exit 1; }
    echo "✅ arduino-cli installed." >&2
  else
    echo "❌ Arduino CLI is required for compile.sh." >&2
    exit 1
  fi
}

ensure_esp32_core() {
  if arduino-cli core list 2>/dev/null | grep -q "^${ESP32_CORE_FQBN}[[:space:]]"; then
    echo "✅ ESP32 core (${ESP32_CORE_FQBN}) is already installed." >&2
    return 0
  fi

  echo "⚠️  ESP32 core not found." >&2
  if confirm "Install ESP32 core (${ESP32_CORE_FQBN}) now?"; then
    echo "➡️  Initializing Arduino config and adding Espressif URL..." >&2
    arduino-cli config init >/dev/null 2>&1 || true
    arduino-cli config add board_manager.additional_urls "${ESP32_BOARD_MANAGER_URL}" >/dev/null 2>&1 || true

    echo "➡️  Updating core index..." >&2
    arduino-cli core update-index

    echo "➡️  Installing ${ESP32_CORE_FQBN}..." >&2
    arduino-cli core install "${ESP32_CORE_FQBN}" || {
      echo "❌ Failed to install ESP32 core." >&2
      exit 1
    }

    echo "✅ ESP32 core installed." >&2
  else
    echo "❌ The ESP32 core is required to build the project." >&2
    exit 1
  fi
}

ensure_libraries() {
  if [[ ! -f "${REQUIREMENTS_FILE}" ]]; then
    echo "⚠️  Requirements file not found at: ${REQUIREMENTS_FILE}" >&2
    echo "    Skipping library installation." >&2
    return 0
  fi

  echo "📦 Checking Arduino libraries in ${LIBRARIES_DIR}..." >&2
  mkdir -p "${LIBRARIES_DIR}"

  while read -r line || [[ -n "$line" ]]; do
    line="$(echo "$line" | xargs)"
    [[ -z "$line" || "$line" =~ ^# ]] && continue

    IFS=' ' read -r -a parts <<< "${line}"

    local repo_url="${parts[0]}"
    local git_args=("${parts[@]:1}")

    local repo_name
    repo_name="$(basename "${repo_url}" .git)"
    local target_path="${LIBRARIES_DIR}/${repo_name}"

    if [[ -d "${target_path}" ]]; then
      echo "   🔹 ${repo_name} already exists." >&2
    else
      echo "   ⬇️  Cloning ${repo_name}..." >&2

      if [[ ${#git_args[@]} -gt 0 ]]; then
        git clone --quiet --depth 1 "${git_args[@]}" "${repo_url}" "${target_path}" || {
          echo "❌ Failed to clone ${repo_url}" >&2
          exit 1
        }
      else
        git clone --quiet --depth 1 "${repo_url}" "${target_path}" || {
          echo "❌ Failed to clone ${repo_url}" >&2
          exit 1
        }
      fi

      rm -rf "${target_path}/.git"
      echo "      (Removed .git from ${repo_name})" >&2
    fi
  done < "${REQUIREMENTS_FILE}"

  echo "✅ Libraries are ready." >&2
}

choose_python() {
  if have_cmd python3; then
    printf "%s" "python3"
    return 0
  fi
  if have_cmd python; then
    printf "%s" "python"
    return 0
  fi
  return 1
}

ensure_python() {
  local py=""
  if py="$(choose_python)"; then
    echo "✅ Python found: $("${py}" --version 2>&1)" >&2
    printf "%s" "${py}"
    return 0
  fi

  echo "⚠️  Python not found (python3/python)." >&2
  if ! is_macos; then
    echo "❌ Auto-install is only implemented for macOS. Install Python 3 and re-run." >&2
    exit 1
  fi

  if confirm "Install Python 3 via Homebrew now?"; then
    brew update
    brew install python
  else
    echo "❌ Python is required (for venv + esptool)." >&2
    exit 1
  fi

  ensure_brew_shellenv

  py="$(choose_python)" || { echo "❌ Python still not found after install." >&2; exit 1; }
  echo "✅ Python installed: $("${py}" --version 2>&1)" >&2
  printf "%s" "${py}"
}

ensure_venv() {
  local py="$1"

  if [[ -d "${VENV_DIR}" && -x "${VENV_DIR}/bin/python" ]]; then
    echo "✅ .venv already exists: ${VENV_DIR}" >&2
    return 0
  fi

  echo "📦 Creating venv at ${VENV_DIR}" >&2
  "${py}" -m venv "${VENV_DIR}"
  [[ -x "${VENV_DIR}/bin/python" ]] || { echo "❌ venv creation failed." >&2; exit 1; }

  "${VENV_DIR}/bin/python" -m pip install --upgrade pip >/dev/null
  echo "✅ venv ready." >&2
}

ensure_esptool() {
  if "${VENV_DIR}/bin/python" -c "import esptool" >/dev/null 2>&1; then
    echo "✅ esptool is present in .venv" >&2
    return 0
  fi

  echo "⚠️  esptool not found in .venv (required for merge/upload fallback)." >&2
  if confirm "Install esptool into .venv now?"; then
    "${VENV_DIR}/bin/python" -m pip install --upgrade esptool
    "${VENV_DIR}/bin/python" -c "import esptool" >/dev/null 2>&1 || {
      echo "❌ esptool install failed." >&2
      exit 1
    }
    echo "✅ esptool installed in .venv" >&2
  else
    echo "❌ esptool is required for compile.sh merge fallback and upload.sh when no merged bin exists." >&2
    exit 1
  fi
}

init_state_file() {
  if [[ ! -f "${STATE_FILE}" ]]; then
    cat > "${STATE_FILE}" <<EOF
MAJOR=0
MINOR=0
PATCH=0
BUILD_ID=0
LAST_BUILD_TS=0
EOF
    echo "✅ Created state file: ${STATE_FILE}" >&2
  else
    echo "✅ State file already exists: ${STATE_FILE}" >&2
  fi
}

init_release_matrix() {
  if [[ ! -f "${RELEASE_MATRIX_FILE}" ]]; then
    cat > "${RELEASE_MATRIX_FILE}" <<EOF
CHIP,LED_PIN_CLOCK,LED_PIN_DATA,_BUILD_NOTES
EOF
    echo "✅ Created release matrix: ${RELEASE_MATRIX_FILE}" >&2
  else
    echo "✅ Release matrix already exists: ${RELEASE_MATRIX_FILE}" >&2
  fi
}

ensure_project_ino() {
  local project_root
  local project_name
  local expected_ino

  project_root="$(cd "${SCRIPT_DIR}/../.." && pwd)"
  project_name="$(basename "${project_root}")"
  expected_ino="${project_root}/${project_name}.ino"

  if [[ -f "${expected_ino}" ]]; then
    echo "✅ Sketch file found: ${expected_ino}" >&2
    return 0
  fi

  shopt -s nullglob
  local ino_files=("${project_root}"/*.ino)
  shopt -u nullglob

  if (( ${#ino_files[@]} > 0 )); then
    echo "⚠️  No matching sketch file found for project root name '${project_name}'." >&2
    echo "    Existing .ino file(s) in project root:" >&2
    for f in "${ino_files[@]}"; do
      echo "      - $(basename "${f}")" >&2
    done
    echo "    Creating: ${expected_ino}" >&2
  else
    echo "⚠️  No .ino file found in project root. Creating: ${expected_ino}" >&2
  fi

  cat > "${expected_ino}" <<EOF
#include "Config.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Hello World");
}

void loop() {
}
EOF

  chmod 644 "${expected_ino}"
  echo "✅ Sketch file ready: ${expected_ino}" >&2
}

ensure_project_config_h() {
  local project_root
  local config_file

  project_root="$(cd "${SCRIPT_DIR}/../.." && pwd)"
  config_file="${project_root}/Config.h"

  if [[ -f "${config_file}" ]]; then
    echo "✅ Config header found: ${config_file}" >&2
    return 0
  fi

  echo "⚠️  Config.h not found in project root. Creating: ${config_file}" >&2

  cat > "${config_file}" <<'EOF'
#ifndef CONFIG_H
#define CONFIG_H

// Project configuration goes here.
// Example:
// #define WIFI_SSID "your-ssid"
// #define WIFI_PASSWORD "your-password"

#endif // CONFIG_H
EOF

  chmod 644 "${config_file}"
  echo "✅ Config header ready: ${config_file}" >&2
}

generate_gitignore() {
  local gitignore_file="${BUILD_ROOT}/.gitignore"
  local required_lines=(
    ".venv/"
    "builds/"
    "libraries/"
    "build_config"
    "version_state"
  )

  touch "${gitignore_file}"

  for line in "${required_lines[@]}"; do
    if ! grep -Fxq "${line}" "${gitignore_file}"; then
      echo "${line}" >> "${gitignore_file}"
    fi
  done

  echo "✅ .gitignore ready: ${gitignore_file}" >&2
}

write_build_config() {
  local py_bin="$1"
  local arduino_cli_path
  local brew_path
  local git_path
  local venv_python
  local venv_pip
  local project_root
  local project_name
  local ino_file
  local config_file

  arduino_cli_path="$(command -v arduino-cli)"
  brew_path="$(command -v brew || true)"
  git_path="$(command -v git || true)"
  venv_python_bin="${VENV_DIR}/bin/python"
  venv_pip="${VENV_DIR}/bin/pip"

  project_root="$(cd "${SCRIPT_DIR}/../.." && pwd)"
  project_name="$(basename "${project_root}")"
  ino_file="${project_root}/${project_name}.ino"
  config_file="${project_root}/Config.h"

  cat > "${BUILD_CONFIG_FILE}" <<EOF
# Auto-generated by setup_build_environment.sh
# Source this file from compile/upload scripts:
#   source "${BUILD_CONFIG_FILE}"

setup_success=true

project_name="${project_name}"
project_root="${project_root}"
project_ino_file="${ino_file}"
project_config_h_file="${config_file}"

build_root="${BUILD_ROOT}"
builds_dir="${BUILD_ROOT}/builds"
builds_cache_dir="${BUILD_ROOT}/builds/cache"
builds_latest_dir="${BUILD_ROOT}/builds/latest"
build_state_file="${STATE_FILE}"

venv_dir="${VENV_DIR}"
venv_python_bin="${venv_python_bin}"
venv_pip="${venv_pip}"

libraries_dir="${LIBRARIES_DIR}"

release_matrix_file="${RELEASE_MATRIX_FILE}"

arduino_cli="${arduino_cli_path}"
brew_bin="${brew_path}"
git_bin="${git_path}"

esp32_core_fqbn="${ESP32_CORE_FQBN}"
esp32_board_manager_url="${ESP32_BOARD_MANAGER_URL}"
build_config_file="${BUILD_CONFIG_FILE}"

####################
# Helper Functions #
####################

get_cfg() {
  local key="\$1"

  if [[ ! "\$key" =~ ^[a-zA-Z_][a-zA-Z0-9_]*$ ]]; then
    echo "invalid key: \$key" >&2
    return 1
  fi

  if [ -n "\${!key+x}" ]; then
    printf '%s\n' "\${!key}"
  else
    echo "missing key: \$key" >&2
    return 1
  fi
}
EOF

  chmod 600 "${BUILD_CONFIG_FILE}"
  echo "✅ Wrote build config: ${BUILD_CONFIG_FILE}" >&2
}

main() {
  ensure_brew
  ensure_arduino_cli
  ensure_esp32_core
  ensure_libraries

  local PY_BIN
  PY_BIN="$(ensure_python)"

  ensure_venv "${PY_BIN}"
  ensure_esptool

  init_state_file
  init_release_matrix
  ensure_project_ino
  ensure_project_config_h
  write_build_config "${PY_BIN}"

  echo >&2
  echo "✅ Setup complete." >&2
  echo "   - arduino-cli: $(command -v arduino-cli)" >&2
  echo "   - python:      $(${PY_BIN} --version 2>&1)" >&2
  echo "   - venv:        ${VENV_DIR}" >&2
  echo "   - libraries:   ${LIBRARIES_DIR}" >&2
  echo "   - config:      ${BUILD_CONFIG_FILE}" >&2
  echo >&2
  echo "Next: source ${BUILD_CONFIG_FILE} from your build/ scripts." >&2
}

main "$@"
#!/usr/bin/env bash
set -euo pipefail

# setup_build_environment.sh — one-time environment setup for this repo.
# - macOS: uses Homebrew to install dependencies
# - Linux: uses system package manager where possible; falls back to Arduino CLI install script if needed
# - Checks Arduino CLI; offers to install if missing
# - Checks ESP32 Core; offers to install if missing
# - Checks Python; offers to install if missing
# - Creates .venv next to this script
# - Checks esptool; offers to install into .venv if missing
# - Clones Arduino libraries from required_libraries.txt into libraries (removes .git)
# - Creates version_state and release_matrix.csv if missing
# - Ensures a project .ino and Config.h exist in the project root
# - Creates build_config for reuse by build/upload scripts
# - Updates build/.gitignore with generated/local-only paths

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

# Optional override (Linux fallback installer):
#   export ARDUINO_CLI_VERSION="0.35.3"
# Default is auto-detected latest (best-effort).
ARDUINO_CLI_VERSION="${ARDUINO_CLI_VERSION:-}"

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
is_linux() { [[ "$(uname -s)" == "Linux" ]]; }

is_root() { [[ "$(id -u)" -eq 0 ]]; }

run_root() {
  if is_root; then
    "$@"
  elif have_cmd sudo; then
    sudo "$@"
  else
    echo "❌ Need root privileges to run: $*" >&2
    echo "   Re-run as root or install 'sudo'." >&2
    exit 1
  fi
}

ensure_local_bin_on_path() {
  if is_linux; then
    mkdir -p "${HOME}/.local/bin"
    export PATH="${HOME}/.local/bin:${PATH}"
  fi
}

detect_pkg_mgr() {
  if have_cmd apt-get; then echo "apt"
  elif have_cmd dnf; then echo "dnf"
  elif have_cmd yum; then echo "yum"
  elif have_cmd pacman; then echo "pacman"
  elif have_cmd zypper; then echo "zypper"
  elif have_cmd apk; then echo "apk"
  else echo ""
  fi
}

# Install packages with the detected package manager (Linux only)
install_pkgs_linux() {
  local mgr
  mgr="$(detect_pkg_mgr)"
  if [[ -z "${mgr}" ]]; then
    echo "❌ No supported Linux package manager found (apt/dnf/yum/pacman/zypper/apk)." >&2
    echo "   Install dependencies manually: git, curl, python3 (+ venv/pip), and re-run." >&2
    exit 1
  fi

  case "${mgr}" in
    apt)
      run_root apt-get update -y
      run_root apt-get install -y "$@"
      ;;
    dnf)
      run_root dnf install -y "$@"
      ;;
    yum)
      run_root yum install -y "$@"
      ;;
    pacman)
      run_root pacman -Sy --noconfirm --needed "$@"
      ;;
    zypper)
      run_root zypper --non-interactive install -y "$@"
      ;;
    apk)
      run_root apk add "$@"
      ;;
  esac
}

# ---------------- macOS: Homebrew helpers ----------------
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
    echo "   On Linux, use your system package manager." >&2
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
    echo "❌ Cannot proceed without Homebrew for automated installs on macOS." >&2
    exit 1
  fi
}
# --------------------------------------------------------

ensure_base_tools_linux() {
  # This script needs git + curl at minimum (libraries + Arduino CLI fallback install).
  local missing=()
  have_cmd git || missing+=("git")
  have_cmd curl || missing+=("curl")

  if ((${#missing[@]})); then
    echo "⚠️  Missing tools: ${missing[*]}" >&2
    if confirm "Install missing tools via system package manager now?"; then
      case "$(detect_pkg_mgr)" in
        apt|dnf|yum|pacman|zypper|apk) install_pkgs_linux ca-certificates "${missing[@]}" ;;
        *) install_pkgs_linux "${missing[@]}" ;;
      esac
    else
      echo "❌ Cannot proceed without: ${missing[*]}" >&2
      exit 1
    fi
  fi
}

# ---------------- Arduino CLI installation (Linux) ----------------
get_latest_arduino_cli_version() {
  # Best-effort. If this fails, returns empty string.
  # Uses GitHub API; no jq dependency.
  curl -fsSL https://api.github.com/repos/arduino/arduino-cli/releases/latest \
    | sed -n 's/.*"tag_name":[[:space:]]*"\(v\{0,1\}[0-9.]\+\)".*/\1/p' \
    | head -n 1 \
    | sed 's/^v//'
}

install_arduino_cli_via_script_linux() {
  ensure_local_bin_on_path

  local ver="${ARDUINO_CLI_VERSION}"
  if [[ -z "${ver}" ]]; then
    ver="$(get_latest_arduino_cli_version || true)"
  fi
  if [[ -z "${ver}" ]]; then
    ver="latest"
  fi

  local install_dir="/usr/local/bin"
  if ! is_root; then
    if [[ -w "${install_dir}" ]]; then
      :
    elif have_cmd sudo; then
      :
    else
      install_dir="${HOME}/.local/bin"
    fi
  fi

  echo "➡️  Installing Arduino CLI (version: ${ver}) to ${install_dir} ..." >&2
  mkdir -p "${install_dir}" 2>/dev/null || true

  if [[ "${install_dir}" == "/usr/local/bin" ]]; then
    if is_root; then
      curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh \
        | BINDIR="${install_dir}" sh -s -- "${ver}"
    else
      run_root sh -c "curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR='${install_dir}' sh -s -- '${ver}'"
    fi
  else
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh \
      | BINDIR="${install_dir}" sh -s -- "${ver}"
    export PATH="${install_dir}:${PATH}"
    echo "⚠️  Installed to ${install_dir}." >&2
    echo "   Ensure it's on your PATH for future shells, e.g.:" >&2
    echo "     echo 'export PATH=\"${install_dir}:\$PATH\"' >> ~/.bashrc" >&2
  fi
}

ensure_arduino_cli() {
  if have_cmd arduino-cli; then
    echo "✅ arduino-cli found: $(arduino-cli version 2>/dev/null || command -v arduino-cli)" >&2
    return 0
  fi

  echo "⚠️  arduino-cli not found." >&2

  if is_macos; then
    if confirm "Install Arduino CLI via Homebrew now?"; then
      brew update
      brew install arduino-cli
      have_cmd arduino-cli || { echo "❌ arduino-cli still not found after install." >&2; exit 1; }
      echo "✅ arduino-cli installed." >&2
    else
      echo "❌ Arduino CLI is required for compile.sh." >&2
      exit 1
    fi
    return 0
  fi

  if is_linux; then
    if confirm "Install Arduino CLI now? (package manager if available; otherwise official installer)"; then
      local mgr
      local installed=0
      mgr="$(detect_pkg_mgr)"

      if [[ -n "${mgr}" ]]; then
        set +e
        case "${mgr}" in
          apt) run_root apt-get update -y && run_root apt-get install -y arduino-cli ;;
          dnf) run_root dnf install -y arduino-cli ;;
          yum) run_root yum install -y arduino-cli ;;
          pacman) run_root pacman -Sy --noconfirm --needed arduino-cli ;;
          zypper) run_root zypper --non-interactive install -y arduino-cli ;;
          apk) run_root apk add arduino-cli ;;
        esac
        if have_cmd arduino-cli; then installed=1; fi
        set -e
      fi

      if [[ "${installed}" -eq 0 ]]; then
        echo "ℹ️  Package install unavailable/failed; using official installer..." >&2
        install_arduino_cli_via_script_linux
      fi

      have_cmd arduino-cli || { echo "❌ arduino-cli still not found after install." >&2; exit 1; }
      echo "✅ arduino-cli installed: $(arduino-cli version 2>/dev/null || command -v arduino-cli)" >&2
      return 0
    else
      echo "❌ Arduino CLI is required for compile.sh." >&2
      exit 1
    fi
  fi

  echo "❌ Unsupported OS. Install Arduino CLI manually and re-run." >&2
  exit 1
}
# ------------------------------------------------------------------

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

ensure_python_linux() {
  local py=""
  if py="$(choose_python)"; then
    echo "✅ Python found: $("${py}" --version 2>&1)" >&2
    printf "%s" "${py}"
    return 0
  fi

  echo "⚠️  Python not found (python3/python)." >&2
  if confirm "Install Python 3 (and venv/pip) via system package manager now?"; then
    local mgr
    mgr="$(detect_pkg_mgr)"
    case "${mgr}" in
      apt) install_pkgs_linux python3 python3-venv python3-pip ;;
      dnf|yum) install_pkgs_linux python3 python3-pip ;;
      pacman) install_pkgs_linux python python-pip ;;
      zypper) install_pkgs_linux python3 python3-pip ;;
      apk) install_pkgs_linux python3 py3-pip py3-virtualenv ;;
      *) install_pkgs_linux python3 ;;
    esac
  else
    echo "❌ Python is required (for venv + esptool)." >&2
    exit 1
  fi

  py="$(choose_python)" || { echo "❌ Python still not found after install." >&2; exit 1; }
  echo "✅ Python installed: $("${py}" --version 2>&1)" >&2
  printf "%s" "${py}"
}

ensure_python_macos() {
  local py=""
  if py="$(choose_python)"; then
    echo "✅ Python found: $("${py}" --version 2>&1)" >&2
    printf "%s" "${py}"
    return 0
  fi

  echo "⚠️  Python not found (python3/python)." >&2
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

ensure_python() {
  if is_macos; then
    ensure_python_macos
  elif is_linux; then
    ensure_python_linux
  else
    local py=""
    if py="$(choose_python)"; then
      echo "✅ Python found: $("${py}" --version 2>&1)" >&2
      printf "%s" "${py}"
      return 0
    fi
    echo "❌ Unsupported OS. Install Python 3 manually and re-run." >&2
    exit 1
  fi
}

ensure_venv() {
  local py="$1"

  if [[ -d "${VENV_DIR}" && -x "${VENV_DIR}/bin/python" ]]; then
    echo "✅ .venv already exists: ${VENV_DIR}" >&2
    return 0
  fi

  echo "📦 Creating venv at ${VENV_DIR}" >&2
  "${py}" -m venv "${VENV_DIR}" 2>/dev/null || {
    echo "❌ venv creation failed." >&2
    if is_linux; then
      echo "   On Debian/Ubuntu, install: python3-venv" >&2
    fi
    exit 1
  }
  [[ -x "${VENV_DIR}/bin/python" ]] || { echo "❌ venv creation failed." >&2; exit 1; }

  # Ensure pip exists in the venv (some distros require ensurepip)
  if ! "${VENV_DIR}/bin/python" -m pip --version >/dev/null 2>&1; then
    "${VENV_DIR}/bin/python" -m ensurepip --upgrade >/dev/null 2>&1 || true
  fi

  "${VENV_DIR}/bin/python" -m pip install --upgrade pip >/dev/null 2>&1 || {
    echo "❌ pip is not available in the venv." >&2
    if is_linux; then
      echo "   Install pip/venv packages and re-run. Examples:" >&2
      echo "   - Debian/Ubuntu: sudo apt-get install -y python3-venv python3-pip" >&2
      echo "   - Fedora:        sudo dnf install -y python3-pip" >&2
    fi
    exit 1
  }

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

write_build_config() {
  local py_bin="$1"
  local arduino_cli_path
  local brew_path
  local git_path
  local venv_python_bin
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

python_cmd="${py_bin}"

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
  ensure_local_bin_on_path

  if is_macos; then
    ensure_brew
  elif is_linux; then
    ensure_base_tools_linux
  else
    echo "❌ Unsupported OS. This script supports macOS and Linux." >&2
    exit 1
  fi

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
  echo "Next: source ${BUILD_CONFIG_FILE} from your build/upload scripts." >&2
}

main "$@"
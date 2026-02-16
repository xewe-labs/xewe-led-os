#!/usr/bin/env bash
set -euo pipefail

# setup_build_enviroment.sh — one-time environment setup for this repo.
# - Checks Homebrew; offers to install if missing
# - Checks Arduino CLI; offers to install via brew if missing
# - Checks ESP32 Core; offers to install if missing
# - Checks Python; offers to install (macOS via brew) if missing
# - Creates .venv next to this script
# - Checks esptool; offers to install into .venv if missing
# - Clones Arduino libraries from ../required_libraries.txt into ../libraries (removes .git) <-- NEW

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${SCRIPT_DIR}/.venv"

# Define library paths relative to script location (build/scripts/)
# According to instructions, requirements is in ../required_libraries.txt (so, build/required_libraries.txt)
# If your file is actually in the project root, change this to "../../required_libraries.txt"
REQUIREMENTS_FILE="${SCRIPT_DIR}/../required_libraries.txt"
LIBRARIES_DIR="${SCRIPT_DIR}/../libraries"

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
    echo "✅ arduino-cli found: $(arduino-cli version 2>/dev/null || echo "$(command -v arduino-cli)")" >&2
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
  if arduino-cli core list 2>/dev/null | grep -q "esp32:esp32"; then
    echo "✅ ESP32 core (esp32:esp32) is already installed." >&2
    return 0
  fi

  echo "⚠️  ESP32 core not found." >&2
  if confirm "Install ESP32 core (esp32:esp32) now?"; then
    echo "➡️  Initializing Arduino config and adding Espressif URL..." >&2
    arduino-cli config init >/dev/null 2>&1 || true
    arduino-cli config add board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json >/dev/null 2>&1 || true

    echo "➡️  Updating core index..." >&2
    arduino-cli core update-index

    echo "➡️  Installing esp32:esp32..." >&2
    arduino-cli core install esp32:esp32 || { echo "❌ Failed to install ESP32 core."; exit 1; }

    echo "✅ ESP32 core installed." >&2
  else
    echo "❌ The ESP32 core is required to build the project." >&2
    exit 1
  fi
}

# --- NEW FUNCTION FOR LIBRARIES ---
ensure_libraries() {
  if [[ ! -f "${REQUIREMENTS_FILE}" ]]; then
    echo "⚠️  Requirements file not found at: ${REQUIREMENTS_FILE}" >&2
    echo "    Skipping library installation." >&2
    return 0
  fi

  echo "📦 Checking Arduino libraries in ${LIBRARIES_DIR}..." >&2
  mkdir -p "${LIBRARIES_DIR}"

  while read -r line || [[ -n "$line" ]]; do
    # Trim whitespace from the whole line
    line=$(echo "$line" | xargs)

    # Skip empty lines or comments
    [[ -z "$line" || "$line" =~ ^# ]] && continue

    # 1. Split the line into an array based on spaces
    #    Example: "https://...git --branch 3.10.3"
    #    Becomes: parts[0]="https://...git", parts[1]="--branch", parts[2]="3.10.3"
    IFS=' ' read -r -a parts <<< "$line"

    local repo_url="${parts[0]}"

    # 2. Extract specific args (all parts after the URL)
    local git_args="${parts[@]:1}"

    # 3. Extract repo name from the URL only
    local repo_name
    repo_name=$(basename "${repo_url}" .git)
    local target_path="${LIBRARIES_DIR}/${repo_name}"

    if [[ -d "${target_path}" ]]; then
      echo "   🔹 ${repo_name} already exists." >&2
    else
      echo "   ⬇️  Cloning ${repo_name}..." >&2

      # 4. Inject $git_args into the command (unquoted so flags expand correctly)
      git clone --quiet --depth 1 $git_args "${repo_url}" "${target_path}" || { echo "❌ Failed to clone ${repo_url}"; exit 1; }

      # CRITICAL: Remove .git folder to keep the root repo clean
      rm -rf "${target_path}/.git"
      echo "      (Removed .git from ${repo_name})" >&2
    fi
  done < "${REQUIREMENTS_FILE}"

  echo "✅ Libraries are ready." >&2
}
# ----------------------------------

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
    echo "✅ Python found: $(${py} --version 2>&1)" >&2
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
  echo "✅ Python installed: $(${py} --version 2>&1)" >&2
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
    "${VENV_DIR}/bin/python" -c "import esptool" >/dev/null 2>&1 || { echo "❌ esptool install failed." >&2; exit 1; }
    echo "✅ esptool installed in .venv" >&2
  else
    echo "❌ esptool is required for compile.sh merge fallback and upload.sh when no merged bin exists." >&2
    exit 1
  fi
}

main() {
  ensure_brew
  ensure_arduino_cli
  ensure_esp32_core

  # --- New Step: Install Libraries ---
  ensure_libraries

  local PY_BIN
  PY_BIN="$(ensure_python)"

  ensure_venv "${PY_BIN}"
  ensure_esptool

  echo >&2
  echo "✅ Setup complete." >&2
  echo "   - arduino-cli: $(command -v arduino-cli)" >&2
  echo "   - python:      $(${PY_BIN} --version 2>&1)" >&2
  echo "   - venv:        ${VENV_DIR}" >&2
  echo "   - libraries:   ${LIBRARIES_DIR}" >&2
  echo >&2
  echo "Next: use your build.sh script."
}

main "$@"
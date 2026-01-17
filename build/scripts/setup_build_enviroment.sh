#!/usr/bin/env bash
set -euo pipefail

# setup_build_enviroment.sh — one-time environment setup for this repo.
# - Checks Homebrew; offers to install if missing
# - Checks Arduino CLI; offers to install via brew if missing
# - Checks Python; offers to install (macOS via brew) if missing
# - Creates .venv next to this script
# - Checks esptool; offers to install into .venv if missing

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${SCRIPT_DIR}/.venv"

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
  # Add brew to PATH for this script session (covers fresh installs and PATH issues).
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

  # Ensure brew's python is visible in this process.
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

  local PY_BIN
  PY_BIN="$(ensure_python)"

  ensure_venv "${PY_BIN}"
  ensure_esptool

  echo >&2
  echo "✅ Setup complete." >&2
  echo "   - arduino-cli: $(command -v arduino-cli)" >&2
  echo "   - python:     $(${PY_BIN} --version 2>&1)" >&2
  echo "   - venv:       ${VENV_DIR}" >&2
  echo >&2
  echo "Next: use your build.sh script."
}

main "$@"

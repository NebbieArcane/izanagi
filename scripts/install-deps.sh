#!/usr/bin/env bash
# Dipendenze di build Linux/macOS per l'editor su restyle.
set -euo pipefail

WITH_QT=1

usage() {
  cat <<'EOF'
Usage: ./scripts/install-deps.sh [--no-qt]

Linux (Debian/Ubuntu): build-essential, cmake, qt6-base-dev
macOS (Homebrew): cmake, qt@6
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-qt) WITH_QT=0; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage; exit 1 ;;
  esac
done

uname_s="$(uname -s)"

install_linux() {
  if ! command -v apt-get >/dev/null 2>&1; then
    echo "apt-get non trovato. Installa: cmake, g++, qt6-base-dev" >&2
    exit 1
  fi
  sudo apt-get update
  local packages=(build-essential cmake)
  if [[ "$WITH_QT" -eq 1 ]]; then
    packages+=(qt6-base-dev libgl1-mesa-dev)
  fi
  sudo apt-get install -y "${packages[@]}"
}

install_macos() {
  if ! command -v brew >/dev/null 2>&1; then
    echo "Homebrew richiesto: https://brew.sh — brew install cmake qt@6" >&2
    exit 1
  fi
  if [[ "${GITHUB_ACTIONS:-}" == "true" ]]; then
    export HOMEBREW_NO_REQUIRE_TAP_TRUST=1
  fi
  brew install cmake
  if [[ "$WITH_QT" -eq 1 ]]; then
    brew install qt@6
    echo "Qt 6 prefix: $(brew --prefix qt@6)"
  fi
}

case "$uname_s" in
  Linux) install_linux ;;
  Darwin) install_macos ;;
  *)
    echo "OS non supportato da install-deps.sh: ${uname_s}" >&2
    exit 1
    ;;
esac

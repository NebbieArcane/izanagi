#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/build"
BUILD_TYPE="${1:-Release}"
JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

CMAKE_ARGS=(-S "${ROOT}" -B "${BUILD}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}")

# Su macOS Homebrew Qt non e' nel path di default di CMake.
if [[ "$(uname -s)" == "Darwin" ]]; then
  if [[ -z "${CMAKE_PREFIX_PATH:-}" ]]; then
    QT_PREFIX=""
    if command -v brew >/dev/null 2>&1; then
      for formula in qt qt@6 qt6; do
        if brew --prefix "${formula}" >/dev/null 2>&1; then
          candidate="$(brew --prefix "${formula}")"
          if [[ -d "${candidate}/lib/cmake/Qt6" || -d "${candidate}/lib/cmake/Qt6Core" ]]; then
            QT_PREFIX="${candidate}"
            break
          fi
        fi
      done
    fi
    if [[ -z "${QT_PREFIX}" ]]; then
      echo "Qt 6 non trovato. Installa con: brew install qt cmake" >&2
      exit 1
    fi
    export CMAKE_PREFIX_PATH="${QT_PREFIX}"
    echo "Usando Qt da: ${QT_PREFIX}"
  fi
  CMAKE_ARGS+=(-DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH}")
fi

cmake "${CMAKE_ARGS[@]}"
cmake --build "${BUILD}" -j"${JOBS}"
ctest --test-dir "${BUILD}" --output-on-failure

if [[ "$(uname -s)" == "Darwin" ]]; then
  echo "App: ${BUILD}/app/eleutheria.app"
  echo "Avvio: open ${BUILD}/app/eleutheria.app"
else
  echo "Binary: ${BUILD}/app/eleutheria"
fi

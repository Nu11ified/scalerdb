#!/usr/bin/env bash
# make.sh — build (and optionally test) the project
# Usage: ./make.sh [build_dir] [--test]
set -euo pipefail

BUILD_DIR="${1:-build}"
RUN_TESTS=false

if [[ "${2:-}" == "--test" ]]; then
  RUN_TESTS=true
fi

# Detect number of CPU cores for parallel build
if command -v nproc &>/dev/null; then
  JOBS=$(nproc)
else
  JOBS=$(sysctl -n hw.logicalcpu || echo 4)
fi

echo "[make] Building project in '${BUILD_DIR}' with ${JOBS} jobs..."
cmake --build "${BUILD_DIR}" --parallel "${JOBS}"

echo "[make] Build complete."

if $RUN_TESTS; then
  echo "[make] Running tests..."
  ctest --test-dir "${BUILD_DIR}"
fi 
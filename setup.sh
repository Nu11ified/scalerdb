#!/usr/bin/env bash
# setup.sh — configure the CMake build directory
# Usage: ./setup.sh [build_dir]
set -euo pipefail

BUILD_DIR="${1:-build}"

echo "[setup] Configuring project in '${BUILD_DIR}'..."
cmake -B "${BUILD_DIR}" -S . -DCMAKE_BUILD_TYPE=Debug

echo "[setup] Done. Now run ./make.sh ${BUILD_DIR} to build the project." 
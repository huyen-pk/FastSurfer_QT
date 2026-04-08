#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
NPROC="${NPROC:-$(nproc || echo 1)}"

echo "Configuring FastSurfer core in: $BUILD_DIR"
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DBUILD_TESTING=ON -DFASTSURFER_CORE_BUILD_TESTS=ON

echo "Building (parallel jobs: $NPROC)"
cmake --build "$BUILD_DIR" --parallel "$NPROC"

echo "Build complete. To run tests:"
echo "  cmake --build \"$BUILD_DIR\" --target test"
echo "or"
echo "  ctest --test-dir \"$BUILD_DIR\" --output-on-failure"

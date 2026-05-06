#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

resolve_build_dir() {
	local requested_dir="$1"
	mkdir -p "$requested_dir"
	cd "$requested_dir" && pwd
}

# Default build dir stays local to core/ so it matches the relocated module layout
# and the validated test workflow. Can be overridden by passing a path as the first
# argument or by setting the BUILD_DIR environment variable.
if [[ -n "${1:-}" ]]; then
	BUILD_DIR="$(resolve_build_dir "$1")"
elif [[ -n "${BUILD_DIR:-}" ]]; then
	BUILD_DIR="$(resolve_build_dir "$BUILD_DIR")"
else
	BUILD_DIR="$SCRIPT_DIR/build"
fi

NPROC="${NPROC:-$(nproc || echo 1)}"

echo "Configuring Open Healthcare imaging MRI FastSurfer core in: $BUILD_DIR"
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DOPENHC_IMAGING_MRI_FASTSURFER_BUILD_TESTS=ON

echo "Building (parallel jobs: $NPROC)"
cmake --build "$BUILD_DIR" --parallel "$NPROC"

echo "Build complete. To run tests:"
echo "  cmake --build \"$BUILD_DIR\" --target test"
echo "or"
echo "  ctest --test-dir \"$BUILD_DIR\" --output-on-failure"

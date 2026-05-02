#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# repo root is two levels up from _app/core (fastsurfer repo root)
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
# Default build dir is out-of-source under repo root to avoid polluting source tree.
# Can be overridden by passing a path as first argument or by setting the BUILD_DIR env var.
if [ -n "${1:-}" ]; then
	BUILD_DIR="$(cd "${1}" && pwd)"
elif [ -n "${BUILD_DIR:-}" ]; then
	BUILD_DIR="$(cd "${BUILD_DIR}" && pwd)"
else
	BUILD_DIR="$REPO_ROOT/build/core"
fi

NPROC="${NPROC:-$(nproc || echo 1)}"

echo "Configuring FastSurfer core in: $BUILD_DIR"
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DBUILD_TESTING=ON -DFASTSURFER_CORE_BUILD_TESTS=ON

echo "Building (parallel jobs: $NPROC)"
cmake --build "$BUILD_DIR" --parallel "$NPROC"

echo "Build complete. To run tests:"
echo "  cmake --build \"$BUILD_DIR\" --target test"
echo "or"
echo "  ctest --test-dir \"$BUILD_DIR\" --output-on-failure"

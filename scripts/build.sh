#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "==> Configuring and Building Linux System Monitor & Scheduler..."
mkdir -p "${PROJECT_ROOT}/build"
cd "${PROJECT_ROOT}/build"

cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -- -j$(nproc)

echo "==> Build completed successfully!"
echo "Executables created:"
echo "  - ${PROJECT_ROOT}/build/system_monitor"
echo "  - ${PROJECT_ROOT}/build/run_unit_tests"

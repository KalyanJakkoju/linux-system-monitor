#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

if [ ! -f "${PROJECT_ROOT}/build/run_unit_tests" ]; then
    echo "==> Test executable not found. Running build first..."
    "${SCRIPT_DIR}/build.sh"
fi

echo "==> Executing Automated Unit Tests..."
"${PROJECT_ROOT}/build/run_unit_tests"

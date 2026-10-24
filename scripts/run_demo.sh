#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

if [ ! -f "${PROJECT_ROOT}/build/system_monitor" ]; then
    echo "==> Application executable not found. Running build first..."
    "${SCRIPT_DIR}/build.sh"
fi

BIN="${PROJECT_ROOT}/build/system_monitor"
EXAMPLES="${PROJECT_ROOT}/examples"

echo "=========================================================================="
echo "DEMO 1: System Monitor Snapshot (CPU, Memory, Uptime)"
echo "Command: system_monitor info"
echo "=========================================================================="
"${BIN}" info

echo "=========================================================================="
echo "DEMO 2: Process Enumeration Snapshot"
echo "Command: system_monitor processes"
echo "=========================================================================="
"${BIN}" processes

echo "=========================================================================="
echo "DEMO 3: CPU Scheduling Simulator - FCFS"
echo "Command: system_monitor schedule --algorithm fcfs --input examples/processes.csv"
echo "=========================================================================="
"${BIN}" schedule --algorithm fcfs --input "${EXAMPLES}/processes.csv"

echo "=========================================================================="
echo "DEMO 4: CPU Scheduling Simulator - SJF (Non-Preemptive)"
echo "Command: system_monitor schedule --algorithm sjf --input examples/processes.csv"
echo "=========================================================================="
"${BIN}" schedule --algorithm sjf --input "${EXAMPLES}/processes.csv"

echo "=========================================================================="
echo "DEMO 5: CPU Scheduling Simulator - Round Robin (Quantum = 2)"
echo "Command: system_monitor schedule --algorithm rr --quantum 2 --input examples/processes.csv"
echo "=========================================================================="
"${BIN}" schedule --algorithm rr --quantum 2 --input "${EXAMPLES}/processes.csv"

echo "=========================================================================="
echo "DEMO 6: CPU Scheduling Simulator - Priority Scheduling"
echo "Command: system_monitor schedule --algorithm priority --input examples/processes.csv"
echo "=========================================================================="
"${BIN}" schedule --algorithm priority --input "${EXAMPLES}/processes.csv"

echo "=========================================================================="
echo "DEMO 7: CPU Scheduling Simulator - Side-by-side All Algorithm Comparison"
echo "Command: system_monitor schedule --all --quantum 2 --input examples/processes.csv"
echo "=========================================================================="
"${BIN}" schedule --all --quantum 2 --input "${EXAMPLES}/processes.csv"

echo "=========================================================================="
echo "DEMO 8: Handling Idle Intervals"
echo "Command: system_monitor schedule --algorithm fcfs --input examples/processes_idle.csv"
echo "=========================================================================="
"${BIN}" schedule --algorithm fcfs --input "${EXAMPLES}/processes_idle.csv"

echo "==> All demonstrations finished successfully!"

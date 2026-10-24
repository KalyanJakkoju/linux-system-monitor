# Testing and Verification Guide

This document details the automated test suite, verification methodologies, test case matrices, and execution instructions for the project.

---

## 1. Automated Test Suite Overview

The project includes an in-tree, zero-dependency unit testing framework implemented in `tests/test_framework.hpp`. It provides structured assertions (`ASSERT_TRUE`, `ASSERT_FALSE`, `ASSERT_EQ`, `ASSERT_STR_EQ`, `ASSERT_NEAR`, `ASSERT_THROW`), automatic test case registration, and ANSI-colored test results.

### Summary of Test Suites

| Suite Name | Target Module | Test Count | Key Invariants Tested |
| :--- | :--- | :---: | :--- |
| **`CpuMetrics`** | `sysmon::CpuMonitor` | 6 | 10-field `/proc/stat` parsing, legacy 4-field parsing, core line filtering (`cpuX`), delta math, zero-delta protection, non-monotonic counter exception handling. |
| **`MemoryMetrics`** | `sysmon::MemoryMonitor` | 4 | Standard `meminfo` parsing, `MemAvailable` fallback calculation for legacy kernels, zero `MemTotal` divide-by-zero protection, underflow prevention. |
| **`ProcessParsing`** | `sysmon::ProcessMonitor` | 6 | Standard stat line, process names with spaces, process names with nested parentheses, combinations of spaces and multiple parentheses, state label translations, malformed line rejection. |
| **`Scheduler`** | `simulator::CpuScheduler` | 6 | FCFS exact metric calculation, SJF shortest burst selection, Round Robin preemption & simultaneous event ordering, Priority selection, CPU idle interval handling, CSV workload parsing & strict validation. |

**Total Automated Tests:** 22  
**Passing Tests:** 22 (100% Pass Rate)

---

## 2. Running Automated Tests

### Via Build and Test Scripts
```bash
# Build the project
./scripts/build.sh

# Run the unit test suite
./scripts/run_tests.sh
```

### Direct Execution
```bash
./build/run_unit_tests
```

### Via CTest
```bash
cd build && ctest --output-on-failure
```

---

## 3. Detailed Test Matrix

### 3.1 CPU Metrics Tests (`tests/test_cpu_metrics.cpp`)

| Test Name | Purpose / Scenario | Expected Outcome |
| :--- | :--- | :--- |
| `ParseValid10FieldLine` | Parses modern Linux 10-field `/proc/stat` line. | Extracts user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice accurately. |
| `ParseOld4FieldLine` | Parses older Linux 4-field line (`user nice system idle`). | Successfully populates initial 4 fields; remaining fields default to 0. |
| `IgnorePerCoreLines` | Ensures per-core lines (`cpu0`, `cpu1`) or non-cpu lines (`intr`) are rejected. | Returns `std::nullopt`. |
| `CalculateDeltaUtilization` | Tests delta mathematics across two samples with known tick increases. | Computes 15.0% total utilization, 10.0% user, 5.0% system, 85.0% idle, 5.0% iowait. |
| `ZeroTotalDeltaHandledGracefully` | Two identical samples with zero elapsed ticks. | Returns `is_valid = false`, avoiding division by zero. |
| `CounterDecreaseThrowsException` | Simulates a tick counter decreasing (wrap or reset). | Throws `sysmon::SystemMonitorError` (`ErrorCode::CounterDecreased`). |

### 3.2 Memory Metrics Tests (`tests/test_memory_metrics.cpp`)

| Test Name | Purpose / Scenario | Expected Outcome |
| :--- | :--- | :--- |
| `ParseStandardMeminfo` | Parses standard Linux `/proc/meminfo` with `MemAvailable`. | Correctly computes used memory ($16384000 - 8192000 = 8192000$ kB) and 50.0% utilization. |
| `FallbackWhenMemAvailableMissing` | Kernels < 3.14 without `MemAvailable`. | Estimates available memory as $\text{MemFree} + \text{Buffers} + \text{Cached} = 6000000$ kB (40.0% used). |
| `ZeroMemTotalAvoidsDivisionByZero` | Edge case where `MemTotal` is reported as 0. | Returns 0.0% utilization without division by zero. |
| `AvailableExceedingTotalUnderflowProtection`| Malformed input where `MemAvailable > MemTotal`. | Clamps available memory to `MemTotal`, preventing unsigned integer underflow. |

### 3.3 Process Parsing Tests (`tests/test_process_parsing.cpp`)

| Test Name | Purpose / Scenario | Expected Outcome |
| :--- | :--- | :--- |
| `StandardProcessLine` | Typical stat line: `1234 (systemd) S 1 ...` | Parses PID `1234`, name `"systemd"`, state `'S'`, PPID `1`. |
| `ProcessNameWithSpaces` | Process name with embedded spaces: `5678 (Web Content) R ...` | Parses name as `"Web Content"`, state `'R'`. |
| `ProcessNameWithParentheses` | Process name with nested parentheses: `9999 (worker (io) pool) D ...` | Parses name as `"worker (io) pool"`, state `'D'`. |
| `ProcessNameWithSpacesAndMultipleParentheses` | Tricky name: `42 (name with ) parentheses ( and spaces) I ...` | Accurately extracts full string between first `(` and last `)`. |
| `ProcessStatesTranslation` | Verifies translation of characters `'R'`, `'S'`, `'D'`, `'T'`, `'Z'`, `'I'`. | Matches documented human-readable descriptions. |
| `MalformedStatLineRejected` | Tests lines missing parentheses or containing non-numeric PIDs. | Returns `std::nullopt`. |

### 3.4 CPU Scheduler Tests (`tests/test_scheduler.cpp`)

| Test Name | Purpose / Scenario | Expected Outcome |
| :--- | :--- | :--- |
| `FCFSExactMetrics` | 3 processes arriving at 0, 1, 2 with bursts 4, 3, 1. | Exactly matches manual trace: CT=[4, 7, 8], TAT=[4, 6, 6], WT=[0, 3, 5], RT=[0, 3, 5]. |
| `SJFSelectionAndMetrics` | 4 processes arriving at 0, 1, 2, 3 with bursts 5, 3, 1, 2. | Selects P1(0..5), P3(5..6), P4(6..8), P2(8..11). |
| `RoundRobinPreemptionAndEventOrdering`| 3 processes, quantum=2. Validates arrivals during execution. | Exact 6-slice timeline: P1[0..2], P2[2..4], P3[4..5], P1[5..7], P2[7..8], P1[8..9]. |
| `PriorityNonPreemptive` | Processes with priorities 3, 1, 2. | Selects highest priority job upon P1 completion: P1[0..4], P2[4..7], P3[7..9]. |
| `IdleIntervalHandling` | Process arrives at $T=3$. | Generates `[0 -> 3: IDLE]` slice. CPU utilization = $\frac{4}{7} \times 100 \approx 57.14\%$. |
| `CSVParserAndValidation` | Valid CSV, duplicate PIDs, negative arrival times, zero burst times. | Validates correct parsing and rejection with descriptive exceptions. |

---

## 4. Cross-Algorithm Invariant Verification

In addition to individual algorithm unit tests, the static method `CpuScheduler::verify_schedule_invariants()` is executed after every schedule generation. It mathematically verifies:

1. **Timeline Contiguity:** No gaps or overlapping intervals exist in the execution timeline.
2. **Conservation of CPU Execution:** The total time allocated to process $P_i$ across all execution slices equals its declared burst time $P_i.\text{burst\_time}$.
3. **No Retroactive Execution:** No process slice begins prior to the process arrival time ($t_{\text{start}} \ge \text{arrival\_time}$).
4. **Formula Consistency:**
   $$\text{Turnaround Time} = \text{Completion Time} - \text{Arrival Time}$$
   $$\text{Waiting Time} = \text{Turnaround Time} - \text{Burst Time} \ge 0$$
   $$\text{Response Time} = \text{First CPU Start Time} - \text{Arrival Time} \le \text{Turnaround Time}$$

# Linux System Resource Monitor and CPU Scheduling Simulator in C++

A modular, high-performance systems programming command-line application built in **C++17** that implements:
1. A **Linux System Resource Monitor** that inspects real-time CPU, memory, uptime, and process accounting data directly via the Linux `/proc` pseudo-filesystem.
2. A **CPU Scheduling Simulator** that executes deterministic discrete-event scheduling simulations (FCFS, SJF, Round Robin, and Priority) on synthetic workloads with ASCII Gantt charts and cross-algorithm metric comparisons.

---

## 1. Problem Statement & Motivation

Operating system scheduling algorithms and kernel telemetry are core concepts in systems software engineering. While textbooks often discuss CPU scheduling theoretically, engineering students and systems programmers frequently lack hands-on experience connecting real-world OS telemetry with discrete scheduling mathematics.

This project bridges that gap by implementing two independent subsystems:
* **Real-time Linux Telemetry:** Demonstrating how user-space systems tools like `top`, `ps`, and `free` actually read, parse, and normalize kernel instrumentation from the Virtual Filesystem (`/proc`) without relying on root privileges or external libraries.
* **Deterministic Scheduling Simulator:** Accurately modeling classic CPU scheduling algorithms in user space, rendering timeline Gantt charts, computing performance metrics, and mathematically verifying scheduling invariants.

> **Note on Scope:** The CPU scheduling component is a user-space simulator. It models scheduling algorithms mathematically and does not modify the host Linux kernel scheduler (such as CFS or EEVDF). The monitoring component reads virtual `/proc` interfaces and does not constitute kernel module or device driver development.

---

## 2. Key Features

### Subsystem A: Linux System Resource Monitor
* **CPU Utilization:** Calculates aggregate CPU utilization between two consecutive time samples from `/proc/stat`. Computes exact percentages for user space, kernel/system, idle, and I/O-wait. Handles non-monotonic counter wraps and zero deltas.
* **Memory Telemetry:** Parses `/proc/meminfo` for `MemTotal`, `MemAvailable`, `MemFree`, `Buffers`, `Cached`, and `Swap`. Computes true memory utilization using `MemAvailable`, with automated fallback for older kernels (<3.14).
* **System Uptime:** Reads `/proc/uptime` and formats uptime into readable `Days:Hours:Minutes:Seconds`.
* **Process Enumeration:** Safely iterates `/proc` directory entries to inspect all accessible processes, extracting PID, Command Name, State, PPID, Virtual Memory (`VmSize`), and Resident Set Size (`VmRSS`).
* **Robust Name Parsing:** Handles process names containing spaces and nested parentheses (e.g., `(worker (io) pool)`) using bidirectional parenthesis boundary matching.
* **Continuous Monitoring Mode:** Live console dashboard with configurable refresh intervals and graceful shutdown on `SIGINT` (Ctrl+C).
* **Race Condition Resilience:** Gracefully tolerates processes terminating asynchronously during enumeration without crashing.

### Subsystem B: CPU Scheduling Simulator
* **Four Algorithms Implemented:**
  1. **First-Come, First-Served (FCFS):** Non-preemptive, arrival-ordered dispatch with deterministic tie-breaking.
  2. **Shortest Job First (SJF):** Non-preemptive, shortest burst selection with idle clock advancement.
  3. **Round Robin (RR):** Preemptive with configurable time quantum $Q > 0$. Strict textbook event-ordering policy prioritizing new arrivals at slice completion over preempted jobs.
  4. **Priority Scheduling:** Non-preemptive, smallest numeric value indicating highest priority.
* **Comprehensive Metrics:** Calculates Completion Time (CT), Turnaround Time (TAT), Waiting Time (WT), Response Time (RT), Total Execution Time, Idle Time, and Simulated CPU Utilization.
* **ASCII Gantt Charts:** Generates visual ASCII timeline diagrams displaying exact time intervals, process dispatch order, and CPU idle gaps.
* **Side-by-Side Algorithm Comparison:** Evaluates and compares all four algorithms side-by-side on any workload.
* **Strict CSV Parsing:** Enforces 4-column headers, reports exact line numbers on validation errors, and rejects duplicate PIDs, negative arrival times, or non-positive burst times.

---

## 3. Technology Stack

* **Language:** C++17 (`std::filesystem`, `std::optional`, `std::clamp`, `std::chrono`, `std::atomic`)
* **Build System:** CMake 3.16+
* **Compiler:** GCC / G++ (v13.3.0 tested) with `-Wall -Wextra -Wpedantic`
* **Target Environment:** Linux / Ubuntu (Native or WSL2)
* **Testing:** Custom zero-dependency unit test framework (`tests/test_framework.hpp`)

---

## 4. Project Directory Structure

```
linux-system-monitor/
├── CMakeLists.txt                 # CMake configuration with C++17 and strict warnings
├── README.md                      # Complete project documentation and guide
├── LICENSE                        # MIT License
├── .gitignore                     # Build and object file exclusions
│
├── include/                       # Public API header files
│   ├── cli.hpp                    # Command line argument parser and dispatcher
│   ├── cpu_monitor.hpp            # CPU accounting counters & delta calculations
│   ├── errors.hpp                 # ErrorCode enums and SystemMonitorError exception
│   ├── memory_monitor.hpp         # Memory & swap statistics parser
│   ├── metrics.hpp                # Metric structs, units & conversion utilities
│   ├── process_model.hpp          # Synthetic process workload & CSV parser
│   ├── process_monitor.hpp        # Linux /proc process scanner & stat parser
│   ├── scheduler.hpp              # Scheduling engine & Gantt chart renderer
│   └── system_monitor.hpp         # High-level monitoring facade
│
├── src/                           # Component implementations
│   ├── cli.cpp
│   ├── cpu_monitor.cpp
│   ├── main.cpp                   # Application entry point & signal handlers
│   ├── memory_monitor.cpp
│   ├── process_model.cpp
│   ├── process_monitor.cpp
│   ├── scheduler.cpp
│   └── system_monitor.cpp
│
├── tests/                         # Automated unit test suite
│   ├── test_framework.hpp         # Lightweight assertion framework
│   ├── test_main.cpp              # Test runner executable
│   ├── test_cpu_metrics.cpp       # CPU delta math & parsing tests
│   ├── test_memory_metrics.cpp    # Meminfo parsing & fallback tests
│   ├── test_process_parsing.cpp   # Stat line & parenthesis parsing tests
│   └── test_scheduler.cpp         # FCFS, SJF, RR, Priority & invariant tests
│
├── examples/                      # Deterministic test workloads
│   ├── processes.csv              # Standard 5-process test workload
│   ├── processes_idle.csv         # Workload testing CPU idle intervals
│   ├── processes_simultaneous.csv # Workload testing simultaneous arrivals
│   └── expected_scheduler_output.md # Manually verified mathematical ground truth
│
├── scripts/                       # Automation scripts
│   ├── build.sh                   # Builds release binaries with CMake
│   ├── run_tests.sh               # Runs automated test suite
│   └── run_demo.sh                # End-to-end multi-command demonstration
│
└── docs/                          # Detailed technical documentation
    ├── ARCHITECTURE.md            # Subsystem design, components & data flow
    ├── DESIGN_DECISIONS.md        # Technical trade-offs, C++17 & /proc rationale
    ├── TESTING.md                 # Test matrices, assertions & invariants
    └── INTERVIEW_PREPARATION.md   # Comprehensive 28-question NVIDIA interview guide
```

---

## 5. Build and Setup

### Prerequisites
* Linux environment (Ubuntu 20.04/22.04/24.04 or WSL2)
* `g++` (C++17 support)
* `cmake` (version 3.16 or higher)
* `make`

### Building the Project
Clone or navigate to the repository directory and run:

```bash
# Make build script executable and compile
chmod +x scripts/*.sh
./scripts/build.sh
```

Alternatively, build manually using CMake:
```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -- -j$(nproc)
```

This produces two binaries in `build/`:
* `system_monitor`: Main command-line application
* `run_unit_tests`: Automated unit testing binary

---

## 6. Usage and CLI Commands

```
Usage:
  ./build/system_monitor info                                  Display a snapshot of Linux CPU, memory, and uptime
  ./build/system_monitor monitor [--interval <sec>]             Run real-time resource monitor continuously (Ctrl+C to stop)
  ./build/system_monitor processes                              Display active Linux processes with CPU and memory usage
  ./build/system_monitor schedule --algorithm <algo> --input <file> [--quantum <q>]
                                                                Run a simulated CPU scheduling algorithm (fcfs, sjf, rr, priority)
  ./build/system_monitor schedule --all --input <file> [--quantum <q>]
                                                                Run and compare all four CPU scheduling algorithms on a workload
  ./build/system_monitor --help, -h                             Display this help message and exit
```

---

## 7. Sample Outputs (Real Execution)

### 7.1 System Monitor Snapshot (`system_monitor info`)
```
=======================================================
         LINUX SYSTEM RESOURCE MONITOR SNAPSHOT        
=======================================================
System Uptime      : 00:02:37 (157.01 seconds)
Active Processes   : 66 enumerated

--- CPU Metrics (/proc/stat) ---
CPU Utilization    : 14.25 %
  User Space       : 5.21 %
  Kernel / System  : 9.04 %
  Idle Time        : 85.75 %
  I/O Wait         : 0.55 %
  [Note: Idle includes idle + iowait as per accounting convention]

--- Memory Metrics (/proc/meminfo) ---
Total Memory       : 7.61 GiB (7795.20 MiB)
Available Memory   : 6.97 GiB (7137.30 MiB)
Used Memory        : 0.64 GiB (657.89 MiB)
Memory Utilization : 8.44 %
Free Memory (Raw)  : 6661.96 MiB
Buffers            : 53.33 MiB
Cached Page Cache  : 564.20 MiB
Swap Total         : 2048.00 MiB
Swap Free          : 2048.00 MiB
Swap Utilization   : 0.00 %
=======================================================
```

### 7.2 Process Listing (`system_monitor processes`)
```
=========================================================================================
                                ACTIVE LINUX PROCESSES                                  
=========================================================================================
PID     COMMAND                 STATE PPID    VM-SIZE (MiB) VM-RSS (MiB)  CPU %     
-----------------------------------------------------------------------------------------
1       systemd                 S     0       21.2          11.7          0.0       
2       init-systemd(Ub         S     1       3.0           1.6           0.0       
96      systemd-udevd           S     1       24.6          6.1           0.0       
140     (udev-worker)           S     96      24.2          3.9           0.0       
224     ollama                  S     1       2029.7        29.1          0.0       
227     snapd                   S     1       1879.3        32.9          0.0       
430     system_monitor          R     429     6.5           3.8           0.0       
-----------------------------------------------------------------------------------------
Total processes displayed: 67 (out of 67 accessible)
States: R=Running, S=Sleep, D=Disk Sleep, T=Stopped, Z=Zombie, I=Idle
=========================================================================================
```

### 7.3 Multi-Algorithm Scheduling Comparison (`system_monitor schedule --all ...`)
```
=====================================================================================================================
                                       SCHEDULING ALGORITHM COMPARISON SUMMARY                                       
=====================================================================================================================
ALGORITHM                                   TOTAL TIME  IDLE TIME   CPU UTIL (%)  AVG WAITING   AVG TURNAROUND  AVG RESPONSE  
---------------------------------------------------------------------------------------------------------------------
First-Come, First-Served (FCFS)             15          0           100.00        4.60          7.60            4.60          
Shortest Job First (SJF - Non-Preemptive)   15          0           100.00        4.00          7.00            4.00          
Round Robin (RR) (q=2)                      15          0           100.00        5.80          8.80            2.40          
Priority Scheduling (Non-Preemptive)        15          0           100.00        5.60          8.60            5.60          
=====================================================================================================================
```

### 7.4 Round Robin Gantt Chart Output
```
--- GANTT CHART ---
Execution Sequence:
[0 -> 2: P1] [2 -> 4: P2] [4 -> 5: P3] [5 -> 7: P1] [7 -> 9: P4] [9 -> 11: P5] [11 -> 12: P2] [12 -> 13: P1] [13 -> 15: P5]

Visual Diagram:
+------+------+------+------+------+------+------+------+------+
|  P1  |  P2  |  P3  |  P1  |  P4  |  P5  |  P2  |  P1  |  P5  |
+------+------+------+------+------+------+------+------+------+
0      2      4      5      7      9      11     12     13     15
```

---

## 8. Automated Testing

To run the full unit test suite:
```bash
./scripts/run_tests.sh
```

Test Results Output:
```
=======================================================
                 RUNNING UNIT TEST SUITE               
=======================================================
[ RUN      ] CpuMetrics.ParseValid10FieldLine
[       OK ] CpuMetrics.ParseValid10FieldLine
[ RUN      ] CpuMetrics.ParseOld4FieldLine
[       OK ] CpuMetrics.ParseOld4FieldLine
[ RUN      ] CpuMetrics.IgnorePerCoreLines
[       OK ] CpuMetrics.IgnorePerCoreLines
[ RUN      ] CpuMetrics.CalculateDeltaUtilization
[       OK ] CpuMetrics.CalculateDeltaUtilization
[ RUN      ] CpuMetrics.ZeroTotalDeltaHandledGracefully
[       OK ] CpuMetrics.ZeroTotalDeltaHandledGracefully
[ RUN      ] CpuMetrics.CounterDecreaseThrowsException
[       OK ] CpuMetrics.CounterDecreaseThrowsException
[ RUN      ] MemoryMetrics.ParseStandardMeminfo
[       OK ] MemoryMetrics.ParseStandardMeminfo
[ RUN      ] MemoryMetrics.FallbackWhenMemAvailableMissing
[       OK ] MemoryMetrics.FallbackWhenMemAvailableMissing
[ RUN      ] MemoryMetrics.ZeroMemTotalAvoidsDivisionByZero
[       OK ] MemoryMetrics.ZeroMemTotalAvoidsDivisionByZero
[ RUN      ] MemoryMetrics.AvailableExceedingTotalUnderflowProtection
[       OK ] MemoryMetrics.AvailableExceedingTotalUnderflowProtection
[ RUN      ] ProcessParsing.StandardProcessLine
[       OK ] ProcessParsing.StandardProcessLine
[ RUN      ] ProcessParsing.ProcessNameWithSpaces
[       OK ] ProcessParsing.ProcessNameWithSpaces
[ RUN      ] ProcessParsing.ProcessNameWithParentheses
[       OK ] ProcessParsing.ProcessNameWithParentheses
[ RUN      ] ProcessParsing.ProcessNameWithSpacesAndMultipleParentheses
[       OK ] ProcessParsing.ProcessNameWithSpacesAndMultipleParentheses
[ RUN      ] ProcessParsing.ProcessStatesTranslation
[       OK ] ProcessParsing.ProcessStatesTranslation
[ RUN      ] ProcessParsing.MalformedStatLineRejected
[       OK ] ProcessParsing.MalformedStatLineRejected
[ RUN      ] Scheduler.FCFSExactMetrics
[       OK ] Scheduler.FCFSExactMetrics
[ RUN      ] Scheduler.SJFSelectionAndMetrics
[       OK ] Scheduler.SJFSelectionAndMetrics
[ RUN      ] Scheduler.RoundRobinPreemptionAndEventOrdering
[       OK ] Scheduler.RoundRobinPreemptionAndEventOrdering
[ RUN      ] Scheduler.PriorityNonPreemptive
[       OK ] Scheduler.PriorityNonPreemptive
[ RUN      ] Scheduler.IdleIntervalHandling
[       OK ] Scheduler.IdleIntervalHandling
[ RUN      ] Scheduler.CSVParserAndValidation
[       OK ] Scheduler.CSVParserAndValidation
=======================================================
Total Tests: 22 | Passed: 22 | Failed: 0
=======================================================
```

---

## 9. Known Limitations

1. **Host-Specific Monitoring:** The monitoring subsystem relies strictly on the Linux `/proc` pseudo-filesystem and is specific to Linux systems (or WSL2). It is not designed to compile natively against Win32 APIs without a POSIX/Linux translation layer.
2. **Sampling Resolution:** CPU utilization is an estimate averaged over the sampling interval. Extremely short bursts occurring within a single millisecond cannot be detected without high-frequency hardware performance counters (PMU) or eBPF.
3. **Discrete-Event Simulation vs. Real Kernels:** The scheduler simulator models CPU execution over discrete time steps. It does not model hardware context switch overheads, cache pollution, memory bus contention, or SMP multi-core load balancing.

---

## 10. Future Improvements

* **Multilevel Feedback Queue (MLFQ):** Implement priority aging and I/O-boosting queues.
* **eBPF Telemetry Connector:** Transition process monitoring from `/proc` directory scanning to event-driven kernel tracepoints (`sched:sched_process_exit`).
* **cgroups v2 Integration:** Support container-level memory and CPU quota monitoring via `/sys/fs/cgroup`.

---

## 11. Resume Integration

**Linux System Monitor and CPU Scheduling Simulator | C++17, Linux, CMake**
* Engineered a modular C++17 system resource monitor collecting CPU, memory, uptime, and process accounting data from Linux `/proc` interfaces with safe two-sample delta calculations and race-condition tolerance.
* Implemented deterministic user-space scheduling simulations for FCFS, SJF, Round Robin, and Priority algorithms, generating visual ASCII Gantt charts and evaluating turnaround, waiting, and response times.
* Developed an automated unit testing suite with 22 test cases and invariant checks, validating algorithm correctness against mathematically verified baseline traces.

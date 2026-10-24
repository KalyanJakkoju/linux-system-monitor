# System Architecture Documentation

**Project:** Linux System Resource Monitor and CPU Scheduling Simulator in C++17  
**Target Platform:** Linux (Ubuntu 24.04 LTS / WSL2)  
**Standard:** C++17  

---

## 1. High-Level Architectural Overview

The application is structured into two logically decoupled, modular subsystems unified under a common command-line interface:

```
                              +--------------------------+
                              |         main.cpp         |
                              |  (Signals & Entry Point) |
                              +-------------+------------+
                                            |
                                            v
                              +--------------------------+
                              |         cli.cpp          |
                              | (Argument Parser/Router) |
                              +-------+----------+-------+
                                      |          |
              +-----------------------+          +-----------------------+
              |                                                          |
              v                                                          v
+-------------------------------+                         +-------------------------------+
|    MODULE A: SYSTEM MONITOR   |                         |   MODULE B: CPU SCHEDULER     |
| (Real Linux Host Telemetry)   |                         | (Deterministic Simulation)    |
+-------------------------------+                         +-------------------------------+
| - SystemMonitor (Facade)      |                         | - CpuScheduler (Engine)       |
| - CpuMonitor (/proc/stat)     |                         | - ProcessModel (CSV Parser)   |
| - MemoryMonitor (/proc/meminfo|                         | - FCFS, SJF, RR, Priority     |
| - ProcessMonitor (/proc/PID)  |                         | - Gantt Chart & Metrics Table |
+-------------------------------+                         +-------------------------------+
              |                                                          |
              v                                                          v
    Linux /proc VFS Interfaces                                 Synthetic Workloads (CSV)
```

### Logical Separation Rationale
1. **Host-Independent Simulation:** The CPU Scheduling Simulator (`simulator::CpuScheduler`, `simulator::Process`) is completely decoupled from operating system APIs. It relies strictly on standard C++ containers and algorithms (`<vector>`, `<queue>`, `<algorithm>`), allowing it to run deterministically and be tested on any POSIX or cross-platform environment without accessing `/proc`.
2. **Platform-Specific Telemetry:** The Resource Monitoring subsystem (`sysmon::SystemMonitor`) is isolated behind distinct classes that consume Linux virtual filesystem interfaces (`/proc/stat`, `/proc/meminfo`, `/proc/uptime`, `/proc/[PID]/stat`, `/proc/[PID]/status`). It provides testable interfaces that accept abstract `std::istream` streams so unit tests can validate parsing on arbitrary mock inputs without filesystem dependencies.

---

## 2. Module Responsibilities & Class Breakdown

### 2.1 Subsystem A: Linux System Resource Monitor (`sysmon`)

| Component | Files | Primary Responsibility |
| :--- | :--- | :--- |
| `CpuMonitor` | `include/cpu_monitor.hpp`<br>`src/cpu_monitor.cpp` | Reads aggregate CPU accounting ticks from `/proc/stat`. Computes safe deltas across two consecutive time samples, handles non-monotonic counter wraps, treats `idle + iowait` as idle time, and produces percentage breakdowns for user, system, idle, and iowait. |
| `MemoryMonitor` | `include/memory_monitor.hpp`<br>`src/memory_monitor.cpp` | Parses `/proc/meminfo` to extract memory figures (Total, Available, Free, Buffers, Cached, Swap Total, Swap Free). Computes accurate memory utilization using `MemAvailable`, with fallback logic for older kernels (<3.14). |
| `ProcessMonitor` | `include/process_monitor.hpp`<br>`src/process_monitor.cpp` | Enumerates active processes via numeric directories in `/proc`. Safely parses `/proc/[PID]/stat` and `/proc/[PID]/status`. Employs bidirectional parenthesis matching to safely extract process names containing spaces and parentheses. Computes per-process CPU utilization using tick deltas. |
| `SystemMonitor` | `include/system_monitor.hpp`<br>`src/system_monitor.cpp` | Facade coordinating CPU, memory, uptime, and process data. Provides high-level snapshot rendering, process table displays, and continuous live monitoring with `SIGINT` (Ctrl+C) graceful shutdown. |

### 2.2 Subsystem B: CPU Scheduling Simulator (`simulator`)

| Component | Files | Primary Responsibility |
| :--- | :--- | :--- |
| `ProcessModel` | `include/process_model.hpp`<br>`src/process_model.cpp` | Represents synthetic workloads (`struct Process`). Implements strict CSV parsing with exact line-number error reporting, duplicate PID detection, and validation of arrival times ($\ge 0$) and burst times ($> 0$). |
| `CpuScheduler` | `include/scheduler.hpp`<br>`src/scheduler.cpp` | Core scheduling engine implementing First-Come First-Served (FCFS), Shortest Job First (SJF), Round Robin (RR), and Priority scheduling. Computes completion, turnaround, waiting, and response times. |
| `GanttRenderer` | `include/scheduler.hpp`<br>`src/scheduler.cpp` | Renders ASCII Gantt charts showing exact start/end intervals, preemption slices, and idle periods, alongside a multi-algorithm comparative summary table. |

---

## 3. Data Flow

### 3.1 Monitoring Data Flow
1. **Periodic Sampling:** `SystemMonitor::take_snapshot()` initiates an initial sample of `/proc/stat`, `/proc/meminfo`, and `/proc/uptime`.
2. **Delta Measurement:** A calibrated delay (default 200–300 ms for one-shot, or user-defined seconds for continuous monitoring) elapses before sampling `/proc/stat` a second time.
3. **Tick Delta Calculation:** Total CPU ticks and idle ticks are subtracted:
   $$\Delta \text{total} = \text{total}_2 - \text{total}_1$$
   $$\Delta \text{idle} = (\text{idle}_2 + \text{iowait}_2) - (\text{idle}_1 + \text{iowait}_1)$$
   $$\text{CPU Utilization \%} = 100 \times \frac{\Delta \text{total} - \Delta \text{idle}}{\Delta \text{total}}$$
4. **Process Listing & Filtering:** `/proc` directory entries matching `[0-9]+` are scanned. Files `/proc/[PID]/stat` and `/proc/[PID]/status` are opened. If a process terminates between directory listing and file open (race condition), `ENOENT` is caught and the process is silently skipped without crashing.

### 3.2 Scheduling Simulation Data Flow
1. **Workload Ingestion:** Workload CSV is parsed into `std::vector<Process>`. Header and rows are verified.
2. **Algorithm Execution:** An algorithm receives a copy of the workload vector.
3. **Discrete Event Simulation:** A simulated clock advances. As time passes, arrived jobs are moved to ready queues. Execution slices are recorded into `std::vector<ExecutionSlice>`.
4. **Metric Derivation:** Upon process completion:
   $$\text{Turnaround Time (TAT)} = \text{Completion Time (CT)} - \text{Arrival Time (AT)}$$
   $$\text{Waiting Time (WT)} = \text{TAT} - \text{Burst Time (BT)}$$
   $$\text{Response Time (RT)} = \text{First CPU Start Time} - \text{AT}$$
5. **Invariant Verification:** `verify_schedule_invariants()` asserts that the sum of execution slices equals total burst times, slices do not overlap, and completion times precede no arrival times.
6. **Rendering:** Formatted metrics table and Gantt chart are generated.

---

## 4. Linux Kernel Interfaces Used

* **/proc/stat:** Exposes kernel CPU accounting counters (USER_HZ clock ticks) since system boot. We parse the aggregate line (`cpu `) extracting:
  `user`, `nice`, `system`, `idle`, `iowait`, `irq`, `softirq`, `steal`, `guest`, `guest_nice`.
* **/proc/meminfo:** Exposes RAM and swap statistics. Extracted keys:
  `MemTotal`, `MemAvailable`, `MemFree`, `Buffers`, `Cached`, `SwapTotal`, `SwapFree`.
* **/proc/uptime:** Contains uptime in seconds and aggregate idle core time.
* **/proc/[PID]/stat:** Kernel process status exposed as space-delimited fields. Field 2 (`comm`) contains the process command name enclosed in parentheses `(...)`. We locate the first `(` and last `)` to correctly parse command names with embedded spaces and parentheses.
* **/proc/[PID]/status:** Human-readable process status. Parsed for `VmSize` (virtual memory peak) and `VmRSS` (resident set size in physical RAM).
* **sysconf(_SC_CLK_TCK):** Determines system clock ticks per second (typically 100 Hz on Linux x86_64).
* **sysconf(_SC_PAGESIZE):** Determines memory page size (typically 4096 bytes).

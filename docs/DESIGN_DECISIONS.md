# Design Decisions and Trade-offs

This document outlines key technical and architectural decisions made during the design and implementation of the **Linux System Resource Monitor and CPU Scheduling Simulator**.

---

## 1. Why C++17?

* **Standard Filesystem (`std::filesystem`):** Directory traversal in `/proc` is implemented cleanly and portably using `<filesystem>`, avoiding legacy POSIX directory traversal code (`opendir`, `readdir`, `closedir`) and manual buffer allocations.
* **Safe Nullability (`std::optional`):** Functions like `parse_cpu_stat_line` and `parse_stat_line` return `std::optional<T>`, explicitly representing parse failures without sentinel values, magic numbers, or throwing exceptions for routine control flow.
* **Range Clamping (`std::clamp`):** Guarantees percentage calculations strictly remain within $[0.0, 100.0]\%$ even during floating-point rounding anomalies.
* **Zero External Dependencies:** C++17 standard libraries provide all required primitives for I/O streams, synchronization (`std::atomic`), time measurements (`std::chrono`), and containers, keeping the application lightweight and free from complex third-party package managers.

---

## 2. Why Read Linux `/proc` Directly?

* **No Kernel Drivers or Root Required:** The `/proc` filesystem is a virtual filesystem (VFS) provided by the Linux kernel that exposes process and kernel accounting statistics in user space without requiring custom kernel modules, root permissions, or kernel rebuilds.
* **Educational & Systems Clarity:** Reading `/proc` demonstrates a fundamental understanding of how Linux exposes kernel instrumentation to user-space utilities (such as `top`, `ps`, and `free`).
* **Non-Invasive:** Reading text-based `/proc` files introduces minimal overhead and eliminates dependence on platform-specific C libraries like `libgtop` or `libprocps`.

---

## 3. Why Is the CPU Scheduler Independent of Linux Interfaces?

* **Separation of Concerns:** The CPU scheduler is an algorithmic discrete-event simulation of process scheduling policies, whereas the resource monitor is an operational telemetry tool. Conflating the two would make the scheduler impossible to unit test deterministically without live system interference.
* **Deterministic Unit Testing:** By decoupling the simulator from live Linux scheduling, we can simulate edge cases (such as simultaneous arrivals, quantum expiration collisions, and CPU idle bubbles) using synthetic workloads and assert deterministic mathematical outcomes.
* **Educational Clarity:** Demonstrates that modern systems programmers design modular domain models that are decoupled from platform I/O.

---

## 4. How CPU Utilization Is Calculated

### Why Two Samples Are Mandatory
A single read of `/proc/stat` contains cumulative clock ticks accrued since system boot. Computing $\frac{\text{busy ticks}}{\text{total ticks}}$ on a single sample yields the historical average CPU utilization since boot, which is useless for understanding current activity.

Real CPU utilization can only be computed over a finite time window $[\tau_1, \tau_2]$:
$$\Delta \text{total} = \text{total}(\tau_2) - \text{total}(\tau_1)$$
$$\Delta \text{idle} = \text{idle}(\tau_2) - \text{idle}(\tau_1)$$
$$\text{Utilization \%} = 100 \times \frac{\Delta \text{total} - \Delta \text{idle}}{\Delta \text{total}}$$

### Idle vs. I/O-Wait Accounting Convention
In this implementation, `idle_time` is defined as:
$$\text{idle\_time} = \text{idle} + \text{iowait}$$

**Rationale:** The Linux kernel documentation specifies that `iowait` represents time where the CPU had no runnable threads and was blocked waiting on an outstanding I/O request (disk or network). Because the CPU core was not executing instructions during this period, counting it as idle correctly reflects the lack of computational load on the processor.

### Edge Case Handling
1. **Zero Total Delta:** If two samples are taken with zero elapsed clock ticks (e.g., immediate re-read), division by zero is prevented by returning `is_valid = false`.
2. **Counter Reset/Wrap:** If counters decrease between samples (tick wrap or counter reset), a `SystemMonitorError` with `ErrorCode::CounterDecreased` is raised rather than producing negative or erratic percentages.

---

## 5. Process Enumeration and Race Condition Safety

Processes on Linux can spawn and terminate at microsecond timescales. When enumerating `/proc`:
1. `directory_iterator` scans directories.
2. A numeric PID folder (e.g. `/proc/1234`) is discovered.
3. Before the application opens `/proc/1234/stat`, the target process may exit.

**Design Decision:** Opening and reading process files uses defensive I/O checks (`file.is_open()`). Any failure to open or read a stat file is treated as a benign transient event (`ENOENT`); the process is skipped silently, ensuring the monitoring loop remains crash-free.

### Comm Parsing with Nested Parentheses
In `/proc/[PID]/stat`, the process name (`comm`) is enclosed in parentheses and may legally contain spaces and nested parentheses (e.g., `1234 (worker (io) pool) S 1 ...`).
Blindly splitting on whitespace corrupts column indices. Our parser locates the **first** open parenthesis `(` and the **last** close parenthesis `)` on the line:
* String before first `(` = PID
* String between first `(` and last `)` = process name (`comm`)
* String following last `)` = remaining fields (state, PPID, utime, stime, etc.)

---

## 6. Scheduling Policy Details and Tie-Breaking

### 1. First-Come, First-Served (FCFS)
* Non-preemptive.
* Arrived processes are dispatched in ascending order of arrival time.
* **Tie-Breaking:** If arrival times are identical, ties are resolved deterministically using lexicographical process ID (`P1` < `P2`).

### 2. Shortest Job First (SJF - Non-Preemptive)
* Non-preemptive.
* Arrived processes are evaluated by CPU burst time; the process with the minimum burst time is dispatched to completion.
* **Tie-Breaking:** If burst times are identical, earliest arrival time is preferred. If still tied, lexicographical PID breaks the tie.

### 3. Round Robin (RR - Preemptive)
* Preemptive with time quantum $Q > 0$.
* **Critical Event Ordering Policy:**
  When a process $P_{curr}$ completes its slice at time $T$:
  1. Any newly arrived processes arriving in $(T - \Delta, T]$ are enqueued into the ready queue **first** (in arrival order, tie-broken by PID).
  2. If $P_{curr}$ has remaining burst time, it is **subsequently** re-enqueued at the tail of the ready queue.
  3. If $P_{curr}$ completed its burst, its metrics are finalized and it is not re-enqueued.

### 4. Priority Scheduling (Non-Preemptive)
* Non-preemptive.
* **Priority Convention:** Smaller numeric values indicate higher priority ($0$ is higher priority than $1$).
* **Tie-Breaking:** If two arrived processes share the same priority, earliest arrival time is chosen; remaining ties are resolved by PID.

---

## 7. Error Handling Philosophy

* **Actionable Errors:** When invalid CLI flags, non-positive burst times, or malformed CSV rows are encountered, clear human-readable error messages detailing exact line numbers are printed to `std::cerr`.
* **No Silent Defaults:** The application will not arbitrarily replace corrupted or negative arrival times with $0$ or guess values.
* **Invariant Verification:** After every scheduling run, `verify_schedule_invariants()` automatically validates cross-algorithm invariants (conservation of burst time, non-negative wait times, chronological consistency).

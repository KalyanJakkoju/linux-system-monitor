# Systems Software Engineering Interview Preparation Guide

**Target Role:** System Software Engineer Intern, NVIDIA  
**Topic Focus:** Linux Systems Programming, Operating Systems, C++17, CPU Scheduling, Performance Engineering  

---

## Part 1: Linux Internals & Interfaces

### 1. What is `/proc`?
`/proc` is a virtual filesystem (VFS) provided by the Linux kernel, traditionally mounted at `/proc`. It acts as an illusion of a regular filesystem; the files within it do not exist on physical storage (like SSD or HDD) and consume zero disk space. Instead, `/proc` is an interface through which user space can inspect kernel state, system hardware parameters, memory mappings, and per-process runtime data in a structured, textual format.

### 2. Why is it called a pseudo-filesystem?
It is called a pseudo-filesystem (or synthetic filesystem) because its directory entries and file contents are generated dynamically in memory by kernel handlers on demand. When a user-space process calls `open()` and `read()` on `/proc/stat` or `/proc/[PID]/stat`, the Virtual Filesystem (VFS) layer invokes kernel hook functions (such as `seq_file` handlers) that read internal kernel structures (like `task_struct` and `kernel_cpustat`) and format them into ASCII text on the fly.

### 3. How do you calculate CPU utilization?
CPU utilization cannot be computed from a single instantaneous reading. Instead, it is calculated as the ratio of busy CPU time to total elapsed CPU time across a measured sampling window $[\tau_1, \tau_2]$:
$$\Delta \text{total} = \text{total ticks}(\tau_2) - \text{total ticks}(\tau_1)$$
$$\Delta \text{idle} = \text{idle ticks}(\tau_2) - \text{idle ticks}(\tau_1)$$
$$\text{Utilization \%} = 100 \times \frac{\Delta \text{total} - \Delta \text{idle}}{\Delta \text{total}}$$
Where total ticks is the sum of all counter categories (`user + nice + system + idle + iowait + irq + softirq + steal + guest + guest_nice`).

### 4. Why do you need two CPU samples?
The counters exposed in `/proc/stat` are cumulative metrics that increment monotonically starting from system boot. If you evaluate only a single sample, you are calculating the lifetime average CPU utilization since the machine was powered on, which obscures recent workload spikes. To determine current or instantaneous CPU activity, you must observe the change in counters ($\Delta$) over a finite, controlled time duration.

### 5. How do you handle processes that disappear during enumeration?
Processes in a preemptive multitasking OS terminate asynchronously at microsecond intervals. Between enumerating numeric PID directories in `/proc` and opening `/proc/[PID]/stat`, the process may exit (`do_exit()`), causing the kernel to unlink its `/proc` entry.  
In our implementation, file opening is treated defensively: if `std::ifstream::is_open()` fails (corresponding to `ENOENT`), the error is recognized as a normal race condition, and the process is silently ignored rather than throwing an unhandled exception or aborting the monitoring loop.

### 6. What is the difference between virtual memory and resident memory?
* **Virtual Memory (VmSize):** The total address space mapped by the process, including executable code sections, stack, heap allocations via `brk`/`mmap`, and shared libraries. Much of this memory may not currently be resident in physical RAM (e.g., mapped but never written pages, or pages swapped to disk).
* **Resident Set Size (VmRSS):** The portion of the process's virtual memory that is currently loaded in physical RAM pages. Pages that have been paged out to swap or lazily allocated without being touched do not count toward VmRSS.

---

## Part 2: Operating Systems & Scheduling

### 7. What is FCFS scheduling?
First-Come, First-Served (FCFS) is a non-preemptive scheduling policy where the CPU is assigned to processes strictly in the order they arrive in the ready queue. Once a process gains the CPU, it runs until its CPU burst completes. While simple and starvation-free, FCFS suffers from the **convoy effect**, where short CPU-bound or I/O-bound jobs wait for extended periods behind a single long compute-intensive process, resulting in high average waiting times.

### 8. What is SJF scheduling?
Shortest Job First (SJF) is a scheduling algorithm that assigns the CPU to the arrived process with the smallest CPU burst time. In its non-preemptive variant, once dispatched, a job runs to completion. SJF is mathematically proven to be optimal in terms of minimizing average waiting time for a given set of stationary processes. Its primary drawbacks are that exact future CPU burst times are generally unknown in general-purpose OS kernels, and long processes can experience starvation if short processes arrive continuously.

### 9. What is Round Robin scheduling?
Round Robin (RR) is a preemptive scheduling algorithm designed for time-sharing systems. A fixed unit of execution time called a **time quantum** (or time slice) is defined. The ready queue is managed as a FIFO queue. When the CPU scheduler dispatches a process, it executes for at most one quantum. If the process does not terminate within the quantum, it is preempted by a timer interrupt, its context is saved, and it is placed back at the tail of the ready queue. RR provides low and predictable response times.

### 10. What is priority scheduling?
Priority scheduling assigns a priority ranking to each process, and the scheduler dispatches the highest-priority arrived process. Priorities can be defined statically or dynamically. In our implementation, smaller numeric values denote higher priorities ($0$ is highest). Priority scheduling can be preemptive or non-preemptive. The primary vulnerability is **starvation** (or indefinite blocking), which can be resolved in production kernels using **aging** (gradually increasing a process's priority the longer it waits in the ready queue).

### 11. How do waiting time and turnaround time differ?
* **Turnaround Time (TAT):** The total elapsed time between process arrival and its final completion:
  $$\text{TAT} = \text{Completion Time} - \text{Arrival Time}$$
  It encompasses waiting for the CPU, actual CPU execution, and any I/O delays.
* **Waiting Time (WT):** The total amount of time a process spent sitting in the ready queue waiting to be granted the CPU:
  $$\text{WT} = \text{Turnaround Time} - \text{CPU Burst Time}$$

### 12. Why does Round Robin depend heavily on the time quantum?
The performance of Round Robin depends critically on the size of the time quantum $Q$:
* **If $Q$ is excessively large ($Q \to \infty$):** RR degenerates into FCFS scheduling, losing all interactive response time advantages.
* **If $Q$ is excessively small ($Q \to 0$):** Known theoretically as Processor Sharing, but in practical systems, CPU cycles are consumed by context switching overhead (register saves, TLB flushes, cache line invalidations), severely reducing throughput.
A well-tuned quantum ensures that the vast majority of short interactive requests complete in a single slice while context switch overhead remains $<1\%$ of total CPU time.

### 13. What is starvation?
Starvation (or indefinite blocking) is a condition where a runnable process waits indefinitely in the ready queue because other processes are continually prioritized over it by the scheduler. It occurs in algorithms like Priority Scheduling and Shortest Job First. Starvation is mitigated in real operating systems via **aging** (aging increments a process's priority over time) or by scheduling algorithms with fairness guarantees like Linux's Completely Fair Scheduler (CFS).

### 14. How does context switching work?
A context switch is the mechanism by which the operating system saves the execution context of a running process or thread and restores the context of another. It entails:
1. Triggering an interrupt (e.g., timer interrupt for quantum expiration) or a system call (`sched_yield`, blocking I/O).
2. Transitioning from user space to kernel space.
3. Saving processor register states (Program Counter, Stack Pointer, general-purpose registers, floating-point/SIMD registers) into the process control block (`task_struct`).
4. Changing the memory address space mapping by switching the page table base register (CR3 on x86_64), which causes Translation Lookaside Buffer (TLB) invalidation.
5. Loading the register state of the new process and executing `iret` / `sysret` to resume execution in user space.

---

## Part 3: C++ & Software Engineering

### 15. How did you structure the application?
The application is structured following the **Separation of Concerns** and **Single Responsibility Principle**:
* `sysmon`: Contains telemetry collection classes (`CpuMonitor`, `MemoryMonitor`, `ProcessMonitor`, `SystemMonitor`) reading Linux `/proc`.
* `simulator`: Contains pure, deterministic discrete-event scheduling components (`ProcessModel`, `CpuScheduler`), completely isolated from platform I/O.
* `cli`: Decouples command-line parsing, argument validation, and routing from core domain logic.
* Standard C++17 build using CMake with distinct library targets (`sysmon_core`) and executable targets (`system_monitor`, `run_unit_tests`).

### 16. Why did you use particular data structures?
* `std::vector<Process>`: Contiguous dynamic array providing cache locality and $O(1)$ indexed access for process workloads.
* `std::queue<size_t>`: Used in Round Robin to model the FIFO ready queue with strict $O(1)$ push and pop semantics.
* `std::unordered_map<int, ProcessSample>`: Hash map for tracking previous CPU tick readings keyed by PID, giving average $O(1)$ lookup for delta calculations.
* `std::optional<T>`: Used for parser return types to express potential parse failure cleanly without throwing exceptions or reserving sentinel error values.

### 17. How did you avoid unsafe memory handling?
* **Zero Raw `new` / `delete`:** All memory allocation is managed through standard library containers (`std::vector`, `std::string`) following RAII principles.
* **Pass-by-value for immutable copies:** Workload vectors are passed by value into scheduling algorithms, ensuring the original workload is never contaminated across multi-algorithm comparisons.
* **Const Correctness:** Const references are used across display and calculation routines (`const ScheduleResult&`, `const CpuCounters&`), preventing unintended side effects.

### 18. How did you validate input?
* **CLI Validation:** Flag parsing validates that options have required arguments, refresh intervals are $>0$, time quantums are $>0$, and algorithms are recognized.
* **CSV Validation:** Enforces exact 4-column headers (`pid,arrival_time,burst_time,priority`), tracks line numbers for syntax errors, verifies that arrival times are $\ge 0$, burst times are $>0$, rejects duplicate PIDs, and prevents empty workloads.

### 19. How did you handle exceptions or file-reading failures?
* Custom exception hierarchy rooted in `sysmon::SystemMonitorError` with explicit `ErrorCode` enumerations.
* Defensive file stream checks (`if (!file.is_open())`) to handle race conditions in `/proc`.
* Top-level `try / catch` blocks in `main.cpp` that catch `SystemMonitorError` and `std::exception`, reporting clear errors to `std::cerr` and returning designated non-zero exit codes.

---

## Part 4: Algorithms & Complexity

### 20. What is the time complexity of each scheduling algorithm?
Given $N$ processes:
* **FCFS:** Sorting by arrival time takes $O(N \log N)$. Single sequential dispatch pass takes $O(N)$.  
  **Total Time Complexity:** $\mathcal{O}(N \log N)$. **Space Complexity:** $\mathcal{O}(N)$.
* **SJF (Non-Preemptive):** Finding the minimum burst among arrived jobs at each step takes $O(N)$ comparisons across $N$ iterations.  
  **Total Time Complexity:** $\mathcal{O}(N^2)$ (can be optimized to $\mathcal{O}(N \log N)$ using a priority queue / min-heap). **Space Complexity:** $\mathcal{O}(N)$.
* **Round Robin:** Let $B = \sum \text{burst\_time}$. The number of slices executed is at most $\lceil B / Q \rceil$. Each slice involves $O(1)$ queue pop and push.  
  **Total Time Complexity:** $\mathcal{O}(N \log N + \frac{B}{Q})$. **Space Complexity:** $\mathcal{O}(N)$.
* **Priority (Non-Preemptive):** Selecting the highest priority job takes $O(N)$ comparisons across $N$ iterations.  
  **Total Time Complexity:** $\mathcal{O}(N^2)$ (or $\mathcal{O}(N \log N)$ with a min-heap). **Space Complexity:** $\mathcal{O}(N)$.

### 21. Which data structures support each algorithm?
* **FCFS:** FIFO Queue or sorted array.
* **SJF:** Min-Heap / Priority Queue keyed by `burst_time`.
* **Round Robin:** FIFO Queue (`std::queue`) with round-robin tail enqueuing.
* **Priority:** Min-Heap / Priority Queue keyed by `priority`.

### 22. How do you handle simultaneous arrivals?
When two or more processes arrive at the exact same simulation timestamp:
1. They are evaluated by algorithm-specific criteria (e.g., shortest burst for SJF, highest priority for Priority).
2. If the primary criteria are identical, a deterministic tie-breaker is applied: the job with the lexicographically smaller PID is scheduled first (`P1` before `P2`).
In Round Robin, all processes arriving at time $T$ are enqueued in deterministic order before any preempted process executing at time $T$ is re-enqueued.

### 23. How did you validate the correctness of your implementation?
Correctness was validated through three independent layers:
1. **Mathematical Ground Truth Traces:** Hand-calculated traces documented in `examples/expected_scheduler_output.md` were used as oracle assertions in unit tests.
2. **Automated Unit Tests:** 22 automated test cases covering metrics, edge cases, zero deltas, and state transitions.
3. **Cross-Algorithm Invariant Assertions:** Runtime checks asserting $\sum \text{slice durations} = \text{burst time}$, chronological ordering, non-negative wait times, and contiguous timelines.

---

## Part 5: Systems Engineering & Production Trade-offs

### 24. What happens if a monitored process terminates unexpectedly?
During process enumeration, a process can exit between directory iteration and file reading. Our implementation treats this as expected POSIX behavior: opening `/proc/[PID]/stat` fails safely, the entry is skipped, and no invalid data is added to the process snapshot. The application never crashes due to transient process exits.

### 25. How would you reduce monitoring overhead in production?
* **Event-Driven Telemetry:** Instead of repeatedly polling `/proc`, modern Linux systems use kernel interfaces like `netlink` process event connectors (`PROC_EVENT_EXIT`, `PROC_EVENT_FORK`) or eBPF tracepoints (`sched:sched_process_exit`).
* **Direct File Descriptor Caching:** Cache open file descriptors for long-lived system daemons to avoid repeated pathname resolution and directory lookups.
* **Batched Buffer I/O:** Use pre-allocated buffers and `read()` syscalls instead of allocating multiple `std::string` objects and string streams per process.

### 26. What limitations does `/proc` introduce?
* **String Parsing Overhead:** Every read requires the kernel to generate ASCII text and user space to parse that text back into binary integers.
* **Non-Atomic Snapshots:** Reading multiple files across `/proc` (e.g. `/proc/stat` and `/proc/[PID]/stat`) is not an atomic transaction; system state can change between reads.
* **Scale Bottlenecks:** On systems with tens of thousands of active threads/processes, iterating `/proc` directory entries incurs significant lock contention and kernel page cache overhead.

### 27. Why is this simulator not the same as the Linux scheduler?
* **User-Space Discrete-Event Model:** Our simulator models textbook scheduling algorithms over discrete, synthetic tick intervals.
* **Linux CFS / EEVDF:** The actual Linux kernel uses the Completely Fair Scheduler (CFS) or Early Evident Virtual Deadline First (EEVDF) scheduler. CFS uses red-black trees (`rb_node`) keyed by `vruntime` (virtual runtime normalized by nice levels), supports multi-core SMP scheduling, CPU affinity, NUMA topology awareness, cgroups v2 bandwidth throttling, and real-time scheduling classes (SCHED_FIFO, SCHED_RR, SCHED_DEADLINE).

### 28. How would you extend this project to support additional scheduling policies?
* **Object-Oriented Strategy Pattern:** Introduce an abstract `ISchedulerStrategy` interface with a virtual method `virtual ScheduleResult schedule(std::vector<Process> workload) = 0`.
* **Multilevel Feedback Queue (MLFQ):** Implement multiple priority queues with decreasing time quantums, boosting jobs that release the CPU for I/O and demoting CPU-bound jobs.
* **Dynamic Nice Levels & Weighting:** Implement virtual runtime decay similar to Linux CFS to model proportional share scheduling.

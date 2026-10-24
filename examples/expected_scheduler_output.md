# Expected CPU Scheduling Simulator Outputs (Ground Truth)

This document contains manually calculated and mathematically verified simulation traces for the provided sample workloads.
These results serve as deterministic baselines to validate simulation accuracy.

---

## Workload 1: `examples/processes.csv`

```csv
pid,arrival_time,burst_time,priority
P1,0,5,2
P2,1,3,1
P3,2,1,3
P4,3,2,4
P5,4,4,2
```

### 1. First-Come, First-Served (FCFS)
* Execution Timeline:
  - `[0 -> 5: P1]` (BT=5)
  - `[5 -> 8: P2]` (BT=3)
  - `[8 -> 9: P3]` (BT=1)
  - `[9 -> 11: P4]` (BT=2)
  - `[11 -> 15: P5]` (BT=4)
* Per-process metrics:
  - **P1**: CT = 5, TAT = 5 - 0 = 5, WT = 5 - 5 = 0, RT = 0 - 0 = 0
  - **P2**: CT = 8, TAT = 8 - 1 = 7, WT = 7 - 3 = 4, RT = 5 - 1 = 4
  - **P3**: CT = 9, TAT = 9 - 2 = 7, WT = 7 - 1 = 6, RT = 8 - 2 = 6
  - **P4**: CT = 11, TAT = 11 - 3 = 8, WT = 8 - 2 = 6, RT = 9 - 3 = 6
  - **P5**: CT = 15, TAT = 15 - 4 = 11, WT = 11 - 4 = 7, RT = 11 - 4 = 7
* Summary:
  - Total Time: 15, Idle: 0, CPU Util: 100.00%
  - Average TAT: (5 + 7 + 7 + 8 + 11) / 5 = 38 / 5 = **7.60**
  - Average WT: (0 + 4 + 6 + 6 + 7) / 5 = 23 / 5 = **4.60**
  - Average RT: (0 + 4 + 6 + 6 + 7) / 5 = **4.60**

---

### 2. Shortest Job First (SJF - Non-Preemptive)
* Scheduling Decisions:
  - T=0: Only P1 is ready -> `[0 -> 5: P1]`
  - T=5: P2 (BT=3), P3 (BT=1), P4 (BT=2), P5 (BT=4) are all ready.
    * Shortest is P3 (BT=1) -> `[5 -> 6: P3]`
  - T=6: P4 (BT=2), P2 (BT=3), P5 (BT=4) are ready.
    * Shortest is P4 (BT=2) -> `[6 -> 8: P4]`
  - T=8: P2 (BT=3), P5 (BT=4) are ready.
    * Shortest is P2 (BT=3) -> `[8 -> 11: P2]`
  - T=11: P5 (BT=4) runs -> `[11 -> 15: P5]`
* Per-process metrics:
  - **P1**: CT = 5, TAT = 5, WT = 0, RT = 0
  - **P2**: CT = 11, TAT = 10, WT = 7, RT = 7
  - **P3**: CT = 6, TAT = 4, WT = 3, RT = 3
  - **P4**: CT = 8, TAT = 5, WT = 3, RT = 3
  - **P5**: CT = 15, TAT = 11, WT = 7, RT = 7
* Summary:
  - Average TAT: (5 + 10 + 4 + 5 + 11) / 5 = 35 / 5 = **7.00**
  - Average WT: (0 + 7 + 3 + 3 + 7) / 5 = 20 / 5 = **4.00**
  - Average RT: **4.00**

---

### 3. Round Robin (Quantum = 2)
* Event-Ordering: Newly arrived processes during a slice are enqueued before the preempted process is re-enqueued.
* Execution Trace:
  - T=0: Ready = [P1]. P1 runs 0->2 (rem=3). Arrivals during (0,2]: P2 (at 1), P3 (at 2).
    Queue: `[P2, P3, P1]`
  - T=2: P2 runs 2->4 (rem=1). Arrivals during (2,4]: P4 (at 3), P5 (at 4).
    Queue: `[P3, P1, P4, P5, P2]`
  - T=4: P3 runs 4->5 (finishes!).
    Queue: `[P1, P4, P5, P2]`
  - T=5: P1 runs 5->7 (rem=1).
    Queue: `[P4, P5, P2, P1]`
  - T=7: P4 runs 7->9 (finishes!).
    Queue: `[P5, P2, P1]`
  - T=9: P5 runs 9->11 (rem=2).
    Queue: `[P2, P1, P5]`
  - T=11: P2 runs 11->12 (finishes!).
    Queue: `[P1, P5]`
  - T=12: P1 runs 12->13 (finishes!).
    Queue: `[P5]`
  - T=13: P5 runs 13->15 (finishes!).
* Per-process metrics:
  - **P1**: First start = 0, CT = 13, TAT = 13 - 0 = 13, WT = 13 - 5 = 8, RT = 0 - 0 = 0
  - **P2**: First start = 2, CT = 12, TAT = 12 - 1 = 11, WT = 11 - 3 = 8, RT = 2 - 1 = 1
  - **P3**: First start = 4, CT = 5, TAT = 5 - 2 = 3, WT = 3 - 1 = 2, RT = 4 - 2 = 2
  - **P4**: First start = 7, CT = 9, TAT = 9 - 3 = 6, WT = 6 - 2 = 4, RT = 7 - 3 = 4
  - **P5**: First start = 9, CT = 15, TAT = 15 - 4 = 11, WT = 11 - 4 = 7, RT = 9 - 4 = 5
* Summary:
  - Average TAT: (13 + 11 + 3 + 6 + 11) / 5 = 44 / 5 = **8.80**
  - Average WT: (8 + 8 + 2 + 4 + 7) / 5 = 29 / 5 = **5.80**
  - Average RT: (0 + 1 + 2 + 4 + 5) / 5 = 12 / 5 = **2.40**

---

### 4. Priority Scheduling (Non-Preemptive, Smaller Value = Higher Priority)
* Scheduling Decisions:
  - T=0: Only P1 is ready -> `[0 -> 5: P1]` (Priority 2)
  - T=5: P2 (Pri 1), P3 (Pri 3), P4 (Pri 4), P5 (Pri 2) are ready.
    * Highest priority is P2 (Pri 1) -> `[5 -> 8: P2]`
  - T=8: P5 (Pri 2), P3 (Pri 3), P4 (Pri 4) are ready.
    * Highest priority is P5 (Pri 2) -> `[8 -> 12: P5]`
  - T=12: P3 (Pri 3), P4 (Pri 4) are ready.
    * Highest priority is P3 (Pri 3) -> `[12 -> 13: P3]`
  - T=13: P4 (Pri 4) runs -> `[13 -> 15: P4]`
* Per-process metrics:
  - **P1**: CT = 5, TAT = 5, WT = 0, RT = 0
  - **P2**: CT = 8, TAT = 7, WT = 4, RT = 4
  - **P3**: CT = 13, TAT = 11, WT = 10, RT = 10
  - **P4**: CT = 15, TAT = 12, WT = 10, RT = 10
  - **P5**: CT = 12, TAT = 8, WT = 4, RT = 4
* Summary:
  - Average TAT: (5 + 7 + 11 + 12 + 8) / 5 = 43 / 5 = **8.60**
  - Average WT: (0 + 4 + 10 + 10 + 4) / 5 = 28 / 5 = **5.60**
  - Average RT: **5.60**

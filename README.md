# Linux System Resource Monitor and CPU Scheduling Simulator

A Linux-based system programming project developed in **C++17** that monitors CPU utilization, memory consumption, system uptime, and active processes through the Linux `/proc` filesystem. It also implements a CPU scheduling simulator to evaluate and compare four classical scheduling algorithms: First-Come, First-Served (FCFS), Shortest Job First (SJF), Round Robin (RR), and Priority Scheduling.

The project combines Linux system programming, operating systems concepts, resource monitoring, process management, scheduling algorithms, and C++ software engineering practices in a single command-line application.

---

## Table of Contents

- [Overview](#overview)
- [Objectives](#objectives)
- [Features](#features)
- [Technology Stack](#technology-stack)
- [System Architecture](#system-architecture)
- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Installation and Setup](#installation-and-setup)
- [Building the Project](#building-the-project)
- [Running the Application](#running-the-application)
- [System Monitoring](#system-monitoring)
- [CPU Scheduling Simulator](#cpu-scheduling-simulator)
- [Scheduling Metrics](#scheduling-metrics)
- [Sample Results](#sample-results)
- [Testing](#testing)
- [Implementation Details](#implementation-details)
- [Limitations](#limitations)
- [Troubleshooting](#troubleshooting)
- [Future Improvements](#future-improvements)
- [Learning Outcomes](#learning-outcomes)
- [License](#license)

---

## Overview

Modern operating systems continuously manage CPU time, memory, and processes. Understanding how these resources are measured and how scheduling algorithms affect process execution is fundamental to operating systems and system software development.

This project provides two complementary capabilities.

### 1. Linux System Resource Monitor

The monitoring component retrieves system information from Linux virtual files exposed through `/proc`.

It reports:

- System uptime.
- CPU utilization and its user, system, idle, and I/O-wait components.
- Total, available, and used memory.
- Memory utilization.
- Buffer and page-cache information.
- Swap capacity and utilization.
- Active processes, including process identifiers, names, states, parent process identifiers, memory usage, and CPU usage where available.

It supports both a one-time system information snapshot and continuous monitoring.

### 2. CPU Scheduling Simulator

The scheduling component simulates classical CPU scheduling algorithms using a supplied workload.

Supported algorithms:

- First-Come, First-Served (FCFS).
- Shortest Job First (non-preemptive SJF).
- Round Robin (RR).
- Priority Scheduling (non-preemptive).

The simulator produces per-process scheduling metrics, aggregate performance measurements, and a textual Gantt chart representing the execution sequence.

**Important:** The scheduling component operates as a user-space simulation. It does not replace or modify the actual Linux kernel scheduler.

---

## Objectives

The main objectives of this project are to:

1. Understand how Linux exposes system and process information through the `/proc` filesystem.
2. Implement resource-monitoring functionality using C++.
3. Parse CPU, memory, uptime, and process information from Linux system interfaces.
4. Implement classical CPU scheduling algorithms.
5. Calculate completion, turnaround, waiting, and response times.
6. Visualize simulated process execution through Gantt charts.
7. Compare scheduling algorithms using average performance metrics.
8. Apply modular programming, error handling, and automated testing.
9. Develop practical experience with Linux, CMake, GCC, and C++ system programming.

---

## Features

### System Resource Monitoring

| Feature               | Description                                                                   |
| --------------------- | ----------------------------------------------------------------------------- |
| System information    | Displays a snapshot of system resource information.                           |
| CPU monitoring        | Calculates CPU utilization from Linux CPU accounting data.                    |
| CPU breakdown         | Displays user, kernel/system, idle, and I/O-wait percentages.                 |
| Memory monitoring     | Reports total, available, and used memory.                                    |
| Swap monitoring       | Displays swap capacity, usage, and utilization.                               |
| System uptime         | Reports uptime in seconds and a human-readable format.                        |
| Process enumeration   | Lists accessible Linux processes.                                             |
| Process details       | Displays PID, command name, state, PPID, virtual memory, and resident memory. |
| Continuous monitoring | Refreshes resource metrics at a configurable interval.                        |

### CPU Scheduling

| Algorithm           | Scheduling policy                                        | Preemptive? |
| ------------------- | -------------------------------------------------------- | ----------- |
| FCFS                | Processes execute in arrival order.                      | No          |
| SJF                 | The shortest available CPU burst is selected.            | No          |
| Round Robin         | Ready processes receive CPU time in bounded time slices. | Yes         |
| Priority Scheduling | The highest-priority available process executes first.   | No          |

For Round Robin, the time quantum is configurable.

The scheduling output includes:

- Completion time.
- Turnaround time.
- Waiting time.
- Response time.
- Average turnaround time.
- Average waiting time.
- Average response time.
- Total simulated execution time.
- Total CPU idle time.
- Simulated CPU utilization.
- Execution sequence and Gantt chart.

---

## Technology Stack

| Technology                           | Purpose                                                |
| ------------------------------------ | ------------------------------------------------------ |
| C++17                                | Core implementation language                           |
| Linux                                | Operating-system environment for system monitoring     |
| `/proc` filesystem                   | Source of CPU, memory, uptime, and process information |
| GCC / G++                            | C++ compilation                                        |
| CMake                                | Build configuration and project generation             |
| CTest                                | Automated test execution                               |
| GDB                                  | Debugging                                              |
| Git                                  | Version control                                        |
| Ubuntu 24.04 LTS                     | Development and testing environment                    |
| Windows Subsystem for Linux 2 (WSL2) | Linux environment on Windows                           |

The application does not require a GPU, CUDA, or an external monitoring service.

---

## System Architecture

The application separates system monitoring, scheduling logic, command-line handling, and test code into distinct components.

```text
                 User
                  |
                  v
          Command-Line Interface
                  |
          +-------+--------+
          |                |
          v                v
   System Monitoring   CPU Scheduling
          |                |
          v                v
     Linux /proc      Workload Input
          |                |
          v                v
   CPU and Memory     Scheduling Algorithms
   Process Metrics    FCFS / SJF / RR / Priority
          |                |
          v                v
    System Report     Scheduling Metrics
                           |
                           v
                       Gantt Chart
```

### Monitoring data flow

1. The user selects a monitoring command.
2. The command-line interface dispatches the request.
3. The monitoring components read the required Linux system information.
4. The application parses the data and calculates the relevant metrics.
5. The results are formatted and displayed in the terminal.

### Scheduling data flow

1. The user supplies a scheduling algorithm and CSV workload.
2. The application loads the process information.
3. The selected scheduling algorithm simulates process execution.
4. The application calculates per-process and aggregate metrics.
5. The execution sequence and Gantt chart are displayed.

---

## Project Structure

```text
linux-system-monitor/
|
|-- CMakeLists.txt
|-- README.md
|-- LICENSE
|-- .gitignore
|
|-- include/
|   |-- cli.hpp
|   |-- cpu_monitor.hpp
|   |-- errors.hpp
|   |-- memory_monitor.hpp
|   |-- metrics.hpp
|   |-- process_model.hpp
|   |-- process_monitor.hpp
|   |-- scheduler.hpp
|   |-- system_monitor.hpp
|
|-- src/
|   |-- main.cpp
|   |-- cli.cpp
|   |-- cpu_monitor.cpp
|   |-- memory_monitor.cpp
|   |-- process_model.cpp
|   |-- process_monitor.cpp
|   |-- scheduler.cpp
|   |-- system_monitor.cpp
|
|-- tests/
|   |-- test_main.cpp
|   |-- test_cpu_metrics.cpp
|   |-- test_memory_metrics.cpp
|   |-- test_process_parsing.cpp
|   |-- test_scheduler.cpp
|
|-- examples/
|   |-- processes.csv
|   |-- processes_idle.csv
|   |-- processes_simultaneous.csv
|   |-- expected_scheduler_output.md
|
|-- scripts/
|   |-- build.sh
|   |-- run_demo.sh
|   |-- run_tests.sh
|
|-- docs/
|   |-- ARCHITECTURE.md
|   |-- DESIGN_DECISIONS.md
|   |-- INTERVIEW_PREPARATION.md
|   |-- TESTING.md
|
|-- build/
|-- build-wsl/
```

**Note:** `build/` and `build-wsl/` are generated build directories, not source-code directories. They may differ between environments. The source tree above reflects the intended project organization; check the actual files in `include/`, `tests/`, and `scripts/` if your local version differs.

---

## Prerequisites

For Linux development, the following tools are required:

- Ubuntu or another compatible Linux distribution.
- GCC/G++ with C++17 support.
- CMake.
- GNU Make or another supported CMake build tool.
- Git (optional, for version control).
- GDB (optional, for debugging).

For Windows users, Ubuntu through WSL2 provides the Linux environment required by the monitoring component.

### Verify the environment

Run:

```bash
g++ --version
cmake --version
git --version
```

The compiler must support C++17.

---

## Installation and Setup

### 1. Open Ubuntu through WSL2

From Windows PowerShell:

```powershell
wsl -d Ubuntu
```

If Ubuntu is already running, you can continue in its terminal.

### 2. Navigate to the project directory

For the current Windows project location:

```bash
cd /mnt/c/Users/JAKKOJU/.gemini/antigravity-ide/scratch/linux-system-monitor
```

Replace the path if your project is stored elsewhere.

### 3. Install the build tools

```bash
sudo apt update
sudo apt install build-essential cmake gdb -y
```

### 4. Verify the project files

```bash
ls
```

The project root should contain `CMakeLists.txt`, `src/`, `include/`, `tests/`, and `examples/`.

---

## Building the Project

Use a separate build directory for Ubuntu to avoid mixing generated files from Windows and Linux build environments.

### Step 1: Configure

From the project root:

```bash
cmake -S . -B build-wsl
```

This configures the project and generates the required build files.

### Step 2: Compile

```bash
cmake --build build-wsl -j2
```

CMake compiles the source files, creates the core library, and builds the application and test executable.

A successful build should include targets similar to:

```text
[ 50%] Built target sysmon_core
[ 81%] Built target system_monitor
[100%] Built target run_unit_tests
```

The exact percentages and order can vary.

### Step 3: Verify the executable

```bash
ls -l build-wsl/system_monitor
```

If the executable exists, the application is ready to run.

---

## Running the Application

Run the following commands from the project root.

### Display help

```bash
./build-wsl/system_monitor --help
```

The help menu lists the supported commands and arguments.

### Display system information

```bash
./build-wsl/system_monitor info
```

Displays a snapshot containing system uptime, CPU metrics, memory information, and other implemented resource metrics.

### List active processes

```bash
./build-wsl/system_monitor processes
```

Displays accessible processes and their reported resource information.

### Start continuous monitoring

```bash
./build-wsl/system_monitor monitor --interval 2
```

Refreshes the monitoring display at a two-second interval.

Press `Ctrl+C` to stop continuous monitoring.

### Command summary

| Command                                           | Purpose                               |
| ------------------------------------------------- | ------------------------------------- |
| `./build-wsl/system_monitor --help`               | Display usage information             |
| `./build-wsl/system_monitor info`                 | Display a system information snapshot |
| `./build-wsl/system_monitor processes`            | List accessible processes             |
| `./build-wsl/system_monitor monitor --interval 2` | Continuously monitor system resources |

---

## System Monitoring

### CPU utilization

The CPU monitor reads Linux CPU accounting information from:

```text
/proc/stat
```

CPU utilization is calculated by comparing CPU accounting counters across samples.

A general utilization calculation is:

$$
\text{CPU Utilization} =
\frac{\Delta T-\Delta I}{\Delta T}\times100
$$

where:

- \(\Delta T\) represents the change in total CPU accounting time.
- \(\Delta I\) represents the change in the idle category used by the implementation.

The calculation uses counter differences rather than treating cumulative counters as instantaneous utilization.

The implementation reports user, system, idle, and I/O-wait information. Its displayed idle convention should be interpreted according to the implementation's documented accounting policy.

### Memory utilization

Memory information is read from:

```text
/proc/meminfo
```

Important Linux memory fields include:

- `MemTotal`
- `MemAvailable`
- `MemFree`
- `Buffers`
- `Cached`
- `SwapTotal`
- `SwapFree`

A common estimate of memory used is:

$$
\text{Used Memory} =
\text{Total Memory}-\text{Available Memory}
$$

Memory utilization is:

$$
\text{Memory Utilization} =
\frac{\text{Used Memory}}{\text{Total Memory}}\times100
$$

The distinction between `MemFree` and `MemAvailable` is important. Linux may use otherwise free memory for caches and buffers, which can be reclaimed when applications need memory.

### System uptime

The monitor reads:

```text
/proc/uptime
```

The uptime value is converted into a human-readable duration and displayed in seconds.

### Process monitoring

Linux process information is obtained from the `/proc` directory.

Relevant paths include:

```text
/proc/<PID>/stat
/proc/<PID>/status
```

The monitor reports information such as:

- Process identifier (PID).
- Process name.
- Process state.
- Parent process identifier (PPID).
- Virtual memory size.
- Resident memory size.
- CPU usage, where available.

Processes may exit while the application is enumerating them. Access to some process information may also be restricted. Consequently, the number of processes displayed can differ from the number initially discovered.

---

## CPU Scheduling Simulator

The simulator models CPU scheduling in user space using process workloads supplied through CSV files.

Each workload represents processes with scheduling attributes such as:

- Process identifier.
- Arrival time.
- CPU burst time.
- Priority.

The precise CSV header and field order are defined by the workload parser and the sample files in `examples/`.

### 1. First-Come, First-Served (FCFS)

FCFS schedules ready processes in arrival order.

Characteristics:

- Non-preemptive.
- Simple to implement.
- Can suffer from the convoy effect when a long process delays shorter processes.

Command:

```bash
./build-wsl/system_monitor schedule --algorithm fcfs --input examples/processes.csv
```

### 2. Shortest Job First (SJF)

Non-preemptive SJF selects the ready process with the shortest CPU burst.

Characteristics:

- Non-preemptive.
- Can reduce average waiting time for suitable workloads.
- Requires knowledge or estimates of CPU burst lengths.
- Long jobs may experience starvation in workloads where shorter jobs continually arrive.

Command:

```bash
./build-wsl/system_monitor schedule --algorithm sjf --input examples/processes.csv
```

### 3. Round Robin (RR)

Round Robin assigns each ready process a bounded CPU time slice called the time quantum.

A process that does not finish during its quantum returns to the ready queue according to the simulator's queue policy.

Characteristics:

- Preemptive.
- Supports time-sharing behavior.
- A very small quantum increases scheduling overhead in real systems.
- A very large quantum makes its behavior more similar to FCFS.

Command:

```bash
./build-wsl/system_monitor schedule --algorithm rr --quantum 2 --input examples/processes.csv
```

Here, the quantum is two simulated time units.

### 4. Priority Scheduling

Priority Scheduling selects the highest-priority process among the processes currently available.

The project specification uses non-preemptive Priority Scheduling.

If the implementation follows the documented convention, a smaller numerical priority value indicates a higher priority.

Command:

```bash
./build-wsl/system_monitor schedule --algorithm priority --input examples/processes.csv
```

### Compare all algorithms

To execute all four algorithms against the same workload:

```bash
./build-wsl/system_monitor schedule --all --input examples/processes.csv
```

Using the same workload makes it easier to compare the resulting scheduling metrics.

---

## Scheduling Metrics

Let:

- \(AT\) = Arrival Time
- \(BT\) = Burst Time
- \(CT\) = Completion Time
- \(ST\) = First Start Time
- \(TAT\) = Turnaround Time
- \(WT\) = Waiting Time
- \(RT\) = Response Time

### Completion Time

The time at which a process finishes execution.

$$
CT = \text{Time when the process finishes}
$$

### Turnaround Time

The total elapsed time from arrival to completion.

$$
TAT = CT-AT
$$

### Waiting Time

The total time spent waiting in the ready queue.

For the scheduling models implemented here:

$$
WT = TAT-BT
$$

### Response Time

The time from arrival until the process first receives the CPU.

$$
RT = ST-AT
$$

For a preemptive algorithm such as Round Robin, response time is determined by the first execution, not the final execution.

### Average Turnaround Time

For \(n\) processes:

$$
\overline{TAT} =
\frac{\sum_{i=1}^{n}TAT_i}{n}
$$

### Average Waiting Time

$$
\overline{WT} =
\frac{\sum_{i=1}^{n}WT_i}{n}
$$

### Average Response Time

$$
\overline{RT} =
\frac{\sum_{i=1}^{n}RT_i}{n}
$$

### Simulated CPU utilization

For a workload with a defined simulation interval:

$$
\text{CPU Utilization} =
\frac{\text{Busy CPU Time}}{\text{Total Simulated Time}}
\times100
$$

This is a simulated workload metric, not the real CPU utilization reported by the Linux system monitor.

---

## Sample Results

The following results were obtained by running the implemented application against `examples/processes.csv`.

### FCFS example

Command:

```bash
./build-wsl/system_monitor schedule --algorithm fcfs --input examples/processes.csv
```

Input workload results:

| Process | Arrival | Burst | Completion | Turnaround | Waiting | Response |
| ------- | ------: | ----: | ---------: | ---------: | ------: | -------: |
| P1      |       0 |     5 |          5 |          5 |       0 |        0 |
| P2      |       1 |     3 |          8 |          7 |       4 |        4 |
| P3      |       2 |     1 |          9 |          7 |       6 |        6 |
| P4      |       3 |     2 |         11 |          8 |       6 |        6 |
| P5      |       4 |     4 |         15 |         11 |       7 |        7 |

Aggregate metrics:

| Metric                    | Result |
| ------------------------- | -----: |
| Average turnaround time   |   7.60 |
| Average waiting time      |   4.60 |
| Average response time     |   4.60 |
| Total simulated time      |     15 |
| Total CPU idle time       |      0 |
| Simulated CPU utilization |   100% |

Gantt chart:

```text
[0 -> 5: P1] [5 -> 8: P2] [8 -> 9: P3] [9 -> 11: P4] [11 -> 15: P5]
```

### Round Robin example

Command:

```bash
./build-wsl/system_monitor schedule --algorithm rr --quantum 2 --input examples/processes.csv
```

Aggregate metrics reported by the application:

| Metric                    | Result |
| ------------------------- | -----: |
| Average turnaround time   |   8.80 |
| Average waiting time      |   5.80 |
| Average response time     |   2.40 |
| Total simulated time      |     15 |
| Total CPU idle time       |      0 |
| Simulated CPU utilization |   100% |

Gantt chart:

```text
[0 -> 2: P1] [2 -> 4: P2] [4 -> 5: P3] [5 -> 7: P1]
[7 -> 9: P4] [9 -> 11: P5] [11 -> 12: P2]
[12 -> 13: P1] [13 -> 15: P5]
```

These results illustrate how different scheduling policies can produce different waiting, turnaround, and response times for the same workload.

The sample results are specific to this workload and configuration; they are not universal performance guarantees.

---

## Testing

The project includes unit tests for CPU metrics, memory metrics, process parsing, and scheduling behavior.

### Build the test executable

The test executable is built as part of the regular CMake build:

```bash
cmake --build build-wsl -j2
```

### Run the unit tests directly

```bash
./build-wsl/run_unit_tests
```

### Run tests through CTest

```bash
ctest --test-dir build-wsl --output-on-failure
```

CTest provides a summary of registered test results and displays failure details when a test fails.

### Verified test result

The current development run reported:

```text
Test project .../build-wsl
    Start 1: AllUnitTests
1/1 Test #1: AllUnitTests ..................... Passed

100% tests passed, 0 tests failed out of 1
```

This confirms that the registered unit-test executable passed in that run. It does not establish that every possible input, operating-system configuration, or edge case has been tested.

### Additional workload tests

Run the simulator against the provided workloads:

```bash
./build-wsl/system_monitor schedule --all --input examples/processes.csv
```

```bash
./build-wsl/system_monitor schedule --all --input examples/processes_idle.csv
```

```bash
./build-wsl/system_monitor schedule --all --input examples/processes_simultaneous.csv
```

These workloads help examine ordinary arrivals, CPU idle intervals, and simultaneous process arrivals.

---

## Implementation Details

### Modular C++ design

The project separates the command-line interface, monitoring components, process model, and scheduling algorithms into individual source and header files.

This separation improves readability, testability, and maintainability.

### Linux system interfaces

The monitoring components use the Linux `/proc` filesystem rather than relying on Windows-specific system APIs.

### Counter-based CPU utilization

CPU utilization is calculated using differences between CPU accounting samples. This is important because `/proc/stat` contains cumulative accounting counters.

### Scheduling simulation

The scheduling algorithms operate on a supplied workload rather than controlling actual operating-system processes.

This separation makes scheduling behavior easier to reproduce and evaluate.

### Automated testing

The project uses a CMake-based build and registered tests executed through CTest.

---

## Limitations

The following limitations should be considered when using or extending the project.

1. **Linux dependency:** System monitoring relies on Linux interfaces and is intended to run under Linux or WSL2.
2. **User-space simulation:** Scheduling algorithms simulate process execution; they do not change the Linux kernel scheduler.
3. **Snapshot timing:** CPU utilization depends on the sampling interval and the counters observed between samples.
4. **Process enumeration races:** Processes may terminate or change state while being inspected.
5. **Permissions:** Some process information may not be accessible to an ordinary user.
6. **Workload dependence:** Scheduling results depend on arrival times, burst times, priorities, and the configured Round Robin quantum.
7. **Testing scope:** Passing unit tests does not guarantee correctness for every possible workload or system configuration.
8. **Display formatting:** Process names may be shortened in tabular output to fit the available column width.

---

## Troubleshooting

### CMake is not found

On Ubuntu:

```bash
sudo apt update
sudo apt install cmake build-essential -y
```

### Compilation fails

Reconfigure and rebuild:

```bash
cmake -S . -B build-wsl
cmake --build build-wsl -j2
```

Read the first meaningful compiler error before investigating subsequent messages.

### Executable not found

Check the generated files:

```bash
find build-wsl -type f -executable
```

Confirm that you are running commands from the project root.

### CPU or process monitoring fails

Confirm that the program is running in Linux rather than Git Bash or ordinary Windows PowerShell.

Check the availability of the relevant Linux interfaces:

```bash
cat /proc/stat
cat /proc/meminfo
cat /proc/uptime
```

### CSV input file not found

Inspect the example directory:

```bash
ls -la examples
```

Use an existing CSV filename and a path relative to the project root.

### Tests fail

Run:

```bash
ctest --test-dir build-wsl --output-on-failure
```

Inspect the reported failure and verify the corresponding implementation and test case.

---

## Future Improvements

Potential extensions include:

- Improve process-name parsing and terminal formatting.
- Add more extensive scheduler edge-case tests.
- Add validation for malformed workloads and invalid scheduling parameters.
- Improve process-level CPU utilization measurement and document its sampling semantics.
- Add configurable monitoring intervals and output formats.
- Export monitoring snapshots and scheduling results to CSV.
- Add historical metrics and summary statistics.
- Improve diagnostic messages and command-line argument validation.
- Add benchmarks for scheduling simulations using larger workloads.
- Expand the automated test suite for scheduling invariants and unusual arrival patterns.

These are potential extensions, not claims about functionality already implemented.

---

## Learning Outcomes

This project provides practical experience with:

- C++17 application development.
- Linux system programming.
- CPU and memory accounting.
- Linux process information and the `/proc` filesystem.
- Operating-system scheduling algorithms.
- Process timing and performance metrics.
- Gantt chart generation.
- CMake-based compilation.
- Unit testing and CTest.
- Debugging with GDB.
- Modular software design.

The project is particularly relevant to learning operating systems, Linux internals, computer architecture fundamentals, and system software development.

---

## License

This project includes a `LICENSE` file. Refer to that file for the applicable license terms.

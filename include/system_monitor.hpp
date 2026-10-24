#pragma once

#include "cpu_monitor.hpp"
#include "memory_monitor.hpp"
#include "process_monitor.hpp"
#include "metrics.hpp"
#include <atomic>
#include <string>

namespace sysmon {

struct SystemSnapshot {
    UptimeStats uptime;
    CpuUtilization cpu;
    MemoryStats memory;
    size_t total_processes{0};
};

class SystemMonitor {
public:
    SystemMonitor();
    SystemMonitor(std::string proc_dir);

    /**
     * Read /proc/uptime.
     */
    static UptimeStats parse_proc_uptime(std::istream& is);
    static UptimeStats parse_uptime_string(const std::string& line);
    UptimeStats read_uptime();

    /**
     * Capture a single snapshot of system metrics.
     * Performs two CPU samples separated by sample_delay_ms if CPU utilization
     * is not already calculated.
     */
    SystemSnapshot take_snapshot(int sample_delay_ms = 200);

    /**
     * Print a formatted single snapshot of system resource metrics to stdout.
     */
    void print_system_info();

    /**
     * Print a table of currently active processes to stdout.
     */
    void print_processes(size_t limit = 50);

    /**
     * Run continuous monitoring loop until stop_flag becomes true or SIGINT received.
     */
    void run_continuous_monitor(int interval_seconds, const std::atomic<bool>& stop_flag);

    CpuMonitor& cpu_monitor() noexcept { return cpu_monitor_; }
    MemoryMonitor& memory_monitor() noexcept { return memory_monitor_; }
    ProcessMonitor& process_monitor() noexcept { return process_monitor_; }

private:
    std::string proc_dir_{"/proc"};
    CpuMonitor cpu_monitor_;
    MemoryMonitor memory_monitor_;
    ProcessMonitor process_monitor_;
};

} // namespace sysmon

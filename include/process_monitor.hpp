#pragma once

#include "errors.hpp"
#include <chrono>
#include <cstdint>
#include <istream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace sysmon {

/**
 * Information regarding an active Linux process read from /proc/[pid]/stat
 * and /proc/[pid]/status.
 */
struct ProcessInfo {
    int pid{0};
    std::string name;
    char state{'?'};
    std::string state_desc{"Unknown"};
    int ppid{0};
    uint64_t vmsize_kb{0};
    uint64_t vmrss_kb{0};
    uint64_t utime_ticks{0};
    uint64_t stime_ticks{0};
    double cpu_percent{0.0}; // Percentage of one CPU core capacity
};

struct ProcessSample {
    uint64_t total_ticks{0};
    std::chrono::steady_clock::time_point timestamp;
};

class ProcessMonitor {
public:
    ProcessMonitor();
    explicit ProcessMonitor(std::string proc_dir);

    /**
     * Parse /proc/[pid]/stat line.
     * Guaranteed to correctly handle process names containing spaces and parentheses
     * by finding the first '(' and the last ')' in the line.
     */
    static std::optional<ProcessInfo> parse_stat_line(const std::string& line, long clk_tck = 100);

    /**
     * Parse /proc/[pid]/status file stream for VmSize and VmRSS memory metrics.
     */
    static void parse_status_stream(std::istream& is, ProcessInfo& info);

    /**
     * Translate Linux kernel process state character to human-readable label.
     */
    static std::string state_char_to_description(char state);

    /**
     * Enumerate all accessible processes under /proc.
     * Silently ignores processes that terminate or are inaccessible due to permissions.
     */
    std::vector<ProcessInfo> enumerate_processes();

    /**
     * Reset per-process CPU tracking cache.
     */
    void reset_cpu_cache() noexcept;

private:
    std::string proc_dir_{"/proc"};
    long clk_tck_{100};
    std::unordered_map<int, ProcessSample> prev_samples_;
    std::chrono::steady_clock::time_point last_sample_time_;
};

} // namespace sysmon

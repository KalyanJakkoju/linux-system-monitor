#pragma once

#include "errors.hpp"
#include "metrics.hpp"
#include <istream>
#include <optional>
#include <string>

namespace sysmon {

class MemoryMonitor {
public:
    MemoryMonitor();
    explicit MemoryMonitor(std::string proc_meminfo_path);

    /**
     * Parse /proc/meminfo stream into MemoryStats.
     * Pure function for unit testing.
     */
    static MemoryStats parse_proc_meminfo(std::istream& stream);
    static MemoryStats parse_proc_meminfo_string(const std::string& content);

    /**
     * Read current /proc/meminfo from host filesystem.
     */
    MemoryStats sample();

    /**
     * Get the latest sampled memory statistics.
     */
    const MemoryStats& get_stats() const noexcept { return stats_; }

private:
    std::string proc_meminfo_path_{"/proc/meminfo"};
    MemoryStats stats_;
};

} // namespace sysmon

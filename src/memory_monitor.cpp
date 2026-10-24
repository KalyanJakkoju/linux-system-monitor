#include "memory_monitor.hpp"
#include <fstream>
#include <sstream>

namespace sysmon {

MemoryMonitor::MemoryMonitor() : MemoryMonitor("/proc/meminfo") {}

MemoryMonitor::MemoryMonitor(std::string proc_meminfo_path)
    : proc_meminfo_path_(std::move(proc_meminfo_path)) {}

MemoryStats MemoryMonitor::parse_proc_meminfo(std::istream& stream) {
    MemoryStats stats{};
    std::string line;

    bool found_available = false;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        auto colon_pos = line.find(':');
        if (colon_pos == std::string::npos) continue;

        std::string key = line.substr(0, colon_pos);
        std::istringstream val_stream(line.substr(colon_pos + 1));
        uint64_t val = 0;
        if (!(val_stream >> val)) continue;

        if (key == "MemTotal") {
            stats.mem_total_kb = val;
        } else if (key == "MemAvailable") {
            stats.mem_available_kb = val;
            found_available = true;
        } else if (key == "MemFree") {
            stats.mem_free_kb = val;
        } else if (key == "Buffers") {
            stats.buffers_kb = val;
        } else if (key == "Cached") {
            stats.cached_kb = val;
        } else if (key == "SwapTotal") {
            stats.swap_total_kb = val;
        } else if (key == "SwapFree") {
            stats.swap_free_kb = val;
        }
    }

    stats.has_mem_available = found_available;

    // Fallback for older Linux kernels (< 3.14) where MemAvailable was not exposed:
    // Approximate MemAvailable as MemFree + Buffers + Cached
    if (!found_available && stats.mem_total_kb > 0) {
        stats.mem_available_kb = stats.mem_free_kb + stats.buffers_kb + stats.cached_kb;
        // Cap at MemTotal
        if (stats.mem_available_kb > stats.mem_total_kb) {
            stats.mem_available_kb = stats.mem_total_kb;
        }
    }

    // Safety guard against underflow / malformed data where available > total
    if (stats.mem_available_kb > stats.mem_total_kb) {
        stats.mem_available_kb = stats.mem_total_kb;
    }

    return stats;
}

MemoryStats MemoryMonitor::parse_proc_meminfo_string(const std::string& content) {
    std::istringstream iss(content);
    return parse_proc_meminfo(iss);
}

MemoryStats MemoryMonitor::sample() {
    std::ifstream file(proc_meminfo_path_);
    if (!file.is_open()) {
        throw SystemMonitorError(ErrorCode::FileNotFound,
            "Could not open " + proc_meminfo_path_ + " for memory metrics.");
    }
    stats_ = parse_proc_meminfo(file);
    return stats_;
}

} // namespace sysmon

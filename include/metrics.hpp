#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sysmon {

/**
 * Raw aggregate CPU time counters from /proc/stat.
 * All units are in Linux USER_HZ / clock ticks.
 */
struct CpuCounters {
    uint64_t user{0};
    uint64_t nice{0};
    uint64_t system{0};
    uint64_t idle{0};
    uint64_t iowait{0};
    uint64_t irq{0};
    uint64_t softirq{0};
    uint64_t steal{0};
    uint64_t guest{0};
    uint64_t guest_nice{0};

    // Total idle time: idle + iowait
    // Note: Linux accounting notes that iowait is time the CPU had nothing to do
    // but was waiting for an outstanding disk/network I/O request.
    // In this project, idle + iowait is counted as idle time.
    uint64_t get_idle_time() const noexcept {
        return idle + iowait;
    }

    // Total busy time (non-idle)
    uint64_t get_busy_time() const noexcept {
        return user + nice + system + irq + softirq + steal + guest + guest_nice;
    }

    // Grand total time (all ticks)
    uint64_t get_total_time() const noexcept {
        return get_idle_time() + get_busy_time();
    }
};

/**
 * Computed CPU utilization metrics calculated between two distinct samples.
 */
struct CpuUtilization {
    double total_utilization_percent{0.0};
    double user_percent{0.0};
    double system_percent{0.0};
    double idle_percent{0.0};
    double iowait_percent{0.0};
    bool is_valid{false};
};

/**
 * Memory metrics parsed from /proc/meminfo.
 * Stored internally in Kilobytes (kB, as exposed by Linux kernel),
 * with conversion helpers to MiB and GiB.
 */
struct MemoryStats {
    uint64_t mem_total_kb{0};
    uint64_t mem_available_kb{0};
    uint64_t mem_free_kb{0};
    uint64_t buffers_kb{0};
    uint64_t cached_kb{0};
    uint64_t swap_total_kb{0};
    uint64_t swap_free_kb{0};

    bool has_mem_available{false};

    // Calculated fields
    uint64_t used_memory_kb() const noexcept {
        if (mem_total_kb >= mem_available_kb) {
            return mem_total_kb - mem_available_kb;
        }
        return 0; // Guard against underflow
    }

    uint64_t used_swap_kb() const noexcept {
        if (swap_total_kb >= swap_free_kb) {
            return swap_total_kb - swap_free_kb;
        }
        return 0;
    }

    double memory_utilization_percent() const noexcept {
        if (mem_total_kb == 0) return 0.0;
        return (static_cast<double>(used_memory_kb()) / static_cast<double>(mem_total_kb)) * 100.0;
    }

    double swap_utilization_percent() const noexcept {
        if (swap_total_kb == 0) return 0.0;
        return (static_cast<double>(used_swap_kb()) / static_cast<double>(swap_total_kb)) * 100.0;
    }

    // Unit conversion helpers (binary mebibytes and gibibytes: 1024-based)
    static double kb_to_mib(uint64_t kb) noexcept {
        return static_cast<double>(kb) / 1024.0;
    }

    static double kb_to_gib(uint64_t kb) noexcept {
        return static_cast<double>(kb) / (1024.0 * 1024.0);
    }
};

/**
 * System Uptime information parsed from /proc/uptime.
 */
struct UptimeStats {
    double uptime_seconds{0.0};
    double idle_seconds{0.0};
    bool is_valid{false};

    std::string to_formatted_string() const;
};

} // namespace sysmon

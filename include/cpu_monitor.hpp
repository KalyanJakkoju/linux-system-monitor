#pragma once

#include "errors.hpp"
#include "metrics.hpp"
#include <istream>
#include <optional>
#include <string>

namespace sysmon {

class CpuMonitor {
public:
    CpuMonitor();
    explicit CpuMonitor(std::string proc_stat_path);

    /**
     * Parse the aggregate 'cpu ' line from /proc/stat stream.
     * Pure function for easy unit testing.
     */
    static std::optional<CpuCounters> parse_cpu_stat_line(const std::string& line);
    static std::optional<CpuCounters> parse_proc_stat(std::istream& stream);

    /**
     * Compute CPU utilization from two successive samples.
     * Throws or returns invalid CpuUtilization on non-monotonicity or zero delta.
     */
    static CpuUtilization calculate_utilization(const CpuCounters& prev, const CpuCounters& curr);

    /**
     * Sample current /proc/stat from host filesystem.
     * Returns true if sample was successfully read.
     */
    bool sample();

    /**
     * Return true if at least two samples have been acquired to calculate utilization.
     */
    bool has_utilization() const noexcept { return has_prev_ && has_curr_; }

    /**
     * Retrieve the latest computed CPU utilization.
     * Throws SystemMonitorError if two samples are not yet available.
     */
    CpuUtilization get_utilization() const;

    /**
     * Reset sampling history.
     */
    void reset() noexcept;

    const std::optional<CpuCounters>& current_sample() const noexcept { return curr_sample_; }
    const std::optional<CpuCounters>& previous_sample() const noexcept { return prev_sample_; }

private:
    std::string proc_stat_path_{"/proc/stat"};
    std::optional<CpuCounters> prev_sample_;
    std::optional<CpuCounters> curr_sample_;
    bool has_prev_{false};
    bool has_curr_{false};
};

} // namespace sysmon

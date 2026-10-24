#include "cpu_monitor.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace sysmon {

CpuMonitor::CpuMonitor() : CpuMonitor("/proc/stat") {}

CpuMonitor::CpuMonitor(std::string proc_stat_path)
    : proc_stat_path_(std::move(proc_stat_path)) {}

std::optional<CpuCounters> CpuMonitor::parse_cpu_stat_line(const std::string& line) {
    // Only aggregate CPU line starts with "cpu " (four chars: 'c', 'p', 'u', ' ')
    if (line.size() < 5 || line.substr(0, 4) != "cpu ") {
        return std::nullopt;
    }

    std::istringstream iss(line);
    std::string cpu_label;
    iss >> cpu_label; // Consume "cpu"

    CpuCounters counters{};
    if (!(iss >> counters.user)) return std::nullopt;
    if (!(iss >> counters.nice)) return std::nullopt;
    if (!(iss >> counters.system)) return std::nullopt;
    if (!(iss >> counters.idle)) return std::nullopt;

    // Optional fields present in modern Linux kernels
    if (iss >> counters.iowait) {
        if (iss >> counters.irq) {
            if (iss >> counters.softirq) {
                if (iss >> counters.steal) {
                    if (iss >> counters.guest) {
                        iss >> counters.guest_nice;
                    }
                }
            }
        }
    }

    return counters;
}

std::optional<CpuCounters> CpuMonitor::parse_proc_stat(std::istream& stream) {
    std::string line;
    while (std::getline(stream, line)) {
        if (line.rfind("cpu ", 0) == 0) {
            return parse_cpu_stat_line(line);
        }
    }
    return std::nullopt;
}

CpuUtilization CpuMonitor::calculate_utilization(const CpuCounters& prev, const CpuCounters& curr) {
    const uint64_t prev_total = prev.get_total_time();
    const uint64_t curr_total = curr.get_total_time();

    const uint64_t prev_idle = prev.get_idle_time();
    const uint64_t curr_idle = curr.get_idle_time();

    // Check for counter decrease / tick wrap
    if (curr_total < prev_total || curr_idle < prev_idle ||
        curr.user < prev.user || curr.system < prev.system) {
        throw SystemMonitorError(ErrorCode::CounterDecreased,
            "Detected non-monotonic decrease in CPU counters (potential counter wrap or reset).");
    }

    const uint64_t total_delta = curr_total - prev_total;
    if (total_delta == 0) {
        // Zero elapsed ticks: impossible to compute meaningful delta
        CpuUtilization res{};
        res.is_valid = false;
        return res;
    }

    const uint64_t idle_delta = curr_idle - prev_idle;
    const uint64_t busy_delta = (total_delta >= idle_delta) ? (total_delta - idle_delta) : 0;

    double util_percent = (100.0 * static_cast<double>(busy_delta)) / static_cast<double>(total_delta);
    util_percent = std::clamp(util_percent, 0.0, 100.0);

    const uint64_t user_delta = (curr.user - prev.user) + (curr.nice - prev.nice);
    const uint64_t sys_delta = (curr.system - prev.system) + (curr.irq - prev.irq) + (curr.softirq - prev.softirq);
    const uint64_t iowait_delta = (curr.iowait >= prev.iowait) ? (curr.iowait - prev.iowait) : 0;

    double user_percent = (100.0 * static_cast<double>(user_delta)) / static_cast<double>(total_delta);
    double sys_percent = (100.0 * static_cast<double>(sys_delta)) / static_cast<double>(total_delta);
    double idle_percent = (100.0 * static_cast<double>(idle_delta)) / static_cast<double>(total_delta);
    double iowait_percent = (100.0 * static_cast<double>(iowait_delta)) / static_cast<double>(total_delta);

    CpuUtilization result;
    result.total_utilization_percent = util_percent;
    result.user_percent = std::clamp(user_percent, 0.0, 100.0);
    result.system_percent = std::clamp(sys_percent, 0.0, 100.0);
    result.idle_percent = std::clamp(idle_percent, 0.0, 100.0);
    result.iowait_percent = std::clamp(iowait_percent, 0.0, 100.0);
    result.is_valid = true;

    return result;
}

bool CpuMonitor::sample() {
    std::ifstream file(proc_stat_path_);
    if (!file.is_open()) {
        return false;
    }

    auto parsed = parse_proc_stat(file);
    if (!parsed) {
        return false;
    }

    if (curr_sample_.has_value()) {
        prev_sample_ = curr_sample_;
        has_prev_ = true;
    }

    curr_sample_ = parsed;
    has_curr_ = true;
    return true;
}

CpuUtilization CpuMonitor::get_utilization() const {
    if (!has_utilization() || !prev_sample_ || !curr_sample_) {
        throw SystemMonitorError(ErrorCode::NotEnoughSamples,
            "At least two successive CPU samples separated by time are required to compute CPU utilization.");
    }
    return calculate_utilization(*prev_sample_, *curr_sample_);
}

void CpuMonitor::reset() noexcept {
    prev_sample_.reset();
    curr_sample_.reset();
    has_prev_ = false;
    has_curr_ = false;
}

} // namespace sysmon

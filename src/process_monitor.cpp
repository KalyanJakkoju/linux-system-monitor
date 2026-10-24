#include "process_monitor.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>

namespace fs = std::filesystem;

namespace sysmon {

ProcessMonitor::ProcessMonitor() : ProcessMonitor("/proc") {}

ProcessMonitor::ProcessMonitor(std::string proc_dir)
    : proc_dir_(std::move(proc_dir)) {
    clk_tck_ = ::sysconf(_SC_CLK_TCK);
    if (clk_tck_ <= 0) {
        clk_tck_ = 100; // Standard Linux default (USER_HZ)
    }
    last_sample_time_ = std::chrono::steady_clock::now();
}

std::string ProcessMonitor::state_char_to_description(char state) {
    switch (state) {
        case 'R': return "Running";
        case 'S': return "Interruptible Sleep";
        case 'D': return "Uninterruptible Sleep (D)";
        case 'T': return "Stopped (Signal)";
        case 't': return "Tracing Stop";
        case 'Z': return "Zombie";
        case 'X':
        case 'x': return "Dead";
        case 'K': return "Wakekill";
        case 'W': return "Waking";
        case 'P': return "Parked";
        case 'I': return "Idle Kernel Thread";
        default: return "Unknown (" + std::string(1, state) + ")";
    }
}

std::optional<ProcessInfo> ProcessMonitor::parse_stat_line(const std::string& line, long /*clk_tck*/) {
    // A valid /proc/[pid]/stat line has:
    // PID (COMM) STATE PPID ...
    // Note: COMM can contain spaces and parentheses: e.g. "1234 (my (cool) proc) S 1 ..."
    // The comm is bounded by the FIRST '(' and the LAST ')' on the line.
    const auto first_open = line.find('(');
    const auto last_close = line.rfind(')');

    if (first_open == std::string::npos || last_close == std::string::npos || first_open >= last_close) {
        return std::nullopt;
    }

    // Parse PID before the first '('
    std::string pid_str = line.substr(0, first_open);
    // Trim whitespace
    while (!pid_str.empty() && std::isspace(static_cast<unsigned char>(pid_str.back()))) {
        pid_str.pop_back();
    }
    while (!pid_str.empty() && std::isspace(static_cast<unsigned char>(pid_str.front()))) {
        pid_str.erase(pid_str.begin());
    }

    int pid = 0;
    try {
        pid = std::stoi(pid_str);
    } catch (...) {
        return std::nullopt;
    }

    // Extract process name between the first '(' and last ')'
    std::string comm = line.substr(first_open + 1, last_close - first_open - 1);

    // Remaining fields follow last_close
    std::string rest = line.substr(last_close + 1);
    std::istringstream iss(rest);

    ProcessInfo info{};
    info.pid = pid;
    info.name = comm;

    // Field 1 (after paren): state char
    if (!(iss >> info.state)) return std::nullopt;
    info.state_desc = state_char_to_description(info.state);

    // Field 2: ppid
    if (!(iss >> info.ppid)) return std::nullopt;

    // We need utime (field 12 after paren, 0-indexed: index 11),
    // stime (field 13 after paren, index 12),
    // vsize (field 21 after paren, index 20),
    // rss (field 22 after paren, index 21)
    //
    // Let's read all subsequent fields into a vector of strings
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok) {
        tokens.push_back(tok);
    }

    // Minimum fields needed: index 0 was state, index 1 was ppid,
    // utime is tokens[9], stime is tokens[10], vsize is tokens[19], rss is tokens[20]
    // tokens contains fields starting from pgrp (which is field 3 in stat, index 2 in rest)
    // token[0] = pgrp
    // token[1] = session
    // token[2] = tty_nr
    // token[3] = tpgid
    // token[4] = flags
    // token[5] = minflt
    // token[6] = cminflt
    // token[7] = majflt
    // token[8] = cmajflt
    // token[9] = utime
    // token[10] = stime
    // ...
    // token[19] = vsize (in bytes)
    // token[20] = rss (in pages)

    if (tokens.size() > 10) {
        try {
            info.utime_ticks = std::stoull(tokens[9]);
            info.stime_ticks = std::stoull(tokens[10]);
        } catch (...) {
            info.utime_ticks = 0;
            info.stime_ticks = 0;
        }
    }

    if (tokens.size() > 20) {
        try {
            uint64_t vsize_bytes = std::stoull(tokens[19]);
            info.vmsize_kb = vsize_bytes / 1024;

            long page_size = ::sysconf(_SC_PAGESIZE);
            if (page_size <= 0) page_size = 4096;
            uint64_t rss_pages = std::stoull(tokens[20]);
            info.vmrss_kb = (rss_pages * static_cast<uint64_t>(page_size)) / 1024;
        } catch (...) {
            // Keep default 0
        }
    }

    return info;
}

void ProcessMonitor::parse_status_stream(std::istream& is, ProcessInfo& info) {
    std::string line;
    while (std::getline(is, line)) {
        if (line.rfind("VmSize:", 0) == 0) {
            std::istringstream iss(line.substr(7));
            uint64_t kb = 0;
            if (iss >> kb) info.vmsize_kb = kb;
        } else if (line.rfind("VmRSS:", 0) == 0) {
            std::istringstream iss(line.substr(6));
            uint64_t kb = 0;
            if (iss >> kb) info.vmrss_kb = kb;
        }
    }
}

std::vector<ProcessInfo> ProcessMonitor::enumerate_processes() {
    std::vector<ProcessInfo> processes;
    std::error_code ec;

    if (!fs::exists(proc_dir_, ec) || !fs::is_directory(proc_dir_, ec)) {
        return processes;
    }

    auto now = std::chrono::steady_clock::now();
    double elapsed_sec = std::chrono::duration<double>(now - last_sample_time_).count();
    bool can_calc_cpu = (elapsed_sec > 0.05);

    std::unordered_map<int, ProcessSample> new_samples;

    // Iterate over directory entries in /proc
    fs::directory_iterator dir_it(proc_dir_, ec);
    if (ec) return processes;

    for (const auto& entry : dir_it) {
        // Safe iteration: entry might vanish during iteration
        std::error_code entry_ec;
        if (!entry.is_directory(entry_ec)) continue;

        std::string filename = entry.path().filename().string();
        if (filename.empty() || !std::all_of(filename.begin(), filename.end(), ::isdigit)) {
            continue; // Not a PID directory
        }

        int pid = 0;
        try {
            pid = std::stoi(filename);
        } catch (...) {
            continue;
        }

        // Read /proc/[pid]/stat
        std::string stat_path = entry.path() / "stat";
        std::ifstream stat_file(stat_path);
        if (!stat_file.is_open()) {
            // Process terminated or inaccessible: safely skip
            continue;
        }

        std::string stat_line;
        if (!std::getline(stat_file, stat_line)) {
            continue;
        }
        stat_file.close();

        auto opt_info = parse_stat_line(stat_line, clk_tck_);
        if (!opt_info) continue;

        ProcessInfo info = *opt_info;

        // Try reading /proc/[pid]/status for precise VmSize and VmRSS
        std::string status_path = entry.path() / "status";
        std::ifstream status_file(status_path);
        if (status_file.is_open()) {
            parse_status_stream(status_file, info);
        }

        // Calculate per-process CPU utilization
        uint64_t total_ticks = info.utime_ticks + info.stime_ticks;
        new_samples[pid] = ProcessSample{total_ticks, now};

        if (can_calc_cpu) {
            auto prev_it = prev_samples_.find(pid);
            if (prev_it != prev_samples_.end()) {
                if (total_ticks >= prev_it->second.total_ticks) {
                    uint64_t tick_delta = total_ticks - prev_it->second.total_ticks;
                    double ticks_per_sec = static_cast<double>(tick_delta) / elapsed_sec;
                    // cpu_percent represents percentage of 1 logical CPU core capacity
                    info.cpu_percent = (ticks_per_sec / static_cast<double>(clk_tck_)) * 100.0;
                    if (info.cpu_percent < 0.0) info.cpu_percent = 0.0;
                }
            }
        }

        processes.push_back(info);
    }

    if (can_calc_cpu) {
        prev_samples_ = std::move(new_samples);
        last_sample_time_ = now;
    }

    return processes;
}

void ProcessMonitor::reset_cpu_cache() noexcept {
    prev_samples_.clear();
    last_sample_time_ = std::chrono::steady_clock::now();
}

} // namespace sysmon

#include "system_monitor.hpp"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace sysmon {

std::string UptimeStats::to_formatted_string() const {
    if (!is_valid) return "Unavailable";

    uint64_t total_sec = static_cast<uint64_t>(std::floor(uptime_seconds));
    uint64_t days = total_sec / 86400;
    uint64_t remainder = total_sec % 86400;
    uint64_t hours = remainder / 3600;
    remainder %= 3600;
    uint64_t minutes = remainder / 60;
    uint64_t seconds = remainder % 60;

    std::ostringstream oss;
    if (days > 0) {
        oss << days << "d ";
    }
    oss << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;
    return oss.str();
}

SystemMonitor::SystemMonitor() : SystemMonitor("/proc") {}

SystemMonitor::SystemMonitor(std::string proc_dir)
    : proc_dir_(std::move(proc_dir)),
      cpu_monitor_(proc_dir_ + "/stat"),
      memory_monitor_(proc_dir_ + "/meminfo"),
      process_monitor_(proc_dir_) {}

UptimeStats SystemMonitor::parse_proc_uptime(std::istream& is) {
    UptimeStats stats{};
    if (is >> stats.uptime_seconds) {
        is >> stats.idle_seconds;
        stats.is_valid = true;
    }
    return stats;
}

UptimeStats SystemMonitor::parse_uptime_string(const std::string& line) {
    std::istringstream iss(line);
    return parse_proc_uptime(iss);
}

UptimeStats SystemMonitor::read_uptime() {
    std::string uptime_path = proc_dir_ + "/uptime";
    std::ifstream file(uptime_path);
    if (!file.is_open()) {
        return UptimeStats{0.0, 0.0, false};
    }
    return parse_proc_uptime(file);
}

SystemSnapshot SystemMonitor::take_snapshot(int sample_delay_ms) {
    SystemSnapshot snap{};
    snap.uptime = read_uptime();
    snap.memory = memory_monitor_.sample();

    // Sample CPU twice if we do not already have two samples
    cpu_monitor_.sample();
    std::this_thread::sleep_for(std::chrono::milliseconds(sample_delay_ms));
    cpu_monitor_.sample();

    if (cpu_monitor_.has_utilization()) {
        snap.cpu = cpu_monitor_.get_utilization();
    }

    auto procs = process_monitor_.enumerate_processes();
    snap.total_processes = procs.size();

    return snap;
}

void SystemMonitor::print_system_info() {
    auto snap = take_snapshot(300);

    std::cout << "\n=======================================================\n";
    std::cout << "         LINUX SYSTEM RESOURCE MONITOR SNAPSHOT        \n";
    std::cout << "=======================================================\n";

    // Uptime
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "System Uptime      : " << snap.uptime.to_formatted_string()
              << " (" << snap.uptime.uptime_seconds << " seconds)\n";
    std::cout << "Active Processes   : " << snap.total_processes << " enumerated\n";

    // CPU Metrics
    std::cout << "\n--- CPU Metrics (/proc/stat) ---\n";
    if (snap.cpu.is_valid) {
        std::cout << "CPU Utilization    : " << snap.cpu.total_utilization_percent << " %\n";
        std::cout << "  User Space       : " << snap.cpu.user_percent << " %\n";
        std::cout << "  Kernel / System  : " << snap.cpu.system_percent << " %\n";
        std::cout << "  Idle Time        : " << snap.cpu.idle_percent << " %\n";
        std::cout << "  I/O Wait         : " << snap.cpu.iowait_percent << " %\n";
        std::cout << "  [Note: Idle includes idle + iowait as per accounting convention]\n";
    } else {
        std::cout << "CPU Utilization    : [Calculating... insufficient samples]\n";
    }

    // Memory Metrics
    std::cout << "\n--- Memory Metrics (/proc/meminfo) ---\n";
    const auto& mem = snap.memory;
    std::cout << "Total Memory       : " << MemoryStats::kb_to_gib(mem.mem_total_kb) << " GiB ("
              << MemoryStats::kb_to_mib(mem.mem_total_kb) << " MiB)\n";
    std::cout << "Available Memory   : " << MemoryStats::kb_to_gib(mem.mem_available_kb) << " GiB ("
              << MemoryStats::kb_to_mib(mem.mem_available_kb) << " MiB)"
              << (mem.has_mem_available ? "" : " [Estimated from Free+Buffers+Cached]") << "\n";
    std::cout << "Used Memory        : " << MemoryStats::kb_to_gib(mem.used_memory_kb()) << " GiB ("
              << MemoryStats::kb_to_mib(mem.used_memory_kb()) << " MiB)\n";
    std::cout << "Memory Utilization : " << mem.memory_utilization_percent() << " %\n";
    std::cout << "Free Memory (Raw)  : " << MemoryStats::kb_to_mib(mem.mem_free_kb) << " MiB\n";
    std::cout << "Buffers            : " << MemoryStats::kb_to_mib(mem.buffers_kb) << " MiB\n";
    std::cout << "Cached Page Cache  : " << MemoryStats::kb_to_mib(mem.cached_kb) << " MiB\n";

    if (mem.swap_total_kb > 0) {
        std::cout << "Swap Total         : " << MemoryStats::kb_to_mib(mem.swap_total_kb) << " MiB\n";
        std::cout << "Swap Free          : " << MemoryStats::kb_to_mib(mem.swap_free_kb) << " MiB\n";
        std::cout << "Swap Utilization   : " << mem.swap_utilization_percent() << " %\n";
    } else {
        std::cout << "Swap               : Not configured / 0 MiB\n";
    }
    std::cout << "=======================================================\n\n";
}

void SystemMonitor::print_processes(size_t limit) {
    // Prime the CPU sample
    process_monitor_.enumerate_processes();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto procs = process_monitor_.enumerate_processes();

    std::cout << "\n=========================================================================================\n";
    std::cout << "                                ACTIVE LINUX PROCESSES                                  \n";
    std::cout << "=========================================================================================\n";
    std::cout << std::left
              << std::setw(8)  << "PID"
              << std::setw(24) << "COMMAND"
              << std::setw(6)  << "STATE"
              << std::setw(8)  << "PPID"
              << std::setw(14) << "VM-SIZE (MiB)"
              << std::setw(14) << "VM-RSS (MiB)"
              << std::setw(10) << "CPU %"
              << "\n";
    std::cout << "-----------------------------------------------------------------------------------------\n";

    size_t count = 0;
    for (const auto& p : procs) {
        if (limit > 0 && count >= limit) break;

        std::string comm_display = p.name;
        if (comm_display.length() > 22) {
            comm_display = comm_display.substr(0, 19) + "...";
        }

        double vmsize_mib = MemoryStats::kb_to_mib(p.vmsize_kb);
        double vmrss_mib = MemoryStats::kb_to_mib(p.vmrss_kb);

        std::cout << std::left
                  << std::setw(8)  << p.pid
                  << std::setw(24) << comm_display
                  << std::setw(6)  << p.state
                  << std::setw(8)  << p.ppid
                  << std::setw(14) << std::fixed << std::setprecision(1) << vmsize_mib
                  << std::setw(14) << std::fixed << std::setprecision(1) << vmrss_mib
                  << std::setw(10) << std::fixed << std::setprecision(1) << p.cpu_percent
                  << "\n";
        count++;
    }

    std::cout << "-----------------------------------------------------------------------------------------\n";
    std::cout << "Total processes displayed: " << count << " (out of " << procs.size() << " accessible)\n";
    std::cout << "States: R=Running, S=Sleep, D=Disk Sleep, T=Stopped, Z=Zombie, I=Idle\n";
    std::cout << "=========================================================================================\n\n";
}

void SystemMonitor::run_continuous_monitor(int interval_seconds, const std::atomic<bool>& stop_flag) {
    if (interval_seconds <= 0) {
        interval_seconds = 1;
    }

    std::cout << "Starting continuous monitor (Refresh interval: " << interval_seconds
              << "s). Press Ctrl+C to exit cleanly.\n";

    // Take initial CPU sample
    cpu_monitor_.sample();

    while (!stop_flag.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
        if (stop_flag.load()) break;

        cpu_monitor_.sample();
        auto uptime = read_uptime();
        auto mem = memory_monitor_.sample();

        std::cout << "\033[2J\033[H"; // Clear screen ANSI
        std::cout << "=======================================================\n";
        std::cout << "          LIVE SYSTEM RESOURCE MONITOR (Ctrl+C to exit)\n";
        std::cout << "=======================================================\n";
        std::cout << "Uptime      : " << uptime.to_formatted_string() << "\n";

        if (cpu_monitor_.has_utilization()) {
            try {
                auto cpu = cpu_monitor_.get_utilization();
                std::cout << std::fixed << std::setprecision(2);
                std::cout << "CPU Total   : " << cpu.total_utilization_percent << " %"
                          << " [usr: " << cpu.user_percent << "% | sys: " << cpu.system_percent
                          << "% | idle: " << cpu.idle_percent << "% | io: " << cpu.iowait_percent << "%]\n";
            } catch (const std::exception& e) {
                std::cout << "CPU Total   : [Error: " << e.what() << "]\n";
            }
        }

        std::cout << "Memory Used : " << MemoryStats::kb_to_gib(mem.used_memory_kb()) << " / "
                  << MemoryStats::kb_to_gib(mem.mem_total_kb) << " GiB ("
                  << std::fixed << std::setprecision(1) << mem.memory_utilization_percent() << " %)\n";
        std::cout << "Available   : " << MemoryStats::kb_to_gib(mem.mem_available_kb) << " GiB\n";
        if (mem.swap_total_kb > 0) {
            std::cout << "Swap Used   : " << MemoryStats::kb_to_gib(mem.used_swap_kb()) << " / "
                      << MemoryStats::kb_to_gib(mem.swap_total_kb) << " GiB ("
                      << std::fixed << std::setprecision(1) << mem.swap_utilization_percent() << " %)\n";
        }
        std::cout << "=======================================================\n";
        std::cout.flush();
    }

    std::cout << "\nContinuous monitor stopped cleanly.\n";
}

} // namespace sysmon

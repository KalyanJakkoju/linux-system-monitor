#include "cli.hpp"
#include "process_model.hpp"
#include "scheduler.hpp"
#include "system_monitor.hpp"
#include <algorithm>
#include <atomic>
#include <csignal>
#include <iostream>

namespace {
std::atomic<bool> g_stop_requested{false};

void handle_signal(int /*sig*/) {
    g_stop_requested.store(true);
}

std::string to_lower_str(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}
} // anonymous namespace

int main(int argc, char* argv[]) {
    // Register signal handlers for graceful shutdown on Ctrl+C (SIGINT) and SIGTERM
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    auto options = cli::CommandLineParser::parse(argc, argv);

    if (!options.error_message.empty()) {
        std::cerr << "Error: " << options.error_message << "\n\n";
        cli::CommandLineParser::print_usage(argv[0]);
        return 1;
    }

    if (options.show_help || options.command == cli::CommandLineOptions::Command::Help) {
        cli::CommandLineParser::print_usage(argv[0]);
        return 0;
    }

    try {
        if (options.command == cli::CommandLineOptions::Command::Info) {
            sysmon::SystemMonitor monitor;
            monitor.print_system_info();
            return 0;
        }

        if (options.command == cli::CommandLineOptions::Command::Processes) {
            sysmon::SystemMonitor monitor;
            monitor.print_processes(100);
            return 0;
        }

        if (options.command == cli::CommandLineOptions::Command::Monitor) {
            sysmon::SystemMonitor monitor;
            monitor.run_continuous_monitor(options.interval_seconds, g_stop_requested);
            return 0;
        }

        if (options.command == cli::CommandLineOptions::Command::Schedule) {
            auto workload = simulator::load_workload_from_file(options.input_file);
            std::cout << "Loaded workload containing " << workload.size() << " processes from: "
                      << options.input_file << "\n";

            if (options.schedule_all) {
                std::vector<simulator::ScheduleResult> results;
                results.push_back(simulator::CpuScheduler::schedule_fcfs(workload));
                results.push_back(simulator::CpuScheduler::schedule_sjf(workload));
                results.push_back(simulator::CpuScheduler::schedule_round_robin(workload, options.quantum));
                results.push_back(simulator::CpuScheduler::schedule_priority(workload));

                for (const auto& res : results) {
                    std::cout << simulator::CpuScheduler::render_metrics_table(res);
                }

                std::cout << simulator::CpuScheduler::render_comparison_table(results);
                return 0;
            }

            std::string algo = to_lower_str(options.algorithm);
            simulator::ScheduleResult result;

            if (algo == "fcfs") {
                result = simulator::CpuScheduler::schedule_fcfs(workload);
            } else if (algo == "sjf") {
                result = simulator::CpuScheduler::schedule_sjf(workload);
            } else if (algo == "rr" || algo == "roundrobin") {
                result = simulator::CpuScheduler::schedule_round_robin(workload, options.quantum);
            } else if (algo == "priority") {
                result = simulator::CpuScheduler::schedule_priority(workload);
            } else {
                std::cerr << "Error: Unknown scheduling algorithm '" << options.algorithm
                          << "'. Supported algorithms: fcfs, sjf, rr, priority\n";
                return 1;
            }

            std::cout << simulator::CpuScheduler::render_metrics_table(result);
            return 0;
        }

    } catch (const sysmon::SystemMonitorError& e) {
        std::cerr << "System Monitor Error [" << sysmon::error_code_to_string(e.code()) << "]: "
                  << e.what() << "\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "Application Error: " << e.what() << "\n";
        return 3;
    }

    return 0;
}

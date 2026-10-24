#include "cli.hpp"
#include <iostream>
#include <string>

namespace cli {

void CommandLineParser::print_usage(const char* program_name) {
    std::cout << "Linux System Resource Monitor and CPU Scheduling Simulator\n";
    std::cout << "Usage:\n";
    std::cout << "  " << program_name << " info                                  Display a snapshot of Linux CPU, memory, and uptime\n";
    std::cout << "  " << program_name << " monitor [--interval <sec>]             Run real-time resource monitor continuously (Ctrl+C to stop)\n";
    std::cout << "  " << program_name << " processes                              Display active Linux processes with CPU and memory usage\n";
    std::cout << "  " << program_name << " schedule --algorithm <algo> --input <file> [--quantum <q>]\n";
    std::cout << "                                                Run a simulated CPU scheduling algorithm (fcfs, sjf, rr, priority)\n";
    std::cout << "  " << program_name << " schedule --all --input <file> [--quantum <q>]\n";
    std::cout << "                                                Run and compare all four CPU scheduling algorithms on a workload\n";
    std::cout << "  " << program_name << " --help, -h                             Display this help message and exit\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " info\n";
    std::cout << "  " << program_name << " monitor --interval 2\n";
    std::cout << "  " << program_name << " processes\n";
    std::cout << "  " << program_name << " schedule --algorithm fcfs --input examples/processes.csv\n";
    std::cout << "  " << program_name << " schedule --algorithm rr --quantum 2 --input examples/processes.csv\n";
    std::cout << "  " << program_name << " schedule --all --input examples/processes.csv\n";
}

CommandLineOptions CommandLineParser::parse(int argc, char* argv[]) {
    CommandLineOptions options;

    if (argc < 2) {
        options.show_help = true;
        return options;
    }

    std::string first_arg = argv[1];
    if (first_arg == "--help" || first_arg == "-h" || first_arg == "help") {
        options.command = CommandLineOptions::Command::Help;
        options.show_help = true;
        return options;
    }

    if (first_arg == "info") {
        options.command = CommandLineOptions::Command::Info;
        return options;
    }

    if (first_arg == "processes") {
        options.command = CommandLineOptions::Command::Processes;
        return options;
    }

    if (first_arg == "monitor") {
        options.command = CommandLineOptions::Command::Monitor;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--interval" || arg == "-i") {
                if (i + 1 < argc) {
                    try {
                        options.interval_seconds = std::stoi(argv[++i]);
                    } catch (...) {
                        options.error_message = "Invalid number for --interval: " + std::string(argv[i]);
                        return options;
                    }
                    if (options.interval_seconds <= 0) {
                        options.error_message = "Refresh interval must be greater than zero.";
                        return options;
                    }
                } else {
                    options.error_message = "--interval requires an argument.";
                    return options;
                }
            } else {
                options.error_message = "Unknown option for monitor: " + arg;
                return options;
            }
        }
        return options;
    }

    if (first_arg == "schedule") {
        options.command = CommandLineOptions::Command::Schedule;
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--algorithm" || arg == "-a") {
                if (i + 1 < argc) {
                    options.algorithm = argv[++i];
                } else {
                    options.error_message = "--algorithm requires an argument (fcfs, sjf, rr, priority).";
                    return options;
                }
            } else if (arg == "--all") {
                options.schedule_all = true;
            } else if (arg == "--quantum" || arg == "-q") {
                if (i + 1 < argc) {
                    try {
                        options.quantum = std::stoi(argv[++i]);
                    } catch (...) {
                        options.error_message = "Invalid number for --quantum: " + std::string(argv[i]);
                        return options;
                    }
                    if (options.quantum <= 0) {
                        options.error_message = "Time quantum must be greater than zero.";
                        return options;
                    }
                } else {
                    options.error_message = "--quantum requires an argument.";
                    return options;
                }
            } else if (arg == "--input" || arg == "-in") {
                if (i + 1 < argc) {
                    options.input_file = argv[++i];
                } else {
                    options.error_message = "--input requires a CSV file path.";
                    return options;
                }
            } else {
                options.error_message = "Unknown option for schedule: " + arg;
                return options;
            }
        }

        if (options.input_file.empty()) {
            options.error_message = "Schedule command requires --input <file.csv>.";
            return options;
        }

        if (!options.schedule_all && options.algorithm.empty()) {
            options.error_message = "Schedule command requires either --algorithm <name> or --all.";
            return options;
        }

        return options;
    }

    options.error_message = "Unknown command: '" + first_arg + "'. Use --help for usage.";
    return options;
}

} // namespace cli

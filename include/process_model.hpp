#pragma once

#include <iostream>
#include <string>
#include <vector>

namespace simulator {

/**
 * Representation of a synthetic process workload for CPU scheduling simulation.
 * Smaller numeric priority value indicates higher scheduling priority.
 */
struct Process {
    std::string id;
    int arrival_time{0};
    int burst_time{0};
    int priority{0};

    // Simulation metrics
    int completion_time{0};
    int turnaround_time{0};
    int waiting_time{0};
    int response_time{-1};
    int remaining_time{0};
    int first_start_time{-1};

    // Helper to reset calculated metrics before running simulation
    void reset_metrics() noexcept {
        completion_time = 0;
        turnaround_time = 0;
        waiting_time = 0;
        response_time = -1;
        remaining_time = burst_time;
        first_start_time = -1;
    }
};

/**
 * Parse and validate a CSV workload.
 * Expected CSV format:
 *   pid,arrival_time,burst_time,priority
 *
 * Throws std::runtime_error on malformed data with exact line numbers.
 */
std::vector<Process> load_workload_from_csv(std::istream& is);
std::vector<Process> load_workload_from_file(const std::string& filepath);

/**
 * Validate a process workload for duplicates, negative arrival times, or non-positive burst times.
 */
void validate_workload(const std::vector<Process>& processes);

} // namespace simulator

#pragma once

#include "process_model.hpp"
#include <string>
#include <vector>

namespace simulator {

enum class AlgorithmType {
    FCFS,
    SJF,
    RoundRobin,
    Priority
};

inline const char* algorithm_name(AlgorithmType algo) noexcept {
    switch (algo) {
        case AlgorithmType::FCFS: return "First-Come, First-Served (FCFS)";
        case AlgorithmType::SJF: return "Shortest Job First (SJF - Non-Preemptive)";
        case AlgorithmType::RoundRobin: return "Round Robin (RR)";
        case AlgorithmType::Priority: return "Priority Scheduling (Non-Preemptive)";
        default: return "Unknown";
    }
}

struct ExecutionSlice {
    int start_time{0};
    int end_time{0};
    std::string process_id; // Process ID or "IDLE"

    int duration() const noexcept { return end_time - start_time; }
    bool is_idle() const noexcept { return process_id == "IDLE"; }
};

struct ScheduleResult {
    AlgorithmType algorithm{AlgorithmType::FCFS};
    int quantum{0}; // Only applicable for RR
    std::vector<Process> processes;
    std::vector<ExecutionSlice> timeline;

    double avg_turnaround_time{0.0};
    double avg_waiting_time{0.0};
    double avg_response_time{0.0};
    int total_simulation_time{0};
    int total_idle_time{0};
    double cpu_utilization_percent{0.0};
};

class CpuScheduler {
public:
    static ScheduleResult schedule_fcfs(std::vector<Process> workload);
    static ScheduleResult schedule_sjf(std::vector<Process> workload);
    static ScheduleResult schedule_round_robin(std::vector<Process> workload, int quantum);
    static ScheduleResult schedule_priority(std::vector<Process> workload);

    /**
     * Cross-algorithm correctness assertion:
     * - Total execution time equals sum of burst times.
     * - Every process completes exactly once.
     * - No process executes before arrival time.
     * - TAT = CT - AT, WT = TAT - BT, RT = FirstStart - AT.
     * - No overlapping execution slices.
     * Throws std::logic_error if any invariant fails.
     */
    static void verify_schedule_invariants(const ScheduleResult& result);

    /**
     * Formats the execution timeline into visual ASCII Gantt chart and table.
     */
    static std::string render_gantt_chart(const ScheduleResult& result);

    /**
     * Formats detailed per-process metrics table.
     */
    static std::string render_metrics_table(const ScheduleResult& result);

    /**
     * Formats side-by-side comparison table of all 4 algorithms.
     */
    static std::string render_comparison_table(const std::vector<ScheduleResult>& results);

private:
    static void compute_aggregate_metrics(ScheduleResult& result);
};

} // namespace simulator

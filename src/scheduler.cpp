#include "scheduler.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <stdexcept>

namespace simulator {

void CpuScheduler::compute_aggregate_metrics(ScheduleResult& result) {
    if (result.processes.empty()) return;

    double sum_tat = 0.0;
    double sum_wt = 0.0;
    double sum_rt = 0.0;
    int total_burst = 0;

    for (const auto& p : result.processes) {
        sum_tat += p.turnaround_time;
        sum_wt += p.waiting_time;
        sum_rt += p.response_time;
        total_burst += p.burst_time;
    }

    double n = static_cast<double>(result.processes.size());
    result.avg_turnaround_time = sum_tat / n;
    result.avg_waiting_time = sum_wt / n;
    result.avg_response_time = sum_rt / n;

    int total_time = result.timeline.empty() ? 0 : result.timeline.back().end_time;
    result.total_simulation_time = total_time;

    int idle_time = 0;
    for (const auto& slice : result.timeline) {
        if (slice.is_idle()) {
            idle_time += slice.duration();
        }
    }
    result.total_idle_time = idle_time;

    if (total_time > 0) {
        result.cpu_utilization_percent = (100.0 * static_cast<double>(total_burst)) / static_cast<double>(total_time);
    } else {
        result.cpu_utilization_percent = 0.0;
    }
}

ScheduleResult CpuScheduler::schedule_fcfs(std::vector<Process> workload) {
    validate_workload(workload);
    for (auto& p : workload) p.reset_metrics();

    ScheduleResult result;
    result.algorithm = AlgorithmType::FCFS;

    // Stable sort by arrival time. Tie-breaking by original index / PID.
    std::vector<size_t> indices(workload.size());
    for (size_t i = 0; i < indices.size(); ++i) indices[i] = i;

    std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        if (workload[a].arrival_time != workload[b].arrival_time) {
            return workload[a].arrival_time < workload[b].arrival_time;
        }
        return workload[a].id < workload[b].id;
    });

    int current_time = 0;
    for (size_t idx : indices) {
        auto& p = workload[idx];

        // Handle CPU idle gap if current_time < arrival_time
        if (current_time < p.arrival_time) {
            result.timeline.push_back(ExecutionSlice{current_time, p.arrival_time, "IDLE"});
            current_time = p.arrival_time;
        }

        int start_time = current_time;
        p.first_start_time = start_time;
        p.completion_time = start_time + p.burst_time;
        p.turnaround_time = p.completion_time - p.arrival_time;
        p.waiting_time = p.turnaround_time - p.burst_time;
        p.response_time = p.first_start_time - p.arrival_time;

        current_time = p.completion_time;
        result.timeline.push_back(ExecutionSlice{start_time, current_time, p.id});
    }

    result.processes = std::move(workload);
    compute_aggregate_metrics(result);
    verify_schedule_invariants(result);
    return result;
}

ScheduleResult CpuScheduler::schedule_sjf(std::vector<Process> workload) {
    validate_workload(workload);
    for (auto& p : workload) p.reset_metrics();

    ScheduleResult result;
    result.algorithm = AlgorithmType::SJF;

    const size_t n = workload.size();
    std::vector<bool> completed(n, false);
    size_t completed_count = 0;
    int current_time = 0;

    while (completed_count < n) {
        // Find arrived, non-completed processes
        int best_idx = -1;
        for (size_t i = 0; i < n; ++i) {
            if (completed[i]) continue;
            if (workload[i].arrival_time <= current_time) {
                if (best_idx == -1) {
                    best_idx = static_cast<int>(i);
                } else {
                    const auto& best_p = workload[best_idx];
                    const auto& cand_p = workload[i];
                    if (cand_p.burst_time < best_p.burst_time) {
                        best_idx = static_cast<int>(i);
                    } else if (cand_p.burst_time == best_p.burst_time) {
                        // Tie-breaking: earlier arrival, then PID
                        if (cand_p.arrival_time < best_p.arrival_time) {
                            best_idx = static_cast<int>(i);
                        } else if (cand_p.arrival_time == best_p.arrival_time && cand_p.id < best_p.id) {
                            best_idx = static_cast<int>(i);
                        }
                    }
                }
            }
        }

        if (best_idx == -1) {
            // CPU is idle. Find the next earliest arrival time among non-completed processes
            int next_arrival = -1;
            for (size_t i = 0; i < n; ++i) {
                if (!completed[i]) {
                    if (next_arrival == -1 || workload[i].arrival_time < next_arrival) {
                        next_arrival = workload[i].arrival_time;
                    }
                }
            }

            result.timeline.push_back(ExecutionSlice{current_time, next_arrival, "IDLE"});
            current_time = next_arrival;
            continue;
        }

        // Execute best process non-preemptively
        auto& p = workload[best_idx];
        int start_time = current_time;
        p.first_start_time = start_time;
        p.completion_time = start_time + p.burst_time;
        p.turnaround_time = p.completion_time - p.arrival_time;
        p.waiting_time = p.turnaround_time - p.burst_time;
        p.response_time = p.first_start_time - p.arrival_time;

        current_time = p.completion_time;
        result.timeline.push_back(ExecutionSlice{start_time, current_time, p.id});

        completed[best_idx] = true;
        completed_count++;
    }

    result.processes = std::move(workload);
    compute_aggregate_metrics(result);
    verify_schedule_invariants(result);
    return result;
}

ScheduleResult CpuScheduler::schedule_round_robin(std::vector<Process> workload, int quantum) {
    if (quantum <= 0) {
        throw std::runtime_error("Invalid Round Robin quantum (" + std::to_string(quantum) +
                                 "): quantum must be greater than zero.");
    }
    validate_workload(workload);
    for (auto& p : workload) p.reset_metrics();

    ScheduleResult result;
    result.algorithm = AlgorithmType::RoundRobin;
    result.quantum = quantum;

    const size_t n = workload.size();
    std::vector<size_t> arrival_order(n);
    for (size_t i = 0; i < n; ++i) arrival_order[i] = i;

    // Sort initial arrival candidates deterministically
    std::sort(arrival_order.begin(), arrival_order.end(), [&](size_t a, size_t b) {
        if (workload[a].arrival_time != workload[b].arrival_time) {
            return workload[a].arrival_time < workload[b].arrival_time;
        }
        return workload[a].id < workload[b].id;
    });

    std::queue<size_t> ready_queue;
    std::vector<bool> enqueued(n, false);
    size_t completed_count = 0;
    int current_time = 0;

    // Check processes arriving at time 0
    for (size_t idx : arrival_order) {
        if (workload[idx].arrival_time <= current_time) {
            ready_queue.push(idx);
            enqueued[idx] = true;
        }
    }

    while (completed_count < n) {
        if (ready_queue.empty()) {
            // CPU is idle: advance to next arrival
            int next_arrival = -1;
            for (size_t idx : arrival_order) {
                if (!enqueued[idx]) {
                    if (next_arrival == -1 || workload[idx].arrival_time < next_arrival) {
                        next_arrival = workload[idx].arrival_time;
                    }
                }
            }

            if (next_arrival > current_time) {
                result.timeline.push_back(ExecutionSlice{current_time, next_arrival, "IDLE"});
                current_time = next_arrival;
            }

            // Enqueue all newly arrived processes at this time
            for (size_t idx : arrival_order) {
                if (!enqueued[idx] && workload[idx].arrival_time <= current_time) {
                    ready_queue.push(idx);
                    enqueued[idx] = true;
                }
            }
            continue;
        }

        size_t curr_idx = ready_queue.front();
        ready_queue.pop();
        auto& p = workload[curr_idx];

        if (p.first_start_time == -1) {
            p.first_start_time = current_time;
        }

        int slice_duration = std::min(quantum, p.remaining_time);
        int slice_start = current_time;
        int slice_end = slice_start + slice_duration;

        result.timeline.push_back(ExecutionSlice{slice_start, slice_end, p.id});
        p.remaining_time -= slice_duration;
        current_time = slice_end;

        // CRITICAL EVENT ORDERING:
        // 1) First enqueue all processes arriving during the interval (slice_start, slice_end]
        for (size_t idx : arrival_order) {
            if (!enqueued[idx] && workload[idx].arrival_time <= current_time) {
                ready_queue.push(idx);
                enqueued[idx] = true;
            }
        }

        // 2) Then, if the current process is still not finished, re-enqueue it at the back
        if (p.remaining_time > 0) {
            ready_queue.push(curr_idx);
        } else {
            p.completion_time = current_time;
            p.turnaround_time = p.completion_time - p.arrival_time;
            p.waiting_time = p.turnaround_time - p.burst_time;
            p.response_time = p.first_start_time - p.arrival_time;
            completed_count++;
        }
    }

    result.processes = std::move(workload);
    compute_aggregate_metrics(result);
    verify_schedule_invariants(result);
    return result;
}

ScheduleResult CpuScheduler::schedule_priority(std::vector<Process> workload) {
    validate_workload(workload);
    for (auto& p : workload) p.reset_metrics();

    ScheduleResult result;
    result.algorithm = AlgorithmType::Priority;

    const size_t n = workload.size();
    std::vector<bool> completed(n, false);
    size_t completed_count = 0;
    int current_time = 0;

    while (completed_count < n) {
        // Find arrived, non-completed process with highest priority (lowest numeric value)
        int best_idx = -1;
        for (size_t i = 0; i < n; ++i) {
            if (completed[i]) continue;
            if (workload[i].arrival_time <= current_time) {
                if (best_idx == -1) {
                    best_idx = static_cast<int>(i);
                } else {
                    const auto& best_p = workload[best_idx];
                    const auto& cand_p = workload[i];
                    if (cand_p.priority < best_p.priority) {
                        best_idx = static_cast<int>(i);
                    } else if (cand_p.priority == best_p.priority) {
                        // Tie-breaking: earlier arrival, then PID
                        if (cand_p.arrival_time < best_p.arrival_time) {
                            best_idx = static_cast<int>(i);
                        } else if (cand_p.arrival_time == best_p.arrival_time && cand_p.id < best_p.id) {
                            best_idx = static_cast<int>(i);
                        }
                    }
                }
            }
        }

        if (best_idx == -1) {
            // CPU idle: advance to next arrival
            int next_arrival = -1;
            for (size_t i = 0; i < n; ++i) {
                if (!completed[i]) {
                    if (next_arrival == -1 || workload[i].arrival_time < next_arrival) {
                        next_arrival = workload[i].arrival_time;
                    }
                }
            }

            result.timeline.push_back(ExecutionSlice{current_time, next_arrival, "IDLE"});
            current_time = next_arrival;
            continue;
        }

        auto& p = workload[best_idx];
        int start_time = current_time;
        p.first_start_time = start_time;
        p.completion_time = start_time + p.burst_time;
        p.turnaround_time = p.completion_time - p.arrival_time;
        p.waiting_time = p.turnaround_time - p.burst_time;
        p.response_time = p.first_start_time - p.arrival_time;

        current_time = p.completion_time;
        result.timeline.push_back(ExecutionSlice{start_time, current_time, p.id});

        completed[best_idx] = true;
        completed_count++;
    }

    result.processes = std::move(workload);
    compute_aggregate_metrics(result);
    verify_schedule_invariants(result);
    return result;
}

void CpuScheduler::verify_schedule_invariants(const ScheduleResult& result) {
    if (result.processes.empty()) {
        throw std::logic_error("Invariant violation: Empty process list in schedule result.");
    }

    // 1. Verify timeline continuity and no overlapping intervals
    int prev_end = 0;
    for (size_t i = 0; i < result.timeline.size(); ++i) {
        const auto& slice = result.timeline[i];
        if (slice.start_time >= slice.end_time) {
            throw std::logic_error("Invariant violation: Invalid slice duration at index " +
                                   std::to_string(i) + " [" + std::to_string(slice.start_time) +
                                   " -> " + std::to_string(slice.end_time) + "]");
        }
        if (i == 0) {
            if (slice.start_time != 0) {
                throw std::logic_error("Invariant violation: First slice does not start at time 0 (starts at " +
                                       std::to_string(slice.start_time) + ").");
            }
        } else {
            if (slice.start_time != prev_end) {
                throw std::logic_error("Invariant violation: Timeline gap or overlap between " +
                                       std::to_string(prev_end) + " and " + std::to_string(slice.start_time));
            }
        }
        prev_end = slice.end_time;
    }

    // 2. Verify total slice execution for each process equals burst time
    for (const auto& p : result.processes) {
        int total_slice_time = 0;
        int first_seen_start = -1;
        int last_seen_end = -1;

        for (const auto& slice : result.timeline) {
            if (slice.process_id == p.id) {
                total_slice_time += slice.duration();
                if (first_seen_start == -1) first_seen_start = slice.start_time;
                last_seen_end = slice.end_time;
            }
        }

        if (total_slice_time != p.burst_time) {
            throw std::logic_error("Invariant violation for process " + p.id +
                                   ": Total execution slice time (" + std::to_string(total_slice_time) +
                                   ") does not equal burst time (" + std::to_string(p.burst_time) + ")");
        }

        if (first_seen_start < p.arrival_time) {
            throw std::logic_error("Invariant violation for process " + p.id +
                                   ": Executed at " + std::to_string(first_seen_start) +
                                   " before arrival time " + std::to_string(p.arrival_time));
        }

        if (first_seen_start != p.first_start_time) {
            throw std::logic_error("Invariant violation for process " + p.id +
                                   ": Recorded first_start_time (" + std::to_string(p.first_start_time) +
                                   ") differs from first timeline slice (" + std::to_string(first_seen_start) + ")");
        }

        if (last_seen_end != p.completion_time) {
            throw std::logic_error("Invariant violation for process " + p.id +
                                   ": Recorded completion_time (" + std::to_string(p.completion_time) +
                                   ") differs from last timeline slice (" + std::to_string(last_seen_end) + ")");
        }

        if (p.waiting_time < 0) {
            throw std::logic_error("Invariant violation for process " + p.id +
                                   ": Negative waiting time (" + std::to_string(p.waiting_time) + ")");
        }

        if (p.turnaround_time != (p.completion_time - p.arrival_time)) {
            throw std::logic_error("Invariant violation for process " + p.id +
                                   ": TAT != CT - AT (" + std::to_string(p.turnaround_time) +
                                   " vs " + std::to_string(p.completion_time - p.arrival_time) + ")");
        }

        if (p.waiting_time != (p.turnaround_time - p.burst_time)) {
            throw std::logic_error("Invariant violation for process " + p.id +
                                   ": WT != TAT - BT (" + std::to_string(p.waiting_time) +
                                   " vs " + std::to_string(p.turnaround_time - p.burst_time) + ")");
        }

        if (p.response_time != (p.first_start_time - p.arrival_time)) {
            throw std::logic_error("Invariant violation for process " + p.id +
                                   ": RT != FirstStart - AT (" + std::to_string(p.response_time) +
                                   " vs " + std::to_string(p.first_start_time - p.arrival_time) + ")");
        }

        if (p.response_time > p.turnaround_time) {
            throw std::logic_error("Invariant violation for process " + p.id +
                                   ": RT (" + std::to_string(p.response_time) +
                                   ") exceeds TAT (" + std::to_string(p.turnaround_time) + ")");
        }
    }
}

std::string CpuScheduler::render_gantt_chart(const ScheduleResult& result) {
    std::ostringstream oss;
    oss << "\n--- GANTT CHART ---\n";

    // Textual interval stream
    oss << "Execution Sequence:\n";
    for (size_t i = 0; i < result.timeline.size(); ++i) {
        const auto& s = result.timeline[i];
        oss << "[" << s.start_time << " -> " << s.end_time << ": " << s.process_id << "]";
        if (i + 1 < result.timeline.size()) oss << " ";
    }
    oss << "\n\nVisual Diagram:\n";

    // Visual ASCII bar chart
    // Top border
    for (const auto& s : result.timeline) {
        int width = std::max(6, s.duration() * 2);
        oss << "+";
        for (int w = 0; w < width; ++w) oss << "-";
    }
    oss << "+\n";

    // Process row
    for (const auto& s : result.timeline) {
        int width = std::max(6, s.duration() * 2);
        std::string label = s.process_id;
        int pad_left = (width - static_cast<int>(label.length())) / 2;
        int pad_right = width - static_cast<int>(label.length()) - pad_left;

        oss << "|";
        for (int w = 0; w < pad_left; ++w) oss << " ";
        oss << label;
        for (int w = 0; w < pad_right; ++w) oss << " ";
    }
    oss << "|\n";

    // Bottom border
    for (const auto& s : result.timeline) {
        int width = std::max(6, s.duration() * 2);
        oss << "+";
        for (int w = 0; w < width; ++w) oss << "-";
    }
    oss << "+\n";

    // Time points
    for (size_t i = 0; i < result.timeline.size(); ++i) {
        const auto& s = result.timeline[i];
        int width = std::max(6, s.duration() * 2);
        std::string t_str = std::to_string(s.start_time);
        oss << t_str;
        int spaces = width + 1 - static_cast<int>(t_str.length());
        for (int sp = 0; sp < spaces; ++sp) oss << " ";
    }
    if (!result.timeline.empty()) {
        oss << result.timeline.back().end_time;
    }
    oss << "\n";

    return oss.str();
}

std::string CpuScheduler::render_metrics_table(const ScheduleResult& result) {
    std::ostringstream oss;
    oss << "\n=========================================================================================\n";
    oss << " " << algorithm_name(result.algorithm);
    if (result.algorithm == AlgorithmType::RoundRobin) {
        oss << " (Quantum = " << result.quantum << ")";
    }
    oss << "\n=========================================================================================\n";

    oss << std::left
        << std::setw(8)  << "PID"
        << std::setw(10) << "ARRIVAL"
        << std::setw(8)  << "BURST"
        << std::setw(10) << "PRIORITY"
        << std::setw(12) << "COMPLETION"
        << std::setw(12) << "TURNAROUND"
        << std::setw(10) << "WAITING"
        << std::setw(10) << "RESPONSE"
        << "\n";
    oss << "-----------------------------------------------------------------------------------------\n";

    for (const auto& p : result.processes) {
        oss << std::left
            << std::setw(8)  << p.id
            << std::setw(10) << p.arrival_time
            << std::setw(8)  << p.burst_time
            << std::setw(10) << p.priority
            << std::setw(12) << p.completion_time
            << std::setw(12) << p.turnaround_time
            << std::setw(10) << p.waiting_time
            << std::setw(10) << p.response_time
            << "\n";
    }

    oss << "-----------------------------------------------------------------------------------------\n";
    oss << std::fixed << std::setprecision(2);
    oss << "Average Turnaround Time : " << result.avg_turnaround_time << "\n";
    oss << "Average Waiting Time    : " << result.avg_waiting_time << "\n";
    oss << "Average Response Time   : " << result.avg_response_time << "\n";
    oss << "Total Simulated Time    : " << result.total_simulation_time << "\n";
    oss << "Total CPU Idle Time     : " << result.total_idle_time << "\n";
    oss << "Simulated CPU Util %    : " << result.cpu_utilization_percent << " %\n";
    oss << "=========================================================================================\n";

    oss << render_gantt_chart(result);
    return oss.str();
}

std::string CpuScheduler::render_comparison_table(const std::vector<ScheduleResult>& results) {
    std::ostringstream oss;
    oss << "\n=====================================================================================================================\n";
    oss << "                                       SCHEDULING ALGORITHM COMPARISON SUMMARY                                       \n";
    oss << "=====================================================================================================================\n";

    oss << std::left
        << std::setw(44) << "ALGORITHM"
        << std::setw(12) << "TOTAL TIME"
        << std::setw(12) << "IDLE TIME"
        << std::setw(14) << "CPU UTIL (%)"
        << std::setw(14) << "AVG WAITING"
        << std::setw(16) << "AVG TURNAROUND"
        << std::setw(14) << "AVG RESPONSE"
        << "\n";
    oss << "---------------------------------------------------------------------------------------------------------------------\n";

    for (const auto& r : results) {
        std::string name = algorithm_name(r.algorithm);
        if (r.algorithm == AlgorithmType::RoundRobin) {
            name += " (q=" + std::to_string(r.quantum) + ")";
        }

        oss << std::left
            << std::setw(44) << name
            << std::setw(12) << r.total_simulation_time
            << std::setw(12) << r.total_idle_time
            << std::setw(14) << std::fixed << std::setprecision(2) << r.cpu_utilization_percent
            << std::setw(14) << std::fixed << std::setprecision(2) << r.avg_waiting_time
            << std::setw(16) << std::fixed << std::setprecision(2) << r.avg_turnaround_time
            << std::setw(14) << std::fixed << std::setprecision(2) << r.avg_response_time
            << "\n";
    }

    oss << "=====================================================================================================================\n\n";
    return oss.str();
}

} // namespace simulator

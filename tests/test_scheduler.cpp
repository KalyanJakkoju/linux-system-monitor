#include "test_framework.hpp"
#include "scheduler.hpp"
#include <sstream>

using namespace simulator;

TEST(Scheduler, FCFSExactMetrics) {
    std::vector<Process> workload = {
        {"P1", 0, 4, 1, 0, 0, 0, -1, 4, -1},
        {"P2", 1, 3, 2, 0, 0, 0, -1, 3, -1},
        {"P3", 2, 1, 3, 0, 0, 0, -1, 1, -1}
    };

    auto res = CpuScheduler::schedule_fcfs(workload);

    // Verify P1
    ASSERT_EQ(res.processes[0].id, "P1");
    ASSERT_EQ(res.processes[0].completion_time, 4);
    ASSERT_EQ(res.processes[0].turnaround_time, 4);
    ASSERT_EQ(res.processes[0].waiting_time, 0);
    ASSERT_EQ(res.processes[0].response_time, 0);

    // Verify P2
    ASSERT_EQ(res.processes[1].id, "P2");
    ASSERT_EQ(res.processes[1].completion_time, 7);
    ASSERT_EQ(res.processes[1].turnaround_time, 6);
    ASSERT_EQ(res.processes[1].waiting_time, 3);
    ASSERT_EQ(res.processes[1].response_time, 3);

    // Verify P3
    ASSERT_EQ(res.processes[2].id, "P3");
    ASSERT_EQ(res.processes[2].completion_time, 8);
    ASSERT_EQ(res.processes[2].turnaround_time, 6);
    ASSERT_EQ(res.processes[2].waiting_time, 5);
    ASSERT_EQ(res.processes[2].response_time, 5);

    // Averages
    ASSERT_NEAR(res.avg_turnaround_time, (4.0 + 6.0 + 6.0) / 3.0, 0.001);
    ASSERT_NEAR(res.avg_waiting_time, (0.0 + 3.0 + 5.0) / 3.0, 0.001);
    ASSERT_NEAR(res.avg_response_time, (0.0 + 3.0 + 5.0) / 3.0, 0.001);
    ASSERT_EQ(res.total_simulation_time, 8);
    ASSERT_EQ(res.total_idle_time, 0);
    ASSERT_NEAR(res.cpu_utilization_percent, 100.0, 0.001);
}

TEST(Scheduler, SJFSelectionAndMetrics) {
    std::vector<Process> workload = {
        {"P1", 0, 5, 1, 0, 0, 0, -1, 5, -1},
        {"P2", 1, 3, 1, 0, 0, 0, -1, 3, -1},
        {"P3", 2, 1, 1, 0, 0, 0, -1, 1, -1},
        {"P4", 3, 2, 1, 0, 0, 0, -1, 2, -1}
    };

    auto res = CpuScheduler::schedule_sjf(workload);

    // At T=0, only P1 arrived -> runs 0..5
    // At T=5, P2(3), P3(1), P4(2) ready -> P3 chosen (5..6)
    // At T=6, P2(3), P4(2) ready -> P4 chosen (6..8)
    // At T=8, P2(3) chosen (8..11)
    ASSERT_EQ(res.processes[0].completion_time, 5);  // P1
    ASSERT_EQ(res.processes[1].completion_time, 11); // P2
    ASSERT_EQ(res.processes[2].completion_time, 6);  // P3
    ASSERT_EQ(res.processes[3].completion_time, 8);  // P4

    ASSERT_EQ(res.processes[2].waiting_time, 3);     // P3: CT(6) - AT(2) = TAT(4) - BT(1) = 3
    ASSERT_EQ(res.processes[3].waiting_time, 3);     // P4: CT(8) - AT(3) = TAT(5) - BT(2) = 3
}

TEST(Scheduler, RoundRobinPreemptionAndEventOrdering) {
    std::vector<Process> workload = {
        {"P1", 0, 5, 1, 0, 0, 0, -1, 5, -1},
        {"P2", 1, 3, 1, 0, 0, 0, -1, 3, -1},
        {"P3", 2, 1, 1, 0, 0, 0, -1, 1, -1}
    };

    int quantum = 2;
    auto res = CpuScheduler::schedule_round_robin(workload, quantum);

    // Exact timeline check:
    // [0->2: P1] [2->4: P2] [4->5: P3] [5->7: P1] [7->8: P2] [8->9: P1]
    ASSERT_EQ(res.timeline.size(), 6ULL);
    ASSERT_EQ(res.timeline[0].process_id, "P1");
    ASSERT_EQ(res.timeline[0].start_time, 0);
    ASSERT_EQ(res.timeline[0].end_time, 2);

    ASSERT_EQ(res.timeline[1].process_id, "P2");
    ASSERT_EQ(res.timeline[1].start_time, 2);
    ASSERT_EQ(res.timeline[1].end_time, 4);

    ASSERT_EQ(res.timeline[2].process_id, "P3");
    ASSERT_EQ(res.timeline[2].start_time, 4);
    ASSERT_EQ(res.timeline[2].end_time, 5);

    ASSERT_EQ(res.timeline[3].process_id, "P1");
    ASSERT_EQ(res.timeline[3].start_time, 5);
    ASSERT_EQ(res.timeline[3].end_time, 7);

    ASSERT_EQ(res.timeline[4].process_id, "P2");
    ASSERT_EQ(res.timeline[4].start_time, 7);
    ASSERT_EQ(res.timeline[4].end_time, 8);

    ASSERT_EQ(res.timeline[5].process_id, "P1");
    ASSERT_EQ(res.timeline[5].start_time, 8);
    ASSERT_EQ(res.timeline[5].end_time, 9);

    // Completion times: P1: 9, P2: 8, P3: 5
    ASSERT_EQ(res.processes[0].completion_time, 9);
    ASSERT_EQ(res.processes[1].completion_time, 8);
    ASSERT_EQ(res.processes[2].completion_time, 5);

    // Response times: P1: 0, P2: 2 - 1 = 1, P3: 4 - 2 = 2
    ASSERT_EQ(res.processes[0].response_time, 0);
    ASSERT_EQ(res.processes[1].response_time, 1);
    ASSERT_EQ(res.processes[2].response_time, 2);
}

TEST(Scheduler, PriorityNonPreemptive) {
    // Smaller numeric priority = higher priority
    std::vector<Process> workload = {
        {"P1", 0, 4, 3, 0, 0, 0, -1, 4, -1},
        {"P2", 1, 3, 1, 0, 0, 0, -1, 3, -1},
        {"P3", 2, 2, 2, 0, 0, 0, -1, 2, -1}
    };

    auto res = CpuScheduler::schedule_priority(workload);

    // P1 runs 0..4 (only one at 0)
    // At 4, P2 (pri 1) and P3 (pri 2) ready -> P2 runs 4..7
    // At 7, P3 runs 7..9
    ASSERT_EQ(res.processes[0].completion_time, 4);
    ASSERT_EQ(res.processes[1].completion_time, 7);
    ASSERT_EQ(res.processes[2].completion_time, 9);
}

TEST(Scheduler, IdleIntervalHandling) {
    // P1 arrives at T=3, so CPU is idle from 0..3
    std::vector<Process> workload = {
        {"P1", 3, 4, 1, 0, 0, 0, -1, 4, -1}
    };

    auto res = CpuScheduler::schedule_fcfs(workload);

    ASSERT_EQ(res.timeline.size(), 2ULL);
    ASSERT_TRUE(res.timeline[0].is_idle());
    ASSERT_EQ(res.timeline[0].start_time, 0);
    ASSERT_EQ(res.timeline[0].end_time, 3);

    ASSERT_EQ(res.timeline[1].process_id, "P1");
    ASSERT_EQ(res.timeline[1].start_time, 3);
    ASSERT_EQ(res.timeline[1].end_time, 7);

    ASSERT_EQ(res.total_idle_time, 3);
    ASSERT_EQ(res.total_simulation_time, 7);
    // CPU utilization = 4 / 7 * 100 ~= 57.14%
    ASSERT_NEAR(res.cpu_utilization_percent, 400.0 / 7.0, 0.01);
}

TEST(Scheduler, CSVParserAndValidation) {
    std::string csv_content =
        "pid,arrival_time,burst_time,priority\n"
        "P1,0,5,2\n"
        "P2,1,3,1\n"
        "P3,2,1,3\n";

    std::istringstream iss(csv_content);
    auto workload = load_workload_from_csv(iss);
    ASSERT_EQ(workload.size(), 3ULL);
    ASSERT_EQ(workload[0].id, "P1");
    ASSERT_EQ(workload[0].arrival_time, 0);
    ASSERT_EQ(workload[0].burst_time, 5);
    ASSERT_EQ(workload[0].priority, 2);

    // Duplicate PID rejection
    std::string dup_csv =
        "pid,arrival_time,burst_time,priority\n"
        "P1,0,5,2\n"
        "P1,2,3,1\n";
    std::istringstream dup_iss(dup_csv);
    ASSERT_THROW(load_workload_from_csv(dup_iss), std::runtime_error);

    // Negative arrival time rejection
    std::string neg_csv =
        "pid,arrival_time,burst_time,priority\n"
        "P1,-1,5,2\n";
    std::istringstream neg_iss(neg_csv);
    ASSERT_THROW(load_workload_from_csv(neg_iss), std::runtime_error);

    // Non-positive burst time rejection
    std::string zero_bt_csv =
        "pid,arrival_time,burst_time,priority\n"
        "P1,0,0,2\n";
    std::istringstream zero_bt_iss(zero_bt_csv);
    ASSERT_THROW(load_workload_from_csv(zero_bt_iss), std::runtime_error);
}

#include "test_framework.hpp"
#include "cpu_monitor.hpp"
#include <sstream>

using namespace sysmon;

TEST(CpuMetrics, ParseValid10FieldLine) {
    std::string line = "cpu  10132153 290696 4084095 86908027 348123 0 119514 0 0 0";
    auto opt = CpuMonitor::parse_cpu_stat_line(line);
    ASSERT_TRUE(opt.has_value());

    const auto& c = *opt;
    ASSERT_EQ(c.user, 10132153ULL);
    ASSERT_EQ(c.nice, 290696ULL);
    ASSERT_EQ(c.system, 4084095ULL);
    ASSERT_EQ(c.idle, 86908027ULL);
    ASSERT_EQ(c.iowait, 348123ULL);
    ASSERT_EQ(c.irq, 0ULL);
    ASSERT_EQ(c.softirq, 119514ULL);
    ASSERT_EQ(c.steal, 0ULL);
    ASSERT_EQ(c.guest, 0ULL);
    ASSERT_EQ(c.guest_nice, 0ULL);

    // Idle must be idle + iowait
    ASSERT_EQ(c.get_idle_time(), 86908027ULL + 348123ULL);
}

TEST(CpuMetrics, ParseOld4FieldLine) {
    std::string line = "cpu  1000 200 300 4000";
    auto opt = CpuMonitor::parse_cpu_stat_line(line);
    ASSERT_TRUE(opt.has_value());

    const auto& c = *opt;
    ASSERT_EQ(c.user, 1000ULL);
    ASSERT_EQ(c.nice, 200ULL);
    ASSERT_EQ(c.system, 300ULL);
    ASSERT_EQ(c.idle, 4000ULL);
    ASSERT_EQ(c.iowait, 0ULL); // Optional fields default to 0
    ASSERT_EQ(c.get_idle_time(), 4000ULL);
    ASSERT_EQ(c.get_total_time(), 5500ULL);
}

TEST(CpuMetrics, IgnorePerCoreLines) {
    // Should ignore cpu0, cpu1, cpuX
    ASSERT_FALSE(CpuMonitor::parse_cpu_stat_line("cpu0 100 200 300 400").has_value());
    ASSERT_FALSE(CpuMonitor::parse_cpu_stat_line("cpu1 100 200 300 400").has_value());
    ASSERT_FALSE(CpuMonitor::parse_cpu_stat_line("intr 12345 6789").has_value());
}

TEST(CpuMetrics, CalculateDeltaUtilization) {
    CpuCounters s1{};
    s1.user = 100;
    s1.nice = 0;
    s1.system = 100;
    s1.idle = 800;
    s1.iowait = 0;
    // Total s1 = 1000, Idle s1 = 800, Busy s1 = 200

    CpuCounters s2{};
    s2.user = 200;   // delta user = 100
    s2.nice = 0;
    s2.system = 150; // delta sys = 50
    s2.idle = 1600;  // delta idle = 800
    s2.iowait = 50;  // delta iowait = 50
    // Total s2 = 2000 (delta total = 1000)
    // Idle s2 = 1650 (delta idle = 850)
    // Busy delta = 150 (15.0%)

    auto util = CpuMonitor::calculate_utilization(s1, s2);
    ASSERT_TRUE(util.is_valid);
    ASSERT_NEAR(util.total_utilization_percent, 15.0, 0.01);
    ASSERT_NEAR(util.user_percent, 10.0, 0.01);
    ASSERT_NEAR(util.system_percent, 5.0, 0.01);
    ASSERT_NEAR(util.idle_percent, 85.0, 0.01);
    ASSERT_NEAR(util.iowait_percent, 5.0, 0.01);
}

TEST(CpuMetrics, ZeroTotalDeltaHandledGracefully) {
    CpuCounters s1{};
    s1.user = 500;
    s1.idle = 500;

    // Identical sample: total_delta == 0
    auto util = CpuMonitor::calculate_utilization(s1, s1);
    ASSERT_FALSE(util.is_valid);
    ASSERT_EQ(util.total_utilization_percent, 0.0);
}

TEST(CpuMetrics, CounterDecreaseThrowsException) {
    CpuCounters s1{};
    s1.user = 1000;
    s1.idle = 5000;

    CpuCounters s2{};
    s2.user = 900; // Counter decreased!
    s2.idle = 5000;

    ASSERT_THROW(CpuMonitor::calculate_utilization(s1, s2), SystemMonitorError);
}

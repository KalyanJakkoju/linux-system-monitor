#include "test_framework.hpp"
#include "memory_monitor.hpp"

using namespace sysmon;

TEST(MemoryMetrics, ParseStandardMeminfo) {
    std::string sample =
        "MemTotal:       16384000 kB\n"
        "MemFree:         4096000 kB\n"
        "MemAvailable:    8192000 kB\n"
        "Buffers:          512000 kB\n"
        "Cached:          2048000 kB\n"
        "SwapTotal:       2097152 kB\n"
        "SwapFree:        1048576 kB\n";

    auto stats = MemoryMonitor::parse_proc_meminfo_string(sample);
    ASSERT_TRUE(stats.has_mem_available);
    ASSERT_EQ(stats.mem_total_kb, 16384000ULL);
    ASSERT_EQ(stats.mem_available_kb, 8192000ULL);
    ASSERT_EQ(stats.mem_free_kb, 4096000ULL);
    ASSERT_EQ(stats.buffers_kb, 512000ULL);
    ASSERT_EQ(stats.cached_kb, 2048000ULL);
    ASSERT_EQ(stats.swap_total_kb, 2097152ULL);
    ASSERT_EQ(stats.swap_free_kb, 1048576ULL);

    // used = MemTotal - MemAvailable = 16384000 - 8192000 = 8192000 kB (50.0%)
    ASSERT_EQ(stats.used_memory_kb(), 8192000ULL);
    ASSERT_NEAR(stats.memory_utilization_percent(), 50.0, 0.001);

    // swap used = 2097152 - 1048576 = 1048576 (50.0%)
    ASSERT_EQ(stats.used_swap_kb(), 1048576ULL);
    ASSERT_NEAR(stats.swap_utilization_percent(), 50.0, 0.001);

    // Unit conversions: 16384000 kB = 16000 MiB = ~15.625 GiB
    ASSERT_NEAR(MemoryStats::kb_to_mib(1048576ULL), 1024.0, 0.001);
    ASSERT_NEAR(MemoryStats::kb_to_gib(1048576ULL), 1.0, 0.001);
}

TEST(MemoryMetrics, FallbackWhenMemAvailableMissing) {
    // Kernels < 3.14 do not expose MemAvailable
    std::string sample =
        "MemTotal:       10000000 kB\n"
        "MemFree:         2000000 kB\n"
        "Buffers:         1000000 kB\n"
        "Cached:          3000000 kB\n"
        "SwapTotal:             0 kB\n"
        "SwapFree:              0 kB\n";

    auto stats = MemoryMonitor::parse_proc_meminfo_string(sample);
    ASSERT_FALSE(stats.has_mem_available);

    // Fallback: MemFree (2000000) + Buffers (1000000) + Cached (3000000) = 6000000 kB
    ASSERT_EQ(stats.mem_available_kb, 6000000ULL);
    ASSERT_EQ(stats.used_memory_kb(), 4000000ULL);
    ASSERT_NEAR(stats.memory_utilization_percent(), 40.0, 0.001);
}

TEST(MemoryMetrics, ZeroMemTotalAvoidsDivisionByZero) {
    std::string sample =
        "MemTotal:              0 kB\n"
        "MemFree:               0 kB\n"
        "SwapTotal:             0 kB\n";

    auto stats = MemoryMonitor::parse_proc_meminfo_string(sample);
    ASSERT_EQ(stats.mem_total_kb, 0ULL);
    ASSERT_EQ(stats.memory_utilization_percent(), 0.0);
    ASSERT_EQ(stats.swap_utilization_percent(), 0.0);
}

TEST(MemoryMetrics, AvailableExceedingTotalUnderflowProtection) {
    // Malformed line where MemAvailable > MemTotal
    std::string sample =
        "MemTotal:        5000000 kB\n"
        "MemAvailable:    6000000 kB\n";

    auto stats = MemoryMonitor::parse_proc_meminfo_string(sample);
    // Should be clamped to MemTotal and used_memory should return 0 (no unsigned underflow)
    ASSERT_EQ(stats.mem_available_kb, 5000000ULL);
    ASSERT_EQ(stats.used_memory_kb(), 0ULL);
    ASSERT_EQ(stats.memory_utilization_percent(), 0.0);
}

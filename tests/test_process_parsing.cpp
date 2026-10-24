#include "test_framework.hpp"
#include "process_monitor.hpp"

using namespace sysmon;

TEST(ProcessParsing, StandardProcessLine) {
    std::string line =
        "1234 (systemd) S 1 1234 1234 0 -1 4194560 1200 0 0 0 150 250 0 0 20 0 1 0 100 123456789 2048 18446744073709551615";
    auto opt = ProcessMonitor::parse_stat_line(line);
    ASSERT_TRUE(opt.has_value());

    const auto& p = *opt;
    ASSERT_EQ(p.pid, 1234);
    ASSERT_STR_EQ(p.name, "systemd");
    ASSERT_EQ(p.state, 'S');
    ASSERT_STR_EQ(p.state_desc, "Interruptible Sleep");
    ASSERT_EQ(p.ppid, 1);
    ASSERT_EQ(p.utime_ticks, 150ULL);
    ASSERT_EQ(p.stime_ticks, 250ULL);
}

TEST(ProcessParsing, ProcessNameWithSpaces) {
    // Process name containing spaces: "Web Content"
    std::string line =
        "5678 (Web Content) R 1234 5678 1234 0 -1 4194560 10 0 0 0 500 100 0 0 20 0 4 0 5000 10485760 1024";
    auto opt = ProcessMonitor::parse_stat_line(line);
    ASSERT_TRUE(opt.has_value());

    const auto& p = *opt;
    ASSERT_EQ(p.pid, 5678);
    ASSERT_STR_EQ(p.name, "Web Content");
    ASSERT_EQ(p.state, 'R');
    ASSERT_STR_EQ(p.state_desc, "Running");
    ASSERT_EQ(p.ppid, 1234);
    ASSERT_EQ(p.utime_ticks, 500ULL);
    ASSERT_EQ(p.stime_ticks, 100ULL);
}

TEST(ProcessParsing, ProcessNameWithParentheses) {
    // Process name containing nested parentheses: "worker (io) pool"
    std::string line =
        "9999 (worker (io) pool) D 1 9999 1 0 -1 0 0 0 0 0 10 20 0 0 20 0 2 0 123 2097152 512";
    auto opt = ProcessMonitor::parse_stat_line(line);
    ASSERT_TRUE(opt.has_value());

    const auto& p = *opt;
    ASSERT_EQ(p.pid, 9999);
    ASSERT_STR_EQ(p.name, "worker (io) pool");
    ASSERT_EQ(p.state, 'D');
    ASSERT_STR_EQ(p.state_desc, "Uninterruptible Sleep (D)");
    ASSERT_EQ(p.ppid, 1);
    ASSERT_EQ(p.utime_ticks, 10ULL);
    ASSERT_EQ(p.stime_ticks, 20ULL);
}

TEST(ProcessParsing, ProcessNameWithSpacesAndMultipleParentheses) {
    // Tricky test: "name with ) parentheses ( and spaces"
    std::string line =
        "42 (name with ) parentheses ( and spaces) I 2 42 2 0 -1 0 0 0 0 0 0 0 0 0 20 0 1 0 50 0 0";
    auto opt = ProcessMonitor::parse_stat_line(line);
    ASSERT_TRUE(opt.has_value());

    const auto& p = *opt;
    ASSERT_EQ(p.pid, 42);
    ASSERT_STR_EQ(p.name, "name with ) parentheses ( and spaces");
    ASSERT_EQ(p.state, 'I');
    ASSERT_STR_EQ(p.state_desc, "Idle Kernel Thread");
    ASSERT_EQ(p.ppid, 2);
}

TEST(ProcessParsing, ProcessStatesTranslation) {
    ASSERT_STR_EQ(ProcessMonitor::state_char_to_description('R'), "Running");
    ASSERT_STR_EQ(ProcessMonitor::state_char_to_description('S'), "Interruptible Sleep");
    ASSERT_STR_EQ(ProcessMonitor::state_char_to_description('D'), "Uninterruptible Sleep (D)");
    ASSERT_STR_EQ(ProcessMonitor::state_char_to_description('T'), "Stopped (Signal)");
    ASSERT_STR_EQ(ProcessMonitor::state_char_to_description('Z'), "Zombie");
    ASSERT_STR_EQ(ProcessMonitor::state_char_to_description('I'), "Idle Kernel Thread");
    ASSERT_STR_EQ(ProcessMonitor::state_char_to_description('?'), "Unknown (?)");
}

TEST(ProcessParsing, MalformedStatLineRejected) {
    // Missing parens
    ASSERT_FALSE(ProcessMonitor::parse_stat_line("1234 badline").has_value());
    // Missing close paren
    ASSERT_FALSE(ProcessMonitor::parse_stat_line("1234 (unclosed S 1 0").has_value());
    // Non-numeric PID
    ASSERT_FALSE(ProcessMonitor::parse_stat_line("abc (proc) S 1 0").has_value());
}

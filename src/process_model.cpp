#include "process_model.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace simulator {

namespace {

std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream token_stream(line);
    while (std::getline(token_stream, token, ',')) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

} // anonymous namespace

void validate_workload(const std::vector<Process>& processes) {
    if (processes.empty()) {
        throw std::runtime_error("Validation error: Workload is empty. At least one process is required.");
    }

    std::unordered_set<std::string> seen_pids;
    for (size_t i = 0; i < processes.size(); ++i) {
        const auto& p = processes[i];
        if (p.id.empty()) {
            throw std::runtime_error("Validation error: Process at index " + std::to_string(i) + " has an empty PID.");
        }
        if (seen_pids.count(p.id) > 0) {
            throw std::runtime_error("Validation error: Duplicate process ID '" + p.id + "' detected.");
        }
        seen_pids.insert(p.id);

        if (p.arrival_time < 0) {
            throw std::runtime_error("Validation error: Process '" + p.id + "' has negative arrival time (" +
                                     std::to_string(p.arrival_time) + ").");
        }
        if (p.burst_time <= 0) {
            throw std::runtime_error("Validation error: Process '" + p.id + "' has non-positive burst time (" +
                                     std::to_string(p.burst_time) + ").");
        }
    }
}

std::vector<Process> load_workload_from_csv(std::istream& is) {
    std::vector<Process> processes;
    std::unordered_set<std::string> seen_pids;
    std::string line;
    size_t line_number = 0;
    bool header_checked = false;

    while (std::getline(is, line)) {
        line_number++;
        std::string trimmed = trim(line);
        if (trimmed.empty()) continue; // Ignore blank lines

        auto tokens = split_csv_line(trimmed);

        if (!header_checked) {
            // First non-empty line must be header: pid,arrival_time,burst_time,priority
            if (tokens.size() != 4) {
                throw std::runtime_error("CSV parse error at line " + std::to_string(line_number) +
                                         ": Expected 4 columns in header (pid,arrival_time,burst_time,priority), got " +
                                         std::to_string(tokens.size()));
            }

            std::string col0 = to_lower(tokens[0]);
            std::string col1 = to_lower(tokens[1]);
            std::string col2 = to_lower(tokens[2]);
            std::string col3 = to_lower(tokens[3]);

            if (col0 != "pid" || col1 != "arrival_time" || col2 != "burst_time" || col3 != "priority") {
                throw std::runtime_error("CSV parse error at line " + std::to_string(line_number) +
                                         ": Invalid header. Expected 'pid,arrival_time,burst_time,priority', got '" +
                                         trimmed + "'");
            }
            header_checked = true;
            continue;
        }

        if (tokens.size() != 4) {
            throw std::runtime_error("CSV parse error at line " + std::to_string(line_number) +
                                     ": Expected 4 columns, got " + std::to_string(tokens.size()) +
                                     " in line: '" + trimmed + "'");
        }

        std::string pid = tokens[0];
        if (pid.empty()) {
            throw std::runtime_error("CSV parse error at line " + std::to_string(line_number) +
                                     ": PID cannot be empty.");
        }
        if (seen_pids.count(pid) > 0) {
            throw std::runtime_error("CSV parse error at line " + std::to_string(line_number) +
                                     ": Duplicate process ID '" + pid + "' found.");
        }

        int arrival_time = 0;
        try {
            arrival_time = std::stoi(tokens[1]);
        } catch (...) {
            throw std::runtime_error("CSV parse error at line " + std::to_string(line_number) +
                                     ": Invalid numeric value for arrival_time: '" + tokens[1] + "'");
        }

        if (arrival_time < 0) {
            throw std::runtime_error("CSV parse error at line " + std::to_string(line_number) +
                                     ": Arrival time must be non-negative, got " + std::to_string(arrival_time));
        }

        int burst_time = 0;
        try {
            burst_time = std::stoi(tokens[2]);
        } catch (...) {
            throw std::runtime_error("CSV parse error at line " + std::to_string(line_number) +
                                     ": Invalid numeric value for burst_time: '" + tokens[2] + "'");
        }

        if (burst_time <= 0) {
            throw std::runtime_error("CSV parse error at line " + std::to_string(line_number) +
                                     ": Burst time must be positive (>0), got " + std::to_string(burst_time));
        }

        int priority = 0;
        try {
            priority = std::stoi(tokens[3]);
        } catch (...) {
            throw std::runtime_error("CSV parse error at line " + std::to_string(line_number) +
                                     ": Invalid numeric value for priority: '" + tokens[3] + "'");
        }

        Process p;
        p.id = pid;
        p.arrival_time = arrival_time;
        p.burst_time = burst_time;
        p.priority = priority;
        p.reset_metrics();

        processes.push_back(p);
        seen_pids.insert(pid);
    }

    if (!header_checked) {
        throw std::runtime_error("CSV parse error: Input file is empty or missing valid header.");
    }

    if (processes.empty()) {
        throw std::runtime_error("CSV parse error: Workload contains no data rows. At least one process is required.");
    }

    return processes;
}

std::vector<Process> load_workload_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open workload CSV file: " + filepath);
    }
    return load_workload_from_csv(file);
}

} // namespace simulator

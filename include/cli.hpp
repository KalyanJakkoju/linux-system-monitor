#pragma once

#include <string>
#include <vector>

namespace cli {

struct CommandLineOptions {
    enum class Command {
        None,
        Info,
        Monitor,
        Processes,
        Schedule,
        Help
    };

    Command command{Command::None};
    int interval_seconds{1};
    std::string algorithm; // "fcfs", "sjf", "rr", "priority", "all"
    bool schedule_all{false};
    int quantum{2};
    std::string input_file;
    bool show_help{false};
    std::string error_message;
};

class CommandLineParser {
public:
    static CommandLineOptions parse(int argc, char* argv[]);
    static void print_usage(const char* program_name);
};

} // namespace cli

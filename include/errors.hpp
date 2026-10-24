#pragma once

#include <string>
#include <system_error>

namespace sysmon {

enum class ErrorCode {
    Success = 0,
    FileNotFound,
    PermissionDenied,
    MalformedData,
    DivisionByZero,
    NotEnoughSamples,
    InvalidParameter,
    ProcessTerminated,
    CounterDecreased,
    EmptyWorkload,
    DuplicateProcessId,
    IoError
};

class SystemMonitorError : public std::runtime_error {
public:
    SystemMonitorError(ErrorCode code, const std::string& message)
        : std::runtime_error(message), code_(code) {}

    ErrorCode code() const noexcept { return code_; }

private:
    ErrorCode code_;
};

inline const char* error_code_to_string(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Success: return "Success";
        case ErrorCode::FileNotFound: return "File not found";
        case ErrorCode::PermissionDenied: return "Permission denied";
        case ErrorCode::MalformedData: return "Malformed data";
        case ErrorCode::DivisionByZero: return "Division by zero";
        case ErrorCode::NotEnoughSamples: return "Not enough samples";
        case ErrorCode::InvalidParameter: return "Invalid parameter";
        case ErrorCode::ProcessTerminated: return "Process terminated";
        case ErrorCode::CounterDecreased: return "CPU counters decreased unexpectedly";
        case ErrorCode::EmptyWorkload: return "Empty workload";
        case ErrorCode::DuplicateProcessId: return "Duplicate process ID";
        case ErrorCode::IoError: return "I/O error";
        default: return "Unknown error";
    }
}

} // namespace sysmon

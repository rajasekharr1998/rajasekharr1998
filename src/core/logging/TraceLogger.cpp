#include "core/logging/TraceLogger.h"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace core::logging {

TraceLogger::TraceLogger(std::string filePath) {
    if (!filePath.empty()) {
        file_.open(filePath, std::ios::app);
    }
}

void TraceLogger::logFrame(Direction direction,
                           const core::transport::CanFrame& frame,
                           std::optional<std::chrono::milliseconds> responseTime) {
    std::ostringstream line;
    line << timestampNow() << " | " << directionText(direction)
         << " | CAN | ID:" << std::hex << std::uppercase << frame.canId
         << " | DLC:" << std::dec << frame.data.size()
         << " | " << hexBytes(frame.data);
    if (responseTime) {
        line << " | RTT:" << responseTime->count() << "ms";
    }

    std::scoped_lock lk(mtx_);
    std::cout << line.str() << '\n';
    if (file_.is_open()) {
        file_ << line.str() << '\n';
        file_.flush();
    }
}

void TraceLogger::logText(Direction direction, const std::string& message) {
    std::scoped_lock lk(mtx_);
    const auto line = direction == Direction::Info || direction == Direction::Error
                          ? timestampNow() + " | " + directionText(direction) + " | " + message
                          : message;
    std::cout << line << '\n';
    if (file_.is_open()) {
        file_ << line << '\n';
        file_.flush();
    }
}

std::string TraceLogger::timestampNow() {
    const auto now = std::chrono::system_clock::now();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << millis.count();
    return out.str();
}

std::string TraceLogger::directionText(Direction direction) {
    switch (direction) {
        case Direction::Tx: return "Tx";
        case Direction::Rx: return "Rx";
        case Direction::Info: return "Info";
        case Direction::Error: return "Error";
    }
    return "Info";
}

std::string TraceLogger::hexBytes(const std::vector<uint8_t>& data) {
    std::ostringstream out;
    for (auto b : data) {
        out << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(b) << ' ';
    }
    return out.str();
}

}  // namespace core::logging

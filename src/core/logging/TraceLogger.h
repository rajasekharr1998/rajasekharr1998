#pragma once

#include "core/transport/IsoTpEngine.h"

#include <chrono>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>

namespace core::logging {

enum class Direction { Tx, Rx, Info, Error };

class TraceLogger {
public:
    explicit TraceLogger(std::string filePath = {});

    void logFrame(Direction direction,
                  const core::transport::CanFrame& frame,
                  std::optional<std::chrono::milliseconds> responseTime = std::nullopt);
    void logText(Direction direction, const std::string& message);

private:
    static std::string timestampNow();
    static std::string directionText(Direction direction);
    static std::string hexBytes(const std::vector<uint8_t>& data);

    std::mutex mtx_;
    std::ofstream file_;
};

}  // namespace core::logging

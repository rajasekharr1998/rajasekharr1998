#pragma once

#include "core/automation/ScriptRunner.h"
#include "core/common/Result.h"
#include "core/diag/UdsClient.h"
#include "core/j2534/IJ2534Api.h"
#include "core/logging/TraceLogger.h"

#include <memory>
#include <string>
#include <vector>

namespace core::ui {

struct ChannelConfig {
    unsigned long protocolId{static_cast<unsigned long>(core::j2534::Protocol::Iso15765)};
    unsigned long flags{0};
    unsigned long baud{500000};
};

class DiagnosticController {
public:
    DiagnosticController(core::j2534::IJ2534Api& api, core::logging::TraceLogger& logger);

    Result<void> connect(const ChannelConfig& config = {});
    Result<void> disconnect();
    Result<std::vector<uint8_t>> readVin();
    Result<std::vector<core::automation::ScriptStepResult>> runScriptLines(const std::vector<std::string>& lines);

    [[nodiscard]] bool connected() const { return connected_; }

private:
    core::j2534::IJ2534Api& api_;
    core::logging::TraceLogger& logger_;
    unsigned long deviceId_{0};
    unsigned long channelId_{0};
    bool connected_{false};
    std::unique_ptr<core::diag::UdsClient> uds_;
};

}  // namespace core::ui

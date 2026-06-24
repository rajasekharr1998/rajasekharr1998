#include "core/ui/DiagnosticController.h"

#include "core/j2534/J2534Error.h"

namespace core::ui {

DiagnosticController::DiagnosticController(core::j2534::IJ2534Api& api, core::logging::TraceLogger& logger)
    : api_(api), logger_(logger) {}

Result<void> DiagnosticController::connect(const ChannelConfig& config) {
    auto status = api_.Open(nullptr, &deviceId_);
    if (status != core::j2534::STATUS_NOERROR) {
        return Result<void>::failure(core::j2534::mapJ2534Error(status), "PassThruOpen failed");
    }
    status = api_.Connect(deviceId_, config.protocolId, config.flags, config.baud, &channelId_);
    if (status != core::j2534::STATUS_NOERROR) {
        return Result<void>::failure(core::j2534::mapJ2534Error(status), "PassThruConnect failed");
    }
    uds_ = std::make_unique<core::diag::UdsClient>(api_, channelId_, core::transport::IsoTpConfig{}, &logger_);
    connected_ = true;
    logger_.logText(core::logging::Direction::Info, "Connected diagnostic channel");
    return Result<void>::success();
}

Result<void> DiagnosticController::disconnect() {
    if (!connected_) return Result<void>::success();
    auto status = api_.Disconnect(channelId_);
    if (status != core::j2534::STATUS_NOERROR) {
        return Result<void>::failure(core::j2534::mapJ2534Error(status), "PassThruDisconnect failed");
    }
    status = api_.Close(deviceId_);
    if (status != core::j2534::STATUS_NOERROR) {
        return Result<void>::failure(core::j2534::mapJ2534Error(status), "PassThruClose failed");
    }
    connected_ = false;
    uds_.reset();
    logger_.logText(core::logging::Direction::Info, "Disconnected diagnostic channel");
    return Result<void>::success();
}

Result<std::vector<uint8_t>> DiagnosticController::readVin() {
    if (!uds_) {
        return Result<std::vector<uint8_t>>::failure(DiagErr::DeviceNotOpen, "Not connected");
    }
    return uds_->readDataByIdentifier(0xF190);
}

Result<std::vector<core::automation::ScriptStepResult>> DiagnosticController::runScriptLines(const std::vector<std::string>& lines) {
    if (!uds_) {
        return Result<std::vector<core::automation::ScriptStepResult>>::failure(DiagErr::DeviceNotOpen, "Not connected");
    }
    core::automation::ScriptRunner runner(*uds_);
    return runner.runLines(lines);
}

}  // namespace core::ui

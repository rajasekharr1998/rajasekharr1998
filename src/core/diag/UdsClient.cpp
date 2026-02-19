#include "core/diag/UdsClient.h"

namespace core::diag {

using core::j2534::J2534Msg;

UdsClient::UdsClient(j2534::IJ2534Api& api, unsigned long channelId)
    : api_(api), channelId_(channelId) {}

Result<std::vector<uint8_t>> UdsClient::sessionControl(uint8_t sessionType) {
    return sendAndReceive({0x10, sessionType});
}

Result<std::vector<uint8_t>> UdsClient::testerPresent() {
    return sendAndReceive({0x3E, 0x00});
}

Result<std::vector<uint8_t>> UdsClient::readDataByIdentifier(uint16_t did) {
    return sendAndReceive({0x22,
                           static_cast<uint8_t>((did >> 8) & 0xFF),
                           static_cast<uint8_t>(did & 0xFF)});
}

Result<std::vector<uint8_t>> UdsClient::sendAndReceive(const std::vector<uint8_t>& payload) {
    J2534Msg tx{};
    tx.protocolId = static_cast<uint32_t>(j2534::Protocol::Iso15765);
    tx.data = payload;
    tx.dataSize = static_cast<uint32_t>(tx.data.size());

    unsigned long txCount = 1;
    const auto wr = api_.WriteMsgs(channelId_, &tx, &txCount, 100);
    if (wr != 0 || txCount != 1) {
        return Result<std::vector<uint8_t>>::failure(DiagErr::DriverError,
                                                     "WriteMsgs failed");
    }

    J2534Msg rx{};
    unsigned long rxCount = 1;
    const auto rd = api_.ReadMsgs(channelId_, &rx, &rxCount, 100);
    if (rd != 0 || rxCount != 1) {
        return Result<std::vector<uint8_t>>::failure(DiagErr::Timeout,
                                                     "ReadMsgs timeout/no data");
    }

    return Result<std::vector<uint8_t>>::success(rx.data);
}

}  // namespace core::diag

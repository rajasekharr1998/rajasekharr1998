#pragma once

#include "core/common/Result.h"
#include "core/j2534/IJ2534Api.h"

#include <cstdint>
#include <vector>

namespace core::diag {

class UdsClient {
public:
    explicit UdsClient(j2534::IJ2534Api& api, unsigned long channelId);

    Result<std::vector<uint8_t>> sessionControl(uint8_t sessionType);
    Result<std::vector<uint8_t>> testerPresent();
    Result<std::vector<uint8_t>> readDataByIdentifier(uint16_t did);

private:
    Result<std::vector<uint8_t>> sendAndReceive(const std::vector<uint8_t>& payload);

    j2534::IJ2534Api& api_;
    unsigned long channelId_;
};

}  // namespace core::diag

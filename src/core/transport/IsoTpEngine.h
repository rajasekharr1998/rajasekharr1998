#pragma once

#include "core/common/Result.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace core::transport {

struct CanFrame {
    uint32_t canId{};
    bool extendedId{false};
    std::vector<uint8_t> data;
};

struct IsoTpConfig {
    uint32_t txCanId{0x7E0};
    uint32_t rxCanId{0x7E8};
    bool extendedId{false};
    uint8_t blockSize{0};
    uint8_t stMinMs{0};
};

class IsoTpEngine {
public:
    explicit IsoTpEngine(IsoTpConfig config = {});

    [[nodiscard]] std::vector<CanFrame> segmentForTransmit(const std::vector<uint8_t>& payload) const;
    [[nodiscard]] Result<std::vector<uint8_t>> reassembleReceived(const std::vector<CanFrame>& frames) const;
    [[nodiscard]] CanFrame makeFlowControl(uint8_t flowStatus = 0x00) const;

private:
    IsoTpConfig config_;
};

}  // namespace core::transport

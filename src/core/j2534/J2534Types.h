#pragma once

#include <cstdint>
#include <vector>

namespace core::j2534 {

struct J2534Msg {
    uint32_t protocolId{};
    uint32_t rxStatus{};
    uint32_t txFlags{};
    uint32_t timestamp{};
    uint32_t dataSize{};
    uint32_t extraDataIndex{};
    std::vector<uint8_t> data;
};

enum class Protocol : uint32_t {
    Can = 1,
    Iso15765 = 6,
};

}  // namespace core::j2534

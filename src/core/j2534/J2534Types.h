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

enum Status : long {
    STATUS_NOERROR = 0x00,
    ERR_NOT_SUPPORTED = 0x01,
    ERR_INVALID_CHANNEL_ID = 0x02,
    ERR_INVALID_DEVICE_ID = 0x03,
    ERR_TIMEOUT = 0x09,
    ERR_DEVICE_NOT_CONNECTED = 0x0C,
    ERR_FAILED = 0x07,
};

}  // namespace core::j2534

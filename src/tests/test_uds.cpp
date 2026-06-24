#include "core/diag/UdsClient.h"
#include "core/j2534/J2534MockApi.h"

#include <cstdlib>
#include <iostream>

int main() {
    core::j2534::J2534MockApi api;
    unsigned long deviceId = 0;
    unsigned long channelId = 0;

    if (api.Open(nullptr, &deviceId) != 0) return 1;
    if (api.Connect(deviceId, static_cast<unsigned long>(core::j2534::Protocol::Iso15765), 0, 500000, &channelId) != 0) return 1;

    core::diag::UdsClient uds(api, channelId);

    const auto s = uds.sessionControl(0x03);
    if (!s.ok() || s.value.empty() || s.value[0] != 0x50) {
        std::cerr << "sessionControl failed\n";
        return 2;
    }

    const auto vin = uds.readDataByIdentifier(0xF190);
    if (!vin.ok() || vin.value.size() < 3 || vin.value[0] != 0x62) {
        std::cerr << "readDID F190 failed\n";
        return 3;
    }

    const auto invalidDid = uds.readDataByIdentifier(0x1234);
    if (invalidDid.ok() || invalidDid.err != core::DiagErr::NegativeResponse) {
        std::cerr << "negative response handling failed\n";
        return 4;
    }

    std::cout << "All UDS tests passed\n";
    return 0;
}

#include "core/diag/UdsClient.h"
#include "core/j2534/J2534MockApi.h"

#include <iomanip>
#include <iostream>

int main() {
    core::j2534::J2534MockApi api;

    unsigned long deviceId = 0;
    if (api.Open(nullptr, &deviceId) != 0) {
        std::cerr << "PassThruOpen failed\n";
        return 1;
    }

    unsigned long channelId = 0;
    if (api.Connect(deviceId, static_cast<unsigned long>(core::j2534::Protocol::Iso15765), 0, 500000, &channelId) != 0) {
        std::cerr << "PassThruConnect failed\n";
        return 1;
    }

    core::diag::UdsClient uds(api, channelId);
    const auto resp = uds.sessionControl(0x03);
    if (!resp.ok()) {
        std::cerr << "UDS request failed: " << resp.message << "\n";
        return 1;
    }

    std::cout << "SessionControl response: ";
    for (const auto byte : resp.value) {
        std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<int>(byte) << ' ';
    }
    std::cout << "\n";

    api.Disconnect(channelId);
    api.Close(deviceId);
    return 0;
}

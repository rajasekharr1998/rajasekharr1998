#include "core/diag/AsyncUdsClient.h"
#include "core/j2534/J2534MockApi.h"

#include <chrono>
#include <iostream>

int main() {
    core::j2534::J2534MockApi api;
    unsigned long deviceId = 0;
    unsigned long channelId = 0;
    if (api.Open(nullptr, &deviceId) != 0) return 1;
    if (api.Connect(deviceId, static_cast<unsigned long>(core::j2534::Protocol::Iso15765), 0, 500000, &channelId) != 0) return 2;

    core::diag::UdsClient syncClient(api, channelId);
    core::diag::AsyncUdsClient asyncClient(syncClient, {std::chrono::milliseconds(10), std::chrono::milliseconds(100), std::chrono::milliseconds(20), 1});

    asyncClient.startTesterPresent();

    auto sessionFuture = asyncClient.sessionControl(0x03);
    auto didFuture = asyncClient.readDataByIdentifier(0xF190);

    const auto session = sessionFuture.get();
    const auto did = didFuture.get();

    asyncClient.stopTesterPresent();
    asyncClient.shutdown();

    if (!session.ok() || session.value.empty() || session.value[0] != 0x50) {
        std::cerr << "async session control failed\n";
        return 3;
    }
    if (!did.ok() || did.value.empty() || did.value[0] != 0x62) {
        std::cerr << "async DID read failed\n";
        return 4;
    }

    std::cout << "All async UDS tests passed\n";
    return 0;
}

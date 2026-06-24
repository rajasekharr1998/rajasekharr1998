#include "core/diag/UdsClient.h"
#include "core/j2534/J2534MockApi.h"

#include <iomanip>
#include <iostream>

namespace {
void printHex(const std::string& label, const std::vector<uint8_t>& payload) {
    std::cout << label;
    for (const auto b : payload) {
        std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<int>(b) << ' ';
    }
    std::cout << std::dec << "\n";
}
}

int main() {
    core::j2534::J2534MockApi api;
    unsigned long deviceId = 0;
    unsigned long channelId = 0;

    if (api.Open(nullptr, &deviceId) != core::j2534::STATUS_NOERROR) return 1;
    if (api.Connect(deviceId, static_cast<unsigned long>(core::j2534::Protocol::Iso15765), 0, 500000, &channelId) != core::j2534::STATUS_NOERROR) return 1;

    core::diag::UdsClient uds(api, channelId);

    const auto session = uds.sessionControl(0x03);
    if (!session.ok()) return 1;
    printHex("SessionControl: ", session.value);

    const auto vin = uds.readDataByIdentifier(0xF190);
    if (!vin.ok()) return 1;
    printHex("ReadDID F190: ", vin.value);

    const auto badDid = uds.readDataByIdentifier(0x1234);
    if (!badDid.ok()) {
        std::cout << "Negative response handled: " << badDid.message << "\n";
    }

    const auto download = uds.requestDownload({0x00, 0x44, 0x00, 0x00, 0x10, 0x00});
    if (!download.ok()) return 1;
    printHex("RequestDownload: ", download.value);

    const auto transfer = uds.transferData(0x01, {0xDE, 0xAD, 0xBE, 0xEF});
    if (!transfer.ok()) return 1;
    printHex("TransferData: ", transfer.value);

    const auto exit = uds.transferExit();
    if (!exit.ok()) return 1;
    printHex("TransferExit: ", exit.value);

    api.Disconnect(channelId);
    api.Close(deviceId);
    return 0;
}

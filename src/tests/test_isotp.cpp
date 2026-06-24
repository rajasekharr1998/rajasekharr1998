#include "core/transport/IsoTpEngine.h"

#include <iostream>
#include <vector>

namespace {
int fail(const char* message) {
    std::cerr << message << '\n';
    return 1;
}
}

int main() {
    core::transport::IsoTpEngine engine({0x7E0, 0x7E8, false, 8, 5});

    const std::vector<uint8_t> shortPayload{0x10, 0x03};
    const auto singleFrames = engine.segmentForTransmit(shortPayload);
    if (singleFrames.size() != 1 || singleFrames[0].data[0] != 0x02) {
        return fail("single-frame segmentation failed");
    }
    const auto singleReassembled = engine.reassembleReceived(singleFrames);
    if (!singleReassembled.ok() || singleReassembled.value != shortPayload) {
        return fail("single-frame reassembly failed");
    }

    const std::vector<uint8_t> longPayload{0x62, 0xF1, 0x90, '1', 'H', 'G', 'C', 'M', '8', '2', '6', '3', '3', 'A', '0', '0', '4', '3', '5', '2'};
    const auto multiFrames = engine.segmentForTransmit(longPayload);
    if (multiFrames.size() != 3 || (multiFrames[0].data[0] & 0xF0) != 0x10 || (multiFrames[1].data[0] & 0x0F) != 1) {
        return fail("multi-frame segmentation failed");
    }
    const auto multiReassembled = engine.reassembleReceived(multiFrames);
    if (!multiReassembled.ok() || multiReassembled.value != longPayload) {
        return fail("multi-frame reassembly failed");
    }

    auto corrupted = multiFrames;
    corrupted[1].data[0] = 0x23;
    const auto mismatch = engine.reassembleReceived(corrupted);
    if (mismatch.ok() || mismatch.err != core::DiagErr::ProtocolViolation) {
        return fail("sequence mismatch validation failed");
    }

    const auto fc = engine.makeFlowControl();
    if (fc.data.size() != 8 || fc.data[0] != 0x30 || fc.data[1] != 8 || fc.data[2] != 5) {
        return fail("flow-control generation failed");
    }

    std::cout << "All ISO-TP tests passed\n";
    return 0;
}

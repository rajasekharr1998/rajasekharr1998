#include "core/j2534/J2534MockApi.h"
#include "core/logging/TraceLogger.h"
#include "core/ui/DiagnosticController.h"

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
    core::logging::TraceLogger logger;
    core::ui::DiagnosticController controller(api, logger);

    const auto connected = controller.connect();
    if (!connected.ok()) {
        std::cerr << connected.message << '\n';
        return 1;
    }

    const auto vin = controller.readVin();
    if (!vin.ok()) {
        std::cerr << vin.message << '\n';
        return 1;
    }
    printHex("ReadDID F190: ", vin.value);

    const auto script = controller.runScriptLines({
        "session 03",
        "read_did F190",
        "download",
    });
    if (!script.ok()) {
        std::cerr << script.message << '\n';
        return 1;
    }
    for (const auto& step : script.value) {
        std::cout << "Script step [" << step.command << "] => " << (step.passed ? "PASS" : "FAIL") << " " << step.detail << '\n';
    }

    const auto disconnected = controller.disconnect();
    if (!disconnected.ok()) {
        std::cerr << disconnected.message << '\n';
        return 1;
    }
    return 0;
}

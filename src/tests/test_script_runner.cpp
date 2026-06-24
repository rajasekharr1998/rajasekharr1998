#include "core/j2534/J2534MockApi.h"
#include "core/logging/TraceLogger.h"
#include "core/ui/DiagnosticController.h"

#include <iostream>

int main() {
    core::j2534::J2534MockApi api;
    core::logging::TraceLogger logger;
    core::ui::DiagnosticController controller(api, logger);

    const auto connected = controller.connect();
    if (!connected.ok()) return 1;

    const auto result = controller.runScriptLines({"session 03", "read_did F190", "download"});
    if (!result.ok() || result.value.size() != 3) {
        std::cerr << "script runner failed\n";
        return 2;
    }
    for (const auto& step : result.value) {
        if (!step.passed) {
            std::cerr << "script step failed: " << step.command << '\n';
            return 3;
        }
    }
    controller.disconnect();
    std::cout << "All script runner tests passed\n";
    return 0;
}

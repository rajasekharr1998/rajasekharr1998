#include "core/automation/ScriptRunner.h"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace core::automation {

namespace {
uint32_t parseHexU32(const std::string& value) {
    uint32_t parsed = 0;
    std::stringstream ss;
    ss << std::hex << value;
    ss >> parsed;
    return parsed;
}

std::string bytesToHex(const std::vector<uint8_t>& data) {
    std::ostringstream out;
    for (auto b : data) {
        out << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(b) << ' ';
    }
    return out.str();
}
}

ScriptRunner::ScriptRunner(core::diag::UdsClient& uds) : uds_(uds) {}

Result<std::vector<ScriptStepResult>> ScriptRunner::runLines(const std::vector<std::string>& lines) {
    std::vector<ScriptStepResult> results;
    bool allPassed = true;
    for (const auto& line : lines) {
        if (line.empty() || line[0] == '#') continue;
        auto result = runLine(line);
        allPassed = allPassed && result.passed;
        results.push_back(std::move(result));
    }
    if (!allPassed) {
        return Result<std::vector<ScriptStepResult>>::failure(DiagErr::NegativeResponse, "One or more script steps failed");
    }
    return Result<std::vector<ScriptStepResult>>::success(results);
}

Result<std::vector<ScriptStepResult>> ScriptRunner::runFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return Result<std::vector<ScriptStepResult>>::failure(DiagErr::InternalError, "Unable to open script file");
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    return runLines(lines);
}

ScriptStepResult ScriptRunner::runLine(const std::string& line) {
    std::istringstream in(line);
    std::string command;
    in >> command;

    Result<std::vector<uint8_t>> response;
    if (command == "session") {
        std::string session;
        in >> session;
        response = uds_.sessionControl(static_cast<uint8_t>(parseHexU32(session)));
    } else if (command == "read_did") {
        std::string did;
        in >> did;
        response = uds_.readDataByIdentifier(static_cast<uint16_t>(parseHexU32(did)));
    } else if (command == "tester_present") {
        response = uds_.testerPresent();
    } else if (command == "download") {
        response = uds_.requestDownload({0x00, 0x44, 0x00, 0x00, 0x10, 0x00});
    } else {
        return {line, false, "Unknown command"};
    }

    if (!response.ok()) {
        return {line, false, response.message};
    }
    return {line, true, bytesToHex(response.value)};
}

}  // namespace core::automation

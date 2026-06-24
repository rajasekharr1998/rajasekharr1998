#pragma once

#include "core/common/Result.h"
#include "core/diag/UdsClient.h"

#include <string>
#include <vector>

namespace core::automation {

struct ScriptStepResult {
    std::string command;
    bool passed{false};
    std::string detail;
};

class ScriptRunner {
public:
    explicit ScriptRunner(core::diag::UdsClient& uds);

    Result<std::vector<ScriptStepResult>> runLines(const std::vector<std::string>& lines);
    Result<std::vector<ScriptStepResult>> runFile(const std::string& path);

private:
    ScriptStepResult runLine(const std::string& line);
    core::diag::UdsClient& uds_;
};

}  // namespace core::automation

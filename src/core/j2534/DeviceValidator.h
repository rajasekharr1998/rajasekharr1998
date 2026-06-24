#pragma once

#include "core/common/Result.h"

#include <string>
#include <vector>

namespace core::j2534 {

struct DeviceValidationReport {
    std::string dllPath;
    std::vector<std::string> missingExports;
    bool loadable{false};

    [[nodiscard]] bool ok() const { return loadable && missingExports.empty(); }
};

Result<DeviceValidationReport> validateVendorDll(const std::string& dllPath);

}  // namespace core::j2534

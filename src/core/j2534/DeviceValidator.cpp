#include "core/j2534/DeviceValidator.h"

#include <array>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace core::j2534 {

namespace {
constexpr std::array<const char*, 11> kRequiredExports{
    "PassThruOpen",
    "PassThruClose",
    "PassThruConnect",
    "PassThruDisconnect",
    "PassThruReadMsgs",
    "PassThruWriteMsgs",
    "PassThruStartPeriodicMsg",
    "PassThruStopPeriodicMsg",
    "PassThruStartMsgFilter",
    "PassThruStopMsgFilter",
    "PassThruIoctl",
};
}

Result<DeviceValidationReport> validateVendorDll(const std::string& dllPath) {
    DeviceValidationReport report{dllPath, {}, false};
#if defined(_WIN32)
    const auto module = ::LoadLibraryA(dllPath.c_str());
    if (!module) {
        return Result<DeviceValidationReport>::failure(DiagErr::DeviceNotOpen, "Unable to load vendor DLL");
    }
    report.loadable = true;
    for (const auto* symbol : kRequiredExports) {
        if (::GetProcAddress(module, symbol) == nullptr) {
            report.missingExports.emplace_back(symbol);
        }
    }
    ::FreeLibrary(module);
    if (!report.ok()) {
        return Result<DeviceValidationReport>::failure(DiagErr::Unsupported, "Vendor DLL is missing required J2534 exports");
    }
    return Result<DeviceValidationReport>::success(report);
#else
    (void)kRequiredExports;
    report.missingExports.assign(kRequiredExports.begin(), kRequiredExports.end());
    return Result<DeviceValidationReport>::failure(DiagErr::Unsupported, "Vendor DLL validation requires Windows LoadLibrary/GetProcAddress");
#endif
}

}  // namespace core::j2534

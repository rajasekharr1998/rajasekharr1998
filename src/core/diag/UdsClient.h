#pragma once

#include "core/common/Result.h"
#include "core/j2534/IJ2534Api.h"
#include "core/logging/TraceLogger.h"
#include "core/transport/IsoTpEngine.h"

#include <cstdint>
#include <mutex>
#include <vector>

namespace core::diag {

class ISecurityAlgorithm {
public:
    virtual ~ISecurityAlgorithm() = default;
    virtual std::vector<uint8_t> computeKey(uint8_t level, const std::vector<uint8_t>& seed) = 0;
};

class UdsClient {
public:
    explicit UdsClient(j2534::IJ2534Api& api,
                       unsigned long channelId,
                       transport::IsoTpConfig transportConfig = {},
                       logging::TraceLogger* logger = nullptr);

    Result<std::vector<uint8_t>> sessionControl(uint8_t sessionType);               // 0x10
    Result<std::vector<uint8_t>> ecuReset(uint8_t resetType);                       // 0x11
    Result<std::vector<uint8_t>> securityAccess(uint8_t subFn, const std::vector<uint8_t>& payload);  // 0x27
    Result<std::vector<uint8_t>> readDataByIdentifier(uint16_t did);                // 0x22
    Result<std::vector<uint8_t>> writeDataByIdentifier(uint16_t did, const std::vector<uint8_t>& val); // 0x2E
    Result<std::vector<uint8_t>> testerPresent();                                    // 0x3E
    Result<std::vector<uint8_t>> requestDownload(const std::vector<uint8_t>& args); // 0x34
    Result<std::vector<uint8_t>> transferData(uint8_t blockCounter, const std::vector<uint8_t>& chunk);//0x36
    Result<std::vector<uint8_t>> transferExit();                                     // 0x37

private:
    Result<std::vector<uint8_t>> sendAndReceive(const std::vector<uint8_t>& payload);
    Result<std::vector<uint8_t>> readIsoTpResponse();

    j2534::IJ2534Api& api_;
    unsigned long channelId_;
    transport::IsoTpEngine isotp_;
    logging::TraceLogger* logger_;
    std::mutex requestMtx_;
};

}  // namespace core::diag

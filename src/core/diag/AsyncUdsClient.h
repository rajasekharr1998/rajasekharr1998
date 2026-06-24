#pragma once

#include "core/common/Result.h"
#include "core/diag/UdsClient.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

namespace core::diag {

struct UdsTimingPolicy {
    std::chrono::milliseconds p2Timeout{50};
    std::chrono::milliseconds p2StarTimeout{5000};
    std::chrono::milliseconds s3Interval{2000};
    uint8_t transportRetries{2};
};

class AsyncUdsClient {
public:
    AsyncUdsClient(UdsClient& client, UdsTimingPolicy timing = {});
    ~AsyncUdsClient();

    AsyncUdsClient(const AsyncUdsClient&) = delete;
    AsyncUdsClient& operator=(const AsyncUdsClient&) = delete;

    std::future<Result<std::vector<uint8_t>>> sessionControl(uint8_t sessionType);
    std::future<Result<std::vector<uint8_t>>> readDataByIdentifier(uint16_t did);
    std::future<Result<std::vector<uint8_t>>> requestDownload(const std::vector<uint8_t>& args);
    std::future<Result<std::vector<uint8_t>>> transferData(uint8_t blockCounter, const std::vector<uint8_t>& chunk);
    std::future<Result<std::vector<uint8_t>>> transferExit();

    void startTesterPresent();
    void stopTesterPresent();
    void shutdown();

private:
    using WorkFn = std::function<Result<std::vector<uint8_t>>() >;

    struct WorkItem {
        WorkFn fn;
        std::promise<Result<std::vector<uint8_t>>> promise;
    };

    std::future<Result<std::vector<uint8_t>>> enqueue(WorkFn fn);
    Result<std::vector<uint8_t>> executeWithRetry(const WorkFn& fn) const;
    void workerLoop();
    void testerPresentLoop();

    UdsClient& client_;
    UdsTimingPolicy timing_;
    mutable std::mutex queueMtx_;
    std::condition_variable queueCv_;
    std::deque<WorkItem> queue_;
    std::thread worker_;
    std::thread testerPresentWorker_;
    std::atomic<bool> stopping_{false};
    std::atomic<bool> testerPresentEnabled_{false};
};

}  // namespace core::diag

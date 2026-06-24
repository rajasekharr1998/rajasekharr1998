#include "core/diag/AsyncUdsClient.h"

namespace core::diag {

AsyncUdsClient::AsyncUdsClient(UdsClient& client, UdsTimingPolicy timing)
    : client_(client), timing_(timing), worker_(&AsyncUdsClient::workerLoop, this) {}

AsyncUdsClient::~AsyncUdsClient() {
    shutdown();
}

std::future<Result<std::vector<uint8_t>>> AsyncUdsClient::sessionControl(uint8_t sessionType) {
    return enqueue([this, sessionType] { return client_.sessionControl(sessionType); });
}

std::future<Result<std::vector<uint8_t>>> AsyncUdsClient::readDataByIdentifier(uint16_t did) {
    return enqueue([this, did] { return client_.readDataByIdentifier(did); });
}

std::future<Result<std::vector<uint8_t>>> AsyncUdsClient::requestDownload(const std::vector<uint8_t>& args) {
    return enqueue([this, args] { return client_.requestDownload(args); });
}

std::future<Result<std::vector<uint8_t>>> AsyncUdsClient::transferData(uint8_t blockCounter, const std::vector<uint8_t>& chunk) {
    return enqueue([this, blockCounter, chunk] { return client_.transferData(blockCounter, chunk); });
}

std::future<Result<std::vector<uint8_t>>> AsyncUdsClient::transferExit() {
    return enqueue([this] { return client_.transferExit(); });
}

void AsyncUdsClient::startTesterPresent() {
    bool expected = false;
    if (!testerPresentEnabled_.compare_exchange_strong(expected, true)) {
        return;
    }
    testerPresentWorker_ = std::thread(&AsyncUdsClient::testerPresentLoop, this);
}

void AsyncUdsClient::stopTesterPresent() {
    testerPresentEnabled_ = false;
    if (testerPresentWorker_.joinable()) {
        testerPresentWorker_.join();
    }
}

void AsyncUdsClient::shutdown() {
    stopTesterPresent();
    const bool wasStopping = stopping_.exchange(true);
    if (!wasStopping) {
        queueCv_.notify_all();
    }
    if (worker_.joinable()) {
        worker_.join();
    }
}

std::future<Result<std::vector<uint8_t>>> AsyncUdsClient::enqueue(WorkFn fn) {
    WorkItem item{std::move(fn), {}};
    auto future = item.promise.get_future();
    {
        std::scoped_lock lk(queueMtx_);
        queue_.push_back(std::move(item));
    }
    queueCv_.notify_one();
    return future;
}

Result<std::vector<uint8_t>> AsyncUdsClient::executeWithRetry(const WorkFn& fn) const {
    const auto deadline = std::chrono::steady_clock::now() + timing_.p2StarTimeout;
    for (uint8_t attempt = 0; attempt <= timing_.transportRetries; ++attempt) {
        auto result = fn();
        if (result.ok()) {
            return result;
        }
        if (result.err != DiagErr::Timeout || std::chrono::steady_clock::now() >= deadline) {
            return result;
        }
        std::this_thread::sleep_for(timing_.p2Timeout);
    }
    return Result<std::vector<uint8_t>>::failure(DiagErr::Timeout, "P2/P2* retry budget exhausted");
}

void AsyncUdsClient::workerLoop() {
    while (!stopping_) {
        WorkItem item;
        {
            std::unique_lock lk(queueMtx_);
            queueCv_.wait(lk, [this] { return stopping_ || !queue_.empty(); });
            if (stopping_ && queue_.empty()) {
                return;
            }
            item = std::move(queue_.front());
            queue_.pop_front();
        }
        item.promise.set_value(executeWithRetry(item.fn));
    }
}

void AsyncUdsClient::testerPresentLoop() {
    while (testerPresentEnabled_ && !stopping_) {
        std::this_thread::sleep_for(timing_.s3Interval);
        if (testerPresentEnabled_ && !stopping_) {
            (void)client_.testerPresent();
        }
    }
}

}  // namespace core::diag

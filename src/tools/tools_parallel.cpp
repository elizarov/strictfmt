#include "tools/tools_parallel.h"

#include <algorithm>
#include <charconv>
#include <condition_variable>
#include <deque>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <system_error>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <pthread.h>
#endif

namespace {

class ToolThread {
public:
#ifdef _WIN32
    explicit ToolThread(ToolWork work) : thread_(std::move(work)) {}
#else
    explicit ToolThread(ToolWork work) : work_(std::move(work)) {
        // Recursive parsing and layout need more than the small default pthread stack on macOS.
        // Match the standalone Windows executable's stack reservation.
        constexpr size_t kStackSize = 16 * 1024 * 1024;
        pthread_attr_t attributes;
        int error = pthread_attr_init(&attributes);
        if (error == 0) {
            error = pthread_attr_setstacksize(&attributes, kStackSize);
            if (error == 0) {
                error = pthread_create(
                    &thread_,
                    &attributes,
                    [](void* context) -> void* {
                        static_cast<ToolThread*>(context)->work_();
                        return nullptr;
                    },
                    this
                );
            }
            pthread_attr_destroy(&attributes);
        }
        if (error != 0) {
            throw std::system_error(error, std::generic_category(), "failed to start tool thread");
        }
        joinable_ = true;
    }
#endif

    ~ToolThread() { Join(); }
    ToolThread(const ToolThread&) = delete;
    ToolThread& operator=(const ToolThread&) = delete;

    void Join() {
#ifdef _WIN32
        if (thread_.joinable()) {
            thread_.join();
        }
#else
        if (joinable_) {
            pthread_join(thread_, nullptr);
            joinable_ = false;
        }
#endif
    }

private:
#ifdef _WIN32
    std::jthread thread_;
#else
    ToolWork work_;
    pthread_t thread_{};
    bool joinable_ = false;
#endif
};

}  // namespace

bool
    ParseToolConcurrency(std::string_view value, size_t& concurrency, std::string& error, std::string_view optionName)
{
    if (value.empty()) {
        error = std::string(optionName) + " requires a value";
        return false;
    }

    unsigned long long parsed = 0;
    const char* first = value.data();
    const char* last = value.data() + value.size();
    const std::from_chars_result result = std::from_chars(first, last, parsed);
    const bool outOfRange = parsed > static_cast<unsigned long long>(std::numeric_limits<size_t>::max());
    if (result.ec != std::errc() || result.ptr != last || parsed == 0 || outOfRange) {
        error = std::string(optionName) + " requires a positive integer";
        return false;
    }
    concurrency = static_cast<size_t>(parsed);
    return true;
}

size_t DefaultToolConcurrency() {
    const unsigned int hardware = std::thread::hardware_concurrency();
    return hardware == 0 ? 4 : static_cast<size_t>(hardware);
}

void RunToolParallel(
    size_t requestedConcurrency, ToolFileProgress* progress, const std::function<void(const ToolWorkSubmit&)>& produce
) {
    const size_t workerLimit = std::max<size_t>(
        1, requestedConcurrency == kToolAutoConcurrency ? DefaultToolConcurrency() : requestedConcurrency
    );
    constexpr size_t kQueueCapacity = 64;
    std::mutex mutex;
    std::condition_variable workAvailable;
    std::condition_variable spaceAvailable;
    std::condition_variable completion;
    std::deque<ToolWork> pending;
    size_t totalFiles = 0;
    size_t completedFiles = 0;
    bool discoveryDone = false;
    std::exception_ptr failure;
    const auto fail = [&](std::exception_ptr error) {
        {
            std::lock_guard lock(mutex);
            if (!failure) {
                failure = error;
            }
        }
        workAvailable.notify_all();
        spaceAvailable.notify_all();
        completion.notify_one();
    };
    const auto runWorker = [&]() {
        try {
            for (;;) {
                ToolWork work;
                {
                    std::unique_lock lock(mutex);
                    workAvailable.wait(lock, [&]() { return failure || discoveryDone || !pending.empty(); });
                    if (failure || pending.empty()) {
                        return;
                    }
                    work = std::move(pending.front());
                    pending.pop_front();
                }
                spaceAvailable.notify_one();
                work();
                {
                    std::lock_guard lock(mutex);
                    ++completedFiles;
                    if (discoveryDone && completedFiles == totalFiles) {
                        completion.notify_one();
                    }
                }
            }
        } catch (...) {
            fail(std::current_exception());
        }
    };
    std::vector<std::unique_ptr<ToolThread>> workers;
    if (progress != nullptr) {
        progress->Update(0, 0, true);
    }
    ToolThread discovery([&]() {
        try {
            produce([&](ToolWork work) {
                {
                    std::unique_lock lock(mutex);
                    spaceAvailable.wait(lock, [&]() { return failure || pending.size() < kQueueCapacity; });
                    if (failure) {
                        std::rethrow_exception(failure);
                    }
                    pending.push_back(std::move(work));
                    ++totalFiles;
                    if (workers.size() < workerLimit) {
                        workers.reserve(workers.size() + 1);
                        workers.push_back(std::make_unique<ToolThread>(runWorker));
                    }
                }
                workAvailable.notify_one();
            });
        } catch (...) {
            fail(std::current_exception());
        }
        {
            std::lock_guard lock(mutex);
            discoveryDone = true;
        }
        workAvailable.notify_all();
        completion.notify_one();
    });

    {
        std::unique_lock lock(mutex);
        const auto finished = [&]() { return discoveryDone && (failure || completedFiles == totalFiles); };
        while (!finished()) {
            completion.wait_for(lock, std::chrono::milliseconds(50), finished);
            const size_t completed = completedFiles;
            const size_t total = totalFiles;
            const bool scanning = !discoveryDone;
            lock.unlock();
            if (progress != nullptr) {
                progress->Update(completed, total, scanning);
            }
            lock.lock();
        }
    }
    discovery.Join();
    for (const auto& worker : workers) {
        worker->Join();
    }
    if (progress != nullptr) {
        progress->Finish();
    }
    if (failure) {
        std::rethrow_exception(failure);
    }
}

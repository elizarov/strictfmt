#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "tools/tools_progress.h"

constexpr size_t kToolAutoConcurrency = 0;

bool ParseToolConcurrency(
    std::string_view value, size_t& concurrency, std::string& error, std::string_view optionName = "--concurrency"
);
size_t DefaultToolConcurrency();
size_t EffectiveToolConcurrency(size_t workSize, size_t requestedConcurrency);

template <typename Worker>
void RunToolParallelFor(size_t workSize, size_t requestedConcurrency, ToolFileProgress* progress, Worker&& worker) {
    if (workSize == 0) {
        if (progress != nullptr) {
            progress->Finish(0);
        }
        return;
    }

    const size_t workerCount = EffectiveToolConcurrency(workSize, requestedConcurrency);
    if (workerCount <= 1) {
        for (size_t index = 0; index < workSize; ++index) {
            worker(index);
            if (progress != nullptr) {
                progress->Update(index + 1);
            }
        }
        if (progress != nullptr) {
            progress->Finish(workSize);
        }
        return;
    }

    std::atomic<size_t> nextIndex = 0;
    std::atomic<size_t> completedCount = 0;
    std::mutex completionMutex;
    std::condition_variable completionCondition;
    std::vector<std::thread> workers;
    workers.reserve(workerCount);
    for (size_t workerIndex = 0; workerIndex < workerCount; ++workerIndex) {
        workers.emplace_back([&]() {
            for (;;) {
                const size_t index = nextIndex.fetch_add(1, std::memory_order_relaxed);
                if (index >= workSize) {
                    return;
                }
                worker(index);
                const size_t completed = completedCount.fetch_add(1, std::memory_order_release) + 1;
                if (completed == workSize) {
                    // Synchronizing the final notification with the wait mutex prevents a lost wake between the
                    // completion predicate and wait. Earlier completions need no notification: progress refreshes
                    // on the existing bounded interval, while final completion wakes the caller immediately.
                    std::lock_guard lock(completionMutex);
                    completionCondition.notify_one();
                }
            }
        });
    }

    std::unique_lock completionLock(completionMutex);
    while (completedCount.load(std::memory_order_acquire) < workSize) {
        if (progress != nullptr) {
            progress->Update(completedCount.load(std::memory_order_acquire));
        }
        completionCondition.wait_for(completionLock, std::chrono::milliseconds(50), [&]() {
            return completedCount.load(std::memory_order_acquire) == workSize;
        });
    }
    completionLock.unlock();
    for (std::thread& thread : workers) {
        thread.join();
    }
    if (progress != nullptr) {
        progress->Finish(completedCount.load(std::memory_order_relaxed));
    }
}

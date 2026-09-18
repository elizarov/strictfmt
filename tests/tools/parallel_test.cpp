#include <array>
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <latch>
#include <stdexcept>
#include <string>

#include "tools/tools_parallel.h"

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void TestWorkStartsDuringDiscovery() {
    for (size_t concurrency : {1, 4}) {
        std::promise<void> firstCompleted;
        std::future<void> completion = firstCompleted.get_future();
        int finished = 0;
        RunToolParallel(concurrency, nullptr, [&](const ToolWorkSubmit& submit) {
            submit([&]() {
                ++finished;
                firstCompleted.set_value();
            });
            Check(
                completion.wait_for(std::chrono::seconds(10)) == std::future_status::ready,
                "work must finish before discovery can continue"
            );
            Check(finished == 1, "the first result must be visible during discovery");
            submit([&]() { ++finished; });
        });
        Check(finished == 2, "later discoveries must also finish");
    }
}

void TestConcurrencyAndQueueDrain() {
    for (size_t concurrency : {1, 4}) {
        std::array<std::atomic<int>, 1000> visits{};
        std::atomic<size_t> active = 0;
        std::atomic<bool> exceededLimit = false;
        std::latch firstWorkers(concurrency);
        RunToolParallel(concurrency, nullptr, [&](const ToolWorkSubmit& submit) {
            for (size_t index = 0; index < visits.size(); ++index) {
                submit([&, index]() {
                    if (active.fetch_add(1) >= concurrency) {
                        exceededLimit = true;
                    }
                    if (index < concurrency) {
                        firstWorkers.arrive_and_wait();
                    }
                    ++visits[index];
                    --active;
                });
            }
        });
        Check(!exceededLimit, "worker concurrency must stay within the requested limit");
        for (const auto& count : visits) {
            Check(count == 1, "every submitted task must execute exactly once");
        }
    }
    RunToolParallel(4, nullptr, [](const ToolWorkSubmit&) {});
}

void TestFailuresJoinWorkers() {
    for (bool workerFailure : {false, true}) {
        bool failed = false;
        try {
            RunToolParallel(1, nullptr, [&](const ToolWorkSubmit& submit) {
                if (!workerFailure) {
                    submit([]() {});
                    throw std::runtime_error("producer failure");
                }
                submit([]() { throw std::runtime_error("worker failure"); });
                for (int index = 0; index < 1000; ++index) {
                    submit([]() {});
                }
            });
        } catch (const std::runtime_error& error) {
            Check(
                error.what() == std::string(workerFailure ? "worker failure" : "producer failure"),
                "thread failures must propagate to the caller"
            );
            failed = true;
        }
        Check(failed, "thread failure must not be silently accepted");
    }
}

}  // namespace

int main() {
    try {
        TestWorkStartsDuringDiscovery();
        TestConcurrencyAndQueueDrain();
        TestFailuresJoinWorkers();
    } catch (const std::exception& error) {
        std::cerr << "Parallel work test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Passed streaming parallel work tests.\n";
}

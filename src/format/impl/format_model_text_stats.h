#pragma once

#include <chrono>

// Optional accumulated timings for the model-to-text pipeline and its advance analyses.
struct FormatModelTextStats {
    std::chrono::nanoseconds tokenize{};
    std::chrono::nanoseconds print{};
    std::chrono::nanoseconds breakModel{};
    std::chrono::nanoseconds solve{};
    std::chrono::nanoseconds emit{};
};

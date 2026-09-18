#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

#include "tools/tools_progress.h"

constexpr size_t kToolAutoConcurrency = 0;

bool ParseToolConcurrency(
    std::string_view value, size_t& concurrency, std::string& error, std::string_view optionName = "--concurrency"
);
size_t DefaultToolConcurrency();

using ToolWork = std::function<void()>;
using ToolWorkSubmit = std::function<void(ToolWork)>;

// The producer runs alongside workers and submits work through a bounded queue.
// This call returns after discovery and all submitted work finish, rethrowing any thread failure.
void RunToolParallel(
    size_t requestedConcurrency, ToolFileProgress* progress, const std::function<void(const ToolWorkSubmit&)>& produce
);

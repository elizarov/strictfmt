#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

struct FormatDiffResult {
    size_t changedLineCount = 0;
    std::string diff;
};

// Without a path, only count changes; do not materialize edits or unified diff text.
FormatDiffResult ComputeFormatDiff(
    std::string_view source,
    std::string_view formatted,
    std::optional<std::string_view> path = std::nullopt,
    size_t contextLines = 3
);

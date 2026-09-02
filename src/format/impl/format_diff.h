#pragma once

#include <cstddef>
#include <string>
#include <string_view>

std::string BuildUnifiedFormatDiff(
    std::string_view source, std::string_view formatted, std::string_view path, size_t contextLines = 3
);

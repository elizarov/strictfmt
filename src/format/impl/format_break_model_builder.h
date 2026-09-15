#pragma once

#include <span>

#include "format/impl/format_break_model.h"

// Builds a complete structural layout and fixes its break costs before returning.
// The result owns break nodes but borrows tokens and syntax, which must outlive it.
// An optional allocation resource must also outlive the result.
// Selection and spacing are scoped to this build; the syntax and other models stay immutable.
FormatBreakModel BuildFormatBreakModel(
    std::span<const PrintToken> tokens,
    FormatBreakWorkspace* workspace = nullptr,
    std::pmr::memory_resource* resource = nullptr
);

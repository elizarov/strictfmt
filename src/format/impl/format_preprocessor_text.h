#pragma once

#include <optional>
#include <string>
#include <string_view>

enum class FormatPreprocessorComma {
    Preserve,
    Remove,
    Add,
};

struct FormatPreprocessorTextPolicy {
    std::optional<int> payloadIndent;
    int indentWidth = 4;
    FormatPreprocessorComma terminalComma = FormatPreprocessorComma::Preserve;
};

// Canonicalizes directive spelling and comment spacing in an identified source
// region. With payloadIndent, preserves source lines, places directives at column
// zero, and indents payload lines; terminalComma then governs conditional branch
// endings. Enclosing syntax, mandatory breaks, and raw macro replacement layout
// remain the caller's responsibility.
std::string FormatPreprocessorText(std::string_view text, const FormatPreprocessorTextPolicy& policy = {});

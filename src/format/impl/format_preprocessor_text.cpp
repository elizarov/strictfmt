#include "format/impl/format_preprocessor_text.h"

#include <algorithm>
#include <vector>

#include "format/impl/format_model.h"
#include "format/impl/format_raw_macro.h"
#include "tools/tools_common.h"
#include "util/strings.h"

namespace {

bool IsPreprocessorDirectiveNameChar(char ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_';
}

std::string CanonicalizePreprocessorDirectiveLine(std::string_view line) {
    const SyntaxNodeKind directiveKind = SyntaxNodeKindFromPreprocessorDirectiveLine(line);
    if (!SyntaxNodeKindHasClass(directiveKind, SyntaxNodeClass::PreprocessorDirective)) {
        return std::string(line);
    }

    size_t cursor = 0;
    while (cursor < line.size() && (line[cursor] == ' ' || line[cursor] == '\t')) {
        ++cursor;
    }
    const size_t prefixEnd = cursor;
    if (cursor >= line.size() || line[cursor] != '#') {
        return std::string(line);
    }
    ++cursor;
    while (cursor < line.size() && (line[cursor] == ' ' || line[cursor] == '\t')) {
        ++cursor;
    }
    while (cursor < line.size() && IsPreprocessorDirectiveNameChar(line[cursor])) {
        ++cursor;
    }

    std::string result;
    result.reserve(line.size());
    result.append(line.data(), prefixEnd);
    result.append(SyntaxNodeKindTokenText(directiveKind));
    while (cursor < line.size() && (line[cursor] == ' ' || line[cursor] == '\t')) {
        ++cursor;
    }
    if (cursor < line.size()) {
        result.push_back(' ');
        result.append(line.data() + cursor, line.size() - cursor);
    }
    return NormalizeTrailingLineCommentSpacing(result);
}

std::string CanonicalizePreprocessorDirectiveLines(std::string_view text) {
    std::string result;
    size_t start = 0;
    while (start <= text.size()) {
        const size_t end = text.find('\n', start);
        const std::string_view line = end == std::string::npos ? text.substr(start) : text.substr(start, end - start);
        if (!result.empty()) {
            result.push_back('\n');
        }
        result.append(CanonicalizePreprocessorDirectiveLine(line));
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return result;
}

std::string FormatPayloadLines(std::string_view text, const FormatPreprocessorTextPolicy& policy) {
    const std::string normalized = PreserveSourceLines(text);
    std::vector<std::string> lines;
    size_t start = 0;
    while (start <= normalized.size()) {
        const size_t end = normalized.find('\n', start);
        const std::string_view rawLine = end == std::string::npos ? std::string_view(normalized).substr(start) :
            std::string_view(normalized).substr(start, end - start);
        const std::string line = NormalizeTrailingLineCommentSpacing(TrimWhitespaceView(rawLine));
        lines.push_back(CanonicalizePreprocessorDirectiveLine(line));
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    std::string result;
    for (const std::string& line : lines) {
        if (!result.empty()) {
            result.push_back('\n');
        }
        if (!line.empty() && line.front() != '#') {
            result.append(static_cast<size_t>(std::max(0, *policy.payloadIndent) * policy.indentWidth), ' ');
        }
        result.append(line);
    }
    return result;
}

}  // namespace

std::string FormatPreprocessorText(std::string_view text, const FormatPreprocessorTextPolicy& policy) {
    if (policy.payloadIndent) {
        return FormatPayloadLines(text, policy);
    }
    const bool hasLineBreak = text.find_first_of("\r\n") != std::string_view::npos;
    return CanonicalizePreprocessorDirectiveLines(
        hasLineBreak ? PreservePreprocessorLines(text) :
            NormalizeTrailingLineCommentSpacing(CollapseSourceWhitespace(text))
    );
}

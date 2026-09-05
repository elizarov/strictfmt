#include "format/impl/format_string_literals.h"

#include <optional>
#include <utility>

namespace {

bool EndsWithEscapedLineFragment(std::string_view text) {
    const size_t quote = text.rfind('"');
    if (quote == std::string_view::npos || quote < 2 || text[quote - 1] != 'n') {
        return false;
    }
    size_t backslashCount = 0;
    for (size_t index = quote - 1; index > 0 && text[index - 1] == '\\'; --index) {
        ++backslashCount;
    }
    return backslashCount % 2 == 1;
}

struct OrdinaryStringLiteralParts {
    std::string_view prefix;
    std::string_view body;
    std::string_view suffix;
};

bool IsOrdinaryStringPrefix(std::string_view prefix) {
    return prefix.empty() || prefix == "L" || prefix == "u8" || prefix == "u" || prefix == "U";
}

std::optional<OrdinaryStringLiteralParts> ParseOrdinaryStringLiteral(std::string_view text) {
    if (text.find_first_of("\r\n") != std::string_view::npos) {
        return std::nullopt;
    }
    const size_t open = text.find('"');
    const size_t close = text.rfind('"');
    if (open == std::string_view::npos || close == open || !IsOrdinaryStringPrefix(text.substr(0, open))) {
        return std::nullopt;
    }
    return OrdinaryStringLiteralParts{
        .prefix = text.substr(0, open),
        .body = text.substr(open + 1, close - open - 1),
        .suffix = text.substr(close + 1),
    };
}

std::optional<std::string_view> JoinStringAffix(std::string_view left, std::string_view right) {
    if (left == right || right.empty()) {
        return left;
    }
    if (left.empty()) {
        return right;
    }
    return std::nullopt;
}

bool IsHexDigit(char value) {
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') || (value >= 'A' && value <= 'F');
}

bool IsOctalDigit(char value) { return value >= '0' && value <= '7'; }

bool IsActiveEscapeBackslash(std::string_view body, size_t position) {
    size_t begin = position;
    while (begin > 0 && body[begin - 1] == '\\') {
        --begin;
    }
    return (position - begin + 1) % 2 == 1;
}

bool StringJoinWouldExtendEscape(std::string_view leftBody, std::string_view rightBody) {
    if (leftBody.empty() || rightBody.empty()) {
        return false;
    }
    size_t hexBegin = leftBody.size();
    while (hexBegin > 0 && IsHexDigit(leftBody[hexBegin - 1])) {
        --hexBegin;
    }
    if (
        IsHexDigit(rightBody.front()) &&
        hexBegin >= 2 &&
        leftBody[hexBegin - 1] == 'x' &&
        leftBody[hexBegin - 2] == '\\' &&
        IsActiveEscapeBackslash(leftBody, hexBegin - 2)
    ) {
        return true;
    }

    size_t octalBegin = leftBody.size();
    while (octalBegin > 0 && IsOctalDigit(leftBody[octalBegin - 1])) {
        --octalBegin;
    }
    const size_t octalLength = leftBody.size() - octalBegin;
    return IsOctalDigit(rightBody.front()) &&
        octalLength > 0 &&
        octalLength < 3 &&
        octalBegin > 0 &&
        leftBody[octalBegin - 1] == '\\' &&
        IsActiveEscapeBackslash(leftBody, octalBegin - 1);
}

std::optional<std::string> JoinOrdinaryStringLiterals(std::string_view left, std::string_view right) {
    const std::optional<OrdinaryStringLiteralParts> leftParts = ParseOrdinaryStringLiteral(left);
    const std::optional<OrdinaryStringLiteralParts> rightParts = ParseOrdinaryStringLiteral(right);
    if (!leftParts || !rightParts || StringJoinWouldExtendEscape(leftParts->body, rightParts->body)) {
        return std::nullopt;
    }
    const std::optional<std::string_view> prefix = JoinStringAffix(leftParts->prefix, rightParts->prefix);
    const std::optional<std::string_view> suffix = JoinStringAffix(leftParts->suffix, rightParts->suffix);
    if (!prefix || !suffix) {
        return std::nullopt;
    }
    std::string result;
    result.reserve(prefix->size() + leftParts->body.size() + rightParts->body.size() + suffix->size() + 2);
    result.append(*prefix);
    result.push_back('"');
    result.append(leftParts->body);
    result.append(rightParts->body);
    result.push_back('"');
    result.append(*suffix);
    return result;
}

}  // namespace

FormatAdjacentStrings AnalyzeAdjacentStrings(std::span<const std::string_view> spellings) {
    FormatAdjacentStrings result;
    result.compactSpellings.reserve(spellings.size());
    size_t compactRunStart = 0;
    for (size_t index = 0; index < spellings.size(); ++index) {
        const std::string_view text = spellings[index];
        if (index + 1 < spellings.size() && EndsWithEscapedLineFragment(text)) {
            result.requiresSplit = true;
        }
        if (result.compactSpellings.empty()) {
            result.compactSpellings.emplace_back(text);
            continue;
        }
        if (
            std::optional<std::string> joined =
                JoinOrdinaryStringLiterals(result.compactSpellings[compactRunStart], text)
        ) {
            result.compactSpellings[compactRunStart] = std::move(*joined);
            result.compactSpellings.emplace_back();
        } else {
            compactRunStart = result.compactSpellings.size();
            result.compactSpellings.emplace_back(text);
        }
    }
    return result;
}

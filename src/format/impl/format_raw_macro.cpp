#include "format/impl/format_raw_macro.h"

#include <algorithm>
#include <limits>

#include "tools/tools_common.h"

namespace {

bool IsNewline(char ch) { return ch == '\r' || ch == '\n'; }

bool StartsWithHorizontalSpace(std::string_view text) {
    return !text.empty() && (text.front() == ' ' || text.front() == '\t');
}

bool IsIdentifierCharacter(char ch) {
    return (ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') ||
        ch == '_' ||
        static_cast<unsigned char>(ch) >= 0x80;
}

size_t PreprocessingNumberEnd(std::string_view text, size_t start) {
    if (start > 0 && IsIdentifierCharacter(text[start - 1])) {
        return start;
    }
    size_t digit = start + (text[start] == '.' ? 1 : 0);
    if (digit >= text.size() || text[digit] < '0' || text[digit] > '9') {
        return start;
    }
    size_t end = digit + 1;
    while (end < text.size()) {
        const char ch = text[end];
        const char previous = text[end - 1];
        if (
            !IsIdentifierCharacter(ch) &&
            ch != '.' &&
            ch != '\'' &&
            !((ch == '+' || ch == '-') && (previous == 'e' || previous == 'E' || previous == 'p' || previous == 'P'))
        ) {
            break;
        }
        ++end;
    }
    return end;
}

struct RawMacroTextInfo {
    bool preserveIndentation = false;
    std::vector<size_t> continuations;
};

bool IsHorizontalSpace(char ch) { return ch == ' ' || ch == '\t' || ch == '\f' || ch == '\v'; }

bool SpliceMayJoinTokens(std::string_view text, size_t splice, size_t next) {
    if (splice == 0 || next == text.size()) {
        return false;
    }
    const char left = text[splice - 1];
    const char right = text[next];
    if (IsHorizontalSpace(left) || IsNewline(left) || IsHorizontalSpace(right) || IsNewline(right)) {
        return false;
    }
    if (
        (IsIdentifierCharacter(left) && (IsIdentifierCharacter(right) || right == '"' || right == '\'')) ||
        ((left == '"' || left == '\'') && IsIdentifierCharacter(right)) ||
        (left == '.' && IsIdentifierCharacter(right)) ||
        ((left >= '0' && left <= '9') && right == '.') ||
        ((left == 'e' || left == 'E' || left == 'p' || left == 'P') && (right == '+' || right == '-'))
    ) {
        return true;
    }
    for (std::string_view token : {
        "##",
        "::",
        ".*",
        "->",
        "->*",
        "...",
        "++",
        "--",
        "<<",
        ">>",
        "<=>",
        "<=",
        ">=",
        "==",
        "!=",
        "&&",
        "||",
        "*=",
        "/=",
        "%=",
        "+=",
        "-=",
        "<<=",
        ">>=",
        "&=",
        "^=",
        "|=",
        "<:",
        ":>",
        "<%",
        "%>",
        "%:",
        "%:%:",
        "//",
        "/*",
        "*/",
        "[[",
        "]]",
    }) {
        for (size_t split = 1; split < token.size(); ++split) {
            if (
                text.substr(0, splice).ends_with(token.substr(0, split)) &&
                text.substr(next).starts_with(token.substr(split))
            ) {
                return true;
            }
        }
    }
    return false;
}

size_t RawStringLiteralEnd(std::string_view text, size_t quote) {
    size_t start = quote;
    while (start > 0 && IsIdentifierCharacter(text[start - 1])) {
        --start;
    }
    const std::string_view prefix = text.substr(start, quote - start);
    if (prefix != "R" && prefix != "u8R" && prefix != "uR" && prefix != "UR" && prefix != "LR") {
        return quote;
    }
    const size_t open = text.find('(', quote + 1);
    if (open == std::string_view::npos) {
        return text.size();
    }
    const std::string close = ")" + std::string(text.substr(quote + 1, open - quote - 1)) + "\"";
    const size_t end = text.find(close, open + 1);
    return end == std::string_view::npos ? text.size() : end + close.size();
}

RawMacroTextInfo InspectRawMacroText(std::string_view text) {
    RawMacroTextInfo result;
    bool blockComment = false;
    bool lineComment = false;
    for (size_t index = 0; index < text.size(); ++index) {
        if (text[index] == '\\' && index + 1 < text.size() && IsNewline(text[index + 1])) {
            const size_t next =
                index + (text[index + 1] == '\r' && index + 2 < text.size() && text[index + 2] == '\n' ? 3 : 2);
            if (SpliceMayJoinTokens(text, index, next)) {
                result.preserveIndentation = true;
            } else {
                result.continuations.push_back(index);
            }
            index = next - 1;
            continue;
        }
        if (lineComment) {
            lineComment = !IsNewline(text[index]);
            continue;
        }
        if (blockComment) {
            if (text.substr(index).starts_with("*/")) {
                blockComment = false;
                ++index;
            }
            continue;
        }
        if (text.substr(index).starts_with("/*")) {
            blockComment = true;
            ++index;
            continue;
        }
        if (text.substr(index).starts_with("//")) {
            lineComment = true;
            ++index;
            continue;
        }
        const size_t numberEnd = PreprocessingNumberEnd(text, index);
        if (numberEnd != index) {
            index = numberEnd - 1;
            continue;
        }
        const char quote = text[index];
        if (quote != '"' && quote != '\'') {
            continue;
        }
        if (quote == '"') {
            const size_t rawEnd = RawStringLiteralEnd(text, index);
            if (rawEnd != index) {
                result.preserveIndentation = true;
                index = rawEnd - 1;
                continue;
            }
        }
        while (++index < text.size() && text[index] != quote) {
            if (text[index] == '\\' && index + 1 < text.size()) {
                if (IsNewline(text[index + 1])) {
                    result.preserveIndentation = true;
                }
                ++index;
            }
        }
    }
    return result;
}

struct SourceIndent {
    size_t length = 0;
    int columns = 0;
};

SourceIndent MeasureSourceIndent(std::string_view line, int tabWidth) {
    SourceIndent result;
    while (result.length < line.size()) {
        if (line[result.length] == ' ') {
            ++result.columns;
        } else if (line[result.length] == '\t') {
            result.columns += tabWidth - result.columns % tabWidth;
        } else {
            break;
        }
        ++result.length;
    }
    return result;
}

std::string ReindentRawMacroBody(std::string_view text, int bodyIndentLevel, int indentWidth, int tabWidth) {
    const size_t firstLineEnd = text.find('\n');
    if (firstLineEnd == std::string_view::npos) {
        return std::string(text);
    }

    int commonIndent = std::numeric_limits<int>::max();
    size_t lineStart = firstLineEnd + 1;
    while (lineStart <= text.size()) {
        const size_t lineEnd = text.find('\n', lineStart);
        const std::string_view line =
            lineEnd == std::string_view::npos ? text.substr(lineStart) : text.substr(lineStart, lineEnd - lineStart);
        const SourceIndent indent = MeasureSourceIndent(line, tabWidth);
        if (indent.length < line.size()) {
            commonIndent = std::min(commonIndent, indent.columns);
        }
        if (lineEnd == std::string_view::npos) {
            break;
        }
        lineStart = lineEnd + 1;
    }
    if (commonIndent == std::numeric_limits<int>::max()) {
        commonIndent = 0;
    }

    std::string result;
    result.reserve(text.size());
    result.append(text.substr(0, firstLineEnd));
    const int bodyIndent = std::max(0, bodyIndentLevel) * std::max(1, indentWidth);
    lineStart = firstLineEnd + 1;
    while (lineStart <= text.size()) {
        const size_t lineEnd = text.find('\n', lineStart);
        const std::string_view line =
            lineEnd == std::string_view::npos ? text.substr(lineStart) : text.substr(lineStart, lineEnd - lineStart);
        const SourceIndent indent = MeasureSourceIndent(line, tabWidth);
        result.push_back('\n');
        if (indent.length < line.size()) {
            result.append(static_cast<size_t>(bodyIndent + std::max(0, indent.columns - commonIndent)), ' ');
            result.append(line.substr(indent.length));
        }
        if (lineEnd == std::string_view::npos) {
            break;
        }
        lineStart = lineEnd + 1;
    }
    return result;
}

}  // namespace

std::string CollapseSourceWhitespace(std::string_view text) {
    std::string result;
    bool pendingSpace = false;
    bool inString = false;
    bool inChar = false;
    for (size_t index = 0; index < text.size(); ++index) {
        const char ch = text[index];
        const char next = index + 1 < text.size() ? text[index + 1] : '\0';
        if (!inString && !inChar && ch == '\\' && IsNewline(next)) {
            pendingSpace = true;
            ++index;
            if (next == '\r' && index + 1 < text.size() && text[index + 1] == '\n') {
                ++index;
            }
            continue;
        }
        if (!inString && !inChar && (ch == ' ' || ch == '\t' || IsNewline(ch))) {
            pendingSpace = true;
            if (ch == '\r' && next == '\n') {
                ++index;
            }
            continue;
        }
        if (pendingSpace && !result.empty()) {
            result.push_back(' ');
        }
        pendingSpace = false;
        if (!inString && !inChar) {
            const size_t numberEnd = PreprocessingNumberEnd(text, index);
            if (numberEnd != index) {
                result.append(text.substr(index, numberEnd - index));
                index = numberEnd - 1;
                continue;
            }
        }
        result.push_back(ch);
        if (ch == '\\' && (inString || inChar) && index + 1 < text.size()) {
            result.push_back(text[index + 1]);
            ++index;
            continue;
        }
        if (ch == '"' && !inChar) {
            inString = !inString;
        } else if (ch == '\'' && !inString) {
            inChar = !inChar;
        }
    }
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    return result;
}

std::string PreserveSourceLines(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (size_t index = 0; index < text.size(); ++index) {
        const char ch = text[index];
        if (ch == '\r') {
            if (index + 1 < text.size() && text[index + 1] == '\n') {
                ++index;
            }
            result.push_back('\n');
        } else {
            result.push_back(ch);
        }
    }
    while (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}

std::string PreservePreprocessorLines(std::string_view text) {
    const std::string normalized = PreserveSourceLines(text);
    std::string result;
    size_t start = 0;
    while (start <= normalized.size()) {
        const size_t end = normalized.find('\n', start);
        const std::string_view line = end == std::string::npos ? std::string_view(normalized).substr(start) :
            std::string_view(normalized).substr(start, end - start);
        if (!result.empty()) {
            result.push_back('\n');
        }
        result.append(NormalizeTrailingLineCommentSpacing(line));
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return result;
}

namespace {

std::string NormalizeRawMacroReplacement(std::string_view text, int bodyIndentLevel, int indentWidth, int tabWidth) {
    // A final splice belongs to this definition, not to the following source line.
    // Drop empty continuation lines before the line-preserving formatter trims newlines.
    while (!text.empty()) {
        const size_t end = text.find_last_not_of(" \t\f\v\r\n");
        if (end == std::string_view::npos) {
            return {};
        }
        const bool trailingSplice = text[end] == '\\' && end + 1 < text.size() && IsNewline(text[end + 1]);
        text = text.substr(0, end + (trailingSplice ? 0 : 1));
        if (!trailingSplice) {
            break;
        }
    }
    if (InspectRawMacroText(text).preserveIndentation) {
        return PreserveSourceLines(text);
    }
    if (text.find_first_of("\r\n") != std::string_view::npos) {
        return
            ReindentRawMacroBody(PreservePreprocessorLines(text), bodyIndentLevel, indentWidth, std::max(1, tabWidth));
    }
    std::string collapsed = NormalizeTrailingLineCommentSpacing(CollapseSourceWhitespace(text));
    if (!collapsed.empty() && StartsWithHorizontalSpace(text)) {
        collapsed.insert(collapsed.begin(), ' ');
    }
    return collapsed;
}

}  // namespace

RawMacroLayout FormatRawMacroReplacement(std::string_view text, int bodyIndentLevel, int indentWidth, int tabWidth) {
    const std::string normalized = NormalizeRawMacroReplacement(text, bodyIndentLevel, indentWidth, tabWidth);
    RawMacroLayout result;
    size_t start = 0;
    for (size_t continuation : InspectRawMacroText(normalized).continuations) {
        std::string_view before = std::string_view(normalized).substr(start, continuation - start);
        while (!before.empty() && IsHorizontalSpace(before.back())) {
            before.remove_suffix(1);
        }
        result.text.append(before);
        result.text.push_back(' ');
        result.continuations.push_back(result.text.size());
        result.text.append("\\\n");
        start = continuation + 2;
    }
    result.text.append(normalized, start);
    return result;
}

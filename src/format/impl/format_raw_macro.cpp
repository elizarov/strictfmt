#include "format/impl/format_raw_macro.h"

#include <algorithm>
#include <limits>

#include "tools/tools_common.h"

namespace {

bool IsNewline(char ch) {
    return ch == '\r' || ch == '\n';
}

bool StartsWithHorizontalSpace(std::string_view text) {
    return !text.empty() && (text.front() == ' ' || text.front() == '\t');
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

std::string FormatRawMacroReplacement(std::string_view text, int bodyIndentLevel, int indentWidth, int tabWidth) {
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

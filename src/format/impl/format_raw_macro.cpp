#include "format/impl/format_raw_macro.h"

#include "tools/tools_common.h"

namespace {

bool IsNewline(char ch) {
    return ch == '\r' || ch == '\n';
}

bool StartsWithHorizontalSpace(std::string_view text) {
    return !text.empty() && (text.front() == ' ' || text.front() == '\t');
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

std::string FormatRawMacroReplacement(std::string_view text) {
    if (text.find_first_of("\r\n") != std::string_view::npos) {
        return PreservePreprocessorLines(text);
    }
    std::string collapsed = NormalizeTrailingLineCommentSpacing(CollapseSourceWhitespace(text));
    if (!collapsed.empty() && StartsWithHorizontalSpace(text)) {
        collapsed.insert(collapsed.begin(), ' ');
    }
    return collapsed;
}

#pragma once

#include <string>
#include <string_view>
#include <vector>

struct RawMacroLayout {
    std::string text;
    std::vector<size_t> continuations;
};

bool IsFormattingControlComment(std::string_view text);
// Removes control comments from raw preprocessing text without joining surrounding tokens.
std::string RemoveFormattingControlComments(std::string_view text);

std::string CollapseSourceWhitespace(std::string_view text, int tabWidth = 4);
std::string PreserveSourceLines(std::string_view text);
// Cleans physical line whitespace outside literals, including inside comments.
std::string NormalizeSourceLineWhitespace(std::string_view text, int tabWidth);
std::string PreservePreprocessorLines(std::string_view text, int tabWidth = 4);
RawMacroLayout FormatRawMacroReplacement(std::string_view text, int bodyIndentLevel, int indentWidth, int tabWidth);

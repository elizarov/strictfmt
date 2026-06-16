#pragma once

#include <string>
#include <string_view>

std::string CollapseSourceWhitespace(std::string_view text);
std::string PreserveSourceLines(std::string_view text);
std::string PreservePreprocessorLines(std::string_view text);
std::string FormatRawMacroReplacement(std::string_view text);

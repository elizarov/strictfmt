#pragma once

#include <string_view>

// Counts extended grapheme clusters; malformed bytes are isolated characters.
int Utf8CharacterCount(std::string_view text);

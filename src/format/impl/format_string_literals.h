#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

struct FormatAdjacentStrings {
    std::vector<std::string> compactSpellings;
    bool requiresSplit = false;
};

struct FormatStringPart {
    std::string_view text;
    bool startsSourceLine = false;
    bool isComment = false;
};

// Analyzes a syntax-identified adjacent string run, preserving same-line boundaries,
// comments, prefixes, suffixes, and escape meaning. Returns owned compact spellings
// in input order; an empty slot is absorbed into the preceding run. The builder owns grouping/depth, while
// solving and emission share these spellings and the required-split fact.
FormatAdjacentStrings AnalyzeAdjacentStrings(std::span<const FormatStringPart> parts);

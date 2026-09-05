#include "util/utf8.h"

#include <cstddef>
#include <cstdint>
#include <iterator>

namespace {

enum class GraphemeBreak : unsigned char {
    Other,
    CR,
    LF,
    Control,
    Extend,
    ZWJ,
    RegionalIndicator,
    Prepend,
    SpacingMark,
    L,
    V,
    T,
    LV,
    LVT,
};

enum class IndicConjunct : unsigned char {
    None,
    Consonant,
    Extend,
    Linker,
};

struct GraphemeProperties {
    GraphemeBreak breaking = GraphemeBreak::Other;
    IndicConjunct conjunct = IndicConjunct::None;
    bool pictographic = false;
};

struct GraphemeRange {
    std::uint32_t first;
    std::uint32_t last;
    GraphemeProperties properties;
};

#include "grapheme_data.inc"

GraphemeProperties Properties(std::uint32_t codepoint) {
    if (codepoint < 0x80) {
        if (codepoint == '\r') {
            return {GraphemeBreak::CR};
        }
        if (codepoint == '\n') {
            return {GraphemeBreak::LF};
        }
        return {codepoint < 0x20 || codepoint == 0x7f ? GraphemeBreak::Control : GraphemeBreak::Other};
    }
    if (codepoint > 0x10ffff) {
        return {GraphemeBreak::Control};
    }
    size_t begin = 0;
    size_t end = std::size(kGraphemeRanges);
    while (begin < end) {
        const size_t middle = begin + (end - begin) / 2;
        const GraphemeRange& range = kGraphemeRanges[middle];
        if (codepoint < range.first) {
            end = middle;
        } else if (codepoint > range.last) {
            begin = middle + 1;
        } else {
            return range.properties;
        }
    }
    return {};
}

size_t CharacterSize(std::string_view text) {
    const auto first = static_cast<unsigned char>(text.front());
    size_t size = 1;
    if (first >= 0xc2 && first <= 0xdf) {
        size = 2;
    } else if (first >= 0xe0 && first <= 0xef) {
        size = 3;
    } else if (first >= 0xf0 && first <= 0xf4) {
        size = 4;
    }
    if (size > text.size()) {
        return 1;
    }
    for (size_t index = 1; index < size; ++index) {
        const auto byte = static_cast<unsigned char>(text[index]);
        if (byte < 0x80 || byte > 0xbf) {
            return 1;
        }
    }
    if (size >= 3) {
        const auto second = static_cast<unsigned char>(text[1]);
        if (
            (first == 0xe0 && second < 0xa0) ||
            (first == 0xed && second >= 0xa0) ||
            (first == 0xf0 && second < 0x90) ||
            (first == 0xf4 && second >= 0x90)
        ) {
            return 1;
        }
    }
    return size;
}

std::uint32_t ReadCodePoint(std::string_view& text) {
    const size_t size = CharacterSize(text);
    auto value = static_cast<unsigned char>(text.front());
    std::uint32_t codepoint = size == 1 ? (value < 0x80 ? value : 0x110000) : value & (0x7f >> size);
    for (size_t index = 1; index < size; ++index) {
        codepoint = (codepoint << 6) | (static_cast<unsigned char>(text[index]) & 0x3f);
    }
    text.remove_prefix(size);
    return codepoint;
}

bool IsControl(GraphemeBreak breaking) {
    return breaking == GraphemeBreak::Control || breaking == GraphemeBreak::CR || breaking == GraphemeBreak::LF;
}

struct GraphemeState {
    GraphemeBreak previous = GraphemeBreak::Control;
    bool oddRegionalIndicators = false;
    bool pictographicExtend = false;
    bool pictographicZwj = false;
    bool indicConsonant = false;
    bool indicLinker = false;

    bool BreakBefore(GraphemeProperties next) const {
        const GraphemeBreak current = next.breaking;
        if (previous == GraphemeBreak::CR && current == GraphemeBreak::LF) {  // GB3
            return false;
        }
        if (IsControl(previous) || IsControl(current)) {  // GB4–GB5
            return true;
        }
        if (previous == GraphemeBreak::L && (
            current == GraphemeBreak::L ||
            current == GraphemeBreak::V ||
            current == GraphemeBreak::LV ||
            current == GraphemeBreak::LVT
        )) {  // GB6
            return false;
        }
        if (
            (previous == GraphemeBreak::LV || previous == GraphemeBreak::V) &&
            (current == GraphemeBreak::V || current == GraphemeBreak::T)
        ) {  // GB7
            return false;
        }
        if ((previous == GraphemeBreak::LVT || previous == GraphemeBreak::T) && current == GraphemeBreak::T) {  // GB8
            return false;
        }
        if (
            current == GraphemeBreak::Extend ||
            current == GraphemeBreak::ZWJ ||
            current == GraphemeBreak::SpacingMark ||
            previous == GraphemeBreak::Prepend
        ) {  // GB9–GB9b
            return false;
        }
        if (indicLinker && next.conjunct == IndicConjunct::Consonant) {  // GB9c
            return false;
        }
        if (pictographicZwj && next.pictographic) {  // GB11
            return false;
        }
        return !(
            previous == GraphemeBreak::RegionalIndicator &&
            current == GraphemeBreak::RegionalIndicator &&
            oddRegionalIndicators
        );  // GB12–GB13, GB999
    }

    void Advance(GraphemeProperties next) {
        oddRegionalIndicators = next.breaking == GraphemeBreak::RegionalIndicator && !oddRegionalIndicators;
        pictographicZwj = pictographicExtend && next.breaking == GraphemeBreak::ZWJ;
        pictographicExtend = next.pictographic || (pictographicExtend && next.breaking == GraphemeBreak::Extend);
        if (next.conjunct == IndicConjunct::Consonant) {
            indicConsonant = true;
            indicLinker = false;
        } else if (next.conjunct == IndicConjunct::Linker) {
            indicLinker = indicConsonant;
        } else if (next.conjunct != IndicConjunct::Extend) {
            indicConsonant = false;
            indicLinker = false;
        }
        previous = next.breaking;
    }
};

int CountGraphemes(std::string_view text) {
    int count = 0;
    GraphemeState state;
    while (!text.empty()) {
        const GraphemeProperties properties = Properties(ReadCodePoint(text));
        if (state.BreakBefore(properties)) {
            ++count;
        }
        state.Advance(properties);
    }
    return count;
}

}  // namespace

int Utf8CharacterCount(std::string_view text) {
    // ASCII has one cluster per byte except CR/LF pairs. Check the entire text:
    // a following non-ASCII combining mark can still join its ASCII predecessor.
    for (unsigned char byte : text) {
        if (byte >= 0x80 || byte == '\r') {
            return CountGraphemes(text);
        }
    }
    return static_cast<int>(text.size());
}

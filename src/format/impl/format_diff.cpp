#include "format/impl/format_diff.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

enum class DiffLineEnding {
    None,
    LineFeed,
    CarriageReturn,
    CarriageReturnLineFeed,
};

struct DiffLine {
    std::string_view text;
    DiffLineEnding ending = DiffLineEnding::None;

    bool operator==(const DiffLine&) const = default;
};

struct DiffLineHash {
    size_t operator()(const DiffLine& line) const {
        const size_t textHash = std::hash<std::string_view>{}(line.text);
        const size_t endingHash = static_cast<size_t>(line.ending);
        return textHash ^ (endingHash + 0x9e3779b9U + (textHash << 6U) + (textHash >> 2U));
    }
};

enum class DiffEditKind {
    Equal,
    Remove,
    Add,
};

struct DiffEdit {
    DiffEditKind kind;
    DiffLine line;
};

using DiffLinePositions = std::unordered_map<DiffLine, std::vector<size_t>, DiffLineHash>;

std::vector<DiffLine> SplitDiffLines(std::string_view text) {
    std::vector<DiffLine> lines;
    size_t start = 0;
    while (start < text.size()) {
        size_t end = start;
        while (end < text.size() && text[end] != '\r' && text[end] != '\n') {
            ++end;
        }
        DiffLineEnding ending = DiffLineEnding::None;
        size_t next = end;
        if (end < text.size()) {
            if (text[end] == '\r' && end + 1 < text.size() && text[end + 1] == '\n') {
                ending = DiffLineEnding::CarriageReturnLineFeed;
                next += 2;
            } else {
                ending = text[end] == '\r' ? DiffLineEnding::CarriageReturn : DiffLineEnding::LineFeed;
                ++next;
            }
        }
        lines.push_back({text.substr(start, end - start), ending});
        start = next;
    }
    return lines;
}

DiffLinePositions IndexLinePositions(const std::vector<DiffLine>& lines) {
    DiffLinePositions positions;
    positions.reserve(lines.size());
    for (size_t index = 0; index < lines.size(); ++index) {
        positions[lines[index]].push_back(index);
    }
    return positions;
}

std::optional<size_t> FindPosition(const DiffLinePositions& positions, const DiffLine& line, size_t minimumPosition) {
    const auto found = positions.find(line);
    if (found == positions.end()) {
        return std::nullopt;
    }
    const auto position = std::lower_bound(found->second.begin(), found->second.end(), minimumPosition);
    if (position == found->second.end()) {
        return std::nullopt;
    }
    return *position;
}

std::optional<std::pair<size_t, size_t>> FindNearbySynchronization(
    const std::vector<DiffLine>& source,
    size_t sourceIndex,
    const std::vector<DiffLine>& formatted,
    size_t formattedIndex
) {
    constexpr size_t kLookahead = 32;
    for (size_t distance = 1; distance <= kLookahead * 2; ++distance) {
        const size_t firstSourceAdvance = distance > kLookahead ? distance - kLookahead : 0;
        const size_t lastSourceAdvance = std::min(distance, kLookahead);
        for (size_t sourceAdvance = firstSourceAdvance; sourceAdvance <= lastSourceAdvance; ++sourceAdvance) {
            const size_t formattedAdvance = distance - sourceAdvance;
            if (
                sourceIndex + sourceAdvance < source.size() &&
                formattedIndex + formattedAdvance < formatted.size() &&
                source[sourceIndex + sourceAdvance] == formatted[formattedIndex + formattedAdvance]
            ) {
                return std::pair{sourceAdvance, formattedAdvance};
            }
        }
    }
    return std::nullopt;
}

void AppendRemovals(std::vector<DiffEdit>& edits, const std::vector<DiffLine>& lines, size_t& index, size_t count) {
    for (size_t end = index + count; index < end; ++index) {
        edits.push_back({DiffEditKind::Remove, lines[index]});
    }
}

void AppendAdditions(std::vector<DiffEdit>& edits, const std::vector<DiffLine>& lines, size_t& index, size_t count) {
    for (size_t end = index + count; index < end; ++index) {
        edits.push_back({DiffEditKind::Add, lines[index]});
    }
}

std::vector<DiffEdit> BuildGreedyEdits(const std::vector<DiffLine>& source, const std::vector<DiffLine>& formatted) {
    const DiffLinePositions sourcePositions = IndexLinePositions(source);
    const DiffLinePositions formattedPositions = IndexLinePositions(formatted);
    std::vector<DiffEdit> edits;
    edits.reserve(source.size() + formatted.size());
    size_t sourceIndex = 0;
    size_t formattedIndex = 0;
    while (sourceIndex < source.size() && formattedIndex < formatted.size()) {
        if (source[sourceIndex] == formatted[formattedIndex]) {
            edits.push_back({DiffEditKind::Equal, source[sourceIndex]});
            ++sourceIndex;
            ++formattedIndex;
            continue;
        }

        std::optional<std::pair<size_t, size_t>> synchronization =
            FindNearbySynchronization(source, sourceIndex, formatted, formattedIndex);
        if (!synchronization) {
            const std::optional<size_t> nextFormatted =
                FindPosition(formattedPositions, source[sourceIndex], formattedIndex + 1);
            const std::optional<size_t> nextSource =
                FindPosition(sourcePositions, formatted[formattedIndex], sourceIndex + 1);
            if (nextFormatted && (!nextSource || *nextFormatted - formattedIndex <= *nextSource - sourceIndex)) {
                synchronization = std::pair{size_t{0}, *nextFormatted - formattedIndex};
            } else if (nextSource) {
                synchronization = std::pair{*nextSource - sourceIndex, size_t{0}};
            }
        }
        if (!synchronization) {
            synchronization = std::pair{size_t{1}, size_t{1}};
        }
        AppendRemovals(edits, source, sourceIndex, synchronization->first);
        AppendAdditions(edits, formatted, formattedIndex, synchronization->second);
    }
    AppendRemovals(edits, source, sourceIndex, source.size() - sourceIndex);
    AppendAdditions(edits, formatted, formattedIndex, formatted.size() - formattedIndex);
    return edits;
}

void AppendDiffLine(std::string& output, char prefix, const DiffLine& line) {
    output.push_back(prefix);
    output.append(line.text);
    switch (line.ending) {
        case DiffLineEnding::LineFeed:
            output.push_back('\n');
            return;
        case DiffLineEnding::CarriageReturn:
            output.push_back('\r');
            return;
        case DiffLineEnding::CarriageReturnLineFeed:
            output.append("\r\n");
            return;
        case DiffLineEnding::None:
            output.append("\n\\ No newline at end of file\n");
            return;
    }
}

void AppendHunkRange(std::string& output, size_t start, size_t count) {
    if (count == 0) {
        output += std::to_string(start - 1);
        output.append(",0");
        return;
    }
    output += std::to_string(start);
    if (count != 1) {
        output.push_back(',');
        output += std::to_string(count);
    }
}

void AppendHunk(
    std::string& output,
    const std::vector<DiffEdit>& edits,
    size_t start,
    size_t end,
    size_t sourceLine,
    size_t formattedLine
) {
    size_t sourceCount = 0;
    size_t formattedCount = 0;
    for (size_t index = start; index < end; ++index) {
        sourceCount += edits[index].kind != DiffEditKind::Add ? 1 : 0;
        formattedCount += edits[index].kind != DiffEditKind::Remove ? 1 : 0;
    }
    output.append("@@ -");
    AppendHunkRange(output, sourceLine, sourceCount);
    output.append(" +");
    AppendHunkRange(output, formattedLine, formattedCount);
    output.append(" @@\n");
    for (size_t index = start; index < end; ++index) {
        char prefix = ' ';
        if (edits[index].kind == DiffEditKind::Remove) {
            prefix = '-';
        } else if (edits[index].kind == DiffEditKind::Add) {
            prefix = '+';
        }
        AppendDiffLine(output, prefix, edits[index].line);
    }
}

}  // namespace

std::string BuildUnifiedFormatDiff(
    std::string_view source, std::string_view formatted, std::string_view path, size_t contextLines
) {
    if (source == formatted) {
        return {};
    }
    const std::vector<DiffEdit> edits = BuildGreedyEdits(SplitDiffLines(source), SplitDiffLines(formatted));
    std::vector<size_t> sourceLineBefore(edits.size() + 1, 1);
    std::vector<size_t> formattedLineBefore(edits.size() + 1, 1);
    std::vector<size_t> changes;
    for (size_t index = 0; index < edits.size(); ++index) {
        sourceLineBefore[index + 1] = sourceLineBefore[index] + (edits[index].kind != DiffEditKind::Add ? 1 : 0);
        formattedLineBefore[index + 1] =
            formattedLineBefore[index] + (edits[index].kind != DiffEditKind::Remove ? 1 : 0);
        if (edits[index].kind != DiffEditKind::Equal) {
            changes.push_back(index);
        }
    }
    if (changes.empty()) {
        return {};
    }

    std::string output;
    output.append("--- ");
    output.append(path);
    output.append("\n+++ ");
    output.append(path);
    output.push_back('\n');

    size_t firstChange = changes.front();
    size_t lastChange = firstChange;
    for (size_t changeIndex = 1; changeIndex <= changes.size(); ++changeIndex) {
        const bool atEnd = changeIndex == changes.size();
        const size_t nextChange = atEnd ? edits.size() : changes[changeIndex];
        const size_t equalLines = nextChange - lastChange - 1;
        if (!atEnd && equalLines <= contextLines * 2) {
            lastChange = nextChange;
            continue;
        }
        const size_t start = firstChange > contextLines ? firstChange - contextLines : 0;
        const size_t end = std::min(edits.size(), lastChange + contextLines + 1);
        AppendHunk(output, edits, start, end, sourceLineBefore[start], formattedLineBefore[start]);
        if (!atEnd) {
            firstChange = nextChange;
            lastChange = nextChange;
        }
    }
    return output;
}

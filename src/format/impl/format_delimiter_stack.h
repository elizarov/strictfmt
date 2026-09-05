#pragma once

#include <optional>
#include <vector>

struct FormatBreakNode;

struct FormatDelimiterStack {
    std::vector<const FormatBreakNode*> delimiters;
    const FormatBreakNode* leaf = nullptr;
};

enum class FormatDelimiterStackPolicy {
    Solving,
    Emission,
};

// Recognizes nested transparent parenthesis groups through single-child sequence
// wrappers. Returns outer-to-inner delimiters and the remaining leaf, all borrowed
// from an immutable model. No partitioning, costing, or output occurs here.
// Emission preserves its stricter closing-blank-line boundary; solving retains
// its existing recognition contract. Both reject separators and semantic lists.
std::optional<FormatDelimiterStack>
    CollectFormatDelimiterStack(const FormatBreakNode& node, FormatDelimiterStackPolicy policy);

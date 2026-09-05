#pragma once

#include <span>

#include "format/impl/format_break_model.h"

// Owns structural prefix surcharges and final subtree discounts; allocation and
// initial raw depths stay with the builder. Normalize prefixes as they are built,
// observe every completed delimiter, then finalize the whole model once before
// solving. Structural shifts update depth and cost together; final discounts only
// change cost, from outer subtrees inward. Use one normalizer per model build.
class FormatBreakCostNormalizer {
public:
    static void NormalizeCallablePrefix(FormatBreakNode& prefix, const FormatBreakNode& declarator);
    static void NormalizeNamedListPrefixes(
        std::span<FormatBreakNode* const> children, FormatBreakNode* qualification = nullptr
    );
    void ObserveDelimited(FormatBreakNode& node);
    void Finalize(FormatBreakNode& root) const;

private:
    bool hasFinalLambdaBody_ = false;
};

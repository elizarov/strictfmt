#pragma once

#include "format/impl/format_break_model.h"

class FormatChainContinuation;

struct FormatLayoutListBoundary {
    const SyntaxNode* owner = nullptr;
    bool forceSplit = false;
};

struct FormatLayoutChainPlacement {
    std::optional<int> baseIndent;
    bool flatSplitIndent = false;
    bool requiredBreak = false;
};

struct FormatLayoutLeadingSeparator {
    const SyntaxNode* token = nullptr;
    int indent = 0;
};

struct FormatLayoutRegionContext {
    // A separator starting a physical line after its preceding operand was emitted.
    std::optional<FormatLayoutLeadingSeparator> leadingSeparator;
    std::vector<FormatLayoutListBoundary> listBoundaries;
    // A body whose header began in an earlier mandatory segment, with its enclosing scope's indentation.
    const SyntaxNode* continuedBodyHeader = nullptr;
    int continuedBodyHeaderOwnerIndent = 0;
    const FormatChainContinuation* chainPlacements = nullptr;
    bool forceSplitStreamChain = false;
};

FormatBreakModel ProjectFormatLayout(
    const FormatBreakModel& complete,
    std::span<const PrintToken> tokens,
    const FormatLayoutRegionContext& context,
    FormatBreakWorkspace* workspace = nullptr
);

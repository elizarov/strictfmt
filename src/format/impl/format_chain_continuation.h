#pragma once

#include <memory>
#include <optional>
#include <span>

#include "format/impl/format_layout_projection.h"

struct PrintToken;
class FormatLayoutTree;
struct FormatLayoutRegionContext;
struct FormatBreakNode;

// The layout tree owns one placement per complete chain. Operators reference
// that owner instead of copying its indentation policy. Boundary analysis reads
// retained models; selected render bases or explicit enclosing-scope bases
// resolve placements before projecting later regions.
class FormatChainContinuation {
public:
    explicit FormatChainContinuation(FormatLayoutTree& tree);
    ~FormatChainContinuation();

    void AnalyzeBlock(size_t tokenIndex);
    void AnalyzeDirective(size_t tokenIndex);
    void Constrain(FormatLayoutRegionContext& context) const;
    std::optional<FormatLayoutChainPlacement> Lookup(const SyntaxNode* token) const;
    std::optional<int> ContinuationIndent(const PrintToken& token) const;
    void RecordSelection(const FormatBreakNode& chain, int baseIndent);
    void FinishBoundary(int fallbackBaseIndent);

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

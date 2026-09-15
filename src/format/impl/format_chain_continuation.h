#pragma once

#include <memory>
#include <optional>
#include <span>

struct PrintToken;
struct FormatBreakModelContext;
struct FormatBreakChainIndent;

// Keeps uniform chain breaks, render bases, and indentation policy consistent
// across mandatory blocks and directives. Analyze each boundary before building
// its segment, constrain subsequent models, accept emitted bases, then finish it
// with a fallback for unresolved groups. Tokens and syntax nodes are borrowed for
// this object's lifetime; constrained contexts borrow its maps until consumed.
class FormatChainContinuation {
public:
    explicit FormatChainContinuation(std::span<const PrintToken> tokens);
    ~FormatChainContinuation();

    void AnalyzeBlock(size_t tokenIndex);
    void AnalyzeDirective(size_t tokenIndex);
    void Constrain(FormatBreakModelContext& context) const;
    std::optional<int> ContinuationIndent(const PrintToken& token) const;
    void AcceptEmission(std::span<const FormatBreakChainIndent> chains);
    void FinishBoundary(int fallbackBaseIndent);

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

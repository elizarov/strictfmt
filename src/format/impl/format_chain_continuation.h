#pragma once

#include <memory>
#include <span>

struct PrintToken;
struct FormatBreakModelContext;
struct FormatBreakChainIndent;

// Keeps uniform chain breaks and their render bases consistent across mandatory
// block boundaries. Analyze each block before building its segment, constrain
// subsequent segment models, accept emitted chain bases, then finish the block
// with a fallback for unresolved groups. Tokens and syntax nodes are borrowed for
// this object's lifetime; constrained contexts borrow its maps until consumed.
class FormatChainContinuation {
public:
    explicit FormatChainContinuation(std::span<const PrintToken> tokens);
    ~FormatChainContinuation();

    void AnalyzeBlock(size_t tokenIndex);
    void Constrain(FormatBreakModelContext& context) const;
    void AcceptEmission(std::span<const FormatBreakChainIndent> chains);
    void FinishBlock(int fallbackBaseIndent);

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

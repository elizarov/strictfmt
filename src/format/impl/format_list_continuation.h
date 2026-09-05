#pragma once

#include <memory>
#include <optional>
#include <span>

struct PrintToken;
struct FormatBreakModelContext;
struct FormatBreakSplitList;

enum class FormatListContinuationKind {
    Preprocessor,
    Block,
};

struct FormatListContinuationBreak {
    bool beforeToken;
    std::optional<int> indent;  // No break for a leading conditional-branch comma.
};

// Plans virtual delimiters and retains selected list indentation across mandatory
// blocks and preprocessor regions. Tokens/syntax are borrowed for this object's
// lifetime. Each Plan returns a context valid until the next plan of that kind;
// Accept follows that segment's emission. Call BeforeToken in source order.
// TakeBoundary consumes a matching closer and returns the physical action; output
// and structural indentation remain with the caller. PlanBlock accepts block-role
// braces selected by the printer; it does not classify mandatory boundaries.
class FormatListContinuation {
public:
    explicit FormatListContinuation(std::span<const PrintToken> tokens);
    ~FormatListContinuation();

    const FormatBreakModelContext* PlanBlock(size_t index);
    std::optional<int> AcceptBlock(std::span<const FormatBreakSplitList> selected);
    const FormatBreakModelContext* PlanPreprocessor(size_t index, std::span<const PrintToken> pending, int itemIndent);
    int AcceptPreprocessor();
    std::optional<int> PreprocessorIndent(const PrintToken& token) const;
    std::optional<bool> ConditionalDirectiveComma(size_t index) const;
    bool IsFinalPreprocessorItem(size_t index) const;

    std::optional<FormatListContinuationBreak> TakeBoundary(const PrintToken& token, FormatListContinuationKind kind);
    void BeforeToken(const PrintToken& token);
    bool ContinuesList(const PrintToken& token) const;
    std::optional<int> CloseBlock(const PrintToken& token, const PrintToken* next);

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

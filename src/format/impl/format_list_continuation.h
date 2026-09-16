#pragma once

#include <memory>
#include <optional>
#include <span>

struct PrintToken;
class FormatLayoutTree;
struct FormatLayoutRegionContext;
struct SyntaxNode;

enum class FormatListContinuationKind {
    Preprocessor,
    Block,
};

struct FormatListContinuationBreak {
    bool beforeToken;
    std::optional<int> indent;  // No break for a leading conditional-branch comma.
};

// The layout tree owns these persistent list placements. Plans identify complete
// list owners; lowering records selected item/closer indentation. Boundary queries
// use lexical ownership and never retire or consume a placement. Tokens and
// syntax outlive the tree; each plan view lasts until the next plan of its kind.
class FormatListContinuation {
public:
    explicit FormatListContinuation(FormatLayoutTree& tree);
    ~FormatListContinuation();

    const FormatLayoutRegionContext* PlanBlock(size_t index);
    void RecordSelection(const SyntaxNode* open, int itemIndent, int closeIndent);
    std::optional<int> SelectedItemIndent(const SyntaxNode* list) const;
    std::optional<int> ResolveBlock();
    const FormatLayoutRegionContext*
        PlanPreprocessor(size_t index, std::span<const PrintToken> pending, int itemIndent);
    int ResolvePreprocessor();
    std::optional<int> PreprocessorIndent(const PrintToken& token) const;
    bool IsConditionalList(size_t index) const;

    std::optional<FormatListContinuationBreak>
        BoundaryFor(const PrintToken& token, FormatListContinuationKind kind) const;
    bool ContinuesList(const PrintToken& token) const;
    std::optional<int> AfterBlock(const PrintToken& token, const PrintToken* next) const;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

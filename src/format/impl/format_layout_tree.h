#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include "format/impl/format_layout_projection.h"
#include "format/impl/format_layout_program.h"
#include "format/impl/format_break_solution.h"

class FormatListContinuation;
class FormatChainContinuation;

struct FormatLayoutOwner {
    FormatLayoutOwnerId id = 0;
    FormatLayoutOwnerId parent = 0;
    const SyntaxNode* syntax = nullptr;
    size_t begin = 0;
    size_t end = 0;
};

// One cost region borrows its enclosing owners but owns its token projection and
// selected layout. Completing a region never invalidates an enclosing owner.
struct FormatLayoutRegion {
    FormatLayoutOwnerId owner = 0;
    std::vector<PrintToken> tokens;
    FormatBreakModel model;
    FormatBreakSolution solution;
};

// The complete source topology is built once. Complete item models are immutable
// and materialized on demand; cost regions and their solutions have this same
// lifetime. Source syntax and the original tokens must outlive the tree.
class FormatLayoutTree {
public:
    explicit FormatLayoutTree(std::span<const PrintToken> tokens);
    ~FormatLayoutTree();

    void VisitCompleteModels(const std::function<void(const FormatBreakModel&)>& visitor) const;
    void Complete(FormatLayoutProgram program);
    const FormatLayoutProgram& Program() const;
    void RecordBlockIndent(const SyntaxNode* token, int indent);
    std::optional<int> BlockIndent(const SyntaxNode* token) const;
    FormatListContinuation& Lists();
    FormatChainContinuation& Chains();

    std::span<const PrintToken> Tokens() const;
    const FormatLayoutOwner& Owner(FormatLayoutOwnerId id) const;
    FormatLayoutOwnerId FindOwner(const SyntaxNode* syntax) const;
    FormatLayoutOwnerId SourceItem(const SyntaxNode* syntax) const;
    const FormatBreakModel& CompleteModel(FormatLayoutOwnerId owner);
    FormatLayoutRegion& AddRegion(std::span<const PrintToken> tokens, const FormatLayoutRegionContext& context);
    void BeginToken(const PrintToken& token, int structuralIndent);
    void ConstrainBodyHeader(FormatLayoutRegionContext& context, std::span<const PrintToken> tokens) const;

private:
    std::optional<FormatLayoutProgram> program_;
    std::unordered_map<const SyntaxNode*, int> blockIndents_;
    std::span<const PrintToken> tokens_;
    std::vector<FormatLayoutOwner> owners_;
    std::unordered_map<const SyntaxNode*, FormatLayoutOwnerId> ownerIds_;
    std::unordered_map<FormatLayoutOwnerId, FormatBreakModel> completeModels_;
    std::deque<FormatLayoutRegion> regions_;
    std::vector<std::optional<int>> structuralIndents_;
    std::unique_ptr<FormatListContinuation> lists_;
    std::unique_ptr<FormatChainContinuation> chains_;

    FormatLayoutOwnerId AddOwner(const SyntaxNode* syntax);
};

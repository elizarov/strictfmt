#include "format/impl/format_chain_continuation.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include "format/impl/format_layout_tree.h"
#include "format/impl/format_break_model_inline_helpers.h"

struct FormatChainContinuation::Impl {
    explicit Impl(FormatLayoutTree& tree) : tree_(tree), tokens_(tree.Tokens()) {}

    FormatLayoutTree& tree_;

    std::span<const PrintToken> tokens_;

    struct Placement {
        const FormatBreakNode* owner = nullptr;
        std::optional<int> baseIndent;
        bool uniform = false;
    };

    std::unordered_map<const SyntaxNode*, Placement> placements_;
    std::unordered_map<const SyntaxNode*, const SyntaxNode*> requiredChainBreakGroups_;
    std::unordered_set<const SyntaxNode*> pendingCrossBlockChainGroups_;

    static bool HasUniformSplitForm(const FormatBreakNode& node) {
        if (node.kind != FormatBreakNodeKind::Chain || node.operators.empty()) {
            return false;
        }
        if (
            node.chainKind == FormatBreakChainKind::MemberBeforeOperator ||
            node.chainKind == FormatBreakChainKind::StreamBeforeOperator
        ) {
            return true;
        }
        if (node.chainKind == FormatBreakChainKind::Ternary) {
            return node.operators.size() > 2;
        }
        return std::all_of(node.operators.begin(), node.operators.end(), [](const FormatBreakToken& token) {
            return FormatBreakTokenKind(token) == PrintTokenKind::Known &&
                SyntaxNodeKindHasClass(FormatBreakTokenSyntaxKind(token), SyntaxNodeClass::ChainOperator);
        });
    }

    bool CollectCrossBlockChainBreaks(const FormatBreakNode& node, const PrintToken& block, bool directive) {
        bool containsBlock = node.kind == FormatBreakNodeKind::Token && node.token.token == &block;
        for (const FormatBreakNode* child : node.children) {
            if (child != nullptr && CollectCrossBlockChainBreaks(*child, block, directive)) {
                containsBlock = true;
            }
        }
        for (const FormatBreakListItem& listItem : node.items) {
            if (listItem.node != nullptr && CollectCrossBlockChainBreaks(*listItem.node, block, directive)) {
                containsBlock = true;
            }
        }
        const bool receiverMayExpand = node.chainKind == FormatBreakChainKind::MemberBeforeOperator ||
            node.chainKind == FormatBreakChainKind::CallApplication;
        bool requiresSplit = false;
        for (size_t index = 0; index < node.operands.size(); ++index) {
            if (
                node.operands[index] != nullptr && CollectCrossBlockChainBreaks(*node.operands[index], block, directive)
            ) {
                containsBlock = true;
                requiresSplit = directive || (index + 1 < node.operands.size() && !(index == 0 && receiverMayExpand));
            }
        }
        if (
            requiresSplit &&
            node.kind == FormatBreakNodeKind::Chain &&
            !node.operators.empty() &&
            (directive || HasUniformSplitForm(node))
        ) {
            const SyntaxNode* group = FormatBreakTokenValue(node.operators.front()).node;
            pendingCrossBlockChainGroups_.insert(group);
            auto& placement = placements_[group];
            placement.owner = &node;
            placement.uniform = HasUniformSplitForm(node);
            for (const FormatBreakToken& token : node.operators) {
                const PrintToken& printToken = FormatBreakTokenValue(token);
                if (printToken.node != nullptr) {
                    requiredChainBreakGroups_.insert_or_assign(printToken.node, group);

                }
            }
        }
        return containsBlock;
    }

    void RecordCrossBlockChainBaseIndents(int baseIndent, const SyntaxNode* selectedGroup = nullptr) {
        for (const auto* group : pendingCrossBlockChainGroups_) {
            if (selectedGroup == nullptr || group == selectedGroup) {
                placements_.at(group).baseIndent = baseIndent;
            }
        }
        if (selectedGroup == nullptr) {
            pendingCrossBlockChainGroups_.clear();
        } else {
            pendingCrossBlockChainGroups_.erase(selectedGroup);
        }
    }

    static bool CanParticipateInUniformCrossBlockChain(const PrintToken& token) {
        if (token.kind != PrintTokenKind::Known) {
            return false;
        }
        if (PrintTokenSyntaxHasClass(token, SyntaxNodeClass::ChainOperator)) {
            return true;
        }
        switch (token.syntaxKind) {
            case SyntaxNodeKind::Comma:
            case SyntaxNodeKind::Dot:
            case SyntaxNodeKind::Arrow:
            case SyntaxNodeKind::DotStar:
            case SyntaxNodeKind::ArrowStar:
            case SyntaxNodeKind::Question:
            case SyntaxNodeKind::Colon:
                return true;
            default:
                return false;
        }
    }

    bool MayHaveCrossBlockChain(size_t afterBlock, size_t end) const {
        // A non-final operand has a following operator outside the block. Operators inside the body belong to a
        // separate subtree; exact operand ownership below rejects unrelated operators later in the source item.
        return std::any_of(
            tokens_.begin() + static_cast<std::ptrdiff_t>(afterBlock),
            tokens_.begin() + static_cast<std::ptrdiff_t>(end),
            CanParticipateInUniformCrossBlockChain
        );
    }

    void AnalyzeBoundary(size_t currentTokenIndex_, bool directive) {
        pendingCrossBlockChainGroups_.clear();
        if (currentTokenIndex_ >= tokens_.size()) {
            return;
        }
        const PrintToken& token = tokens_[currentTokenIndex_];
        if (directive && (
            PrintTokenSyntaxHasClass(token, SyntaxNodeClass::ConditionalPreprocessorTree) ||
            PrintTokenSyntaxHasClass(token, SyntaxNodeClass::ConditionalPreprocessorDirective)
        )) {
            return;
        }
        const SyntaxNode* block = directive ? token.node : (token.node == nullptr ? nullptr : token.node->parent);
        const auto ownerId = tree_.SourceItem(block);
        if (ownerId == 0) {
            return;
        }
        const auto& item = tree_.Owner(ownerId);
        const size_t end = item.end;
        size_t afterBlock = currentTokenIndex_ + 1;
        for (size_t index = afterBlock; index < end; ++index) {
            if (
                tokens_[index].syntaxKind == SyntaxNodeKind::RightBrace &&
                tokens_[index].node != nullptr &&
                tokens_[index].node->parent == block
            ) {
                afterBlock = index + 1;
            }
        }
        if (!directive && !MayHaveCrossBlockChain(afterBlock, end)) {
            return;
        }
        const FormatBreakModel& model = tree_.CompleteModel(ownerId);
        if (model.root != nullptr) {
            CollectCrossBlockChainBreaks(*model.root, token, directive);
        }
    }

    std::optional<FormatLayoutChainPlacement> Lookup(const SyntaxNode* token) const {
        const auto group = requiredChainBreakGroups_.find(token);
        if (group == requiredChainBreakGroups_.end()) {
            return std::nullopt;
        }
        const auto& placement = placements_.at(group->second);
        return FormatLayoutChainPlacement{
            .baseIndent = placement.baseIndent,
            .flatSplitIndent = placement.owner->flatSplitIndent,
            .requiredBreak = placement.uniform &&
                (placement.owner->chainKind != FormatBreakChainKind::Ternary || token->kind == SyntaxNodeKind::Colon),
        };
    }
    std::optional<int> ContinuationIndent(const PrintToken& token) const {
        const auto layout = Lookup(token.node);
        if (!layout || !layout->baseIndent) {
            return std::nullopt;
        }
        return *layout->baseIndent + (layout->flatSplitIndent ? 0 : 1);
    }
    void RecordSelection(const FormatBreakNode& chain, int baseIndent) {
        for (const FormatBreakToken& op : chain.operators) {
            const auto group = requiredChainBreakGroups_.find(FormatBreakTokenValue(op).node);
            if (group != requiredChainBreakGroups_.end()) {
                RecordCrossBlockChainBaseIndents(baseIndent, group->second);
            }
        }
    }

};

FormatChainContinuation::FormatChainContinuation(FormatLayoutTree& tree) : impl_(std::make_unique<Impl>(tree)) {}
FormatChainContinuation::~FormatChainContinuation() = default;
void FormatChainContinuation::AnalyzeBlock(size_t tokenIndex) { impl_->AnalyzeBoundary(tokenIndex, false); }
void FormatChainContinuation::AnalyzeDirective(size_t tokenIndex) { impl_->AnalyzeBoundary(tokenIndex, true); }
void FormatChainContinuation::Constrain(FormatLayoutRegionContext& context) const {
    if (!impl_->placements_.empty()) {
        context.chainPlacements = this;
    }
}
std::optional<FormatLayoutChainPlacement> FormatChainContinuation::Lookup(const SyntaxNode* token) const {
    return impl_->Lookup(token);
}
std::optional<int> FormatChainContinuation::ContinuationIndent(const PrintToken& token) const {
    return impl_->ContinuationIndent(token);
}
void FormatChainContinuation::RecordSelection(const FormatBreakNode& chain, int baseIndent) {
    impl_->RecordSelection(chain, baseIndent);
}
void FormatChainContinuation::FinishBoundary(int fallbackBaseIndent) {
    impl_->RecordCrossBlockChainBaseIndents(fallbackBaseIndent);
}

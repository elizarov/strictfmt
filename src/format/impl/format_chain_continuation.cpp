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
    std::unordered_map<const SyntaxNode*, FormatBuilderStep> builderSteps_;
    std::unordered_set<const SyntaxNode*> pendingCrossBlockChainGroups_;

    struct ParentLink {
        const FormatBreakNode* node = nullptr;
        std::optional<size_t> operand;
    };

    struct ModelIndex {
        std::unordered_map<const FormatBreakNode*, ParentLink> parents;
        std::unordered_map<const PrintToken*, const FormatBreakNode*> leaves;

        void Add(const FormatBreakNode& node, ParentLink parent) {
            parents.emplace(&node, parent);
            if (node.kind == FormatBreakNodeKind::Token && node.token.token != nullptr) {
                leaves.emplace(node.token.token, &node);
            }
            for (const auto* child : node.children) {
                if (child != nullptr) {
                    Add(*child, {&node, std::nullopt});
                }
            }
            for (const auto& item : node.items) {
                if (item.node != nullptr) {
                    Add(*item.node, {&node, std::nullopt});
                }
            }
            for (size_t index = 0; index < node.operands.size(); ++index) {
                if (node.operands[index] != nullptr) {
                    Add(*node.operands[index], {&node, index});
                }
            }
        }
    };

    std::unordered_map<const FormatBreakNode*, ModelIndex> modelIndexes_;

    static const SyntaxNode* ChainGroup(const FormatBreakNode& node) {
        const SyntaxNode* op = FormatBreakTokenValue(node.operators.front()).node;
        return op == nullptr ? node.syntaxOwner : op;
    }

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

    void RequireChainBreaks(const FormatBreakNode& node, size_t operand, bool directive) {
        const bool receiverMayExpand = node.chainKind == FormatBreakChainKind::MemberBeforeOperator ||
            node.chainKind == FormatBreakChainKind::CallApplication;
        const bool requiresSplit =
            directive || (operand + 1 < node.operands.size() && !(operand == 0 && receiverMayExpand));
        if (
            requiresSplit &&
            node.kind == FormatBreakNodeKind::Chain &&
            !node.operators.empty() &&
            (directive || HasUniformSplitForm(node))
        ) {
            const SyntaxNode* group = ChainGroup(node);
            pendingCrossBlockChainGroups_.insert(group);
            auto& placement = placements_[group];
            if (placement.owner == &node) {
                return;
            }
            placement.owner = &node;
            placement.uniform = HasUniformSplitForm(node);
            for (size_t index = 0; index < node.operators.size(); ++index) {
                const auto& token = node.operators[index];
                const PrintToken& printToken = FormatBreakTokenValue(token);
                if (printToken.node != nullptr) {
                    requiredChainBreakGroups_.insert_or_assign(printToken.node, group);
                    if (!node.builderSteps.empty()) {
                        builderSteps_.insert_or_assign(printToken.node, FormatBreakMemberStep(node, index));
                    }
                }
            }
        }
    }

    void CollectCrossBlockChainBreaks(const FormatBreakNode& root, const PrintToken& block, bool directive) {
        const bool conditional = block.conditionalOperand != nullptr ||
            PrintTokenSyntaxHasClass(block, SyntaxNodeClass::ConditionalPreprocessorTree) ||
            PrintTokenSyntaxHasClass(block, SyntaxNodeClass::ConditionalPreprocessorDirective);
        auto [entry, inserted] = modelIndexes_.try_emplace(&root);
        auto& index = entry->second;
        if (inserted) {
            index.Add(root, {});
        }
        const auto leaf = index.leaves.find(&block);
        if (leaf == index.leaves.end()) {
            return;
        }
        bool crossesDelimiter = false;
        for (
            auto parent = index.parents.at(leaf->second);
            parent.node != nullptr;
            parent = index.parents.at(parent.node)
        ) {
            crossesDelimiter |= parent.node->kind == FormatBreakNodeKind::Delimited;
            if (directive && conditional && parent.node->kind == FormatBreakNodeKind::Delimited) {
                break;
            }
            if (parent.operand) {
                if (
                    directive &&
                    conditional &&
                    block.conditionalOperand == nullptr &&
                    (*parent.operand == 0 || parent.node->chainKind == FormatBreakChainKind::StreamBeforeOperator)
                ) {
                    // Declaration prefixes do not continue a value; selected stream tails own their layout.
                    continue;
                }
                RequireChainBreaks(*parent.node, *parent.operand, directive);
                if (
                    directive &&
                    !crossesDelimiter &&
                    (block.conditionalOperand != nullptr || *parent.operand > 0) &&
                    parent.node->kind == FormatBreakNodeKind::Chain &&
                    !parent.node->operators.empty()
                ) {
                    const SyntaxNode* group = ChainGroup(*parent.node);
                    requiredChainBreakGroups_.insert_or_assign(block.node, group);
                    if (!parent.node->builderSteps.empty() && *parent.operand > 0) {
                        builderSteps_
                            .insert_or_assign(block.node, FormatBreakMemberStep(*parent.node, *parent.operand - 1));
                    }
                }
            }
        }
    }

    void RegisterResumedOperand(const FormatBreakNode& root, const PrintToken& token) {
        if (token.syntaxKind == SyntaxNodeKind::Semicolon) {
            return;
        }
        const auto& index = modelIndexes_.at(&root);
        const auto leaf = index.leaves.find(&token);
        if (leaf == index.leaves.end()) {
            return;
        }
        for (
            auto parent = index.parents.at(leaf->second);
            parent.node != nullptr;
            parent = index.parents.at(parent.node)
        ) {
            if (
                parent.node->kind == FormatBreakNodeKind::PrefixList ||
                (parent.node->kind == FormatBreakNodeKind::Delimited && parent.node->children.front() != leaf->second)
            ) {
                break;
            }
            if (!parent.operand || *parent.operand == 0 || parent.node->operators.empty()) {
                continue;
            }
            const auto* group = ChainGroup(*parent.node);
            const auto placement = placements_.find(group);
            if (placement == placements_.end() || placement->second.owner != parent.node) {
                continue;
            }
            requiredChainBreakGroups_.try_emplace(token.node, group);
            if (!parent.node->builderSteps.empty()) {
                builderSteps_.try_emplace(token.node, FormatBreakMemberStep(*parent.node, *parent.operand - 1));
            }
            return;
        }
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
        if (directive && token.conditionalOperand == nullptr && !token.structuredPreprocessor && (
            PrintTokenSyntaxHasClass(token, SyntaxNodeClass::ConditionalPreprocessorTree) ||
            PrintTokenSyntaxHasClass(token, SyntaxNodeClass::ConditionalPreprocessorDirective)
        )) {
            // Opaque conditional fragments do not expose their operand boundaries.
            return;
        }
        const SyntaxNode* block = directive ? token.node : (token.node == nullptr ? nullptr : token.node->parent);
        while (
            directive &&
            block != nullptr &&
            block->parent != nullptr &&
            SyntaxNodeHasClass(*block->parent, SyntaxNodeClass::ConditionalPreprocessorTree)
        ) {
            block = block->parent;
        }
        if (
            directive &&
            block != nullptr &&
            SyntaxNodeHasClass(*block, SyntaxNodeClass::ConditionalPreprocessorTree) && (
                SyntaxNodeHasClass(*block, SyntaxNodeClass::ConditionalRhsPreprocessor) ||
                (block->parent != nullptr && SyntaxNodeHasClass(*block->parent, SyntaxNodeClass::SourceItemScope))
            )
        ) {
            // Whole source items and branch-owned statements resume at their own scope indentation.
            return;
        }
        const auto ownerId = tree_.SourceItem(block);
        if (ownerId == 0) {
            return;
        }
        const auto& item = tree_.Owner(ownerId);
        const size_t end = item.end;
        size_t afterBlock = currentTokenIndex_ + 1;
        for (size_t index = afterBlock; !directive && index < end; ++index) {
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
            if (
                directive &&
                token.conditionalOperand != nullptr &&
                PrintTokenSyntaxHasClass(token, SyntaxNodeClass::EndifDirective)
            ) {
                // Shared operands resume the enclosing chain, not the completed branch's local chains.
                for (size_t index = currentTokenIndex_ + 1; index < end; ++index) {
                    const auto& next = tokens_[index];
                    if (IsCommentToken(next.kind) || next.kind == PrintTokenKind::BlankLine) {
                        continue;
                    }
                    RegisterResumedOperand(*model.root, next);
                    break;
                }
            }
        }
    }

    std::optional<FormatLayoutChainPlacement> Lookup(const SyntaxNode* token) const {
        const auto group = requiredChainBreakGroups_.find(token);
        if (group == requiredChainBreakGroups_.end()) {
            return std::nullopt;
        }
        const auto& placement = placements_.at(group->second);
        const auto builder = builderSteps_.find(token);
        return FormatLayoutChainPlacement{
            .baseIndent = placement.baseIndent,
            .flatSplitIndent = placement.owner->flatSplitIndent,
            .requiredBreak = placement.uniform &&
                (builder == builderSteps_.end() || builder->second.breakBefore) &&
                (placement.owner->chainKind != FormatBreakChainKind::Ternary || token->kind == SyntaxNodeKind::Colon),
            .indentOffset =
                builder != builderSteps_.end() ? builder->second.indent : (placement.owner->flatSplitIndent ? 0 : 1),
        };
    }
    std::optional<int> ContinuationIndent(const PrintToken& token) const {
        auto layout = Lookup(token.node);
        if (!layout && token.conditionalOperand != nullptr) {
            layout = Lookup(token.conditionalOperand);
        }
        if (!layout || !layout->baseIndent) {
            return std::nullopt;
        }
        return *layout->baseIndent + layout->indentOffset;
    }
    void RecordSelection(const FormatBreakNode& chain, int baseIndent) {
        if (!chain.operators.empty() && chain.operators.front().token == nullptr) {
            RecordCrossBlockChainBaseIndents(baseIndent, ChainGroup(chain));
        }
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

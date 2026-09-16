#include "format/impl/format_list_continuation.h"

#include <algorithm>
#include <vector>
#include <unordered_map>

#include "format/impl/format_break_model.h"
#include "format/impl/format_layout_tree.h"
#include "format/impl/format_syntax_helpers.h"

namespace {

struct MandatoryBlockSplitListContext {
    const SyntaxNode* openToken = nullptr;
    const SyntaxNode* list = nullptr;
    const SyntaxNode* itemRightBrace = nullptr;
    const SyntaxNode* closeToken = nullptr;
    int itemIndent = 0;
    int closeIndent = 0;
};

struct MandatoryBlockSplitListPlan {
    FormatLayoutRegionContext breakContext;
    std::vector<MandatoryBlockSplitListContext> deferredContexts;
};

struct PreprocessorSplitListContext {
    const SyntaxNode* list = nullptr;
    const SyntaxNode* openToken = nullptr;
    const SyntaxNode* closeToken = nullptr;
    int itemIndent = 0;
    int closeIndent = 0;
};

struct PreprocessorSplitListPlan {
    FormatLayoutRegionContext breakContext;
    PreprocessorSplitListContext deferredContext;
};

bool HasDirectCommentChild(const SyntaxNode& node) {
    for (const SyntaxNode* child : node.children) {
        if (
            child != nullptr &&
            (child->kind == SyntaxNodeKind::Comment || child->kind == SyntaxNodeKind::TrailingComment)
        ) {
            return true;
        }
    }
    return false;
}

}  // namespace

struct FormatListContinuation::Impl {
    explicit Impl(FormatLayoutTree& tree) : tree_(tree), tokens_(tree.Tokens()) {}

    FormatLayoutTree& tree_;

    std::span<const PrintToken> tokens_;
    std::unordered_map<const SyntaxNode*, MandatoryBlockSplitListContext> mandatoryBlockSplitListContexts_;
    std::unordered_map<const SyntaxNode*, PreprocessorSplitListContext> preprocessorSplitListContexts_;

    static bool ListOwnsToken(const SyntaxNode* list, const PrintToken& token) {
        if (token.node == nullptr) {
            return false;
        }
        for (const SyntaxNode* parent = token.node->parent; parent != nullptr; parent = parent->parent) {
            if (parent == list) {
                return true;
            }
            if (DirectOpeningDelimiterChild(*parent) != nullptr) {
                return false;
            }
        }
        return false;
    }

    static bool IsListComma(const PrintToken& token, const SyntaxNode* list) {
        return token.kind == PrintTokenKind::Known &&
            token.syntaxKind == SyntaxNodeKind::Comma &&
            ListOwnsToken(list, token);
    }

    static std::vector<const SyntaxNode*> ListAncestorsBefore(const PrintToken& token, const SyntaxNode* before) {
        for (const SyntaxNode* cursor = token.node; cursor != nullptr; cursor = cursor->parent) {
            if (cursor == before) {
                std::vector<const SyntaxNode*> result;
                for (cursor = cursor->parent; cursor != nullptr; cursor = cursor->parent) {
                    if (
                        SyntaxNodeKindHasClass(cursor->kind, SyntaxNodeClass::CompoundBlock) &&
                        cursor->kind != SyntaxNodeKind::EnumeratorList
                    ) {
                        break;
                    }
                    const SyntaxNode* open = DirectOpeningDelimiterChild(*cursor);
                    if (
                        SyntaxNodeHasClass(*cursor, SyntaxNodeClass::PrefixList) ||
                        (open != nullptr && DirectMatchingClosingDelimiterChild(*cursor, open) != nullptr)
                    ) {
                        result.push_back(cursor);
                    }
                }
                return result;
            }
        }
        return {};
    }

    static bool StartsPreprocessorSplitList(const PrintToken& token) {
        if (token.node == nullptr || (
            token.kind != PrintTokenKind::Preprocessor &&
            token.syntaxKind != SyntaxNodeKind::PreprocessorDirectiveDefine
        )) {
            return false;
        }
        return NearestPreprocessorSplitListAncestor(token) != nullptr;
    }

    static const SyntaxNode* NearestPreprocessorSplitListAncestor(const PrintToken& token) {
        for (const SyntaxNode* cursor = token.node; cursor != nullptr; cursor = cursor->parent) {
            if (SyntaxNodeKindHasClass(cursor->kind, SyntaxNodeClass::PreprocessorSplitList)) {
                return cursor;
            }
            if (SyntaxNodeHasClass(*cursor, SyntaxNodeClass::CompoundBlock)) {
                return nullptr;
            }
            const SyntaxNode* open = DirectOpeningDelimiterChild(*cursor);
            if (open != nullptr && DirectMatchingClosingDelimiterChild(*cursor, open) != nullptr) {
                return cursor;
            }
        }
        return nullptr;
    }

    std::optional<size_t> FindTokenIndex(const SyntaxNode* node, size_t begin) const {
        const auto owner = tree_.FindOwner(node);
        if (owner == 0) {
            return std::nullopt;
        }
        const auto index = tree_.Owner(owner).begin;
        return index >= begin && index < tokens_.size() && tokens_[index].node == node ? std::optional(index) :
            std::nullopt;
    }

    const SyntaxNode*
        FindPendingOpeningDelimiterFor(const SyntaxNode* list, std::span<const PrintToken> pendingTokens_) const
    {
        for (auto token = pendingTokens_.rbegin(); token != pendingTokens_.rend(); ++token) {
            if (
                token->kind == PrintTokenKind::Known &&
                PrintTokenSyntaxHasClass(*token, SyntaxNodeClass::OpeningDelimiter) &&
                token->node != nullptr &&
                token->node->parent == list
            ) {
                return token->node;
            }
        }
        return nullptr;
    }

    std::optional<size_t>
        FindFutureClosingDelimiterFor(const SyntaxNode* list, SyntaxNodeKind openKind, size_t currentTokenIndex_) const
    {
        const SyntaxNodeKind closeKind = MatchingListCloseToken(openKind);
        if (closeKind == SyntaxNodeKind::Unknown) {
            return std::nullopt;
        }
        for (size_t index = currentTokenIndex_ + 1; index < tokens_.size(); ++index) {
            const PrintToken& candidate = tokens_[index];
            if (
                candidate.kind == PrintTokenKind::Known &&
                candidate.syntaxKind == closeKind &&
                candidate.node != nullptr &&
                candidate.node->parent == list
            ) {
                return index;
            }
        }
        return std::nullopt;
    }

    std::optional<MandatoryBlockSplitListPlan> BuildMandatoryBlockSplitListPlan(size_t currentTokenIndex_) const {
        const PrintToken& token = tokens_[currentTokenIndex_];
        if (
            token.kind != PrintTokenKind::Known ||
            token.syntaxKind != SyntaxNodeKind::LeftBrace ||
            token.node == nullptr ||
            token.node->parent == nullptr
        ) {
            return std::nullopt;
        }
        const SyntaxNode* block = token.node->parent;
        const std::vector<const SyntaxNode*> lists = ListAncestorsBefore(token, block);
        if (lists.empty()) {
            return std::nullopt;
        }
        const SyntaxNode* itemClose = DirectTokenChild(*block, SyntaxNodeKind::RightBrace);
        if (itemClose == nullptr) {
            return std::nullopt;
        }
        const std::optional<size_t> itemCloseIndex = FindTokenIndex(itemClose, currentTokenIndex_ + 1);
        if (!itemCloseIndex) {
            return std::nullopt;
        }

        MandatoryBlockSplitListPlan result;
        for (const SyntaxNode* list : lists) {
            if (SyntaxNodeHasClass(*list, SyntaxNodeClass::PrefixList)) {
                const SyntaxNode* prefix = DirectTokenChild(*list, SyntaxNodeKind::Colon);
                if (prefix != nullptr) {
                    result.deferredContexts.push_back({.openToken = prefix, .list = list, .itemRightBrace = itemClose});
                }
                continue;
            }
            const SyntaxNode* listOpen = DirectOpeningDelimiterChild(*list);
            const SyntaxNode* listClose = DirectMatchingClosingDelimiterChild(*list, listOpen);
            if (listOpen == nullptr || listClose == nullptr) {
                continue;
            }
            const std::optional<size_t> closeIndex = FindTokenIndex(listClose, currentTokenIndex_ + 1);
            if (!closeIndex || *itemCloseIndex >= *closeIndex) {
                continue;
            }
            bool hasFollowingListItem = false;
            for (size_t index = *itemCloseIndex + 1; index < *closeIndex; ++index) {
                const PrintToken& candidate = tokens_[index];
                if (IsListComma(candidate, list)) {
                    hasFollowingListItem = true;
                    break;
                }
            }
            result
                .breakContext
                .listBoundaries
                .push_back({.owner = list, .forceSplit = hasFollowingListItem || HasDirectCommentChild(*list)});
            result
                .deferredContexts
                .push_back({.openToken = listOpen, .list = list, .itemRightBrace = itemClose, .closeToken = listClose});
        }
        if (result.deferredContexts.empty()) {
            return std::nullopt;
        }
        return result;
    }

    std::optional<PreprocessorSplitListPlan> BuildPreprocessorSplitListPlan(
        size_t currentTokenIndex_, std::span<const PrintToken> pendingTokens_, int itemIndentLevel
    ) const {
        const PrintToken& token = tokens_[currentTokenIndex_];
        if (token.node == nullptr || !StartsPreprocessorSplitList(token)) {
            return std::nullopt;
        }
        const SyntaxNode* list = NearestPreprocessorSplitListAncestor(token);
        if (list == nullptr) {
            return std::nullopt;
        }
        if (SyntaxNodeHasClass(*list, SyntaxNodeClass::PrefixList)) {
            return PreprocessorSplitListPlan{.deferredContext = {
                .list = list,
                .openToken = DirectTokenChild(*list, SyntaxNodeKind::Colon),
                .itemIndent = itemIndentLevel,
                .closeIndent = std::max(0, itemIndentLevel - 1),
            }};
        }
        const SyntaxNode* listOpen = DirectOpeningDelimiterChild(*list);
        const SyntaxNode* pendingOpen = FindPendingOpeningDelimiterFor(list, pendingTokens_);
        if (pendingOpen != nullptr) {
            listOpen = pendingOpen;
        }
        if (listOpen == nullptr) {
            return std::nullopt;
        }
        const SyntaxNode* listClose = DirectMatchingClosingDelimiterChild(*list, listOpen);
        std::optional<size_t> closeIndex = FindTokenIndex(listClose, currentTokenIndex_ + 1);
        if (!closeIndex) {
            closeIndex = FindFutureClosingDelimiterFor(list, listOpen->kind, currentTokenIndex_);
        }
        if (!closeIndex) {
            return std::nullopt;
        }
        listClose = tokens_[*closeIndex].node;
        return PreprocessorSplitListPlan{
            .breakContext = {.listBoundaries = {{.owner = list, .forceSplit = true}}},
            .deferredContext = {
                .list = list,
                .openToken = listOpen,
                .closeToken = listClose,
                .itemIndent = itemIndentLevel,
                .closeIndent = std::max(0, itemIndentLevel - 1),
            },
        };
    }

    const MandatoryBlockSplitListContext* ActiveMandatoryBlockSplitListContext(const PrintToken& token) const {
        const auto tokenId = tree_.FindOwner(token.node);
        if (tokenId == 0) {
            return nullptr;
        }
        const auto index = tree_.Owner(tokenId).begin;
        for (const auto* list = token.node->parent; list != nullptr; list = list->parent) {
            const auto found = mandatoryBlockSplitListContexts_.find(list);
            if (found != mandatoryBlockSplitListContexts_.end()) {
                const auto close = tree_.FindOwner(found->second.itemRightBrace);
                if (close != 0 && tree_.Owner(close).begin <= index) {
                    return &found->second;
                }
            }
            if (DirectOpeningDelimiterChild(*list) != nullptr) {
                break;
            }
        }
        return nullptr;
    }

    const PreprocessorSplitListContext* ActivePreprocessorSplitListContextFor(const PrintToken& token) const {
        for (const auto* list = token.node; list != nullptr; list = list->parent) {
            if (
                const auto found = preprocessorSplitListContexts_.find(list);
                found != preprocessorSplitListContexts_.end()
            ) {
                return &found->second;
            }
        }
        return nullptr;
    }

    static bool IsForcedLeadingPreprocessorListComma(const PrintToken& token) {
        return token.forcedLeadingPreprocessorListComma;
    }

    struct Selection {
        int itemIndent;
        int closeIndent;
    };

    std::unordered_map<const SyntaxNode*, Selection> selections_;
    std::optional<MandatoryBlockSplitListPlan> blockPlan_;
    std::optional<PreprocessorSplitListPlan> preprocessorPlan_;

    const FormatLayoutRegionContext* PlanBlock(size_t index) {
        blockPlan_ = BuildMandatoryBlockSplitListPlan(index);
        return blockPlan_ ? &blockPlan_->breakContext : nullptr;
    }
    std::optional<int> ResolveBlock() {
        std::optional<int> splitListItemIndent;
        if (blockPlan_) {
            for (
                auto context = blockPlan_->deferredContexts.rbegin();
                context != blockPlan_->deferredContexts.rend();
                ++context
            ) {
                const auto selected = selections_.find(context->list);
                if (selected != selections_.end()) {
                    context->itemIndent = selected->second.itemIndent;
                    context->closeIndent = selected->second.closeIndent;
                    splitListItemIndent = context->itemIndent;
                    mandatoryBlockSplitListContexts_.insert_or_assign(context->list, *context);
                }
            }
        }
        return splitListItemIndent;
    }
    const FormatLayoutRegionContext*
        PlanPreprocessor(size_t index, std::span<const PrintToken> pending, int itemIndent)
    {
        preprocessorPlan_ = BuildPreprocessorSplitListPlan(index, pending, itemIndent);
        return preprocessorPlan_ ? &preprocessorPlan_->breakContext : nullptr;
    }
    int ResolvePreprocessor() {
        const auto selected = selections_.find(preprocessorPlan_->deferredContext.list);
        if (selected != selections_.end()) {
            preprocessorPlan_->deferredContext.itemIndent = selected->second.itemIndent;
            preprocessorPlan_->deferredContext.closeIndent = selected->second.closeIndent;
        }
        const auto& selectedContext = preprocessorPlan_->deferredContext;
        preprocessorSplitListContexts_.insert_or_assign(selectedContext.list, selectedContext);
        return selectedContext.itemIndent;
    }
    std::optional<int> PreprocessorIndent(const PrintToken& token) const {
        const auto* context = ActivePreprocessorSplitListContextFor(token);
        return context == nullptr ? std::nullopt : std::optional(context->itemIndent);
    }
    bool ContinuesList(const PrintToken& token) const {
        for (
            const auto* list = token.node == nullptr ? nullptr : token.node->parent;
            list != nullptr;
            list = list->parent
        ) {
            if (mandatoryBlockSplitListContexts_.contains(list) || preprocessorSplitListContexts_.contains(list)) {
                return true;
            }
            if (DirectOpeningDelimiterChild(*list) != nullptr) {
                break;
            }
        }
        return false;
    }
    std::optional<int> AfterBlock(const PrintToken& token, const PrintToken* next) const {
        if (next == nullptr) {
            return std::nullopt;
        }
        const auto* context = ActiveMandatoryBlockSplitListContext(*next);
        if (
            context == nullptr ||
            next->node == context->closeToken ||
            IsListComma(*next, context->list) ||
            context->itemRightBrace != token.node
        ) {
            return std::nullopt;
        }
        return context->itemIndent;
    }
    std::optional<FormatListContinuationBreak>
        BoundaryFor(const PrintToken& token, FormatListContinuationKind kind) const
    {
        if (kind == FormatListContinuationKind::Preprocessor) {
            const PreprocessorSplitListContext* context = ActivePreprocessorSplitListContextFor(token);
            if (context == nullptr) {
                return std::nullopt;
            }
            if (IsListComma(token, context->list)) {
                return FormatListContinuationBreak{
                    false,
                    IsForcedLeadingPreprocessorListComma(token) ? std::nullopt : std::optional(context->itemIndent),
                };
            }
            if (token.kind == PrintTokenKind::Known && context->closeToken == token.node) {
                const int indent = context->closeIndent;
                return FormatListContinuationBreak{true, indent};
            }
        } else {
            const MandatoryBlockSplitListContext* context = ActiveMandatoryBlockSplitListContext(token);
            if (context == nullptr) {
                return std::nullopt;
            }
            if (IsListComma(token, context->list)) {
                return FormatListContinuationBreak{false, context->itemIndent};
            }
            if (token.kind == PrintTokenKind::Known && context->closeToken == token.node) {
                const int indent = context->closeIndent;
                return FormatListContinuationBreak{true, indent};
            }
        }
        return std::nullopt;
    }
    bool IsConditionalList(size_t index) const {
        const PrintToken& token = tokens_[index];
        const SyntaxNode* list = token.kind == PrintTokenKind::Preprocessor &&
            PrintTokenSyntaxHasClass(token, SyntaxNodeClass::ConditionalPreprocessorOpen) ?
            NearestPreprocessorSplitListAncestor(token) : nullptr;
        return list != nullptr;
    }
};

FormatListContinuation::FormatListContinuation(FormatLayoutTree& tree) : impl_(std::make_unique<Impl>(tree)) {}
FormatListContinuation::~FormatListContinuation() = default;
const FormatLayoutRegionContext* FormatListContinuation::PlanBlock(size_t index) { return impl_->PlanBlock(index); }
std::optional<int> FormatListContinuation::ResolveBlock() { return impl_->ResolveBlock(); }
const FormatLayoutRegionContext*
    FormatListContinuation::PlanPreprocessor(size_t index, std::span<const PrintToken> pending, int itemIndent)
{
    return impl_->PlanPreprocessor(index, pending, itemIndent);
}
int FormatListContinuation::ResolvePreprocessor() { return impl_->ResolvePreprocessor(); }
std::optional<int> FormatListContinuation::PreprocessorIndent(const PrintToken& token) const {
    return impl_->PreprocessorIndent(token);
}
bool FormatListContinuation::IsConditionalList(size_t index) const { return impl_->IsConditionalList(index); }
std::optional<FormatListContinuationBreak>
    FormatListContinuation::BoundaryFor(const PrintToken& token, FormatListContinuationKind kind) const
{
    return impl_->BoundaryFor(token, kind);
}
bool FormatListContinuation::ContinuesList(const PrintToken& token) const { return impl_->ContinuesList(token); }
std::optional<int> FormatListContinuation::AfterBlock(const PrintToken& token, const PrintToken* next) const {
    return impl_->AfterBlock(token, next);
}

void FormatListContinuation::RecordSelection(const SyntaxNode* open, int itemIndent, int closeIndent) {
    impl_->selections_.insert_or_assign(open->parent, Impl::Selection{itemIndent, closeIndent});
}

std::optional<int> FormatListContinuation::SelectedItemIndent(const SyntaxNode* list) const {
    const auto selected = impl_->selections_.find(list);
    return selected == impl_->selections_.end() ? std::nullopt : std::optional(selected->second.itemIndent);
}

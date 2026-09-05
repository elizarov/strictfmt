#include "format/impl/format_list_continuation.h"

#include <algorithm>
#include <vector>

#include "format/impl/format_break_emitter.h"
#include "format/impl/format_break_model.h"
#include "format/impl/format_syntax_helpers.h"

namespace {

struct MandatoryBlockSplitListContext {
    const SyntaxNode* openToken = nullptr;
    const SyntaxNode* list = nullptr;
    const SyntaxNode* itemRightBrace = nullptr;
    const SyntaxNode* closeToken = nullptr;
    int itemIndent = 0;
    int closeIndent = 0;
    bool afterItemClose = false;
};

struct MandatoryBlockSplitListPlan {
    FormatBreakModelContext breakContext;
    std::vector<MandatoryBlockSplitListContext> deferredContexts;
};

struct PreprocessorSplitListContext {
    const SyntaxNode* list = nullptr;
    const SyntaxNode* closeToken = nullptr;
    int itemIndent = 0;
    int closeIndent = 0;
};

struct PreprocessorSplitListPlan {
    FormatBreakModelContext breakContext;
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
    explicit Impl(std::span<const PrintToken> tokens) : tokens_(tokens) {}

    std::span<const PrintToken> tokens_;
    std::vector<MandatoryBlockSplitListContext> mandatoryBlockSplitListContexts_;
    std::vector<PreprocessorSplitListContext> preprocessorSplitListContexts_;

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
        return token.kind == PrintTokenKind::Preprocessor &&
            PrintTokenSyntaxHasClass(token, SyntaxNodeClass::ConditionalPreprocessorOpen);
    }

    static const SyntaxNode* NearestPreprocessorSplitListAncestor(const PrintToken& token) {
        for (const SyntaxNode* cursor = token.node; cursor != nullptr; cursor = cursor->parent) {
            if (SyntaxNodeKindHasClass(cursor->kind, SyntaxNodeClass::PreprocessorSplitList)) {
                return cursor;
            }
        }
        return nullptr;
    }

    std::optional<size_t> FindTokenIndex(const SyntaxNode* node, size_t begin) const {
        if (node == nullptr) {
            return std::nullopt;
        }
        for (size_t index = begin; index < tokens_.size(); ++index) {
            if (tokens_[index].node == node) {
                return index;
            }
        }
        return std::nullopt;
    }

    const SyntaxNode*
        FindPendingOpeningDelimiterFor(const SyntaxNode* list, std::span<const PrintToken> pendingTokens_) const
    {
        for (auto token = pendingTokens_.rbegin(); token != pendingTokens_.rend(); ++token) {
            if (
                token->kind == PrintTokenKind::Known &&
                PrintTokenSyntaxHasClass(*token, SyntaxNodeClass::OpeningDelimiter) &&
                PrintTokenSyntaxPathContains(*token, list)
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
                PrintTokenSyntaxPathContains(candidate, list)
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
            result.breakContext.virtualDelimiters.push_back({
                .open = listOpen,
                .close = FormatBreakToken{&tokens_[*closeIndex], false, true},
                .forceSplit = hasFollowingListItem || HasDirectCommentChild(*list),
            });
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
        FormatBreakToken virtualClose{&tokens_[*closeIndex], false, true};
        return PreprocessorSplitListPlan{
            .breakContext = {.virtualDelimiters = {{.open = listOpen, .close = virtualClose, .forceSplit = true}}},
            .deferredContext = {
                .list = list,
                .closeToken = listClose,
                .itemIndent = itemIndentLevel,
                .closeIndent = std::max(0, itemIndentLevel - 1),
            },
        };
    }

    MandatoryBlockSplitListContext* ActiveMandatoryBlockSplitListContext() {
        return mandatoryBlockSplitListContexts_.empty() ? nullptr : &mandatoryBlockSplitListContexts_.back();
    }

    PreprocessorSplitListContext* ActivePreprocessorSplitListContextFor(const PrintToken& token) {
        for (
            auto context = preprocessorSplitListContexts_.rbegin();
            context != preprocessorSplitListContexts_.rend();
            ++context
        ) {
            if (PrintTokenSyntaxPathContains(token, context->list)) {
                return &*context;
            }
        }
        return nullptr;
    }

    static bool IsForcedLeadingPreprocessorListComma(const PrintToken& token) {
        return token.forcedLeadingPreprocessorListComma;
    }

    bool IsFinalPreprocessorSplitListItem(size_t currentTokenIndex_) const {
        const PrintToken& token = tokens_[currentTokenIndex_];

        if (token.node == nullptr) {
            return false;
        }
        const SyntaxNode* list = NearestPreprocessorSplitListAncestor(token);
        const SyntaxNode* open = list == nullptr ? nullptr : DirectOpeningDelimiterChild(*list);
        const SyntaxNode* close = list == nullptr ? nullptr : DirectMatchingClosingDelimiterChild(*list, open);
        if (close == nullptr) {
            return false;
        }

        for (size_t index = currentTokenIndex_ + 1; index < tokens_.size(); ++index) {
            const PrintToken& candidate = tokens_[index];
            if (IsStructuralTriviaToken(candidate)) {
                continue;
            }
            if (!PrintTokenSyntaxPathContains(candidate, list)) {
                continue;
            }
            return candidate.kind == PrintTokenKind::Known && candidate.node == close;
        }
        return false;
    }

    void MarkMandatoryBlockSplitListItemClosed(const PrintToken& token) {
        for (MandatoryBlockSplitListContext& context : mandatoryBlockSplitListContexts_) {
            if (context.itemRightBrace == token.node) {
                context.afterItemClose = true;
            }
        }
    }

    void RetireFinishedMandatoryBlockSplitListContexts(const PrintToken& token) {
        while (!mandatoryBlockSplitListContexts_.empty()) {
            const MandatoryBlockSplitListContext& context = mandatoryBlockSplitListContexts_.back();
            if (
                context.closeToken != nullptr ||
                !context.afterItemClose ||
                token.node == nullptr ||
                PrintTokenSyntaxPathContains(token, context.list)
            ) {
                return;
            }
            mandatoryBlockSplitListContexts_.pop_back();
        }
    }

    std::optional<MandatoryBlockSplitListPlan> blockPlan_;
    std::optional<PreprocessorSplitListPlan> preprocessorPlan_;

    const FormatBreakModelContext* PlanBlock(size_t index) {
        blockPlan_ = BuildMandatoryBlockSplitListPlan(index);
        return blockPlan_ ? &blockPlan_->breakContext : nullptr;
    }
    std::optional<int> AcceptBlock(std::span<const FormatBreakSplitList> splitContexts) {
        std::optional<int> splitListItemIndent;
        if (blockPlan_) {
            for (
                auto context = blockPlan_->deferredContexts.rbegin();
                context != blockPlan_->deferredContexts.rend();
                ++context
            ) {
                const auto selected = std::find_if(
                    splitContexts.begin(), splitContexts.end(), [&](const FormatBreakSplitList& candidate) {
                        return candidate.openToken == context->openToken;
                    }
                );
                if (selected != splitContexts.end()) {
                    context->itemIndent = selected->itemIndent;
                    context->closeIndent = selected->closeIndent;
                    splitListItemIndent = context->itemIndent;
                    mandatoryBlockSplitListContexts_.push_back(*context);
                }
            }
        }
        return splitListItemIndent;
    }
    const FormatBreakModelContext* PlanPreprocessor(size_t index, std::span<const PrintToken> pending, int itemIndent) {
        preprocessorPlan_ = BuildPreprocessorSplitListPlan(index, pending, itemIndent);
        return preprocessorPlan_ ? &preprocessorPlan_->breakContext : nullptr;
    }
    int AcceptPreprocessor() {
        preprocessorSplitListContexts_.push_back(preprocessorPlan_->deferredContext);
        return preprocessorSplitListContexts_.back().itemIndent;
    }
    std::optional<int> PreprocessorIndent(const PrintToken& token) {
        const auto* context = ActivePreprocessorSplitListContextFor(token);
        return context == nullptr ? std::nullopt : std::optional(context->itemIndent);
    }
    bool ContinuesList(const PrintToken& token) const {
        return std::any_of(
            mandatoryBlockSplitListContexts_.begin(),
            mandatoryBlockSplitListContexts_.end(),
            [&](const MandatoryBlockSplitListContext& context) { return ListOwnsToken(context.list, token); }
        ) || std::any_of(
            preprocessorSplitListContexts_.begin(),
            preprocessorSplitListContexts_.end(),
            [&](const PreprocessorSplitListContext& context) { return ListOwnsToken(context.list, token); }
        );
    }
    std::optional<int> CloseBlock(const PrintToken& token, const PrintToken* next) {
        MarkMandatoryBlockSplitListItemClosed(token);
        std::optional<int> splitListContinuationIndent;
        if (
            MandatoryBlockSplitListContext* context = ActiveMandatoryBlockSplitListContext();
            context != nullptr &&
            context->afterItemClose &&
            next != nullptr &&
            next->node != context->closeToken &&
            !IsListComma(*next, context->list) &&
            ListOwnsToken(context->list, *next)
        ) {
            splitListContinuationIndent = context->itemIndent;
        }
        return splitListContinuationIndent;
    }
    std::optional<FormatListContinuationBreak> TakeBoundary(const PrintToken& token, FormatListContinuationKind kind) {
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
                preprocessorSplitListContexts_.pop_back();
                return FormatListContinuationBreak{true, indent};
            }
        } else {
            const MandatoryBlockSplitListContext* context = ActiveMandatoryBlockSplitListContext();
            if (context == nullptr || !context->afterItemClose) {
                return std::nullopt;
            }
            if (IsListComma(token, context->list)) {
                return FormatListContinuationBreak{false, context->itemIndent};
            }
            if (token.kind == PrintTokenKind::Known && context->closeToken == token.node) {
                const int indent = context->closeIndent;
                mandatoryBlockSplitListContexts_.pop_back();
                return FormatListContinuationBreak{true, indent};
            }
        }
        return std::nullopt;
    }
    std::optional<bool> ConditionalDirectiveComma(size_t index) const {
        const PrintToken& token = tokens_[index];
        const SyntaxNode* list =
            StartsPreprocessorSplitList(token) ? NearestPreprocessorSplitListAncestor(token) : nullptr;
        if (list == nullptr) {
            return std::nullopt;
        }
        const SyntaxNode* open = DirectOpeningDelimiterChild(*list);
        return open != nullptr &&
            open->kind == SyntaxNodeKind::LeftBrace &&
            SyntaxNodeHasClass(*list, SyntaxNodeClass::AllowedListPreprocessorContainer);
    }
};

FormatListContinuation::FormatListContinuation(std::span<const PrintToken> tokens) :
    impl_(std::make_unique<Impl>(tokens)) {}
FormatListContinuation::~FormatListContinuation() = default;
const FormatBreakModelContext* FormatListContinuation::PlanBlock(size_t index) { return impl_->PlanBlock(index); }
std::optional<int> FormatListContinuation::AcceptBlock(std::span<const FormatBreakSplitList> selected) {
    return impl_->AcceptBlock(selected);
}
const FormatBreakModelContext*
    FormatListContinuation::PlanPreprocessor(size_t index, std::span<const PrintToken> pending, int itemIndent)
{
    return impl_->PlanPreprocessor(index, pending, itemIndent);
}
int FormatListContinuation::AcceptPreprocessor() { return impl_->AcceptPreprocessor(); }
std::optional<int> FormatListContinuation::PreprocessorIndent(const PrintToken& token) const {
    return impl_->PreprocessorIndent(token);
}
std::optional<bool> FormatListContinuation::ConditionalDirectiveComma(size_t index) const {
    return impl_->ConditionalDirectiveComma(index);
}
bool FormatListContinuation::IsFinalPreprocessorItem(size_t index) const {
    return impl_->IsFinalPreprocessorSplitListItem(index);
}
std::optional<FormatListContinuationBreak>
    FormatListContinuation::TakeBoundary(const PrintToken& token, FormatListContinuationKind kind)
{
    return impl_->TakeBoundary(token, kind);
}
void FormatListContinuation::BeforeToken(const PrintToken& token) {
    impl_->RetireFinishedMandatoryBlockSplitListContexts(token);
}
bool FormatListContinuation::ContinuesList(const PrintToken& token) const { return impl_->ContinuesList(token); }
std::optional<int> FormatListContinuation::CloseBlock(const PrintToken& token, const PrintToken* next) {
    return impl_->CloseBlock(token, next);
}

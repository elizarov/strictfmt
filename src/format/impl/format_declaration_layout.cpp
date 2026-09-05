#include "format/impl/format_declaration_layout.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "format/impl/format_break_model_builder.h"
#include "format/impl/format_break_solver.h"
#include "format/impl/format_model_text_stats.h"

namespace {

enum class DeclarationGroupKind {
    None,
    Type,
    ForwardType,
    Callable,
    Object,
    Alias,
};

struct DeclarationGroupState {
    const SyntaxNode* previousItem = nullptr;
    const SyntaxNode* preparedItem = nullptr;
};

struct CachedDeclarationLayout {
    std::uint32_t endSourceIndex = 0;
    int startColumn = 0;
    int baseIndentLevel = 0;
    FormatBreakModel model;
    FormatBreakSolution solution;
};

const SyntaxNode* DeclarationScopeItem(const SyntaxNode* node) {
    for (const SyntaxNode* cursor = node; cursor != nullptr && cursor->parent != nullptr; cursor = cursor->parent) {
        // DeclarationScope is a syntax-local normalized class, so its authoritative value is stored on the node.
        if ((cursor->parent->classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationScope)) != 0) {
            return cursor;
        }
    }
    return nullptr;
}

}  // namespace

struct FormatDeclarationLayout::Impl {
    const FormatterConfig& config_;
    std::span<const PrintToken> tokens_;
    FormatModelTextStats* stats_;
    int indentWidth_;
    std::unordered_set<const SyntaxNode*> isolatedDeclarationItems_;
    std::unordered_map<const SyntaxNode*, DeclarationGroupState> declarationGroupStates_;
    std::vector<std::unique_ptr<CachedDeclarationLayout>> declarationLayoutsBySourceIndex_;
    std::vector<const SyntaxNode*> nextDeclarationItemsBySourceIndex_;

    Impl(
        const FormatterConfig& config,
        std::span<const PrintToken> tokens,
        std::span<const std::uint8_t> mandatoryBlockOpens,
        FormatModelTextStats* stats
    ) : config_(config), tokens_(tokens), stats_(stats), indentWidth_(std::max(1, config.indentWidth)) {
        AnalyzeDeclarationGroups(tokens, mandatoryBlockOpens);
    }

    static DeclarationGroupKind DeclarationGroup(const SyntaxNode* item) {
        if (item == nullptr) {
            return DeclarationGroupKind::None;
        }
        // Declaration-group classes are normalized per node and intentionally excluded from static kind classes.
        // Reading the stored bits is therefore equivalent to five generic class queries and reuses that analysis.
        const std::uint64_t classes = item->classes;
        if ((classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupType)) != 0) {
            return DeclarationGroupKind::Type;
        }
        if ((classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupForwardType)) != 0) {
            return DeclarationGroupKind::ForwardType;
        }
        if ((classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupCallable)) != 0) {
            return DeclarationGroupKind::Callable;
        }
        if ((classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupObject)) != 0) {
            return DeclarationGroupKind::Object;
        }
        if ((classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupAlias)) != 0) {
            return DeclarationGroupKind::Alias;
        }
        return DeclarationGroupKind::None;
    }

    bool HasLargeDeclarationValue(
        const FormatBreakModel& model, const FormatBreakSolution& solution, const SyntaxNode& item
    ) const {
        if (model.nodes == nullptr) {
            return false;
        }
        return std::any_of(model.nodes->begin(), model.nodes->end(), [&](const FormatBreakNode& node) {
            const size_t index = static_cast<size_t>(node.id);
            return node.declarationValueOwner != nullptr &&
                DeclarationScopeItem(node.declarationValueOwner) == &item &&
                index < solution.declarationValueContinuationLines.size() &&
                solution.declarationValueContinuationLines[index] > 1;
        });
    }

    static int DeclarationIndent(const SyntaxNode& item) {
        int indent = 0;
        for (const SyntaxNode* cursor = item.parent; cursor != nullptr; cursor = cursor->parent) {
            if (cursor->kind == SyntaxNodeKind::FieldDeclarationList) {
                ++indent;
            }
        }
        return indent;
    }

    bool HasProvablyCompactDeclarationLayout(
        std::span<const PrintToken> tokens,
        size_t begin,
        size_t end,
        int declarationIndent,
        std::span<const std::uint8_t> mandatoryBlockOpens
    ) const {
        int column = declarationIndent * indentWidth_;
        bool hasText = false;
        bool previousStringLike = false;
        const PrintToken* previous = nullptr;
        for (size_t index = begin; index < end; ++index) {
            const PrintToken& token = tokens[index];
            if (
                (token.kind != PrintTokenKind::Known && token.kind != PrintTokenKind::Text) ||
                token.containsSourceLineBreak ||
                token.inMacroValue ||
                token.macroDefinition != nullptr ||
                token.inMacroStatementSequence ||
                token.inLeadingStreamOperatorChain ||
                token.inConditionalStreamOperatorChain ||
                token.inTemplateDeclaration ||
                (token.stringLike && previousStringLike) ||
                token.inFieldInitializerList ||
                mandatoryBlockOpens[index] != 0
            ) {
                return false;
            }
            if (FormatTokenNeedsSpace(previous, token) && hasText) {
                ++column;
            }
            const int tokenWidth = FormatTokenWidth(token);
            column += tokenWidth;
            if (column > config_.columnLimit) {
                return false;
            }
            hasText = hasText || tokenWidth > 0;
            previous = &token;
            previousStringLike = token.stringLike;
        }
        // A compact one-line declaration has zero continuation lines; no solved layout can make it an isolated
        // declaration-group item under the same compact eligibility and width checks.
        return true;
    }

    void AnalyzeDeclarationGroups(std::span<const PrintToken> tokens, std::span<const std::uint8_t> mandatoryBlockOpens)
    {
        // A large declaration value must be known before its first token is emitted so the mandatory blank line
        // can precede it. Pre-solving uses the ordinary break model and solver; the printer only observes the
        // number of continuation lines selected for each declaration owner/value relation.
        isolatedDeclarationItems_.clear();
        declarationGroupStates_.clear();
        declarationLayoutsBySourceIndex_.clear();
        declarationLayoutsBySourceIndex_.resize(tokens.size());
        nextDeclarationItemsBySourceIndex_.resize(tokens.size());
        const SyntaxNode* nextDeclarationItem = nullptr;
        for (size_t index = tokens.size(); index-- > 0;) {
            nextDeclarationItemsBySourceIndex_[index] = nextDeclarationItem;
            const SyntaxNode* item = tokens[index].declarationScopeItem;
            if (DeclarationGroup(item) != DeclarationGroupKind::None) {
                nextDeclarationItem = item;
            }
        }
        std::unordered_set<const SyntaxNode*> analyzedItems;
        for (size_t index = 0; index < tokens.size();) {
            const SyntaxNode* item = tokens[index].declarationScopeItem;
            if (item == nullptr || !analyzedItems.insert(item).second) {
                ++index;
                continue;
            }
            const DeclarationGroupKind group = DeclarationGroup(item);
            if (group != DeclarationGroupKind::Object && group != DeclarationGroupKind::Alias) {
                ++index;
                continue;
            }
            size_t end = index + 1;
            while (end < tokens.size() && PrintTokenSyntaxPathContains(tokens[end], item)) {
                ++end;
            }
            const int declarationIndent = DeclarationIndent(*item);
            if (HasProvablyCompactDeclarationLayout(tokens, index, end, declarationIndent, mandatoryBlockOpens)) {
                ++index;
                continue;
            }
            const auto modelStart =
                stats_ == nullptr ? std::chrono::steady_clock::time_point{} : std::chrono::steady_clock::now();
            FormatBreakModel model =
                BuildFormatBreakModel(std::span<const PrintToken>{tokens.data() + index, end - index});
            if (stats_ != nullptr) {
                stats_->breakModel += std::chrono::steady_clock::now() - modelStart;
            }
            const bool hasDeclarationValue = model.nodes != nullptr &&
                std::any_of(model.nodes->begin(), model.nodes->end(), [&](const FormatBreakNode& node) {
                    return node.declarationValueOwner != nullptr &&
                        DeclarationScopeItem(node.declarationValueOwner) == item;
                });
            if (!hasDeclarationValue || model.root == nullptr) {
                ++index;
                continue;
            }
            const auto solveStart =
                stats_ == nullptr ? std::chrono::steady_clock::time_point{} : std::chrono::steady_clock::now();
            FormatBreakSolution solution =
                SolveFormatBreaks(config_, model, declarationIndent * indentWidth_, declarationIndent, indentWidth_, 0);
            if (stats_ != nullptr) {
                stats_->solve += std::chrono::steady_clock::now() - solveStart;
            }
            if (HasLargeDeclarationValue(model, solution, *item)) {
                isolatedDeclarationItems_.insert(item);
            }
            const std::uint32_t sourceIndex = tokens[index].sourceIndex;
            if (sourceIndex < declarationLayoutsBySourceIndex_.size()) {
                declarationLayoutsBySourceIndex_[sourceIndex] =
                    std::make_unique<CachedDeclarationLayout>(CachedDeclarationLayout{
                        .endSourceIndex = tokens[end - 1].sourceIndex + 1,
                        .startColumn = declarationIndent * indentWidth_,
                        .baseIndentLevel = declarationIndent,
                        .model = std::move(model),
                        .solution = std::move(solution),
                    });
            }
            ++index;
        }
    }

    const CachedDeclarationLayout* ReusableDeclarationLayout(
        std::span<const PrintToken> pendingTokens,
        const FormatBreakModelContext& context,
        int startColumn,
        int baseIndentLevel,
        int breakLineSuffixWidth
    ) const {
        if (
            breakLineSuffixWidth != 0 ||
            context.forceSplitStreamChain ||
            !context.virtualDelimiters.empty() ||
            context.requiredChainBreakOperators != nullptr ||
            context.requiredChainBreakBaseIndents != nullptr ||
            pendingTokens.empty()
        ) {
            return nullptr;
        }
        const std::uint32_t begin = pendingTokens.front().sourceIndex;
        if (begin >= declarationLayoutsBySourceIndex_.size()) {
            return nullptr;
        }
        const std::unique_ptr<CachedDeclarationLayout>& cached = declarationLayoutsBySourceIndex_[begin];
        if (
            cached == nullptr ||
            cached->endSourceIndex != pendingTokens.back().sourceIndex + 1 ||
            cached->endSourceIndex - begin != pendingTokens.size() ||
            cached->startColumn != startColumn ||
            cached->baseIndentLevel != baseIndentLevel
        ) {
            return nullptr;
        }
        // The complete consecutive source-token span, incoming state, suffix width, and every model context input
        // are identical to declaration pre-analysis. Break-model construction and solving are pure in those inputs,
        // so reusing both objects produces the same choices and token emission as rebuilding them here.
        return cached.get();
    }

    bool RequiresDeclarationGroupSeparation(const SyntaxNode* left, const SyntaxNode* right) const {
        if (
            left == nullptr ||
            right == nullptr ||
            left->parent == nullptr ||
            left->parent != right->parent ||
            !SyntaxNodeHasClass(*left->parent, SyntaxNodeClass::DeclarationScope)
        ) {
            return false;
        }
        if (isolatedDeclarationItems_.contains(left) || isolatedDeclarationItems_.contains(right)) {
            return true;
        }
        const DeclarationGroupKind leftGroup = DeclarationGroup(left);
        const DeclarationGroupKind rightGroup = DeclarationGroup(right);
        if (leftGroup == DeclarationGroupKind::None || rightGroup == DeclarationGroupKind::None) {
            return false;
        }
        if (leftGroup == DeclarationGroupKind::ForwardType && rightGroup == DeclarationGroupKind::ForwardType) {
            return false;
        }
        return leftGroup == DeclarationGroupKind::Type ||
            leftGroup == DeclarationGroupKind::ForwardType ||
            rightGroup == DeclarationGroupKind::Type ||
            rightGroup == DeclarationGroupKind::ForwardType ||
            leftGroup != rightGroup;
    }

    const SyntaxNode* NextDeclarationItem(size_t index) const {
        if (index >= nextDeclarationItemsBySourceIndex_.size()) {
            return nullptr;
        }
        // This is the exact result of the former forward scan: the immutable token order is summarized backward,
        // replacing repeated searches without changing which following declaration item is selected.
        return nextDeclarationItemsBySourceIndex_[index];
    }

    bool NeedsBlankLineBefore(size_t index) {
        const PrintToken& token = tokens_[index];
        bool blankLine = false;
        const SyntaxNode* item = token.declarationScopeItem;
        const DeclarationGroupKind group = DeclarationGroup(item);
        if (group != DeclarationGroupKind::None) {
            DeclarationGroupState& state = declarationGroupStates_[item->parent];
            if (item != state.previousItem) {
                if (item != state.preparedItem && RequiresDeclarationGroupSeparation(state.previousItem, item)) {
                    blankLine = true;
                }
                state.previousItem = item;
                state.preparedItem = nullptr;
            }
            return blankLine;
        }
        if (item == nullptr || item->parent == nullptr) {
            return blankLine;
        }
        // A declaration terminator may be a declaration-scope sibling when the parser flattens a bare
        // class, struct, or enum declaration. It completes the preceding group item; it is not a prefix
        // of the next declaration.
        if (
            token.kind == PrintTokenKind::TrailingComment ||
            (token.node != nullptr && token.node->kind == SyntaxNodeKind::Semicolon)
        ) {
            return blankLine;
        }

        DeclarationGroupState& state = declarationGroupStates_[item->parent];
        const SyntaxNode* nextItem = NextDeclarationItem(index);
        const bool prefixesNextItem = nextItem != nullptr && nextItem->parent == item->parent;
        const bool separates = prefixesNextItem && RequiresDeclarationGroupSeparation(state.previousItem, nextItem);
        if (separates && state.preparedItem != nextItem) {
            blankLine = true;
            state.preparedItem = nextItem;
        }
        return blankLine;
    }

};

FormatDeclarationLayout::FormatDeclarationLayout(
    const FormatterConfig& config,
    std::span<const PrintToken> tokens,
    std::span<const std::uint8_t> mandatoryBlockOpens,
    FormatModelTextStats* stats
) : impl_(std::make_unique<Impl>(config, tokens, mandatoryBlockOpens, stats)) {}

FormatDeclarationLayout::~FormatDeclarationLayout() = default;

bool FormatDeclarationLayout::NeedsBlankLineBefore(size_t tokenIndex) {
    return impl_->NeedsBlankLineBefore(tokenIndex);
}

std::optional<FormatDeclarationLayoutView> FormatDeclarationLayout::FindReusableLayout(
    std::span<const PrintToken> tokens,
    const FormatBreakModelContext& context,
    int startColumn,
    int baseIndentLevel,
    int breakLineSuffixWidth
) const {
    const CachedDeclarationLayout* cached =
        impl_->ReusableDeclarationLayout(tokens, context, startColumn, baseIndentLevel, breakLineSuffixWidth);
    if (cached == nullptr) {
        return std::nullopt;
    }
    return FormatDeclarationLayoutView{&cached->model, &cached->solution};
}

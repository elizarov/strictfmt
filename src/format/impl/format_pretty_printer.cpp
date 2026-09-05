#include "format/impl/format_pretty_printer.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "format/impl/format_break_model_builder.h"
#include "format/impl/format_break_emitter.h"
#include "format/impl/format_break_model_dump.h"
#include "format/impl/format_break_model_inline_helpers.h"
#include "format/impl/format_break_solver.h"
#include "format/impl/format_include_sort.h"
#include "format/impl/format_declaration_layout.h"
#include "format/impl/format_raw_macro.h"
#include "format/impl/format_preprocessor_text.h"
#include "format/impl/format_print_token_builder.h"
#include "format/impl/format_spacing.h"
#include "format/impl/format_output.h"
#include "format/impl/format_list_continuation.h"
#include "format/impl/format_syntax_helpers.h"
#include "format/impl/format_chain_continuation.h"
#include "tools/tools_common.h"
#include "util/utf8.h"

namespace {

enum class BraceRole {
    Compact,
    Block,
    Enum,
    NamespaceLike,
    CaseBlock,
};

struct BraceFrame {
    BraceRole role = BraceRole::Compact;
    int parenDepth = 0;
    int indentRestore = 0;
    int closeIndent = 0;
};

bool BreakModelHasLayoutChoice(const FormatBreakModel& model) {
    // Token and sequence nodes have exactly one layout. Emitting them with the default compact solution is the same
    // as running the solver, while a dump still runs it so the diagnostic model remains complete. The builder sets
    // this summary monotonically for every created node, making it equivalent to scanning the completed model.
    return model.hasLayoutChoice;
}

BraceRole RoleForBraceParent(SyntaxNodeKind parentKind) {
    switch (parentKind) {
        case SyntaxNodeKind::CompoundStatement:
        case SyntaxNodeKind::FieldDeclarationList:
        case SyntaxNodeKind::DeclarationList:
        case SyntaxNodeKind::RequirementSeq:
            return BraceRole::Block;
        case SyntaxNodeKind::EnumeratorList:
            return BraceRole::Enum;
        default:
            return BraceRole::Compact;
    }
}

bool IsNamespaceDefinitionDeclarationList(const PrintToken& token) {
    return token.parentKind == SyntaxNodeKind::DeclarationList &&
        token.grandParentKind == SyntaxNodeKind::NamespaceDefinition;
}

bool IsLinkageSpecificationDeclarationList(const PrintToken& token) {
    return token.parentKind == SyntaxNodeKind::DeclarationList &&
        token.grandParentKind == SyntaxNodeKind::LinkageSpecification;
}

bool IsNamespaceLikeBrace(const PrintToken& token) {
    return token.parentKind == SyntaxNodeKind::LinkageSpecification ||
        IsNamespaceDefinitionDeclarationList(token) ||
        IsLinkageSpecificationDeclarationList(token);
}

BraceRole RoleForBrace(const PrintToken& token) {
    if (
        token.inCompactSingleStatementBody &&
        token.parentKind == SyntaxNodeKind::CompoundStatement &&
        token.grandParentKind == SyntaxNodeKind::LambdaExpression
    ) {
        return BraceRole::Compact;
    }
    if (IsNamespaceLikeBrace(token)) {
        return BraceRole::NamespaceLike;
    }
    return RoleForBraceParent(token.parentKind);
}

bool IsCompactSingleStatementFunctionBodyBrace(const PrintToken& token) {
    return token.inCompactSingleStatementBody &&
        token.parentKind == SyntaxNodeKind::CompoundStatement &&
        token.grandParentKind == SyntaxNodeKind::FunctionDefinition;
}

bool IsDeclarationModifierPreprocessorToken(const PrintToken& token) {
    return token.node != nullptr &&
        (token.node->classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationModifierPreprocessor)) != 0;
}

bool IsConditionalRhsPreprocessorToken(const PrintToken& token) {
    return token.node != nullptr &&
        (token.node->classes & static_cast<std::uint64_t>(SyntaxNodeClass::ConditionalRhsPreprocessor)) != 0;
}

bool HasDirectListDelimiterPair(const SyntaxNode& node) {
    for (const SyntaxNode* child : node.children) {
        if (child == nullptr || !SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::OpeningDelimiter)) {
            continue;
        }
        const SyntaxNodeKind close = MatchingListCloseToken(child->kind);
        if (close != SyntaxNodeKind::Unknown && HasDirectKnownChild(node, close)) {
            return true;
        }
    }
    return false;
}

bool HasDirectDelimiterPair(const SyntaxNode& node, SyntaxNodeKind openKind) {
    return HasDirectKnownChild(node, openKind) && HasDirectKnownChild(node, MatchingListCloseToken(openKind));
}

bool HasDirectListPrefix(const SyntaxNode& node) { return HasDirectKnownChild(node, SyntaxNodeKind::Colon); }

bool IsSeparatedListContainer(const SyntaxNode& node) {
    if (HasDirectKnownChild(node, SyntaxNodeKind::Comma)) {
        return HasDirectListDelimiterPair(node) || HasDirectListPrefix(node);
    }
    return
        HasDirectKnownChild(node, SyntaxNodeKind::Semicolon) && HasDirectDelimiterPair(node, SyntaxNodeKind::LeftParen);
}

bool SyntaxSubtreeEndsWith(const SyntaxNode& node, SyntaxNodeKind kind) {
    if (node.kind == kind) {
        return true;
    }
    for (auto child = node.children.rbegin(); child != node.children.rend(); ++child) {
        if (*child == nullptr || SyntaxNodeHasClass(**child, SyntaxNodeClass::Trivia)) {
            continue;
        }
        return SyntaxSubtreeEndsWith(**child, kind);
    }
    return false;
}

bool TrailingCommentReturnsToStructuralIndent(const PrintToken& token) {
    if (token.node == nullptr || token.node->parent == nullptr) {
        return false;
    }
    if (SyntaxNodeKindHasClass(token.node->parent->kind, SyntaxNodeClass::CompoundBlock)) {
        return true;
    }
    const SyntaxNode* previous = nullptr;
    for (const SyntaxNode* child : token.node->parent->children) {
        if (child == token.node) {
            break;
        }
        if (child != nullptr && !SyntaxNodeHasClass(*child, SyntaxNodeClass::Trivia)) {
            previous = child;
        }
    }
    return previous != nullptr && (
        previous->kind == SyntaxNodeKind::LeftBrace || (
            previous->kind == SyntaxNodeKind::TemplateParameterList &&
            token.node->parent->kind == SyntaxNodeKind::TemplateDeclaration
        ) ||
        SyntaxSubtreeEndsWith(*previous, SyntaxNodeKind::RightBrace) ||
        (previous->kind == SyntaxNodeKind::Colon && token.node->parent->kind == SyntaxNodeKind::CaseStatement)
    );
}

bool HasSeparatedListAncestor(const SyntaxNode* node) {
    for (
        const SyntaxNode* cursor = node == nullptr ? nullptr : node->parent;
        cursor != nullptr;
        cursor = cursor->parent
    ) {
        if (
            SyntaxNodeHasClass(*cursor, SyntaxNodeClass::SemanticDelimitedParent) || IsSeparatedListContainer(*cursor)
        ) {
            return true;
        }
    }
    return false;
}

const SyntaxNode* BraceListTerminalCommaOpen(const PrintToken& token) {
    if (
        token.kind != PrintTokenKind::Known ||
        token.syntaxKind != SyntaxNodeKind::Comma ||
        token.node == nullptr ||
        token.node->parent == nullptr ||
        !SyntaxNodeHasClass(*token.node->parent, SyntaxNodeClass::AllowedListPreprocessorContainer)
    ) {
        return nullptr;
    }
    const SyntaxNode* open = nullptr;
    bool afterComma = false;
    for (const SyntaxNode* child : token.node->parent->children) {
        if (child != nullptr && child->kind == SyntaxNodeKind::LeftBrace) {
            open = child;
        }
        if (child == token.node) {
            afterComma = true;
            continue;
        }
        if (!afterComma || child == nullptr || SyntaxNodeHasClass(*child, SyntaxNodeClass::Trivia)) {
            continue;
        }
        return child->kind == SyntaxNodeKind::RightBrace ? open : nullptr;
    }
    return nullptr;
}

bool IsFormatterOwnedChain(const SyntaxNode& node) {
    if (
        node.kind != SyntaxNodeKind::FieldExpression &&
        node.kind != SyntaxNodeKind::BinaryExpression &&
        node.kind != SyntaxNodeKind::ConditionalExpression
    ) {
        return false;
    }
    return std::any_of(node.children.begin(), node.children.end(), [&node](const SyntaxNode* child) {
        return child != nullptr && (
            SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::ChainOperator) || (
                node.kind == SyntaxNodeKind::FieldExpression &&
                (child->kind == SyntaxNodeKind::Dot || child->kind == SyntaxNodeKind::Arrow)
            ) ||
            (
                node.kind == SyntaxNodeKind::ConditionalExpression &&
                (child->kind == SyntaxNodeKind::Question || child->kind == SyntaxNodeKind::Colon)
            )
        );
    });
}

bool KeepsStructuralCommentInBreakModel(const PrintToken& token) {
    if (!IsCommentToken(token.kind)) {
        return false;
    }
    return HasSeparatedListAncestor(token.node) ||
        (token.node != nullptr && token.node->parent != nullptr && IsFormatterOwnedChain(*token.node->parent));
}

class Printer final : private FormatBreakOutput {
public:
    Printer(
        const FormatterConfig& config,
        std::string_view sourcePath,
        FormatModelTextStats* stats,
        FormatBreakModelDumpWriter* breakModelDump = nullptr
    ) :
        config_(config),
        sourcePath_(sourcePath),
        stats_(stats),
        breakModelDump_(breakModelDump),
        indentWidth_(std::max(1, config.indentWidth)),
        tabWidth_(std::max(1, config.tabWidth)),
        output_(indentWidth_, config.columnLimit) {}

    std::string Print(const std::vector<PrintToken>& tokens, size_t sourceSize) {
        activeTokens_ = &tokens;
        listContinuation_ = std::make_unique<FormatListContinuation>(tokens);
        chainContinuation_ = std::make_unique<FormatChainContinuation>(tokens);
        std::vector<std::uint8_t> mandatoryBlockOpens(tokens.size());
        for (size_t index = 0; index < tokens.size(); ++index) {
            mandatoryBlockOpens[index] = IsMandatoryBlockOpen(index);
        }
        declarationLayout_ = std::make_unique<FormatDeclarationLayout>(config_, tokens, mandatoryBlockOpens, stats_);
        output_.Reserve(std::max(tokens.size() * 8, sourceSize));
        pendingTokens_.reserve(64);
        const PrintToken* previous = nullptr;
        size_t nextIndex = 0;
        for (size_t index = 0; index < tokens.size(); ++index) {
            currentTokenIndex_ = index;
            nextIndex = std::max(nextIndex, index + 1);
            while (nextIndex < tokens.size() && IsStructuralTriviaToken(tokens[nextIndex])) {
                ++nextIndex;
            }
            const PrintToken* rawPrevious = index == 0 ? nullptr : &tokens[index - 1];
            const PrintToken* next = nextIndex < tokens.size() ? &tokens[nextIndex] : nullptr;
            const PrintToken* rawNext = RawNextToken(tokens, index);
            PrintOne(tokens[index], previous, rawPrevious, next, rawNext);
            if (!IsStructuralTriviaToken(tokens[index])) {
                previous = &tokens[index];
            }
        }
        activeTokens_ = nullptr;
        FlushPendingTokens();
        return output_.Finish();
    }

private:
    const FormatterConfig& config_;
    std::string_view sourcePath_;
    FormatModelTextStats* stats_ = nullptr;
    FormatBreakModelDumpWriter* breakModelDump_ = nullptr;
    int indentWidth_ = 4;
    int tabWidth_ = 4;
    FormatOutput output_;
    std::unique_ptr<FormatListContinuation> listContinuation_;
    std::unique_ptr<FormatChainContinuation> chainContinuation_;
    std::vector<PrintToken> pendingTokens_;
    std::unique_ptr<FormatDeclarationLayout> declarationLayout_;
    bool pendingSourceBlankLine_ = false;
    int indentLevel_ = 0;
    bool emittingMacroDefinition_ = false;
    bool firstIncludeRun_ = true;
    const std::vector<PrintToken>* activeTokens_ = nullptr;
    size_t currentTokenIndex_ = 0;
    std::optional<int> emittedBlockOpenIndent_;
    std::vector<BraceRole> compactRightBraceRoles_;
    int switchDepth_ = 0;
    int parenDepth_ = 0;
    int bracketDepth_ = 0;
    std::vector<BraceFrame> braceStack_;
    std::vector<int> activeCaseBodySwitchDepths_;
    std::vector<int> conditionalFunctionIndents_;
    std::optional<int> pendingIndentRestoreAfterFlush_;
    std::unordered_set<std::uint32_t> prebufferedTokenSourceIndices_;

    static const PrintToken* RawNextToken(const std::vector<PrintToken>& tokens, size_t index) {
        return index + 1 < tokens.size() ? &tokens[index + 1] : nullptr;
    }

    void BufferFollowingAttachedComments() {
        if (activeTokens_ == nullptr) {
            return;
        }
        for (size_t index = currentTokenIndex_ + 1; index < activeTokens_->size(); ++index) {
            const PrintToken& candidate = (*activeTokens_)[index];
            if (candidate.kind == PrintTokenKind::TrailingComment) {
                BufferToken(candidate);
                prebufferedTokenSourceIndices_.insert(candidate.sourceIndex);
                return;
            }
            if (
                candidate.kind != PrintTokenKind::Text ||
                candidate.node == nullptr ||
                !SyntaxNodeHasClass(*candidate.node, SyntaxNodeClass::Comment)
            ) {
                return;
            }
            BufferToken(candidate);
            prebufferedTokenSourceIndices_.insert(candidate.sourceIndex);
        }
    }

    bool IsMandatoryBlockOpen(size_t index) const {
        if (activeTokens_ == nullptr || index >= activeTokens_->size()) {
            return false;
        }
        const PrintToken& token = (*activeTokens_)[index];
        if (token.kind != PrintTokenKind::Known || token.syntaxKind != SyntaxNodeKind::LeftBrace) {
            return false;
        }
        if (token.inConditionalFunctionHeader) {
            return true;
        }
        if (RoleForBrace(token) == BraceRole::Compact || IsCompactSingleStatementFunctionBodyBrace(token)) {
            return false;
        }
        const PrintToken* next = index + 1 < activeTokens_->size() ? &(*activeTokens_)[index + 1] : nullptr;
        return !(
            next != nullptr && next->kind == PrintTokenKind::Known && next->syntaxKind == SyntaxNodeKind::RightBrace
        ) && !BecomesEmptyAfterNullItemRemoval(token);
    }

    static bool ContinuesBlockExpression(const PrintToken& token, const PrintToken& next) {
        if (token.node == nullptr || next.node == nullptr || token.node->parent == nullptr) {
            return false;
        }
        const SyntaxNode* blockExpression = token.node->parent->parent;
        if (blockExpression == nullptr || !SyntaxNodeHasClass(*blockExpression, SyntaxNodeClass::Expression)) {
            return false;
        }
        if (PrintTokenSyntaxPathContains(next, blockExpression)) {
            return true;
        }

        for (const SyntaxNode* ancestor = blockExpression->parent; ancestor != nullptr; ancestor = ancestor->parent) {
            if (PrintTokenSyntaxPathContains(next, ancestor)) {
                if (SyntaxNodeHasClass(*ancestor, SyntaxNodeClass::Expression)) {
                    return true;
                }
                break;
            }
        }

        const SyntaxNode* continuation =
            SyntaxNodeHasClass(*next.node, SyntaxNodeClass::SemanticDelimitedParent) ? next.node : next.node->parent;
        if (continuation == nullptr || !SyntaxNodeHasClass(*continuation, SyntaxNodeClass::SemanticDelimitedParent)) {
            return false;
        }

        // Call and subscript expression wrappers are flattened. Their semantic list
        // remains a following sibling of the expression being invoked or indexed.
        if (
            (continuation == next.node || PrintTokenSyntaxHasClass(next, SyntaxNodeClass::OpeningDelimiter)) &&
            continuation->parent == blockExpression->parent
        ) {
            return true;
        }
        return false;
    }

    static bool AttachesToFollowingBlockKeyword(const PrintToken& token, const PrintToken& next) {
        if (next.kind != PrintTokenKind::Known) {
            return false;
        }
        if (
            PrintTokenSyntaxHasClass(next, SyntaxNodeClass::AttachAfterBlockKeyword) &&
            next.syntaxKind != SyntaxNodeKind::KeywordWhile
        ) {
            return true;
        }
        return next.syntaxKind == SyntaxNodeKind::KeywordWhile && next.parentKind == SyntaxNodeKind::DoStatement;
    }

    static bool ClosesDeclaredTypeBody(const PrintToken& token) {
        return (
            token.parentKind == SyntaxNodeKind::FieldDeclarationList ||
            token.parentKind == SyntaxNodeKind::EnumeratorList
        ) && SyntaxNodeKindHasClass(token.grandParentKind, SyntaxNodeClass::DeclaredTypeSpecifier);
    }

    static bool ShouldAttachAfterBlockClose(const PrintToken& token, const PrintToken* next) {
        if (next == nullptr) {
            return false;
        }
        if (ContinuesBlockExpression(token, *next) || ClosesContainingDelimiter(token, *next)) {
            return true;
        }
        if (next->kind == PrintTokenKind::Known && (
            next->syntaxKind == SyntaxNodeKind::Semicolon ||
            next->syntaxKind == SyntaxNodeKind::Comma ||
            AttachesToFollowingBlockKeyword(token, *next)
        )) {
            return true;
        }
        return ClosesDeclaredTypeBody(token);
    }

    static bool ClosesContainingDelimiter(const PrintToken& token, const PrintToken& next) {
        if (token.node == nullptr || next.node == nullptr || next.node->parent == nullptr) {
            return false;
        }
        const SyntaxNode* container = next.node->parent;
        if (!SyntaxNodeHasClass(*container, SyntaxNodeClass::SemanticDelimitedParent)) {
            return false;
        }
        return PrintTokenSyntaxPathContains(token, container) &&
            DirectMatchingClosingDelimiterChild(*container, DirectOpeningDelimiterChild(*container)) == next.node;
    }

    static const SyntaxNode* NearestAncestor(const PrintToken& token, SyntaxNodeKind kind) {
        for (const SyntaxNode* cursor = token.node; cursor != nullptr; cursor = cursor->parent) {
            if (cursor->kind == kind) {
                return cursor;
            }
        }
        return nullptr;
    }

    static bool IsStatementItemContainer(SyntaxNodeKind kind) {
        switch (kind) {
            case SyntaxNodeKind::TranslationUnit:
            case SyntaxNodeKind::DeclarationList:
            case SyntaxNodeKind::FieldDeclarationList:
            case SyntaxNodeKind::CompoundStatement:
            case SyntaxNodeKind::CaseStatement:
            case SyntaxNodeKind::PreprocIf:
            case SyntaxNodeKind::PreprocIfdef:
            case SyntaxNodeKind::PreprocElse:
            case SyntaxNodeKind::PreprocElif:
                return true;
            default:
                return false;
        }
    }

    static bool ClosesStatementPositionMacroCallItem(const PrintToken& token) {
        if (
            token.kind != PrintTokenKind::Known ||
            token.syntaxKind != SyntaxNodeKind::RightParen ||
            token.node == nullptr
        ) {
            return false;
        }
        const SyntaxNode* arguments = token.node->parent;
        if (arguments == nullptr || arguments->kind != SyntaxNodeKind::ArgumentList) {
            return false;
        }
        const SyntaxNode* macroCall = arguments->parent;
        if (macroCall == nullptr || macroCall->kind != SyntaxNodeKind::MacroCallItem) {
            return false;
        }
        const SyntaxNode* macroCallParent = macroCall->parent;
        return macroCallParent != nullptr && IsStatementItemContainer(macroCallParent->kind);
    }

    const SyntaxNode* ImmediateConditionalPreprocessorListParent(const PrintToken& token) {
        const SyntaxNode* parent = token.node == nullptr ? nullptr : token.node->parent;
        if (
            parent == nullptr ||
            !SyntaxNodeKindHasClass(parent->kind, SyntaxNodeClass::PreprocessorSplitList) ||
            (parent->classes & static_cast<std::uint64_t>(SyntaxNodeClass::ContainsConditionalPreprocessor)) == 0
        ) {
            return nullptr;
        }
        return parent;
    }

    bool AppendCompactWidthToken(
        const PrintToken& token,
        int& width,
        bool& hasText,
        const PrintToken*& previous,
        bool& previousStringLike,
        bool allowFieldInitializerList = false
    ) const {
        if (token.kind != PrintTokenKind::Known && token.kind != PrintTokenKind::Text) {
            return false;
        }
        if (
            token.inMacroValue ||
            token.macroDefinition != nullptr ||
            (token.stringLike && previousStringLike) ||
            (!allowFieldInitializerList && token.inFieldInitializerList)
        ) {
            return false;
        }
        if (FormatTokenNeedsSpace(previous, token) && hasText) {
            ++width;
        }
        const int tokenWidth = FormatTokenWidth(token);
        width += tokenWidth;
        hasText = hasText || tokenWidth > 0;
        previous = &token;
        previousStringLike = token.stringLike;
        return true;
    }

    static bool IsAccessLabel(const PrintToken& token, const PrintToken* next) {
        return IsAccessKeyword(token) &&
            next != nullptr &&
            next->kind == PrintTokenKind::Known &&
            next->syntaxKind == SyntaxNodeKind::Colon;
    }

    void NewLine(bool macroContinuation = false) { output_.NewLine(macroContinuation); }
    void BlankLine() { output_.BlankLine(); }
    void ReopenLastOutputLine() { output_.ReopenLastLine(); }
    void Write(std::string_view text) override { output_.Write(text, indentLevel_); }
    void Space() override { output_.Space(); }
    int CurrentColumn() const { return output_.CurrentColumn(indentLevel_); }
    int CurrentLineIndentLevel() const { return output_.CurrentLineIndentLevel(); }
    void WriteWithIndentOffset(std::string_view text, int offset) {
        output_.WriteAtIndent(text, indentLevel_ + offset);
    }

    void NewLineWithIndent(int indentLevel) {
        NewLine(emittingMacroDefinition_);
        output_.SetPendingIndent(std::max(0, indentLevel));
    }

    void BlankLineWithIndent(int indentLevel) {
        BlankLine();
        output_.SetPendingIndent(std::max(0, indentLevel));
    }

    void BreakListLine(int indentLevel, bool blankLine) {
        if (blankLine) {
            BlankLineWithIndent(indentLevel);
            return;
        }
        NewLineWithIndent(indentLevel);
    }

    static const SyntaxNode* LineCommentAlignmentGroup(const PrintToken& token) {
        const SyntaxNode* group = token.node == nullptr ? nullptr : token.node->parent;
        while (
            group != nullptr &&
            SyntaxNodeHasClass(*group, SyntaxNodeClass::Expression) &&
            !HasDirectListDelimiterPair(*group)
        ) {
            group = group->parent;
        }
        return group;
    }

    void WriteTrailingComment(const PrintToken& token, std::string_view text, bool spaceBefore) {
        output_.WriteComment(
            text,
            indentLevel_,
            LineCommentAlignmentGroup(token),
            FormatOutputComment::Trailing,
            IsLineCommentToken(token),
            spaceBefore
        );
    }

    void WriteStandaloneTrailingComment(const PrintToken& token, std::string_view text) {
        output_.WriteComment(
            text,
            indentLevel_,
            LineCommentAlignmentGroup(token),
            FormatOutputComment::Standalone,
            IsLineCommentToken(token)
        );
    }

    void WriteCommentContinuation(const PrintToken& token, std::string_view text) {
        output_.WriteComment(
            text,
            indentLevel_,
            LineCommentAlignmentGroup(token),
            FormatOutputComment::Continuation,
            IsLineCommentToken(token)
        );
    }

    void CloseCaseBodyIndentIfNeeded() {
        if (!activeCaseBodySwitchDepths_.empty() && activeCaseBodySwitchDepths_.back() == switchDepth_) {
            indentLevel_ = std::max(0, indentLevel_ - 1);
            activeCaseBodySwitchDepths_.pop_back();
        }
    }

    bool CanFlushPendingTokensCompact(const FormatBreakModelContext& context) const {
        if (pendingSourceBlankLine_) {
            return false;
        }
        if (!context.virtualDelimiters.empty() || (
            context.requiredChainBreakOperators != nullptr &&
            std::any_of(pendingTokens_.begin(), pendingTokens_.end(), [&](const PrintToken& token) {
                return token.node != nullptr && context.requiredChainBreakOperators->contains(token.node);
            })
        )) {
            return false;
        }
        int width = 0;
        bool hasText = output_.State().lineHasText;
        bool previousStringLike = false;
        bool hasTemplateHeader = false;
        bool hasTemplateDeclaredEntity = false;
        bool omittedTerminalComma = false;
        for (const PrintToken& token : pendingTokens_) {
            if (token.kind != PrintTokenKind::Known && token.kind != PrintTokenKind::Text) {
                return false;
            }
            if (token.containsSourceLineBreak) {
                return false;
            }
            if (token.inMacroValue || token.macroDefinition != nullptr) {
                return false;
            }
            if (token.inMacroStatementSequence) {
                return false;
            }
            if (token.inLeadingStreamOperatorChain || token.inConditionalStreamOperatorChain) {
                return false;
            }
            if (token.inTemplateDeclaration && !token.inTemplateDeclarationBlock) {
                hasTemplateHeader = hasTemplateHeader || token.inTemplateDeclarationHeader;
                hasTemplateDeclaredEntity = hasTemplateDeclaredEntity || !token.inTemplateDeclarationHeader;
                if (hasTemplateHeader && hasTemplateDeclaredEntity) {
                    return false;
                }
            }
            if (token.stringLike && previousStringLike) {
                return false;
            }
            const SyntaxNode* terminalCommaOpen = BraceListTerminalCommaOpen(token);
            const bool ownsBraceList = terminalCommaOpen != nullptr &&
                std::any_of(pendingTokens_.begin(), pendingTokens_.end(), [&](const PrintToken& candidate) {
                    return candidate.node == terminalCommaOpen;
                });
            if (ownsBraceList) {
                omittedTerminalComma = true;
                continue;
            }
            if (token.spaceBefore && hasText && !omittedTerminalComma) {
                ++width;
            }
            const int tokenWidth = FormatTokenWidth(token);
            width += tokenWidth;
            hasText = hasText || tokenWidth > 0;
            previousStringLike = token.stringLike;
            omittedTerminalComma = false;
        }
        if (hasTemplateHeader && (
            pendingTokens_.empty() ||
            pendingTokens_.back().syntaxKind != SyntaxNodeKind::Greater ||
            pendingTokens_.back().parentKind != SyntaxNodeKind::TemplateParameterList
        )) {
            return false;
        }
        return CurrentColumn() + width <= config_.columnLimit;
    }

    void FlushPendingTokensCompact() {
        bool omittedTerminalComma = false;
        for (const PrintToken& token : pendingTokens_) {
            const SyntaxNode* terminalCommaOpen = BraceListTerminalCommaOpen(token);
            const bool ownsBraceList = terminalCommaOpen != nullptr &&
                std::any_of(pendingTokens_.begin(), pendingTokens_.end(), [&](const PrintToken& candidate) {
                    return candidate.node == terminalCommaOpen;
                });
            if (ownsBraceList) {
                omittedTerminalComma = true;
                continue;
            }
            if (token.spaceBefore && !output_.State().atLineStart && !omittedTerminalComma) {
                Space();
            }
            Write(FormatTokenText(token));
            omittedTerminalComma = false;
        }
        pendingTokens_.clear();
    }

    void BufferToken(const PrintToken& token, const PrintToken* previous = nullptr) {
        if (!pendingTokens_.empty()) {
            previous = &pendingTokens_.back();
        }
        const bool sourceAdjacent = previous != nullptr && previous->sourceIndex + 1 == token.sourceIndex;
        const bool spaceBefore =
            sourceAdjacent && token.spaceBeforeKnown ? token.spaceBefore : FormatTokenNeedsSpace(previous, token);
        PrintToken buffered = token;
        buffered.spaceBefore = spaceBefore;
        buffered.spaceBeforeKnown = true;
        pendingTokens_.push_back(buffered);
    }

    void WriteToken(
        const FormatBreakToken& token,
        std::string_view text,
        std::optional<int> continuationBaseIndent,
        bool suppressSpace
    ) override {
        if (token.contextOnly) {
            return;
        }
        const PrintToken& printToken = FormatBreakTokenValue(token);
        if (
            printToken.macroDefinition != nullptr &&
            !printToken.inMacroValue &&
            output_.State().atLineStart &&
            !output_.State().macroContinuation
        ) {
            output_.ForceColumnZero();
        }
        if (printToken.kind == PrintTokenKind::Comment) {
            const int commentIndent =
                output_.State().atLineStart ? CurrentColumn() / indentWidth_ : CurrentLineIndentLevel();
            if (!output_.State().atLineStart) {
                NewLineWithIndent(commentIndent);
            }
            if (printToken.commentContinuation) {
                WriteCommentContinuation(printToken, text);
            } else {
                Write(text);
            }
            NewLineWithIndent(commentIndent);
            return;
        }
        if (printToken.kind == PrintTokenKind::TrailingComment) {
            const int breakModelContinuationIndent = continuationBaseIndent ?
                *continuationBaseIndent + (*continuationBaseIndent == indentLevel_ ? 1 : 0) : 0;
            const int commentIndent =
                output_.State().atLineStart ? CurrentColumn() / indentWidth_ : CurrentLineIndentLevel();
            const int continuationIndent = std::max(commentIndent, breakModelContinuationIndent);
            if (!output_.State().atLineStart) {
                WriteTrailingComment(printToken, text, token.spaceBefore);
            } else {
                WriteStandaloneTrailingComment(printToken, text);
            }
            const PrintToken* nextToken =
                activeTokens_ != nullptr && printToken.sourceIndex + 1 < activeTokens_->size() ?
                    &(*activeTokens_)[printToken.sourceIndex + 1] : nullptr;
            if (
                TrailingCommentReturnsToStructuralIndent(printToken) ||
                (printToken.macroDefinition != nullptr && !ShouldContinueMacroLine(printToken, nextToken))
            ) {
                NewLine(false);
            } else {
                NewLineWithIndent(continuationIndent);
            }
            return;
        }
        if (token.spaceBefore && !suppressSpace && !output_.State().atLineStart) {
            Space();
        }
        Write(text);
    }

    FormatBreakOutputState State() const override {
        return {
            .atLineStart = output_.State().atLineStart,
            .lineHasText = output_.State().lineHasText,
            .pendingIndentLevel = output_.State().pendingIndentLevel,
        };
    }

    void BreakLine(int indentLevel, bool blankLine) override { BreakListLine(indentLevel, blankLine); }
    void SetPendingIndent(int indentLevel) override { output_.SetPendingIndent(indentLevel); }

    std::vector<FormatBreakSplitList> FlushPendingTokens(const FormatBreakModelContext& context = {}) {
        emittedBlockOpenIndent_.reset();
        if (pendingTokens_.empty()) {
            return {};
        }
        FormatBreakModelContext effectiveContext = context;
        chainContinuation_->Constrain(effectiveContext);
        if (breakModelDump_ == nullptr && CanFlushPendingTokensCompact(effectiveContext)) {
            FlushPendingTokensCompact();
            if (pendingIndentRestoreAfterFlush_) {
                indentLevel_ = *pendingIndentRestoreAfterFlush_;
                pendingIndentRestoreAfterFlush_.reset();
            }
            return {};
        }
        const auto modelStart =
            stats_ == nullptr ? std::chrono::steady_clock::time_point{} : std::chrono::steady_clock::now();
        effectiveContext.forceSplitStreamChain = effectiveContext.forceSplitStreamChain ||
            std::any_of(pendingTokens_.begin(), pendingTokens_.end(), [](const PrintToken& token) {
                return token.inConditionalStreamOperatorChain;
            });
        const bool previousEmittingMacroDefinition = emittingMacroDefinition_;
        emittingMacroDefinition_ =
            std::any_of(pendingTokens_.begin(), pendingTokens_.end(), [](const PrintToken& token) {
                return token.macroDefinition != nullptr;
            });
        if (emittingMacroDefinition_ && pendingTokens_.front().inMacroValue && output_.State().atLineStart) {
            output_.SetPendingIndent(std::max(output_.State().pendingIndentLevel.value_or(0), indentLevel_ + 1));
        }
        const int baseIndentLevel = output_.State().pendingIndentLevel.value_or(indentLevel_);
        const int startColumn = CurrentColumn();
        const int breakLineSuffixWidth = emittingMacroDefinition_ ? 2 : 0;
        const std::optional<FormatDeclarationLayoutView> cached =
            breakModelDump_ != nullptr ? std::nullopt : declarationLayout_->FindReusableLayout(
                pendingTokens_, effectiveContext, startColumn, baseIndentLevel, breakLineSuffixWidth
            );
        FormatBreakModel model;
        if (!cached) {
            model = BuildFormatBreakModel(pendingTokens_, effectiveContext);
        }
        if (stats_ != nullptr) {
            stats_->breakModel += std::chrono::steady_clock::now() - modelStart;
        }
        const FormatBreakModel& effectiveModel = !cached ? model : *cached->model;
        FormatBreakSolution solution;
        const FormatBreakSolution* effectiveSolution = !cached ? &solution : cached->solution;
        if (!cached && (breakModelDump_ != nullptr || BreakModelHasLayoutChoice(effectiveModel))) {
            const auto solveStart =
                stats_ == nullptr ? std::chrono::steady_clock::time_point{} : std::chrono::steady_clock::now();
            solution = SolveFormatBreaks(
                config_, effectiveModel, startColumn, baseIndentLevel, indentWidth_, breakLineSuffixWidth
            );
            if (stats_ != nullptr) {
                stats_->solve += std::chrono::steady_clock::now() - solveStart;
            }
        }
        if (breakModelDump_ != nullptr) {
            breakModelDump_->WriteSegment(
                pendingTokens_, effectiveModel, *effectiveSolution, startColumn, baseIndentLevel, breakLineSuffixWidth
            );
        }
        std::vector<FormatBreakSplitList> splitContexts;
        if (effectiveModel.root) {
            const auto emitStart =
                stats_ == nullptr ? std::chrono::steady_clock::time_point{} : std::chrono::steady_clock::now();
            const FormatBreakEmissionSummary emission = EmitFormatBreakModel(
                config_, effectiveModel, *effectiveSolution, baseIndentLevel, pendingTokens_.back().node, *this
            );
            emittedBlockOpenIndent_ = emission.blockOpenIndent;
            splitContexts = emission.splitLists;
            chainContinuation_->AcceptEmission(emission.chainIndents);
            if (stats_ != nullptr) {
                stats_->emit += std::chrono::steady_clock::now() - emitStart;
            }
        }
        emittingMacroDefinition_ = previousEmittingMacroDefinition;
        pendingTokens_.clear();
        pendingSourceBlankLine_ = false;
        if (pendingIndentRestoreAfterFlush_) {
            indentLevel_ = *pendingIndentRestoreAfterFlush_;
            pendingIndentRestoreAfterFlush_.reset();
        }
        return splitContexts;
    }

    bool HasBufferedLineText() const { return output_.State().lineHasText || !pendingTokens_.empty(); }

    bool ShouldBreakAfterSemicolon() const {
        if (braceStack_.empty()) {
            return parenDepth_ == 0;
        }
        return parenDepth_ <= braceStack_.back().parenDepth;
    }

    bool ShouldContinueMacroLine(const PrintToken& token, const PrintToken* next) const {
        return token.inMacroValue && next != nullptr && next->macroDefinition == token.macroDefinition;
    }

    const PrintToken* RawTokenAfterCurrent(size_t offset) const {
        if (activeTokens_ == nullptr || currentTokenIndex_ + offset >= activeTokens_->size()) {
            return nullptr;
        }
        return &(*activeTokens_)[currentTokenIndex_ + offset];
    }

    bool TryPrintListBoundary(const PrintToken& token, FormatListContinuationKind kind) {
        const auto boundary = listContinuation_->TakeBoundary(token, kind);
        if (!boundary) {
            return false;
        }
        if (boundary->beforeToken) {
            if (HasBufferedLineText()) {
                FlushPendingTokens();
            }
            NewLineWithIndent(*boundary->indent);
            BufferToken(token);
        } else {
            BufferToken(token);
            if (boundary->indent) {
                FlushPendingTokens();
                NewLineWithIndent(*boundary->indent);
            }
        }
        return true;
    }

    bool TryPrintConditionalPreprocessorListOpen(const PrintToken& token) {
        if (
            token.kind != PrintTokenKind::Known ||
            !PrintTokenSyntaxHasClass(token, SyntaxNodeClass::OpeningDelimiter) ||
            ImmediateConditionalPreprocessorListParent(token) == nullptr
        ) {
            return false;
        }
        BufferToken(token);
        FlushPendingTokens();
        NewLineWithIndent(CurrentLineIndentLevel() + 1);
        return true;
    }

    bool TryPrintConditionalPreprocessorListComma(const PrintToken& token) {
        if (
            token.kind != PrintTokenKind::Known ||
            token.syntaxKind != SyntaxNodeKind::Comma ||
            ImmediateConditionalPreprocessorListParent(token) == nullptr
        ) {
            return false;
        }
        BufferToken(token);
        FlushPendingTokens();
        NewLineWithIndent(CurrentLineIndentLevel());
        return true;
    }

    bool TryPrintConditionalPreprocessorListClose(const PrintToken& token) {
        if (
            token.kind != PrintTokenKind::Known ||
            MatchingListCloseToken(token.syntaxKind) != SyntaxNodeKind::Unknown ||
            ImmediateConditionalPreprocessorListParent(token) == nullptr
        ) {
            return false;
        }
        if (HasBufferedLineText()) {
            FlushPendingTokens();
        }
        NewLineWithIndent(indentLevel_);
        BufferToken(token);
        return true;
    }

    void PrepareMacroBoundary(const PrintToken* previous, const PrintToken& current) {
        if (current.macroDefinition != nullptr && !current.inMacroValue && output_.State().atLineStart) {
            output_.ForceColumnZero();
        }
        if (
            previous != nullptr &&
            previous->macroDefinition != nullptr &&
            previous->macroDefinition != current.macroDefinition
        ) {
            if (HasBufferedLineText()) {
                FlushPendingTokens();
                NewLine(false);
            }
            if (current.macroDefinition != nullptr && !current.inMacroValue) {
                output_.ForceColumnZero();
            }
        }
        if (
            current.macroDefinition != nullptr &&
            (previous == nullptr || previous->macroDefinition != current.macroDefinition) &&
            HasBufferedLineText()
        ) {
            FlushPendingTokens();
            NewLine(false);
            if (!current.inMacroValue) {
                output_.ForceColumnZero();
            }
        }
    }

    void PrepareBareMacroItemBoundary(const PrintToken* previous, const PrintToken& current) {
        if (previous == nullptr || current.kind == PrintTokenKind::TrailingComment || !previous->inBareMacroItem) {
            return;
        }
        if (HasBufferedLineText()) {
            FlushPendingTokens();
        }
        if (output_.State().lineHasText) {
            NewLine(ShouldContinueMacroLine(*previous, &current));
        }
    }

    bool CanAttachToPreviousPreprocessorLine(const PrintToken& token, const PrintToken* rawPrevious) const {
        return token.kind == PrintTokenKind::TrailingComment &&
            rawPrevious != nullptr &&
            rawPrevious->kind == PrintTokenKind::Preprocessor &&
            SyntaxNodeKindHasClass(rawPrevious->syntaxKind, SyntaxNodeClass::EndifDirective);
    }

    static const SyntaxNode* DirectChildAtLevel(const SyntaxNode* node, const SyntaxNode* parent) {
        for (const SyntaxNode* cursor = node; cursor != nullptr; cursor = cursor->parent) {
            if (cursor->parent == parent) {
                return cursor;
            }
        }
        return nullptr;
    }

    bool IsRemovableNullTerminator(const PrintToken& token, const PrintToken* previous) const {
        if (
            token.kind != PrintTokenKind::Known ||
            token.syntaxKind != SyntaxNodeKind::Semicolon ||
            token.node == nullptr ||
            token.node->parent == nullptr
        ) {
            return false;
        }
        const SyntaxNode* nullItem = token.node;
        while (
            nullItem->parent != nullptr &&
            nullItem->parent->children.size() == 1 &&
            nullItem->parent->children.front() == nullItem
        ) {
            nullItem = nullItem->parent;
        }
        const SyntaxNode* level = nullItem->parent;
        if (level == nullptr) {
            return false;
        }
        const bool sourceItem = SyntaxNodeHasClass(*level, SyntaxNodeClass::SourceItemScope);
        if (nullItem == token.node && !sourceItem) {
            return false;
        }
        const bool requiredDeclaredTypeTerminator = previous != nullptr &&
            previous->kind == PrintTokenKind::Known &&
            previous->syntaxKind == SyntaxNodeKind::RightBrace &&
            ClosesDeclaredTypeBody(*previous);
        if (sourceItem) {
            if (previous == nullptr) {
                return true;
            }
            if (previous->kind == PrintTokenKind::Preprocessor || previous->kind == PrintTokenKind::IncludeRun) {
                return false;
            }
            if (ClosesStatementPositionMacroCallItem(*previous) || previous->inBareMacroItem) {
                return true;
            }
            if (previous->kind != PrintTokenKind::Known) {
                return false;
            }
            if (
                previous->syntaxKind == SyntaxNodeKind::Semicolon ||
                PrintTokenSyntaxHasClass(*previous, SyntaxNodeClass::OpeningDelimiter) ||
                previous->syntaxKind == SyntaxNodeKind::Colon
            ) {
                return true;
            }
            return previous->syntaxKind == SyntaxNodeKind::RightBrace && !requiredDeclaredTypeTerminator;
        }
        if (previous == nullptr || previous->node == nullptr) {
            return false;
        }
        const bool followsCompleteItem = (
            previous->kind == PrintTokenKind::Known &&
            (previous->syntaxKind == SyntaxNodeKind::Semicolon || previous->syntaxKind == SyntaxNodeKind::RightBrace)
        ) ||
            ClosesStatementPositionMacroCallItem(*previous) ||
            previous->inBareMacroItem;
        if (!followsCompleteItem) {
            return false;
        }
        const SyntaxNode* previousItem = DirectChildAtLevel(previous->node, level);
        if (
            previousItem == nullptr ||
            previousItem == nullItem ||
            SyntaxNodeHasClass(*previousItem, SyntaxNodeClass::PreprocessorDirective)
        ) {
            return false;
        }
        return !requiredDeclaredTypeTerminator;
    }

    static bool BecomesEmptyAfterNullItemRemoval(const PrintToken& token) {
        if (token.node == nullptr || token.node->parent == nullptr) {
            return false;
        }
        bool hasNullItem = false;
        for (const SyntaxNode* child : token.node->parent->children) {
            if (child == nullptr) {
                continue;
            }
            if (
                child->kind == SyntaxNodeKind::LeftBrace ||
                child->kind == SyntaxNodeKind::RightBrace ||
                child->kind == SyntaxNodeKind::BlankLine
            ) {
                continue;
            }
            const SyntaxNode* item = child;
            while (item->children.size() == 1 && item->children.front() != nullptr) {
                item = item->children.front();
            }
            if (item->kind == SyntaxNodeKind::Semicolon) {
                hasNullItem = true;
                continue;
            }
            return false;
        }
        return hasNullItem;
    }

    bool
        ShouldPreserveSourceBlankLine(const PrintToken& token, const PrintToken* previous, const PrintToken* next) const
    {
        if (
            HasBufferedLineText() ||
            token.node == nullptr ||
            token.node->parent == nullptr ||
            previous == nullptr ||
            previous->node == nullptr ||
            next == nullptr ||
            next->node == nullptr
        ) {
            return false;
        }
        const SyntaxNode* level = token.node->parent;
        const SyntaxNode* previousItem = DirectChildAtLevel(previous->node, level);
        const SyntaxNode* nextItem = DirectChildAtLevel(next->node, level);
        if (previousItem == nullptr || nextItem == nullptr || previousItem == nextItem) {
            return false;
        }
        if (previous->kind == PrintTokenKind::Known && (
            PrintTokenSyntaxHasClass(*previous, SyntaxNodeClass::OpeningDelimiter) ||
            previous->syntaxKind == SyntaxNodeKind::Colon
        )) {
            return false;
        }
        if (next->kind != PrintTokenKind::Known) {
            return true;
        }
        if (next->syntaxKind == SyntaxNodeKind::RightBrace) {
            return SyntaxNodeHasClass(*level, SyntaxNodeClass::CompoundBlock);
        }
        return next->syntaxKind != SyntaxNodeKind::RightParen &&
            next->syntaxKind != SyntaxNodeKind::RightBracket &&
            next->syntaxKind != SyntaxNodeKind::Greater;
    }

    void PrintOne(
        const PrintToken& token,
        const PrintToken* previous,
        const PrintToken* rawPrevious,
        const PrintToken* next,
        const PrintToken* rawNext
    ) {
        if (!token.commentContinuation && token.kind != PrintTokenKind::TrailingComment) {
            output_.ResetCommentContinuation();
        }
        if (prebufferedTokenSourceIndices_.erase(token.sourceIndex) != 0) {
            return;
        }
        listContinuation_->BeforeToken(token);
        if (declarationLayout_->NeedsBlankLineBefore(currentTokenIndex_)) {
            FlushPendingTokens();
            BlankLine();
        }
        PrepareBareMacroItemBoundary(rawPrevious, token);
        PrepareMacroBoundary(rawPrevious, token);
        if (token.kind == PrintTokenKind::BlankLine) {
            pendingSourceBlankLine_ = pendingSourceBlankLine_ || !pendingTokens_.empty();
            const PrintToken* sourcePrevious =
                rawPrevious != nullptr && rawPrevious->kind != PrintTokenKind::BlankLine ? rawPrevious : previous;
            const PrintToken* sourceNext =
                rawNext != nullptr && rawNext->kind != PrintTokenKind::BlankLine ? rawNext : next;
            if (ShouldPreserveSourceBlankLine(token, sourcePrevious, sourceNext)) {
                const bool continuesSplitList = listContinuation_->ContinuesList(token);
                const std::optional<int> pendingIndent =
                    continuesSplitList ? output_.State().pendingIndentLevel : std::nullopt;
                BlankLine();
                output_.SetPendingIndent(pendingIndent);
            }
            return;
        }
        if (IsCommentToken(token.kind)) {
            if (KeepsStructuralCommentInBreakModel(token)) {
                BufferToken(token, rawPrevious);
                return;
            }
            if (
                token.kind == PrintTokenKind::TrailingComment &&
                !CanAttachToPreviousPreprocessorLine(token, rawPrevious)
            ) {
                BufferToken(token, rawPrevious);
                FlushPendingTokens();
                return;
            }
            FlushPendingTokens();
            if (
                token.kind == PrintTokenKind::Comment &&
                output_.State().atLineStart &&
                next != nullptr &&
                IsCaseLabelKeyword(*next)
            ) {
                CloseCaseBodyIndentIfNeeded();
            }
            if (CanAttachToPreviousPreprocessorLine(token, rawPrevious)) {
                ReopenLastOutputLine();
            }
            PrintComment(token, rawPrevious, next);
            return;
        }
        if (token.kind == PrintTokenKind::Preprocessor) {
            FlushPendingTokens();
            PrintPreprocessor(token, next);
            return;
        }
        if (token.kind == PrintTokenKind::IncludeRun) {
            FlushPendingTokens();
            PrintIncludeRun(token, next);
            return;
        }
        if (token.kind == PrintTokenKind::Known) {
            PrintKnown(token, previous, next, rawNext);
            return;
        }
        if (token.syntaxKind == SyntaxNodeKind::RawMacroReplacement) {
            FlushPendingTokens();
            Write(FormatRawMacroReplacement(token.text, CurrentLineIndentLevel() + 1, indentWidth_, tabWidth_));
            NewLine();
            return;
        }
        BufferToken(token, rawPrevious);
    }

    void PrintComment(const PrintToken& token, const PrintToken* previous, const PrintToken* next) {
        if (token.kind == PrintTokenKind::TrailingComment && output_.State().lineHasText) {
            WriteTrailingComment(token, token.text, FormatTokenNeedsSpace(previous, token));
            NewLine(ShouldContinueMacroLine(token, next));
            if (
                previous != nullptr &&
                previous->kind == PrintTokenKind::Known &&
                previous->syntaxKind == SyntaxNodeKind::LeftBrace &&
                RoleForBrace(*previous) == BraceRole::NamespaceLike
            ) {
                BlankLine();
            }
            return;
        }
        if (output_.State().lineHasText) {
            NewLine(ShouldContinueMacroLine(token, next));
        }
        if (token.commentContinuation) {
            WriteCommentContinuation(token, token.text);
        } else if (token.kind == PrintTokenKind::TrailingComment) {
            WriteStandaloneTrailingComment(token, token.text);
        } else {
            Write(token.text);
        }
        NewLine(ShouldContinueMacroLine(token, next));
    }

    void PrintIncludeRun(const PrintToken& token, const PrintToken* next) {
        if (token.node == nullptr) {
            return;
        }
        if (output_.State().lineHasText) {
            NewLine();
        }
        const std::string text = FormatIncludeRunText(config_, *token.node, sourcePath_, firstIncludeRun_);
        firstIncludeRun_ = false;
        output_.AppendCompleteLines(text);
        if (
            !text.empty() &&
            next != nullptr &&
            !SyntaxNodeKindHasClass(next->syntaxKind, SyntaxNodeClass::ConditionalBranchSeparatorDirective)
        ) {
            BlankLine();
        }
    }

    void PrintPreprocessor(const PrintToken& token, const PrintToken* next) {
        const std::string line = FormatPreprocessorText(token.text);
        const SyntaxNodeKind lineDirectiveKind = SyntaxNodeKindFromPreprocessorDirectiveLine(line);
        const bool isInclude = PrintTokenSyntaxHasClass(token, SyntaxNodeClass::IncludeDirective) ||
            SyntaxNodeKindHasClass(lineDirectiveKind, SyntaxNodeClass::IncludeDirective);
        const std::optional<int> includeInitializerContinuationIndent =
            isInclude && token.parentKind == SyntaxNodeKind::InitDeclarator && output_.State().lineHasText ?
                std::optional<int>(CurrentLineIndentLevel() + 1) : std::nullopt;
        const std::optional<bool> conditionalComma = listContinuation_->ConditionalDirectiveComma(currentTokenIndex_);
        const bool listConditional = conditionalComma.has_value();
        const bool trailingListComma = conditionalComma.value_or(false);
        const bool closesConditionalFunctionHeader = (
            (token.node != nullptr && SyntaxNodeKindHasClass(token.node->kind, SyntaxNodeClass::EndifDirective)) ||
            token.syntaxKind == SyntaxNodeKind::PreprocessorDirectiveEndif ||
            lineDirectiveKind == SyntaxNodeKind::PreprocessorDirectiveEndif
        ) && token.inConditionalFunctionHeader;
        if (IsConditionalRhsPreprocessorToken(token)) {
            if (HasBufferedLineText()) {
                FlushPendingTokens();
            }
            const int continuationIndent = (output_.State().lineHasText ? CurrentLineIndentLevel() : indentLevel_) + 1;
            if (output_.State().lineHasText) {
                NewLine();
            }
            const std::string outputLine =
                FormatPreprocessorText(token.text, {.payloadIndent = continuationIndent, .indentWidth = indentWidth_});
            output_.WriteVerbatim(outputLine);
            NewLine();
            return;
        }
        if (IsDeclarationModifierPreprocessorToken(token)) {
            if (HasBufferedLineText()) {
                FlushPendingTokens();
            }
            if (output_.State().lineHasText) {
                NewLine();
            }
            const int declarationIndent = output_.State().pendingIndentLevel.value_or(indentLevel_);
            const std::string outputLine =
                FormatPreprocessorText(token.text, {.payloadIndent = declarationIndent, .indentWidth = indentWidth_});
            output_.WriteVerbatim(outputLine);
            NewLine();
            output_.SetPendingIndent(declarationIndent);
            return;
        }
        if (token.structuredPreprocessor || isInclude || listConditional) {
            std::optional<int> listItemIndent = listContinuation_->PreprocessorIndent(token);
            const FormatBreakModelContext* splitListPlan =
                listItemIndent ? nullptr : listContinuation_->PlanPreprocessor(
                    currentTokenIndex_, pendingTokens_, output_.State().pendingIndentLevel.value_or(indentLevel_ + 1)
                );
            if (splitListPlan != nullptr) {
                FlushPendingTokens(*splitListPlan);
                listItemIndent = listContinuation_->AcceptPreprocessor();
            } else if ((listItemIndent || token.structuredPreprocessor) && HasBufferedLineText()) {
                FlushPendingTokens();
            }
            if (!listItemIndent && listConditional) {
                listItemIndent = output_.State().pendingIndentLevel.value_or(indentLevel_ + 1);
            }
            if (output_.State().lineHasText) {
                NewLine();
            }
            const std::string outputLine = listConditional && !token.structuredPreprocessor && listItemIndent ?
                FormatPreprocessorText(token.text, {
                    .payloadIndent = *listItemIndent,
                    .indentWidth = indentWidth_,
                    .terminalComma = !listContinuation_->IsFinalPreprocessorItem(currentTokenIndex_) ?
                        FormatPreprocessorComma::Preserve :
                        (trailingListComma ? FormatPreprocessorComma::Add : FormatPreprocessorComma::Remove),
                }) : line;
            output_.WriteVerbatim(outputLine);
            NewLine();
            if (closesConditionalFunctionHeader) {
                conditionalFunctionIndents_.push_back(indentLevel_);
                ++indentLevel_;
            }
            if (listItemIndent) {
                output_.SetPendingIndent(*listItemIndent);
            } else if (includeInitializerContinuationIndent) {
                output_.SetPendingIndent(*includeInitializerContinuationIndent);
            }
            return;
        }
        if (output_.State().lineHasText) {
            NewLine();
        }
        output_.WriteVerbatim(line);
        NewLine();
        if (closesConditionalFunctionHeader) {
            conditionalFunctionIndents_.push_back(indentLevel_);
            ++indentLevel_;
            return;
        }
    }

    void PrintKnown(
        const PrintToken& token, const PrintToken* previous, const PrintToken* next, const PrintToken* rawNext
    ) {
        switch (token.syntaxKind) {
            case SyntaxNodeKind::LeftParen:
                if (TryPrintConditionalPreprocessorListOpen(token)) {
                    ++parenDepth_;
                    return;
                }
                BufferToken(token);
                ++parenDepth_;
                return;
            case SyntaxNodeKind::RightParen:
                if (TryPrintListBoundary(token, FormatListContinuationKind::Preprocessor)) {
                    if (parenDepth_ > 0) {
                        --parenDepth_;
                    }
                    return;
                }
                if (TryPrintConditionalPreprocessorListClose(token)) {
                    if (parenDepth_ > 0) {
                        --parenDepth_;
                    }
                    return;
                }
                if (TryPrintListBoundary(token, FormatListContinuationKind::Block)) {
                    if (parenDepth_ > 0) {
                        --parenDepth_;
                    }
                    return;
                }
                BufferToken(token);
                if (parenDepth_ > 0) {
                    --parenDepth_;
                }
                if (ClosesStatementPositionMacroCallItem(token) && !(rawNext != nullptr && (
                    rawNext->kind == PrintTokenKind::TrailingComment || rawNext->syntaxKind == SyntaxNodeKind::Semicolon
                ))) {
                    FlushPendingTokens();
                    NewLine(ShouldContinueMacroLine(token, next));
                    return;
                }
                if (
                    token.parentKind == SyntaxNodeKind::RequiresClause &&
                    token.grandParentKind == SyntaxNodeKind::TemplateDeclaration
                ) {
                    FlushPendingTokens();
                    NewLine(ShouldContinueMacroLine(token, next));
                }
                return;
            case SyntaxNodeKind::LeftBracket:
                if (TryPrintConditionalPreprocessorListOpen(token)) {
                    ++bracketDepth_;
                    return;
                }
                BufferToken(token);
                ++bracketDepth_;
                return;
            case SyntaxNodeKind::RightBracket:
                if (TryPrintListBoundary(token, FormatListContinuationKind::Preprocessor)) {
                    if (bracketDepth_ > 0) {
                        --bracketDepth_;
                    }
                    return;
                }
                if (TryPrintConditionalPreprocessorListClose(token)) {
                    if (bracketDepth_ > 0) {
                        --bracketDepth_;
                    }
                    return;
                }
                if (TryPrintListBoundary(token, FormatListContinuationKind::Block)) {
                    if (bracketDepth_ > 0) {
                        --bracketDepth_;
                    }
                    return;
                }
                BufferToken(token);
                if (bracketDepth_ > 0) {
                    --bracketDepth_;
                }
                return;
            case SyntaxNodeKind::Less:
                if (TryPrintConditionalPreprocessorListOpen(token)) {
                    return;
                }
                BufferToken(token);
                return;
            case SyntaxNodeKind::Greater:
                if (TryPrintListBoundary(token, FormatListContinuationKind::Preprocessor)) {
                    return;
                }
                if (TryPrintConditionalPreprocessorListClose(token)) {
                    return;
                }
                BufferToken(token);
                if (
                    token.parentKind == SyntaxNodeKind::TemplateParameterList &&
                    token.grandParentKind == SyntaxNodeKind::TemplateDeclaration &&
                    !(rawNext != nullptr && rawNext->kind == PrintTokenKind::TrailingComment) &&
                    !(
                        next != nullptr &&
                        next->kind == PrintTokenKind::Known &&
                        next->syntaxKind == SyntaxNodeKind::KeywordRequires
                    )
                ) {
                    FlushPendingTokens();
                    NewLine(ShouldContinueMacroLine(token, next));
                }
                return;
            case SyntaxNodeKind::LeftBrace:
                if (TryPrintConditionalPreprocessorListOpen(token)) {
                    return;
                }
                PrintLeftBrace(token, previous, rawNext);
                return;
            case SyntaxNodeKind::RightBrace:
                if (TryPrintListBoundary(token, FormatListContinuationKind::Preprocessor)) {
                    return;
                }
                if (TryPrintConditionalPreprocessorListClose(token)) {
                    return;
                }
                if (TryPrintListBoundary(token, FormatListContinuationKind::Block)) {
                    return;
                }
                PrintRightBrace(token, next, rawNext);
                return;
            case SyntaxNodeKind::Semicolon:
                if (IsRemovableNullTerminator(token, previous)) {
                    if (rawNext != nullptr && rawNext->kind == PrintTokenKind::TrailingComment) {
                        if (!HasBufferedLineText() && output_.State().atLineStart) {
                            output_.ReopenLastLine(true);
                        }
                    } else if (!token.inCompactSingleStatementBody && HasBufferedLineText()) {
                        FlushPendingTokens();
                        NewLine(ShouldContinueMacroLine(token, next));
                    }
                    return;
                }
                BufferToken(token);
                if (
                    !token.inCompactSingleStatementBody &&
                    ShouldBreakAfterSemicolon() &&
                    !(rawNext != nullptr && rawNext->kind == PrintTokenKind::TrailingComment)
                ) {
                    FlushPendingTokens();
                    NewLine(ShouldContinueMacroLine(token, next));
                }
                return;
            case SyntaxNodeKind::Comma:
                if (TryPrintConditionalPreprocessorListComma(token)) {
                    return;
                }
                if (TryPrintListBoundary(token, FormatListContinuationKind::Preprocessor)) {
                    return;
                }
                if (TryPrintListBoundary(token, FormatListContinuationKind::Block)) {
                    return;
                }
                BufferToken(token);
                if (
                    token.parentKind == SyntaxNodeKind::EnumeratorList &&
                    !(rawNext != nullptr && rawNext->kind == PrintTokenKind::TrailingComment)
                ) {
                    FlushPendingTokens();
                    NewLine(ShouldContinueMacroLine(token, next));
                }
                return;
            case SyntaxNodeKind::Colon:
                BufferToken(token);
                if (token.parentKind == SyntaxNodeKind::CaseStatement) {
                    FlushPendingTokens();
                    ++indentLevel_;
                    activeCaseBodySwitchDepths_.push_back(switchDepth_);
                    if (
                        next != nullptr &&
                        next->kind == PrintTokenKind::Known &&
                        next->syntaxKind == SyntaxNodeKind::LeftBrace
                    ) {
                        return;
                    }
                    if (rawNext != nullptr && rawNext->kind == PrintTokenKind::TrailingComment) {
                        return;
                    }
                    NewLine(ShouldContinueMacroLine(token, next));
                    return;
                }
                if (previous != nullptr && previous->kind == PrintTokenKind::Known && (
                    previous->syntaxKind == SyntaxNodeKind::KeywordDefault ||
                    PrintTokenSyntaxHasClass(*previous, SyntaxNodeClass::AccessKeyword)
                )) {
                    FlushPendingTokens();
                    if (rawNext == nullptr || rawNext->kind != PrintTokenKind::TrailingComment) {
                        NewLine(ShouldContinueMacroLine(token, next));
                    }
                }
                return;
            default:
                if (IsCaseLabelKeyword(token) && output_.State().atLineStart) {
                    CloseCaseBodyIndentIfNeeded();
                }
                if (IsAccessLabel(token, next)) {
                    FlushPendingTokens();
                    WriteWithIndentOffset(FormatTokenText(token), -1);
                    return;
                }
                BufferToken(token);
                return;
        }
    }

    void PrintLeftBrace(const PrintToken& token, const PrintToken* previous, const PrintToken* rawNext) {
        if (IsMandatoryBlockOpen(currentTokenIndex_)) {
            chainContinuation_->AnalyzeBlock(currentTokenIndex_);
        }
        const int crossBlockFallbackBaseIndent = std::max(0, output_.State().pendingIndentLevel.value_or(indentLevel_));
        const bool followedByTrailingComment = rawNext != nullptr && rawNext->kind == PrintTokenKind::TrailingComment;
        if (token.inConditionalFunctionHeader) {
            BufferToken(token);
            if (followedByTrailingComment) {
                BufferToken(*rawNext);
                prebufferedTokenSourceIndices_.insert(rawNext->sourceIndex);
            }
            FlushPendingTokens();
            chainContinuation_->FinishBlock(crossBlockFallbackBaseIndent);
            if (!followedByTrailingComment) {
                NewLine(ShouldContinueMacroLine(token, rawNext));
            }
            return;
        }
        const bool isEmptyBracePair = (
            rawNext != nullptr &&
            rawNext->kind == PrintTokenKind::Known &&
            rawNext->syntaxKind == SyntaxNodeKind::RightBrace
        ) || BecomesEmptyAfterNullItemRemoval(token);
        const bool isCaseBlock = previous != nullptr &&
            previous->kind == PrintTokenKind::Known &&
            previous->syntaxKind == SyntaxNodeKind::Colon &&
            previous->parentKind == SyntaxNodeKind::CaseStatement;
        const BraceRole role = isCaseBlock ? BraceRole::CaseBlock : RoleForBrace(token);
        if (role == BraceRole::CaseBlock && output_.State().lineHasText) {
            Space();
        }
        if (isEmptyBracePair) {
            PrintToken compact = token;
            compact.kind = PrintTokenKind::Text;
            compact.syntaxKind = SyntaxNodeKind::Unknown;
            compact.text = "{}";
            BufferToken(compact);
            compactRightBraceRoles_.push_back(role);
            return;
        }
        const FormatBreakModelContext* splitListPlan =
            RoleForBrace(token) == BraceRole::Block ? listContinuation_->PlanBlock(currentTokenIndex_) : nullptr;
        BufferToken(token);
        if (role == BraceRole::Compact || IsCompactSingleStatementFunctionBodyBrace(token)) {
            return;
        }
        if (followedByTrailingComment) {
            BufferToken(*rawNext);
            prebufferedTokenSourceIndices_.insert(rawNext->sourceIndex);
        }
        const std::vector<FormatBreakSplitList> splitContexts =
            FlushPendingTokens(splitListPlan != nullptr ? *splitListPlan : FormatBreakModelContext{});
        const std::optional<int> splitListItemIndent =
            splitListPlan == nullptr ? std::nullopt : listContinuation_->AcceptBlock(splitContexts);
        chainContinuation_->FinishBlock(splitListItemIndent.value_or(crossBlockFallbackBaseIndent));
        const bool functionBlock = token.parentKind == SyntaxNodeKind::CompoundStatement &&
            token.grandParentKind == SyntaxNodeKind::FunctionDefinition;
        int openLineIndent = emittedBlockOpenIndent_.value_or(splitListItemIndent.value_or(
            token.inMacroValue || functionBlock ? indentLevel_ :
                (output_.State().lineHasText ? CurrentLineIndentLevel() : indentLevel_)
        ));
        if (
            token.parentKind == SyntaxNodeKind::RequirementSeq && token.inTemplateDeclaration && token.inRequiresClause
        ) {
            openLineIndent = std::max(openLineIndent, indentLevel_ + 1);
        }
        braceStack_.push_back({
            .role = role,
            .parenDepth = parenDepth_,
            .indentRestore = indentLevel_,
            .closeIndent = role == BraceRole::Block || role == BraceRole::Enum ? openLineIndent : indentLevel_,
        });
        if (
            token.parentKind == SyntaxNodeKind::CompoundStatement &&
            token.grandParentKind == SyntaxNodeKind::SwitchStatement
        ) {
            ++switchDepth_;
        }
        if (role == BraceRole::Block || role == BraceRole::Enum) {
            indentLevel_ = std::max(indentLevel_, openLineIndent) + 1;
            if (!followedByTrailingComment) {
                NewLine(ShouldContinueMacroLine(token, rawNext));
            }
        } else if (role == BraceRole::NamespaceLike || role == BraceRole::CaseBlock) {
            if (!followedByTrailingComment) {
                NewLine(ShouldContinueMacroLine(token, rawNext));
                if (role == BraceRole::NamespaceLike) {
                    BlankLine();
                }
            }
        }
    }

    void PrintRightBrace(const PrintToken& token, const PrintToken* next, const PrintToken* rawNext) {
        if (!compactRightBraceRoles_.empty()) {
            const BraceRole role = compactRightBraceRoles_.back();
            compactRightBraceRoles_.pop_back();
            if (role == BraceRole::Compact) {
                return;
            }
            const bool followedByAttachmentKeyword = next != nullptr && AttachesToFollowingBlockKeyword(token, *next);
            if (
                role != BraceRole::CaseBlock && !followedByAttachmentKeyword && ShouldAttachAfterBlockClose(token, next)
            ) {
                return;
            }
            if (followedByAttachmentKeyword) {
                BufferFollowingAttachedComments();
            }
            FlushPendingTokens();
            if (rawNext == nullptr || rawNext->kind != PrintTokenKind::TrailingComment) {
                NewLine(ShouldContinueMacroLine(token, next));
            }
            return;
        }
        if (
            token.parentKind == SyntaxNodeKind::CompoundStatement &&
            token.grandParentKind == SyntaxNodeKind::FunctionDefinition &&
            token.node != nullptr &&
            token.node->parent != nullptr &&
            !HasDirectKnownChild(*token.node->parent, SyntaxNodeKind::LeftBrace) &&
            !conditionalFunctionIndents_.empty()
        ) {
            FlushPendingTokens();
            if (output_.State().lineHasText) {
                NewLine(token.inMacroValue);
            }
            indentLevel_ = conditionalFunctionIndents_.back();
            conditionalFunctionIndents_.pop_back();
            BufferToken(token);
            FlushPendingTokens();
            if (rawNext != nullptr && rawNext->kind == PrintTokenKind::TrailingComment) {
                return;
            }
            NewLine(ShouldContinueMacroLine(token, next));
            return;
        }
        if (IsCompactSingleStatementFunctionBodyBrace(token)) {
            BufferToken(token);
            FlushPendingTokens();
            if (rawNext == nullptr || rawNext->kind != PrintTokenKind::TrailingComment) {
                NewLine(ShouldContinueMacroLine(token, next));
            }
            return;
        }
        const BraceRole tokenRole = RoleForBrace(token);
        if (tokenRole == BraceRole::Compact) {
            BufferToken(token);
            return;
        }
        const BraceRole role = braceStack_.empty() ? tokenRole : braceStack_.back().role;
        FlushPendingTokens();
        // A non-compact closing brace is positioned by its brace frame, never by a pending
        // declaration or list-item indent left by the preceding construct.
        output_.SetPendingIndent(std::nullopt);
        std::optional<int> restoreIndent;
        std::optional<int> closeIndent;
        if (!braceStack_.empty()) {
            restoreIndent = braceStack_.back().indentRestore;
            closeIndent = braceStack_.back().closeIndent;
            braceStack_.pop_back();
        }
        if (role == BraceRole::NamespaceLike) {
            if (output_.State().lineHasText) {
                NewLine(token.inMacroValue);
            }
            BlankLine();
            BufferToken(token);
            FlushPendingTokens();
            if (rawNext != nullptr && rawNext->kind == PrintTokenKind::TrailingComment) {
                return;
            }
            NewLine(ShouldContinueMacroLine(token, next));
            return;
        }
        if (role == BraceRole::CaseBlock) {
            if (output_.State().lineHasText) {
                NewLine(token.inMacroValue);
            }
            WriteWithIndentOffset("}", -1);
            if (rawNext == nullptr || rawNext->kind != PrintTokenKind::TrailingComment) {
                NewLine(ShouldContinueMacroLine(token, next));
            }
            return;
        }
        if (role != BraceRole::Compact) {
            if (output_.State().lineHasText) {
                NewLine(token.inMacroValue);
            }
            const bool isSwitchBody = token.parentKind == SyntaxNodeKind::CompoundStatement &&
                token.grandParentKind == SyntaxNodeKind::SwitchStatement;
            if (isSwitchBody) {
                CloseCaseBodyIndentIfNeeded();
            }
            indentLevel_ = closeIndent.value_or(restoreIndent.value_or(std::max(0, indentLevel_ - 1)));
            if (restoreIndent && *restoreIndent != indentLevel_) {
                pendingIndentRestoreAfterFlush_ = *restoreIndent;
            }
            BufferToken(token);
            const std::optional<int> splitListContinuationIndent = listContinuation_->CloseBlock(token, next);
            if (isSwitchBody) {
                switchDepth_ = std::max(0, switchDepth_ - 1);
            }
            if (ShouldAttachAfterBlockClose(token, next)) {
                return;
            }
            FlushPendingTokens();
            if (rawNext == nullptr || rawNext->kind != PrintTokenKind::TrailingComment) {
                NewLine(ShouldContinueMacroLine(token, next));
                output_.SetPendingIndent(splitListContinuationIndent);
            }
            return;
        }
    }
};

}  // namespace

namespace {

std::string PrintFormatModel(
    const FormatterConfig& config,
    const FormatModel& model,
    std::string_view sourcePath,
    FormatModelTextStats* stats,
    FormatBreakModelDumpWriter* breakModelDump
) {
    if (!model.root) {
        return {};
    }
    const auto tokenizeStart =
        stats == nullptr ? std::chrono::steady_clock::time_point{} : std::chrono::steady_clock::now();
    std::vector<PrintToken> tokens = BuildPrintTokens(model, config.tabWidth);
    if (stats != nullptr) {
        stats->tokenize += std::chrono::steady_clock::now() - tokenizeStart;
    }
    const auto printStart =
        stats == nullptr ? std::chrono::steady_clock::time_point{} : std::chrono::steady_clock::now();
    const size_t sourceSize = model.sourceText == nullptr ? 0 : model.sourceText->size();
    std::string result = Printer(config, sourcePath, stats, breakModelDump).Print(tokens, sourceSize);
    if (stats != nullptr) {
        stats->print += std::chrono::steady_clock::now() - printStart;
    }
    return result;
}

}  // namespace

std::string FormatModelText(const FormatterConfig& config, const FormatModel& model, std::string_view sourcePath) {
    return PrintFormatModel(config, model, sourcePath, nullptr, nullptr);
}

std::string FormatModelText(
    const FormatterConfig& config, const FormatModel& model, std::string_view sourcePath, FormatModelTextStats& stats
) {
    return PrintFormatModel(config, model, sourcePath, &stats, nullptr);
}

void DumpFormatBreakTrees(
    const FormatterConfig& config, const FormatModel& model, std::string_view sourcePath, FILE* output
) {
    FormatBreakModelDumpWriter dump(output);
    (void)PrintFormatModel(config, model, sourcePath, nullptr, &dump);
}

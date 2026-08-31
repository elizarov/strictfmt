#include "format/impl/format_break_model_builder.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <utility>

#include "format/impl/format_break_model_inline_helpers.h"

namespace {

using ConstSyntaxChildList = std::vector<const SyntaxNode*>;

bool SyntaxNodeHasLocalClass(const SyntaxNode& node, SyntaxNodeClass syntaxNodeClass) {
    return (node.classes & static_cast<std::uint64_t>(syntaxNodeClass)) != 0;
}

bool IsTemplateAngleToken(const FormatBreakToken& token) {
    return IsTemplateAnglePrintToken(FormatBreakTokenValue(token));
}

FormatBreakDelimiterKind OpeningDelimiter(const FormatBreakToken& token) {
    const PrintToken& printToken = FormatBreakTokenValue(token);
    if (printToken.kind != PrintTokenKind::Known) {
        return FormatBreakDelimiterKind::None;
    }
    FormatBreakDelimiterKind delimiter = FormatBreakDelimiterKind::None;
    switch (printToken.syntaxKind) {
        case SyntaxNodeKind::LeftParen:
            delimiter = FormatBreakDelimiterKind::Paren;
            break;
        case SyntaxNodeKind::LeftBracket:
            delimiter = FormatBreakDelimiterKind::Bracket;
            break;
        case SyntaxNodeKind::LeftBrace:
            delimiter = FormatBreakDelimiterKind::Brace;
            break;
        case SyntaxNodeKind::Less:
            delimiter = IsTemplateAngleToken(token) ? FormatBreakDelimiterKind::Angle : FormatBreakDelimiterKind::None;
            break;
        default:
            break;
    }
    if (delimiter == FormatBreakDelimiterKind::None) {
        return FormatBreakDelimiterKind::None;
    }
    if (printToken.inCompilerCallModifier) {
        return FormatBreakDelimiterKind::None;
    }
    return delimiter;
}

FormatBreakDelimiterKind ClosingDelimiter(const FormatBreakToken& token) {
    const PrintToken& printToken = FormatBreakTokenValue(token);
    if (printToken.kind != PrintTokenKind::Known) {
        return FormatBreakDelimiterKind::None;
    }
    switch (printToken.syntaxKind) {
        case SyntaxNodeKind::RightParen:
            return FormatBreakDelimiterKind::Paren;
        case SyntaxNodeKind::RightBracket:
            return FormatBreakDelimiterKind::Bracket;
        case SyntaxNodeKind::RightBrace:
            return FormatBreakDelimiterKind::Brace;
        case SyntaxNodeKind::Greater:
            return IsTemplateAngleToken(token) ? FormatBreakDelimiterKind::Angle : FormatBreakDelimiterKind::None;
        default:
            return FormatBreakDelimiterKind::None;
    }
}

bool IsSelectedSeparator(const FormatBreakToken& token) {
    return FormatBreakTokenKind(token) == PrintTokenKind::Known && (
        FormatBreakTokenSyntaxKind(token) == SyntaxNodeKind::Comma ||
        FormatBreakTokenSyntaxKind(token) == SyntaxNodeKind::Semicolon
    );
}

bool IsBinaryOperatorForNode(const FormatBreakToken& token) {
    const PrintToken& printToken = FormatBreakTokenValue(token);
    return printToken.kind == PrintTokenKind::Known &&
        SyntaxNodeKindHasClass(printToken.syntaxKind, SyntaxNodeClass::BinaryOperator) &&
        printToken.parentKind == SyntaxNodeKind::BinaryExpression;
}

bool IsAssignmentOperatorForNode(const FormatBreakToken& token) {
    const PrintToken& printToken = FormatBreakTokenValue(token);
    return printToken.kind == PrintTokenKind::Known &&
        SyntaxNodeKindHasClass(printToken.syntaxKind, SyntaxNodeClass::AssignmentOperator) && (
            printToken.parentKind == SyntaxNodeKind::AssignmentExpression ||
            printToken.parentKind == SyntaxNodeKind::InitDeclarator ||
            printToken.parentKind == SyntaxNodeKind::FieldDeclaration ||
            printToken.parentKind == SyntaxNodeKind::AliasDeclaration ||
            printToken.parentKind == SyntaxNodeKind::FunctionPointerAliasDeclaration ||
            printToken.parentKind == SyntaxNodeKind::InitializerList
        );
}

bool IsConditionalOperatorForNode(const FormatBreakToken& token) {
    const PrintToken& printToken = FormatBreakTokenValue(token);
    return printToken.kind == PrintTokenKind::Known &&
        printToken.parentKind == SyntaxNodeKind::ConditionalExpression &&
        (printToken.syntaxKind == SyntaxNodeKind::Question || printToken.syntaxKind == SyntaxNodeKind::Colon);
}

bool IsMemberChainOperatorForNode(const FormatBreakToken& token) {
    const PrintToken& printToken = FormatBreakTokenValue(token);
    return printToken.kind == PrintTokenKind::Known &&
        printToken.parentKind == SyntaxNodeKind::FieldExpression &&
        (printToken.syntaxKind == SyntaxNodeKind::Dot || printToken.syntaxKind == SyntaxNodeKind::Arrow);
}

bool IsCommaOperatorForNode(const FormatBreakToken& token) {
    const PrintToken& printToken = FormatBreakTokenValue(token);
    return printToken.kind == PrintTokenKind::Known &&
        printToken.parentKind == SyntaxNodeKind::CommaExpression &&
        SyntaxNodeKindHasClass(printToken.syntaxKind, SyntaxNodeClass::ChainOperator);
}

bool IsForHeaderDelimiter(const FormatBreakToken& open) {
    const PrintToken& printToken = FormatBreakTokenValue(open);
    return printToken.parentKind == SyntaxNodeKind::ForStatement ||
        printToken.grandParentKind == SyntaxNodeKind::ForStatement;
}

bool IsMultiItemDesignatedInitializer(const FormatBreakNode& delimited, const FormatBreakToken& open) {
    const PrintToken& printToken = FormatBreakTokenValue(open);
    const SyntaxNode* initializer = printToken.node == nullptr ? nullptr : printToken.node->parent;
    return delimited.items.size() > 1 &&
        printToken.parentKind == SyntaxNodeKind::InitializerList &&
        initializer != nullptr &&
        std::any_of(initializer->children.begin(), initializer->children.end(), [](const SyntaxNode* child) {
            return child != nullptr && child->kind == SyntaxNodeKind::FieldDesignator;
        });
}

bool IsChainOperatorToken(const FormatBreakToken& token) {
    return FormatBreakTokenKind(token) == PrintTokenKind::Known &&
        SyntaxNodeKindHasClass(FormatBreakTokenSyntaxKind(token), SyntaxNodeClass::ChainOperator);
}

bool EndsWithEscapedLineFragment(std::string_view text) {
    const size_t quote = text.rfind('"');
    if (quote == std::string_view::npos || quote < 2 || text[quote - 1] != 'n') {
        return false;
    }
    size_t backslashCount = 0;
    for (size_t index = quote - 1; index > 0 && text[index - 1] == '\\'; --index) {
        ++backslashCount;
    }
    return backslashCount % 2 == 1;
}

bool ForcesStringBoundarySplit(const FormatBreakToken& token) {
    const std::string_view text = FormatTokenText(FormatBreakTokenValue(token));
    return EndsWithEscapedLineFragment(text);
}

struct OrdinaryStringLiteralParts {
    std::string_view prefix;
    std::string_view body;
    std::string_view suffix;
};

bool IsOrdinaryStringPrefix(std::string_view prefix) {
    return prefix.empty() || prefix == "L" || prefix == "u8" || prefix == "u" || prefix == "U";
}

std::optional<OrdinaryStringLiteralParts> ParseOrdinaryStringLiteral(std::string_view text) {
    if (text.find_first_of("\r\n") != std::string_view::npos) {
        return std::nullopt;
    }
    const size_t open = text.find('"');
    const size_t close = text.rfind('"');
    if (open == std::string_view::npos || close == open || !IsOrdinaryStringPrefix(text.substr(0, open))) {
        return std::nullopt;
    }
    return OrdinaryStringLiteralParts{
        .prefix = text.substr(0, open),
        .body = text.substr(open + 1, close - open - 1),
        .suffix = text.substr(close + 1)
    };
}

std::optional<std::string_view> JoinStringAffix(std::string_view left, std::string_view right) {
    if (left == right || right.empty()) {
        return left;
    }
    if (left.empty()) {
        return right;
    }
    return std::nullopt;
}

bool IsHexDigit(char value) {
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') || (value >= 'A' && value <= 'F');
}

bool IsOctalDigit(char value) { return value >= '0' && value <= '7'; }

bool IsActiveEscapeBackslash(std::string_view body, size_t position) {
    size_t begin = position;
    while (begin > 0 && body[begin - 1] == '\\') {
        --begin;
    }
    return (position - begin + 1) % 2 == 1;
}

bool StringJoinWouldExtendEscape(std::string_view leftBody, std::string_view rightBody) {
    if (leftBody.empty() || rightBody.empty()) {
        return false;
    }
    size_t hexBegin = leftBody.size();
    while (hexBegin > 0 && IsHexDigit(leftBody[hexBegin - 1])) {
        --hexBegin;
    }
    if (
        IsHexDigit(rightBody.front()) &&
        hexBegin >= 2 &&
        leftBody[hexBegin - 1] == 'x' &&
        leftBody[hexBegin - 2] == '\\' &&
        IsActiveEscapeBackslash(leftBody, hexBegin - 2)
    ) {
        return true;
    }

    size_t octalBegin = leftBody.size();
    while (octalBegin > 0 && IsOctalDigit(leftBody[octalBegin - 1])) {
        --octalBegin;
    }
    const size_t octalLength = leftBody.size() - octalBegin;
    return IsOctalDigit(rightBody.front()) &&
        octalLength > 0 &&
        octalLength < 3 &&
        octalBegin > 0 &&
        leftBody[octalBegin - 1] == '\\' &&
        IsActiveEscapeBackslash(leftBody, octalBegin - 1);
}

std::optional<std::string> JoinOrdinaryStringLiterals(std::string_view left, std::string_view right) {
    const std::optional<OrdinaryStringLiteralParts> leftParts = ParseOrdinaryStringLiteral(left);
    const std::optional<OrdinaryStringLiteralParts> rightParts = ParseOrdinaryStringLiteral(right);
    if (!leftParts || !rightParts || StringJoinWouldExtendEscape(leftParts->body, rightParts->body)) {
        return std::nullopt;
    }
    const std::optional<std::string_view> prefix = JoinStringAffix(leftParts->prefix, rightParts->prefix);
    const std::optional<std::string_view> suffix = JoinStringAffix(leftParts->suffix, rightParts->suffix);
    if (!prefix || !suffix) {
        return std::nullopt;
    }
    std::string result;
    result.reserve(prefix->size() + leftParts->body.size() + rightParts->body.size() + suffix->size() + 2);
    result.append(*prefix);
    result.push_back('"');
    result.append(leftParts->body);
    result.append(rightParts->body);
    result.push_back('"');
    result.append(*suffix);
    return result;
}

bool IsLogicalOperatorToken(const FormatBreakToken& token) {
    return IsChainOperatorToken(token) && (
        FormatBreakTokenSyntaxKind(token) == SyntaxNodeKind::AmpersandAmpersand ||
        FormatBreakTokenSyntaxKind(token) == SyntaxNodeKind::PipePipe
    );
}

bool IsLogicalChain(const FormatBreakNode& node) {
    return node.kind == FormatBreakNodeKind::Chain &&
        node.chainKind == FormatBreakChainKind::AfterOperator &&
        !node.operators.empty() &&
        std::all_of(node.operators.begin(), node.operators.end(), IsLogicalOperatorToken);
}

bool IsFlatChainOperator(const FormatBreakToken& token) { return IsChainOperatorToken(token); }

bool IsFlatParenthesizedChain(const FormatBreakNode& node) {
    return node.kind == FormatBreakNodeKind::Chain &&
        node.chainKind == FormatBreakChainKind::AfterOperator &&
        !node.operators.empty() &&
        std::all_of(node.operators.begin(), node.operators.end(), IsFlatChainOperator);
}

bool IsAssignmentChain(const FormatBreakNode& node) {
    return node.kind == FormatBreakNodeKind::Chain &&
        std::any_of(node.operators.begin(), node.operators.end(), IsAssignmentOperatorForNode);
}

bool HasAssignmentContinuation(const FormatBreakNode& node) {
    return IsAssignmentChain(node) ||
        std::any_of(node.operands.begin(), node.operands.end(), [](const FormatBreakNode* operand) {
            return operand != nullptr && IsAssignmentChain(*operand);
        });
}

bool UsesFlatLogicalContinuation(const FormatBreakToken& open, const FormatBreakNode& item) {
    if (!IsLogicalChain(item)) {
        return false;
    }
    const PrintToken& printToken = FormatBreakTokenValue(open);
    if (printToken.parentKind == SyntaxNodeKind::RequiresClause) {
        return true;
    }
    if (
        printToken.parentKind == SyntaxNodeKind::ForStatement ||
        printToken.grandParentKind == SyntaxNodeKind::ForStatement
    ) {
        return false;
    }
    return SyntaxNodeKindHasClass(printToken.parentKind, SyntaxNodeClass::FlatLogicalHeader) ||
        SyntaxNodeKindHasClass(printToken.grandParentKind, SyntaxNodeClass::FlatLogicalHeader);
}

bool UsesFlatNonCallParenthesisContinuation(const FormatBreakToken& open) {
    const PrintToken& printToken = FormatBreakTokenValue(open);
    if (printToken.kind != PrintTokenKind::Known || printToken.syntaxKind != SyntaxNodeKind::LeftParen) {
        return false;
    }
    return printToken.parentKind != SyntaxNodeKind::ArgumentList &&
        printToken.parentKind != SyntaxNodeKind::ParameterList &&
        printToken.parentKind != SyntaxNodeKind::ForStatement &&
        printToken.grandParentKind != SyntaxNodeKind::ForStatement;
}

class BreakModelBuilder {
public:
    BreakModelBuilder(std::span<const PrintToken> tokens, const FormatBreakModelContext& context) : context_(context) {
        model_.nodes = std::make_unique<std::deque<FormatBreakNode>>();
        selectionMark_ = NextSelectionMark();
        const PrintToken* previous = nullptr;
        for (size_t index = 0; index < tokens.size(); ++index) {
            const PrintToken& token = tokens[index];
            bool spaceBefore = FormatTokenNeedsSpace(previous, token);
            if (token.node != nullptr) {
                token.node->formatPrintToken = &token;
                token.node->formatSpaceBefore = spaceBefore;
                token.node->formatTokenMark = selectionMark_;
            }
            for (
                const SyntaxNode* ancestor = token.node;
                ancestor != nullptr && ancestor->formatSelectionMark != selectionMark_;
                ancestor = ancestor->parent
            ) {
                ancestor->formatSelectionMark = selectionMark_;
            }
            previous = &token;
        }
        root_ = CommonRoot(tokens);
    }

    FormatBreakModel Build() {
        if (root_ != nullptr) {
            model_.root = BuildSyntaxNode(*root_, 0);
        }
        if (!model_.root) {
            model_.root = MakeNode(FormatBreakNodeKind::Sequence, 0);
        }
        ApplyRequiredChainBreaks(*model_.root);
        NormalizeBreakCosts(*model_.root);
        return std::move(model_);
    }

private:
    const FormatBreakModelContext& context_;
    FormatBreakModel model_;
    const SyntaxNode* root_ = nullptr;
    std::uint32_t selectionMark_ = 0;
    int nextId_ = 1;

    static size_t AncestorCount(const SyntaxNode* node) { return node == nullptr ? 0 : node->depth + 1; }

    static const SyntaxNode* CommonAncestor(const SyntaxNode* left, const SyntaxNode* right) {
        size_t leftDepth = AncestorCount(left);
        size_t rightDepth = AncestorCount(right);
        while (leftDepth > rightDepth && left != nullptr) {
            left = left->parent;
            --leftDepth;
        }
        while (rightDepth > leftDepth && right != nullptr) {
            right = right->parent;
            --rightDepth;
        }
        while (left != right) {
            if (left == nullptr || right == nullptr) {
                return nullptr;
            }
            left = left->parent;
            right = right->parent;
        }
        return left;
    }

    static const SyntaxNode* CommonRoot(std::span<const PrintToken> tokens) {
        if (tokens.empty() || tokens.front().node == nullptr) {
            return nullptr;
        }
        const SyntaxNode* common = tokens.front().node;
        for (const PrintToken& token : tokens.subspan(1)) {
            common = CommonAncestor(common, token.node);
            if (common == nullptr) {
                return nullptr;
            }
        }
        return common;
    }

    static std::uint32_t NextSelectionMark() {
        thread_local std::uint32_t next = 1;
        const std::uint32_t mark = next++;
        if (next == 0) {
            next = 1;
        }
        return mark;
    }

    bool ContainsSelected(const SyntaxNode& node) const { return node.formatSelectionMark == selectionMark_; }

    bool RequiresChainBreak(const FormatBreakToken& token) const {
        return context_.requiredChainBreakOperators != nullptr &&
            FormatBreakTokenValue(token).node != nullptr &&
            context_.requiredChainBreakOperators->contains(FormatBreakTokenValue(token).node);
    }

    void ApplyRequiredChainBreaks(FormatBreakNode& node) {
        if (
            node.kind == FormatBreakNodeKind::Chain &&
            std::any_of(node.operators.begin(), node.operators.end(), [this](const FormatBreakToken& token) {
                return RequiresChainBreak(token);
            })
        ) {
            if (node.chainKind == FormatBreakChainKind::Ternary) {
                node.ternaryRequiresColonBreaks = true;
            } else {
                node.forceSplit = true;
            }
            if (context_.requiredChainBreakBaseIndents != nullptr) {
                for (const FormatBreakToken& token : node.operators) {
                    const SyntaxNode* operatorNode = FormatBreakTokenValue(token).node;
                    const auto base = context_.requiredChainBreakBaseIndents->find(operatorNode);
                    if (base != context_.requiredChainBreakBaseIndents->end()) {
                        node.requiredChainBreakBaseIndent = base->second;
                        break;
                    }
                }
            }
        }
        for (FormatBreakNode* child : node.children) {
            if (child != nullptr) {
                ApplyRequiredChainBreaks(*child);
            }
        }
        for (FormatBreakListItem& item : node.items) {
            if (item.node != nullptr) {
                ApplyRequiredChainBreaks(*item.node);
            }
        }
        for (FormatBreakNode* operand : node.operands) {
            if (operand != nullptr) {
                ApplyRequiredChainBreaks(*operand);
            }
        }
    }

    bool ContainsUnselectedSourceLeaf(const SyntaxNode& node) const {
        if (node.children.empty()) {
            return !node.text.empty() && !ContainsSelected(node);
        }
        return std::any_of(node.children.begin(), node.children.end(), [this](const SyntaxNode* child) {
            return child != nullptr && ContainsUnselectedSourceLeaf(*child);
        });
    }

    SyntaxNodeKind ParentKind(const SyntaxNode& node) const {
        return node.parent == nullptr ? SyntaxNodeKind::Unknown : node.parent->kind;
    }

    std::optional<FormatBreakToken> TokenForNode(const SyntaxNode& node) const {
        if (node.formatTokenMark != selectionMark_ || node.formatPrintToken == nullptr) {
            return std::nullopt;
        }
        return FormatBreakToken{.token = node.formatPrintToken, .spaceBefore = node.formatSpaceBefore};
    }

    std::span<FormatBreakNode*> StoreNodePointers(std::span<FormatBreakNode* const> nodes) {
        return model_.nodePointers.Append(nodes);
    }

    std::span<FormatBreakNode*> StoreNodePointers(const std::vector<FormatBreakNode*>& nodes) {
        return StoreNodePointers(std::span<FormatBreakNode* const>{nodes.data(), nodes.size()});
    }

    std::span<FormatBreakNode*> StoreNodePointers(std::initializer_list<FormatBreakNode*> nodes) {
        return StoreNodePointers(std::span<FormatBreakNode* const>{nodes.begin(), nodes.size()});
    }

    std::span<FormatBreakToken> StoreTokens(std::span<const FormatBreakToken> tokens) {
        return model_.tokens.Append(tokens);
    }

    std::span<FormatBreakToken> StoreTokens(std::initializer_list<FormatBreakToken> tokens) {
        return StoreTokens(std::span<const FormatBreakToken>{tokens.begin(), tokens.size()});
    }

    FormatBreakNode* MakeNode(FormatBreakNodeKind kind, int depth) {
        model_.nodes->emplace_back();
        FormatBreakNode& node = model_.nodes->back();
        node.id = nextId_++;
        node.kind = kind;
        node.structuralDepth = depth;
        return &node;
    }

    FormatBreakNode* BuildToken(const FormatBreakToken& token, int depth) {
        auto node = MakeNode(FormatBreakNodeKind::Token, depth);
        node->token = token;
        return node;
    }

    static const FormatBreakToken* TokenChild(const FormatBreakNode* node) {
        if (!node || node->kind != FormatBreakNodeKind::Token) {
            return nullptr;
        }
        return &node->token;
    }

    static bool IsStringTokenChild(const FormatBreakNode* node) {
        const FormatBreakToken* token = TokenChild(node);
        return token != nullptr && IsStringLike(FormatBreakTokenValue(*token));
    }

    static bool IsArgumentList(const FormatBreakNode* node) {
        if (node == nullptr) {
            return false;
        }
        if (const FormatBreakToken* atom = TokenChild(node)) {
            return FormatBreakTokenSyntaxKind(*atom) == SyntaxNodeKind::ArgumentList;
        }
        if (node->kind != FormatBreakNodeKind::Delimited || node->children.empty()) {
            return false;
        }
        const FormatBreakToken* open = TokenChild(node->children.front());
        return open != nullptr && FormatBreakTokenValue(*open).parentKind == SyntaxNodeKind::ArgumentList;
    }

    static bool IsStandaloneCommentItem(const FormatBreakNode& node, size_t index) {
        if (index >= node.items.size()) {
            return false;
        }
        const FormatBreakToken* token = TokenChild(node.items[index].node);
        return token != nullptr && FormatBreakTokenKind(*token) == PrintTokenKind::Comment;
    }

    static bool ShouldPreservePendingBlankLine(const FormatBreakNode& list, bool pendingBlankLine, bool beforeComment) {
        return pendingBlankLine && (beforeComment || IsStandaloneCommentItem(list, list.items.size() - 1));
    }

    void AppendListItem(FormatBreakNode& list, FormatBreakNode* item, bool blankLineBefore) {
        list.items.push_back(FormatBreakListItem{.node = item, .blankLineBefore = blankLineBefore});
    }

    void AppendDelimitedItem(
        FormatBreakNode& delimited,
        ConstSyntaxChildList& itemChildren,
        const FormatBreakToken& open,
        int depth,
        bool blankLineBefore = false
    ) {
        if (itemChildren.empty()) {
            return;
        }
        FormatBreakNode* item = BuildDelimitedAssignmentItem(itemChildren, depth + 1);
        if (item == nullptr) {
            item = BuildSequenceFromPointers(itemChildren, depth + 1);
        }
        const bool virtualDelimiter = FormatBreakTokenValue(open).parentKind == SyntaxNodeKind::Unknown;
        if (
            delimited.delimiterKind == FormatBreakDelimiterKind::Paren &&
            item &&
            item->kind == FormatBreakNodeKind::Chain &&
            item->chainKind != FormatBreakChainKind::Ternary &&
            !HasAssignmentContinuation(*item) && (
                virtualDelimiter || (
                    IsFlatParenthesizedChain(*item) &&
                    (UsesFlatLogicalContinuation(open, *item) || UsesFlatNonCallParenthesisContinuation(open))
                )
            )
        ) {
            item->flatSplitIndent = true;
        }
        AppendListItem(delimited, item, blankLineBefore);
        itemChildren.clear();
    }

    void AppendEmptyDelimitedItem(FormatBreakNode& delimited, int depth, bool blankLineBefore = false) {
        AppendListItem(delimited, MakeNode(FormatBreakNodeKind::Sequence, depth + 1), blankLineBefore);
    }

    void AppendStandaloneCommentItem(
        FormatBreakNode& list, const FormatBreakToken& comment, int depth, bool blankLineBefore = false
    ) {
        AppendListItem(list, BuildToken(comment, depth + 1), blankLineBefore);
    }

    void AttachTrailingCommentToPreviousItem(FormatBreakNode& list, const FormatBreakToken& comment) {
        if (list.items.empty()) {
            list.leadingTrailingComment = comment;
            return;
        }
        list.items.back().trailingComment = comment;
    }

    static bool AttachSeparatorToPreviousItem(FormatBreakNode& list, const FormatBreakToken& separator) {
        if (list.items.empty()) {
            return false;
        }
        list.items.back().separator = separator;
        return true;
    }

    static bool EndsWithBodyHeader(const FormatBreakNode& node) {
        if (node.kind == FormatBreakNodeKind::BodyHeader) {
            return true;
        }
        if (node.kind != FormatBreakNodeKind::Sequence || node.children.empty()) {
            return false;
        }
        const FormatBreakNode* tail = node.children.back();
        return tail != nullptr && EndsWithBodyHeader(*tail);
    }

    static bool ContainsSyntaxKind(const SyntaxNode& node, SyntaxNodeKind kind) {
        if (node.kind == kind) {
            return true;
        }
        return std::any_of(node.children.begin(), node.children.end(), [kind](const SyntaxNode* child) {
            return child != nullptr && ContainsSyntaxKind(*child, kind);
        });
    }

    static bool ContainsNonemptyRequirementSequence(const SyntaxNode& node) {
        if (node.kind == SyntaxNodeKind::RequirementSeq) {
            return std::any_of(node.children.begin(), node.children.end(), [](const SyntaxNode* child) {
                return child != nullptr &&
                    child->kind != SyntaxNodeKind::LeftBrace &&
                    child->kind != SyntaxNodeKind::RightBrace;
            });
        }
        return std::any_of(node.children.begin(), node.children.end(), [](const SyntaxNode* child) {
            return child != nullptr && ContainsNonemptyRequirementSequence(*child);
        });
    }

    static bool ContainsFunctionPointerDeclarator(const SyntaxNode& node) {
        return ContainsSyntaxKind(node, SyntaxNodeKind::PointerDeclarator) ||
            ContainsSyntaxKind(node, SyntaxNodeKind::AbstractPointerDeclarator) ||
            ContainsSyntaxKind(node, SyntaxNodeKind::ParenthesizedDeclarator) ||
            ContainsSyntaxKind(node, SyntaxNodeKind::AbstractParenthesizedDeclarator);
    }

    static bool FunctionDeclaratorTargetContainsFunctionPointerDeclarator(const SyntaxNode& node) {
        if (node.kind != SyntaxNodeKind::FunctionDeclarator) {
            return false;
        }
        for (const SyntaxNode* child : node.children) {
            if (child == nullptr) {
                continue;
            }
            if (child->kind == SyntaxNodeKind::ParameterList) {
                return false;
            }
            if (ContainsFunctionPointerDeclarator(*child)) {
                return true;
            }
        }
        return false;
    }

    static bool OwnsStructuralBreak(const FormatBreakNode& node) {
        return node.kind != FormatBreakNodeKind::Token && node.kind != FormatBreakNodeKind::Sequence;
    }

    static std::optional<int> MinimumStructuralBreakDepth(const FormatBreakNode& node) {
        std::optional<int> result = OwnsStructuralBreak(node) ? std::optional(node.structuralDepth) : std::nullopt;
        const auto include = [&](const FormatBreakNode* child) {
            if (child == nullptr) {
                return;
            }
            const std::optional<int> childDepth = MinimumStructuralBreakDepth(*child);
            if (childDepth && (!result || *childDepth < *result)) {
                result = childDepth;
            }
        };
        for (const FormatBreakNode* child : node.children) {
            include(child);
        }
        for (const FormatBreakListItem& item : node.items) {
            include(item.node);
        }
        for (const FormatBreakNode* operand : node.operands) {
            include(operand);
        }
        return result;
    }

    static std::optional<int> FirstParameterListBreakDepth(const FormatBreakNode& node) {
        if (node.kind == FormatBreakNodeKind::Delimited && !node.children.empty()) {
            const FormatBreakNode* open = node.children.front();
            if (
                open != nullptr &&
                open->kind == FormatBreakNodeKind::Token &&
                FormatBreakTokenValue(open->token).parentKind == SyntaxNodeKind::ParameterList
            ) {
                return node.structuralDepth;
            }
        }
        for (const FormatBreakNode* child : node.children) {
            if (child == nullptr) {
                continue;
            }
            if (std::optional<int> depth = FirstParameterListBreakDepth(*child)) {
                return depth;
            }
        }
        for (const FormatBreakListItem& item : node.items) {
            if (item.node == nullptr) {
                continue;
            }
            if (std::optional<int> depth = FirstParameterListBreakDepth(*item.node)) {
                return depth;
            }
        }
        for (const FormatBreakNode* operand : node.operands) {
            if (operand == nullptr) {
                continue;
            }
            if (std::optional<int> depth = FirstParameterListBreakDepth(*operand)) {
                return depth;
            }
        }
        return std::nullopt;
    }

    static void ShiftStructuralDepth(FormatBreakNode& node, int delta, bool includeChainLinks = true) {
        if (includeChainLinks || !IsFormatBreakUniformChain(node)) {
            node.structuralDepth += delta;
        }
        for (FormatBreakNode* child : node.children) {
            if (child != nullptr) {
                ShiftStructuralDepth(*child, delta, includeChainLinks);
            }
        }
        for (FormatBreakListItem& item : node.items) {
            if (item.node != nullptr) {
                ShiftStructuralDepth(*item.node, delta, includeChainLinks);
            }
        }
        for (FormatBreakNode* operand : node.operands) {
            if (operand != nullptr) {
                ShiftStructuralDepth(*operand, delta, includeChainLinks);
            }
        }
    }

    static FormatBreakNode* UnwrapSingleChildSequence(FormatBreakNode* node) {
        while (node != nullptr && node->kind == FormatBreakNodeKind::Sequence && node->children.size() == 1) {
            node = node->children.front();
        }
        return node;
    }

    static void DiscountFinalLambdaBody(FormatBreakNode& list) {
        size_t end = list.items.size();
        while (end != 0 && IsStandaloneCommentItem(list, end - 1)) {
            --end;
        }
        FormatBreakNode* lambda = end == 0 ? nullptr : UnwrapSingleChildSequence(list.items[end - 1].node);
        if (lambda == nullptr || !lambda->bodyHeaderIsLambda || lambda->children.size() != 2) {
            return;
        }
        FormatBreakNode* body = UnwrapSingleChildSequence(lambda->children.back());
        if (body != nullptr && body->kind == FormatBreakNodeKind::Delimited) {
            body->breakCost = 0;
        }
    }

    static void NormalizeBreakCosts(FormatBreakNode& node) {
        node.breakCost = node.structuralDepth;
        for (FormatBreakNode* child : node.children) {
            if (child != nullptr) {
                NormalizeBreakCosts(*child);
            }
        }
        for (FormatBreakListItem& item : node.items) {
            if (item.node != nullptr) {
                NormalizeBreakCosts(*item.node);
            }
        }
        for (FormatBreakNode* operand : node.operands) {
            if (operand != nullptr) {
                NormalizeBreakCosts(*operand);
            }
        }
        if (node.kind == FormatBreakNodeKind::Delimited) {
            DiscountFinalLambdaBody(node);
        }
    }

    static void NormalizeCallablePrefixBreakDepth(FormatBreakNode& prefix, const FormatBreakNode& declarator) {
        const std::optional<int> parameterDepth = FirstParameterListBreakDepth(declarator);
        const std::optional<int> prefixDepth = MinimumStructuralBreakDepth(prefix);
        if (!parameterDepth || !prefixDepth || *prefixDepth > *parameterDepth) {
            return;
        }
        ShiftStructuralDepth(prefix, *parameterDepth + 1 - *prefixDepth);
    }

    static void MarkBodyHeaderSplitAtParentIndentWhenLineStarts(FormatBreakNode& node) {
        if (node.kind == FormatBreakNodeKind::BodyHeader) {
            node.bodyHeaderSplitAtParentIndentWhenLineStarts = true;
        }
        for (FormatBreakNode* child : node.children) {
            if (child) {
                MarkBodyHeaderSplitAtParentIndentWhenLineStarts(*child);
            }
        }
        for (FormatBreakListItem& item : node.items) {
            if (item.node) {
                MarkBodyHeaderSplitAtParentIndentWhenLineStarts(*item.node);
            }
        }
        for (FormatBreakNode* operand : node.operands) {
            if (operand) {
                MarkBodyHeaderSplitAtParentIndentWhenLineStarts(*operand);
            }
        }
    }

    static bool ContainsForceSplitAdjacentStrings(const FormatBreakNode& node) {
        if (node.kind == FormatBreakNodeKind::AdjacentStrings && node.forceSplit) {
            return true;
        }
        for (const FormatBreakNode* child : node.children) {
            if (child && ContainsForceSplitAdjacentStrings(*child)) {
                return true;
            }
        }
        for (const FormatBreakListItem& item : node.items) {
            if (item.node && ContainsForceSplitAdjacentStrings(*item.node)) {
                return true;
            }
        }
        for (const FormatBreakNode* operand : node.operands) {
            if (operand && ContainsForceSplitAdjacentStrings(*operand)) {
                return true;
            }
        }
        return false;
    }

    static void MarkForceSplitAdjacentStringsFlat(FormatBreakNode& node) {
        if (node.kind == FormatBreakNodeKind::AdjacentStrings && node.forceSplit) {
            node.flatSplitIndent = true;
        }
        if (node.kind == FormatBreakNodeKind::Delimited && !node.children.empty()) {
            const FormatBreakNode* open = node.children.front();
            if (open != nullptr && open->kind == FormatBreakNodeKind::Token) {
                const PrintToken& token = FormatBreakTokenValue(open->token);
                if (
                    SyntaxNodeKindHasClass(token.parentKind, SyntaxNodeClass::SemanticDelimitedParent) ||
                    SyntaxNodeKindHasClass(token.grandParentKind, SyntaxNodeClass::SemanticDelimitedParent)
                ) {
                    return;
                }
            }
        }
        for (FormatBreakNode* child : node.children) {
            if (child) {
                MarkForceSplitAdjacentStringsFlat(*child);
            }
        }
        for (FormatBreakListItem& item : node.items) {
            if (item.node) {
                MarkForceSplitAdjacentStringsFlat(*item.node);
            }
        }
        for (FormatBreakNode* operand : node.operands) {
            if (operand) {
                MarkForceSplitAdjacentStringsFlat(*operand);
            }
        }
    }

    static bool IsEmptyCompoundBlock(const SyntaxNode& node) {
        if (!SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::CompoundBlock)) {
            return false;
        }
        for (const SyntaxNode* child : node.children) {
            if (
                child != nullptr &&
                child->kind != SyntaxNodeKind::LeftBrace &&
                child->kind != SyntaxNodeKind::RightBrace
            ) {
                return false;
            }
        }
        return true;
    }

    void GroupAdjacentStrings(FormatBreakNode& sequence, int depth) {
        std::vector<FormatBreakNode*> grouped;
        grouped.reserve(sequence.children.size());
        for (size_t index = 0; index < sequence.children.size();) {
            if (!IsStringTokenChild(sequence.children[index])) {
                grouped.push_back(sequence.children[index]);
                ++index;
                continue;
            }

            const size_t begin = index;
            while (index < sequence.children.size() && IsStringTokenChild(sequence.children[index])) {
                ++index;
            }
            if (index - begin == 1) {
                grouped.push_back(sequence.children[begin]);
                continue;
            }

            auto strings = MakeNode(FormatBreakNodeKind::AdjacentStrings, depth + 1);
            std::vector<FormatBreakNode*> operands;
            operands.reserve(index - begin);
            size_t compactRunStart = 0;
            for (size_t cursor = begin; cursor < index; ++cursor) {
                if (cursor + 1 < index && ForcesStringBoundarySplit(*TokenChild(sequence.children[cursor]))) {
                    strings->forceSplit = true;
                }
                operands.push_back(sequence.children[cursor]);
                const std::string_view text =
                    FormatTokenText(FormatBreakTokenValue(*TokenChild(sequence.children[cursor])));
                if (strings->compactStringTexts.empty()) {
                    strings->compactStringTexts.emplace_back(text);
                    continue;
                }
                if (std::optional<std::string> joined = JoinOrdinaryStringLiterals(
                    strings->compactStringTexts[compactRunStart], text
                )) {
                    strings->compactStringTexts[compactRunStart] = std::move(*joined);
                    strings->compactStringTexts.emplace_back();
                } else {
                    compactRunStart = strings->compactStringTexts.size();
                    strings->compactStringTexts.emplace_back(text);
                }
            }
            strings->operands = StoreNodePointers(operands);
            grouped.push_back(strings);
        }
        sequence.children = StoreNodePointers(grouped);
    }

    static void NormalizeNamedListBreakDepth(std::vector<FormatBreakNode*>& children) {
        for (size_t index = 1; index < children.size(); ++index) {
            const FormatBreakNode* list = children[index];
            if (list->kind != FormatBreakNodeKind::Delimited || list->children.empty()) {
                continue;
            }
            const FormatBreakToken* open = TokenChild(list->children.front());
            if (
                open == nullptr ||
                !SyntaxNodeKindHasClass(FormatBreakTokenValue(*open).parentKind, SyntaxNodeClass::NamedList)
            ) {
                continue;
            }
            for (size_t prefixIndex = 0; prefixIndex < index; ++prefixIndex) {
                FormatBreakNode& prefix = *children[prefixIndex];
                const std::optional<int> prefixDepth = MinimumStructuralBreakDepth(prefix);
                if (prefixDepth && *prefixDepth <= list->structuralDepth) {
                    ShiftStructuralDepth(prefix, list->structuralDepth + 1 - *prefixDepth, false);
                }
            }
        }
    }

    void GroupMemberCallArguments(std::vector<FormatBreakNode*>& children, int depth) {
        for (size_t index = 0; index + 1 < children.size();) {
            FormatBreakNode* chain = children[index];
            if (
                chain == nullptr ||
                chain->kind != FormatBreakNodeKind::Chain ||
                chain->chainKind != FormatBreakChainKind::MemberBeforeOperator ||
                chain->operands.empty() ||
                !IsArgumentList(children[index + 1])
            ) {
                ++index;
                continue;
            }

            auto call = MakeNode(FormatBreakNodeKind::Sequence, depth + 1);
            call->children = StoreNodePointers({chain->operands.back(), children[index + 1]});
            chain->operands.back() = call;
            children.erase(children.begin() + static_cast<std::ptrdiff_t>(index + 1));
        }
    }

    FormatBreakNode* BuildRequiredTernarySuffix(std::vector<FormatBreakNode*>& children, int depth) {
        std::vector<size_t> operatorIndices;
        for (size_t index = 0; index < children.size(); ++index) {
            const FormatBreakToken* token = TokenChild(children[index]);
            if (
                token != nullptr &&
                FormatBreakTokenSyntaxKind(*token) == SyntaxNodeKind::Colon &&
                RequiresChainBreak(*token)
            ) {
                operatorIndices.push_back(index);
            }
        }
        if (operatorIndices.empty()) {
            return nullptr;
        }
        const auto buildOperand = [&](size_t begin, size_t end) {
            if (begin == end) {
                return MakeNode(FormatBreakNodeKind::Sequence, depth + 1);
            }
            if (begin + 1 == end) {
                return children[begin];
            }
            auto sequence = MakeNode(FormatBreakNodeKind::Sequence, depth + 1);
            sequence->children =
                StoreNodePointers(std::span<FormatBreakNode* const>{children.data() + begin, end - begin});
            return sequence;
        };

        std::vector<FormatBreakNode*> operands;
        std::vector<FormatBreakToken> operators;
        size_t operandBegin = 0;
        for (size_t operatorIndex : operatorIndices) {
            operands.push_back(buildOperand(operandBegin, operatorIndex));
            operators.push_back(*TokenChild(children[operatorIndex]));
            operandBegin = operatorIndex + 1;
        }
        operands.push_back(buildOperand(operandBegin, children.size()));

        auto chain = MakeNode(FormatBreakNodeKind::Chain, depth);
        chain->forceSplit = true;
        chain->chainKind = FormatBreakChainKind::AfterOperator;
        chain->operands = StoreNodePointers(operands);
        chain->operators = StoreTokens(operators);
        return chain;
    }

    static bool ChainOperatorsMatch(const FormatBreakNode& chain, std::optional<SyntaxNodeKind> operatorKind) {
        return !operatorKind ||
            std::all_of(chain.operators.begin(), chain.operators.end(), [operatorKind](const FormatBreakToken& token) {
                return FormatBreakTokenSyntaxKind(token) == *operatorKind;
            });
    }

    static FormatBreakNode* MatchingChain(
        FormatBreakNode* node, FormatBreakChainKind chainKind, std::optional<SyntaxNodeKind> operatorKind = std::nullopt
    ) {
        if (node == nullptr || node->kind != FormatBreakNodeKind::Chain) {
            return nullptr;
        }
        return node->chainKind == chainKind && ChainOperatorsMatch(*node, operatorKind) ? node : nullptr;
    }

    static std::vector<FormatBreakToken> DetachTrailingStandaloneComments(FormatBreakNode*& node) {
        std::vector<FormatBreakToken> comments;
        if (node == nullptr || node->kind != FormatBreakNodeKind::Sequence) {
            return comments;
        }
        size_t commentBegin = node->children.size();
        while (commentBegin > 0) {
            const FormatBreakNode* child = node->children[commentBegin - 1];
            if (
                child == nullptr ||
                child->kind != FormatBreakNodeKind::Token ||
                FormatBreakTokenKind(child->token) != PrintTokenKind::Comment
            ) {
                break;
            }
            --commentBegin;
        }
        if (commentBegin == node->children.size()) {
            return comments;
        }
        comments.reserve(node->children.size() - commentBegin);
        for (size_t index = commentBegin; index < node->children.size(); ++index) {
            comments.push_back(node->children[index]->token);
        }
        node->children = node->children.first(commentBegin);
        if (node->children.size() == 1) {
            node = node->children.front();
        }
        return comments;
    }

    FormatBreakNode* BuildFunctionPointerAliasDeclaration(const SyntaxNode& node, int depth) {
        std::optional<size_t> operatorIndex;
        std::optional<size_t> declaratorIndex;
        for (size_t index = 0; index < node.children.size(); ++index) {
            const SyntaxNode* child = node.children[index];
            if (child == nullptr) {
                continue;
            }
            if (child->kind == SyntaxNodeKind::Equal) {
                operatorIndex = index;
                continue;
            }
            if (operatorIndex && child->kind == SyntaxNodeKind::LeftParen) {
                declaratorIndex = index;
                break;
            }
        }
        if (!operatorIndex || !declaratorIndex || *declaratorIndex <= *operatorIndex + 1) {
            return nullptr;
        }
        const std::optional<FormatBreakToken> op = TokenForNode(*node.children[*operatorIndex]);
        if (!op || !IsAssignmentOperatorForNode(*op)) {
            return nullptr;
        }

        FormatBreakNode* left = BuildSequenceFromChildren(node.children, 0, *operatorIndex, depth + 1);
        FormatBreakNode* returnType =
            BuildSequenceFromChildren(node.children, *operatorIndex + 1, *declaratorIndex, depth + 2);
        FormatBreakNode* declarator =
            BuildSequenceFromChildren(node.children, *declaratorIndex, node.children.size(), depth + 2);
        if (left == nullptr || returnType == nullptr || declarator == nullptr) {
            return nullptr;
        }

        auto signature = MakeNode(FormatBreakNodeKind::FunctionSignature, depth + 1);
        NormalizeCallablePrefixBreakDepth(*returnType, *declarator);
        signature->children = StoreNodePointers({returnType, declarator});

        auto chain = MakeNode(FormatBreakNodeKind::Chain, depth);
        chain->declarationValueOwner = &node;
        chain->operands = StoreNodePointers({left, signature});
        chain->operators = StoreTokens({*op});
        return chain;
    }

    FormatBreakNode* BuildFunctionPointerDeclaratorDeclaration(const SyntaxNode& node, int depth) {
        std::optional<size_t> declaratorIndex;
        for (size_t index = 0; index < node.children.size(); ++index) {
            const SyntaxNode* child = node.children[index];
            if (
                child != nullptr &&
                child->kind == SyntaxNodeKind::FunctionDeclarator &&
                FunctionDeclaratorTargetContainsFunctionPointerDeclarator(*child)
            ) {
                declaratorIndex = index;
                break;
            }
        }
        if (!declaratorIndex || *declaratorIndex == 0 || !ContainsSelected(*node.children[*declaratorIndex])) {
            return nullptr;
        }

        FormatBreakNode* returnType = BuildSequenceFromChildren(node.children, 0, *declaratorIndex, depth + 1);
        FormatBreakNode* declarator =
            BuildSequenceFromChildren(node.children, *declaratorIndex, *declaratorIndex + 1, depth + 1);
        if (!returnType || !declarator) {
            return nullptr;
        }

        auto signature = MakeNode(FormatBreakNodeKind::FunctionSignature, depth);
        NormalizeCallablePrefixBreakDepth(*returnType, *declarator);
        std::array<FormatBreakNode*, 3> signatureChildren{returnType, declarator, nullptr};
        size_t signatureChildCount = 2;
        if (*declaratorIndex + 1 < node.children.size()) {
            FormatBreakNode* tail =
                BuildSequenceFromChildren(node.children, *declaratorIndex + 1, node.children.size(), depth + 1);
            if (tail) {
                signatureChildren[signatureChildCount++] = tail;
            }
        }
        signature->children =
            StoreNodePointers(std::span<FormatBreakNode* const>{signatureChildren.data(), signatureChildCount});
        return signature;
    }

    FormatBreakNode* BuildOwnedValue(
        FormatBreakNode* owner, FormatBreakNode* value, int depth, bool splitTrailingBodyHeaderAtParentIndent = true
    ) {
        if (owner == nullptr || value == nullptr) {
            return nullptr;
        }
        auto result = MakeNode(FormatBreakNodeKind::Chain, depth);
        result->operands = StoreNodePointers({owner, value});
        result->operators = StoreTokens({{}});
        MarkForceSplitAdjacentStringsFlat(*value);
        if (splitTrailingBodyHeaderAtParentIndent && EndsWithBodyHeader(*value)) {
            result->splitTrailingBodyHeaderAtParentIndent = true;
            MarkBodyHeaderSplitAtParentIndentWhenLineStarts(*value);
        }
        return result;
    }

    FormatBreakNode* BuildMacroReplacementCallSequence(const SyntaxNode& replacement, int depth) {
        std::vector<ConstSyntaxChildList> itemChildren;
        size_t callCount = 0;
        for (const SyntaxNode* child : replacement.children) {
            if (child == nullptr || !ContainsSelected(*child)) {
                continue;
            }
            if (child->kind == SyntaxNodeKind::MacroCallItem) {
                itemChildren.emplace_back();
                itemChildren.back().push_back(child);
                ++callCount;
                continue;
            }
            if (
                child->kind == SyntaxNodeKind::Comment ||
                child->kind == SyntaxNodeKind::TrailingComment ||
                child->kind == SyntaxNodeKind::BlankLine
            ) {
                itemChildren.emplace_back();
                itemChildren.back().push_back(child);
                continue;
            }
            if (itemChildren.empty()) {
                return nullptr;
            }
            itemChildren.back().push_back(child);
        }
        if (callCount < 2) {
            return nullptr;
        }

        auto sequence = MakeNode(FormatBreakNodeKind::StatementSequence, depth);
        sequence->forceSplit = true;
        for (const ConstSyntaxChildList& item : itemChildren) {
            AppendListItem(*sequence, BuildSequenceFromPointers(item, depth + 1), false);
        }
        return sequence;
    }

    FormatBreakNode* BuildMacroDefinition(const SyntaxNode& node, int depth) {
        std::optional<size_t> valueIndex;
        bool ownerSelected = false;
        for (size_t index = 0; index < node.children.size(); ++index) {
            const SyntaxNode* child = node.children[index];
            if (child == nullptr) {
                continue;
            }
            if (child->kind == SyntaxNodeKind::MacroReplacementList) {
                if (ContainsSelected(*child)) {
                    valueIndex = index;
                }
                break;
            }
            ownerSelected = ownerSelected || ContainsSelected(*child);
        }
        if (!ownerSelected || !valueIndex) {
            return nullptr;
        }

        FormatBreakNode* owner = BuildSequenceFromChildren(node.children, 0, *valueIndex, depth + 1);
        const SyntaxNode& replacement = *node.children[*valueIndex];
        FormatBreakNode* value = BuildMacroReplacementCallSequence(replacement, depth + 1);
        if (value == nullptr) {
            value = BuildSyntaxNode(replacement, depth + 1);
        }
        FormatBreakNode* definition = BuildOwnedValue(owner, value, depth, false);
        if (definition != nullptr) {
            // The structured-macro grammar has only two header-level forms: one physical line, or a break after
            // the complete definition header. A later mandatory formatting segment makes the one-line form
            // structurally impossible; otherwise the solver retains it only when its complete candidate fits.
            // Neither constraint inspects a chosen child layout or makes a local break decision in the printer.
            definition->forceSplit = ContainsUnselectedSourceLeaf(replacement);
            definition->chainCompactRequiresFitOnOneLine = true;
        }
        return definition;
    }

    FormatBreakNode* BuildSyntaxNode(const SyntaxNode& node, int depth) {
        if (!ContainsSelected(node)) {
            return nullptr;
        }
        if (std::optional<FormatBreakToken> token = TokenForNode(node)) {
            return BuildToken(*token, depth);
        }
        if (!SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::Tree)) {
            return nullptr;
        }
        if (SyntaxNodeHasLocalClass(node, SyntaxNodeClass::QualifiedName)) {
            if (auto qualifiedName = BuildQualifiedName(node, depth)) {
                return qualifiedName;
            }
        }
        if (SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::MacroDefinition)) {
            if (auto definition = BuildMacroDefinition(node, depth)) {
                return definition;
            }
        }
        if (node.kind == SyntaxNodeKind::FunctionPointerAliasDeclaration) {
            if (auto alias = BuildFunctionPointerAliasDeclaration(node, depth)) {
                return alias;
            }
        }
        if (SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::DeclarationNode)) {
            if (auto declaration = BuildFunctionPointerDeclaratorDeclaration(node, depth)) {
                return declaration;
            }
        }
        if (SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::DeclarationNode)) {
            if (auto declaration = BuildCommaSeparatedDeclaration(node, depth)) {
                return declaration;
            }
        }
        if (SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::DeclarationNode)) {
            if (auto declaration = BuildDirectInitializedDeclaration(node, depth)) {
                return declaration;
            }
        }
        if (SyntaxNodeHasLocalClass(node, SyntaxNodeClass::KeywordOwnedValue)) {
            if (auto ownedValue = BuildKeywordOwnedValue(node, depth)) {
                return ownedValue;
            }
        }
        if (node.kind == SyntaxNodeKind::TemplateDeclaration) {
            if (auto declaration = BuildTemplateDeclaration(node, depth)) {
                return declaration;
            }
        }
        if (node.kind == SyntaxNodeKind::Declaration) {
            if (auto declaration = BuildAssignedDeclaration(node, depth)) {
                return declaration;
            }
        }
        if (node.kind == SyntaxNodeKind::FunctionDefinition) {
            if (auto header = BuildFunctionBodyHeader(node, depth)) {
                return header;
            }
        }
        if (node.kind == SyntaxNodeKind::Declaration || node.kind == SyntaxNodeKind::FunctionDefinition) {
            if (auto signature = BuildFunctionSignature(node, depth)) {
                return signature;
            }
        }
        if (node.kind == SyntaxNodeKind::FunctionDefinition) {
            if (auto header = BuildInitializerListBodyHeader(node, depth)) {
                return header;
            }
        }
        if (node.kind == SyntaxNodeKind::LambdaExpression) {
            if (auto header = BuildLambdaBodyHeader(node, depth)) {
                return header;
            }
        }
        if (SyntaxNodeHasLocalClass(node, SyntaxNodeClass::DeclaredTypeSpecifier)) {
            if (auto header = BuildDeclaredTypeBodyHeader(node, depth)) {
                return header;
            }
        }
        if (
            node.kind == SyntaxNodeKind::BinaryExpression ||
            node.kind == SyntaxNodeKind::AssignmentExpression ||
            node.kind == SyntaxNodeKind::InitDeclarator ||
            node.kind == SyntaxNodeKind::FieldDeclaration ||
            node.kind == SyntaxNodeKind::AliasDeclaration ||
            node.kind == SyntaxNodeKind::CommaExpression ||
            node.kind == SyntaxNodeKind::ConditionalExpression
        ) {
            if (auto expression = BuildOperatorExpression(node, depth)) {
                return expression;
            }
        }
        if (node.kind == SyntaxNodeKind::FieldExpression) {
            if (auto expression = BuildMemberExpression(node, depth)) {
                return expression;
            }
        }
        if (SyntaxNodeHasLocalClass(node, SyntaxNodeClass::PrefixList)) {
            if (auto list = BuildPrefixList(node, depth)) {
                return list;
            }
        }
        if (node.kind == SyntaxNodeKind::MacroStatementSequence) {
            if (auto sequence = BuildStatementSequence(node, depth)) {
                return sequence;
            }
        }
        return BuildSequenceFromChildren(node.children, 0, node.children.size(), depth);
    }

    struct QualifiedNameParts {
        std::vector<ConstSyntaxChildList> operands;
        std::vector<FormatBreakToken> operators;
    };

    bool CollectQualifiedNameParts(const SyntaxNode& node, QualifiedNameParts& result) const {
        QualifiedNameParts local;
        ConstSyntaxChildList pendingOperand;
        for (const SyntaxNode* child : node.children) {
            if (child == nullptr || !ContainsSelected(*child)) {
                continue;
            }
            if (SyntaxNodeHasLocalClass(*child, SyntaxNodeClass::QualifiedName)) {
                QualifiedNameParts nested;
                if (!CollectQualifiedNameParts(*child, nested) || nested.operands.empty()) {
                    return false;
                }
                if (!pendingOperand.empty()) {
                    if (!local.operands.empty() || !local.operators.empty()) {
                        return false;
                    }
                    nested
                        .operands
                        .front()
                        .insert(nested.operands.front().begin(), pendingOperand.begin(), pendingOperand.end());
                    pendingOperand.clear();
                }
                if (local.operands.size() != local.operators.size()) {
                    return false;
                }
                local.operands.insert(local.operands.end(), nested.operands.begin(), nested.operands.end());
                local.operators.insert(local.operators.end(), nested.operators.begin(), nested.operators.end());
                continue;
            }
            const std::optional<FormatBreakToken> token = TokenForNode(*child);
            if (token && FormatBreakTokenSyntaxKind(*token) == SyntaxNodeKind::ColonColon) {
                if (pendingOperand.empty() && local.operands.empty() && local.operators.empty()) {
                    // A leading global-scope operator is part of the first operand and is not breakable.
                    pendingOperand.push_back(child);
                    continue;
                }
                if (pendingOperand.empty() || local.operands.size() != local.operators.size()) {
                    return false;
                }
                local.operands.push_back(std::move(pendingOperand));
                pendingOperand.clear();
                local.operators.push_back(*token);
                continue;
            }
            if (!local.operands.empty() && local.operands.size() > local.operators.size()) {
                return false;
            }
            pendingOperand.push_back(child);
        }
        if (!pendingOperand.empty()) {
            if (local.operands.size() != local.operators.size()) {
                return false;
            }
            local.operands.push_back(std::move(pendingOperand));
        }
        if (local.operands.empty() || local.operands.size() != local.operators.size() + 1) {
            return false;
        }
        result.operands.insert(result.operands.end(), local.operands.begin(), local.operands.end());
        result.operators.insert(result.operators.end(), local.operators.begin(), local.operators.end());
        return true;
    }

    FormatBreakNode* BuildQualifiedNamePrefix(const QualifiedNameParts& parts, size_t operandCount, int depth) {
        if (operandCount == 1) {
            return BuildSequenceFromPointers(parts.operands.front(), depth);
        }
        FormatBreakNode* left = BuildQualifiedNamePrefix(parts, operandCount - 1, depth + 1);
        FormatBreakNode* right = BuildSequenceFromPointers(parts.operands[operandCount - 1], depth + 1);
        if (left == nullptr || right == nullptr) {
            return nullptr;
        }
        auto binary = MakeNode(FormatBreakNodeKind::Chain, depth);
        binary->operands = StoreNodePointers({left, right});
        binary->operators = StoreTokens({parts.operators[operandCount - 2]});
        return binary;
    }

    FormatBreakNode* BuildQualifiedName(const SyntaxNode& node, int depth) {
        QualifiedNameParts parts;
        if (!CollectQualifiedNameParts(node, parts)) {
            return nullptr;
        }
        if (parts.operators.empty()) {
            return nullptr;
        }
        return BuildQualifiedNamePrefix(parts, parts.operands.size(), depth);
    }

    FormatBreakNode* BuildKeywordOwnedValue(const SyntaxNode& node, int depth) {
        std::optional<size_t> valueIndex;
        std::optional<size_t> semicolonIndex;
        for (size_t index = 0; index < node.children.size(); ++index) {
            const SyntaxNode* child = node.children[index];
            if (child == nullptr) {
                continue;
            }
            if (child->kind == SyntaxNodeKind::Semicolon) {
                semicolonIndex = index;
                break;
            }
            if (
                valueIndex == std::nullopt &&
                index > 0 &&
                ContainsSelected(*child) &&
                child->kind != SyntaxNodeKind::Comment &&
                child->kind != SyntaxNodeKind::TrailingComment
            ) {
                valueIndex = index;
            }
        }
        const size_t valueEnd = semicolonIndex.value_or(node.children.size());
        if (!valueIndex || *valueIndex >= valueEnd) {
            return nullptr;
        }

        FormatBreakNode* prefix = BuildSequenceFromChildren(node.children, 0, *valueIndex, depth + 1);
        FormatBreakNode* value = BuildSequenceFromChildren(node.children, *valueIndex, valueEnd, depth + 1);
        if (prefix == nullptr || value == nullptr) {
            return nullptr;
        }

        FormatBreakNode* ownedValue = BuildOwnedValue(prefix, value, depth + 1);
        if (ownedValue == nullptr || !semicolonIndex) {
            return ownedValue;
        }

        FormatBreakNode* suffix =
            BuildSequenceFromChildren(node.children, *semicolonIndex, node.children.size(), depth + 1);
        if (suffix == nullptr) {
            return nullptr;
        }

        auto sequence = MakeNode(FormatBreakNodeKind::Sequence, depth);
        sequence->children = StoreNodePointers({ownedValue, suffix});
        return sequence;
    }

    bool HasSelectedBefore(const SyntaxNode& node, size_t end) const {
        for (size_t index = 0; index < end && index < node.children.size(); ++index) {
            if (node.children[index] != nullptr && ContainsSelected(*node.children[index])) {
                return true;
            }
        }
        return false;
    }

    FormatBreakNode*
        BuildTemplatePrefix(const ConstSyntaxChildList& templateHeadChildren, const SyntaxNode* requiresNode, int depth)
    {
        FormatBreakNode* templateHead = BuildSequenceFromPointers(templateHeadChildren, depth + 1);
        if (templateHead == nullptr || requiresNode == nullptr) {
            return templateHead;
        }

        FormatBreakNode* requiresClause = BuildSyntaxNode(*requiresNode, depth + 1);
        if (requiresClause == nullptr) {
            return nullptr;
        }

        auto prefix = MakeNode(FormatBreakNodeKind::Chain, depth);
        prefix->operands = StoreNodePointers({templateHead, requiresClause});
        prefix->operators = StoreTokens({{}});
        prefix->chainPrefersSplitWhenCompactBreaks = true;
        prefix->forceSplit = ContainsNonemptyRequirementSequence(*requiresNode);
        return prefix;
    }

    FormatBreakNode*
        BuildDetachedTemplateDeclaration(FormatBreakNode* prefix, FormatBreakNode* declaration, int depth)
    {
        if (prefix == nullptr || declaration == nullptr) {
            return nullptr;
        }
        auto result = MakeNode(FormatBreakNodeKind::BodyHeader, depth);
        result->bodyHeaderRequiresDetachedBody = true;
        result->bodyHeaderDetachBodyAfterExpandedHeader = true;
        result->children = StoreNodePointers({prefix, declaration});
        return result;
    }

    FormatBreakNode* BuildTemplateDeclaration(const SyntaxNode& node, int depth) {
        std::optional<size_t> parametersIndex;
        for (size_t index = 0; index < node.children.size(); ++index) {
            if (node.children[index] && node.children[index]->kind == SyntaxNodeKind::TemplateParameterList) {
                parametersIndex = index;
                break;
            }
        }
        if (!parametersIndex || *parametersIndex + 1 >= node.children.size()) {
            return nullptr;
        }

        ConstSyntaxChildList templateHeadChildren;
        templateHeadChildren.reserve(*parametersIndex + 1);
        for (size_t index = 0; index <= *parametersIndex; ++index) {
            templateHeadChildren.push_back(node.children[index]);
        }

        size_t declarationIndex = *parametersIndex + 1;
        const SyntaxNode* requiresNode = nullptr;
        if (
            declarationIndex < node.children.size() &&
            node.children[declarationIndex] != nullptr &&
            node.children[declarationIndex]->kind == SyntaxNodeKind::RequiresClause
        ) {
            requiresNode = node.children[declarationIndex];
            ++declarationIndex;
        }
        if (declarationIndex >= node.children.size()) {
            return nullptr;
        }

        FormatBreakNode* declaration = nullptr;
        const SyntaxNode* introduced = node.children[declarationIndex];
        if (introduced != nullptr) {
            std::optional<size_t> introducedRequiresIndex;
            for (size_t index = 0; index < introduced->children.size(); ++index) {
                if (
                    introduced->children[index] != nullptr &&
                    introduced->children[index]->kind == SyntaxNodeKind::RequiresClause
                ) {
                    introducedRequiresIndex = index;
                    break;
                }
            }
            if (introducedRequiresIndex && !HasSelectedBefore(*introduced, *introducedRequiresIndex)) {
                requiresNode = introduced->children[*introducedRequiresIndex];
                ConstSyntaxChildList declarationChildren;
                declarationChildren.reserve(
                    introduced->children.size() - *introducedRequiresIndex - 1 + node.children.size() - declarationIndex
                );
                for (size_t index = *introducedRequiresIndex + 1; index < introduced->children.size(); ++index) {
                    declarationChildren.push_back(introduced->children[index]);
                }
                for (size_t index = declarationIndex + 1; index < node.children.size(); ++index) {
                    declarationChildren.push_back(node.children[index]);
                }
                declaration = BuildSequenceFromPointers(declarationChildren, depth + 1);
            }
        }
        if (declaration == nullptr) {
            declaration = BuildSequenceFromChildren(node.children, declarationIndex, node.children.size(), depth + 1);
        }

        FormatBreakNode* prefix = BuildTemplatePrefix(templateHeadChildren, requiresNode, depth + 1);
        if (declaration == nullptr && introduced != nullptr && !ContainsSelected(*introduced)) {
            return prefix;
        }
        return BuildDetachedTemplateDeclaration(prefix, declaration, depth);
    }

    FormatBreakNode* BuildAdjacentTemplateDeclaration(
        const ::SyntaxChildList& children, size_t index, size_t end, int depth, size_t& after
    ) {
        const SyntaxNode* templateNode = children[index];
        if (templateNode == nullptr || templateNode->kind != SyntaxNodeKind::TemplateDeclaration) {
            return nullptr;
        }

        std::optional<size_t> parametersIndex;
        for (size_t childIndex = 0; childIndex < templateNode->children.size(); ++childIndex) {
            if (
                templateNode->children[childIndex] != nullptr &&
                templateNode->children[childIndex]->kind == SyntaxNodeKind::TemplateParameterList
            ) {
                parametersIndex = childIndex;
                break;
            }
        }
        if (!parametersIndex) {
            return nullptr;
        }

        ConstSyntaxChildList templateHeadChildren;
        templateHeadChildren.reserve(*parametersIndex + 1);
        for (size_t childIndex = 0; childIndex <= *parametersIndex; ++childIndex) {
            templateHeadChildren.push_back(templateNode->children[childIndex]);
        }

        size_t templateTail = *parametersIndex + 1;
        const SyntaxNode* requiresNode = nullptr;
        if (
            templateTail < templateNode->children.size() &&
            templateNode->children[templateTail] != nullptr &&
            templateNode->children[templateTail]->kind == SyntaxNodeKind::RequiresClause
        ) {
            requiresNode = templateNode->children[templateTail];
            ++templateTail;
        }
        if (templateTail < templateNode->children.size()) {
            return nullptr;
        }

        size_t declarationIndex = index + 1;
        while (
            declarationIndex < end &&
            (children[declarationIndex] == nullptr || !ContainsSelected(*children[declarationIndex]))
        ) {
            ++declarationIndex;
        }
        if (declarationIndex >= end) {
            return nullptr;
        }

        FormatBreakNode* declaration = nullptr;
        const SyntaxNode* introduced = children[declarationIndex];
        if (introduced != nullptr) {
            std::optional<size_t> introducedRequiresIndex;
            for (size_t childIndex = 0; childIndex < introduced->children.size(); ++childIndex) {
                if (
                    introduced->children[childIndex] != nullptr &&
                    introduced->children[childIndex]->kind == SyntaxNodeKind::RequiresClause
                ) {
                    introducedRequiresIndex = childIndex;
                    break;
                }
            }
            if (introducedRequiresIndex && !HasSelectedBefore(*introduced, *introducedRequiresIndex)) {
                requiresNode = introduced->children[*introducedRequiresIndex];
                ConstSyntaxChildList declarationChildren;
                declarationChildren.reserve(introduced->children.size() - *introducedRequiresIndex - 1);
                for (
                    size_t childIndex = *introducedRequiresIndex + 1;
                    childIndex < introduced->children.size();
                    ++childIndex
                ) {
                    declarationChildren.push_back(introduced->children[childIndex]);
                }
                declaration = BuildSequenceFromPointers(declarationChildren, depth + 1);
            }
        }
        if (declaration == nullptr && introduced != nullptr) {
            declaration = BuildSyntaxNode(*introduced, depth + 1);
        }

        FormatBreakNode* prefix = BuildTemplatePrefix(templateHeadChildren, requiresNode, depth + 1);
        after = declarationIndex + 1;
        if (declaration == nullptr && introduced != nullptr && !ContainsSelected(*introduced)) {
            return prefix;
        }
        return BuildDetachedTemplateDeclaration(prefix, declaration, depth);
    }

    FormatBreakNode*
        BuildCodeBlockBodyHeader(const SyntaxNode& bodyNode, FormatBreakNode* header, FormatBreakNode* body, int depth)
    {
        auto result = MakeNode(FormatBreakNodeKind::BodyHeader, depth);
        result->bodyHeaderDetachBodyAfterExpandedHeader = !IsEmptyCompoundBlock(bodyNode);
        result->children = StoreNodePointers({header, body});
        return result;
    }

    FormatBreakNode* BuildLambdaCallableHeader(const SyntaxNode& node, size_t end, int depth) {
        std::optional<size_t> declaratorIndex;
        for (size_t index = 0; index < end; ++index) {
            const SyntaxNode* child = node.children[index];
            if (
                child != nullptr &&
                child->kind == SyntaxNodeKind::AbstractFunctionDeclarator &&
                ContainsSyntaxKind(*child, SyntaxNodeKind::ParameterList)
            ) {
                declaratorIndex = index;
                break;
            }
        }
        if (!declaratorIndex || *declaratorIndex == 0) {
            return BuildSequenceFromChildren(node.children, 0, end, depth);
        }

        FormatBreakNode* prefix = BuildSequenceFromChildren(node.children, 0, *declaratorIndex, depth);
        FormatBreakNode* declarator = BuildSequenceFromChildren(node.children, *declaratorIndex, end, depth);
        if (prefix == nullptr || declarator == nullptr) {
            return nullptr;
        }
        NormalizeCallablePrefixBreakDepth(*prefix, *declarator);

        auto header = MakeNode(FormatBreakNodeKind::Sequence, depth);
        header->children = StoreNodePointers({prefix, declarator});
        return header;
    }

    FormatBreakNode* BuildLambdaBodyHeader(const SyntaxNode& node, int depth) {
        std::optional<size_t> bodyIndex;
        for (size_t index = 0; index < node.children.size(); ++index) {
            if (node.children[index] && node.children[index]->kind == SyntaxNodeKind::CompoundStatement) {
                bodyIndex = index;
                break;
            }
        }
        if (!bodyIndex || *bodyIndex == 0 || !ContainsSelected(*node.children[*bodyIndex])) {
            return nullptr;
        }
        FormatBreakNode* header = BuildLambdaCallableHeader(node, *bodyIndex, depth + 1);
        FormatBreakNode* body = BuildSequenceFromChildren(node.children, *bodyIndex, node.children.size(), depth + 1);
        if (!header || !body) {
            return nullptr;
        }

        FormatBreakNode* result = BuildCodeBlockBodyHeader(*node.children[*bodyIndex], header, body, depth);
        result->bodyHeaderIsLambda = true;
        result->bodyHeaderSingleStatementBody =
            CallableBodyAllowsCompactSingleStatementForm(*node.children[*bodyIndex], node.kind);
        return result;
    }

    FormatBreakNode* BuildFunctionBodyHeader(const SyntaxNode& node, int depth) {
        std::optional<size_t> bodyIndex;
        for (size_t index = 0; index < node.children.size(); ++index) {
            if (node.children[index] && node.children[index]->kind == SyntaxNodeKind::CompoundStatement) {
                bodyIndex = index;
                break;
            }
        }
        if (
            !bodyIndex ||
            *bodyIndex == 0 ||
            !CallableBodyAllowsCompactSingleStatementForm(*node.children[*bodyIndex], node.kind) ||
            !ContainsSelected(*node.children[*bodyIndex])
        ) {
            return nullptr;
        }

        FormatBreakNode* header = BuildFunctionSignature(node, depth + 1, *bodyIndex);
        if (header == nullptr) {
            header = BuildSequenceFromChildren(node.children, 0, *bodyIndex, depth + 1);
        }
        FormatBreakNode* body = BuildSequenceFromChildren(node.children, *bodyIndex, node.children.size(), depth + 1);
        if (header == nullptr || body == nullptr) {
            return nullptr;
        }

        FormatBreakNode* result = BuildCodeBlockBodyHeader(*node.children[*bodyIndex], header, body, depth);
        result->bodyHeaderSingleStatementBody = true;
        return result;
    }

    FormatBreakNode* BuildDeclaredTypeBodyHeader(const SyntaxNode& node, int depth) {
        std::optional<size_t> bodyIndex;
        for (size_t index = 0; index < node.children.size(); ++index) {
            const SyntaxNode* child = node.children[index];
            if (child != nullptr && SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::CompoundBlock)) {
                bodyIndex = index;
                break;
            }
        }
        if (!bodyIndex || *bodyIndex == 0 || !ContainsSelected(*node.children[*bodyIndex])) {
            return nullptr;
        }

        FormatBreakNode* header = BuildSequenceFromChildren(node.children, 0, *bodyIndex, depth + 1);
        FormatBreakNode* body = BuildSequenceFromChildren(node.children, *bodyIndex, node.children.size(), depth + 1);
        if (header == nullptr || body == nullptr) {
            return nullptr;
        }

        return BuildCodeBlockBodyHeader(*node.children[*bodyIndex], header, body, depth);
    }

    FormatBreakNode* BuildInitializerListBodyHeader(const SyntaxNode& node, int depth) {
        std::optional<size_t> bodyIndex;
        bool hasInitializerList = false;
        for (size_t index = 0; index < node.children.size(); ++index) {
            const SyntaxNode* child = node.children[index];
            if (child == nullptr) {
                continue;
            }
            if (child->kind == SyntaxNodeKind::CompoundStatement) {
                bodyIndex = index;
                break;
            }
            hasInitializerList = hasInitializerList || child->kind == SyntaxNodeKind::FieldInitializerList;
        }
        if (!bodyIndex || *bodyIndex == 0 || !hasInitializerList || !ContainsSelected(*node.children[*bodyIndex])) {
            return nullptr;
        }

        FormatBreakNode* header = BuildSequenceFromChildren(node.children, 0, *bodyIndex, depth + 1);
        FormatBreakNode* body = BuildSequenceFromChildren(node.children, *bodyIndex, node.children.size(), depth + 1);
        if (!header || !body) {
            return nullptr;
        }

        return BuildCodeBlockBodyHeader(*node.children[*bodyIndex], header, body, depth);
    }

    static std::optional<size_t> DirectFunctionDeclaratorChildIndex(const SyntaxNode& node) {
        for (size_t index = 0; index < node.children.size(); ++index) {
            if (node.children[index] != nullptr && node.children[index]->kind == SyntaxNodeKind::FunctionDeclarator) {
                return index;
            }
        }
        return std::nullopt;
    }

    static bool CanWrapFunctionDeclarator(SyntaxNodeKind kind) {
        return SyntaxNodeKindHasClass(kind, SyntaxNodeClass::DeclaratorReferenceParent) ||
            SyntaxNodeKindHasClass(kind, SyntaxNodeClass::ParenthesizedDeclarator);
    }

    FormatBreakNode* BuildNestedFunctionSignature(
        const SyntaxNode& node, size_t declaratorIndex, size_t functionDeclaratorIndex, int depth, size_t end
    ) {
        const SyntaxNode* wrapper = node.children[declaratorIndex];
        if (wrapper == nullptr || !ContainsSelected(*wrapper->children[functionDeclaratorIndex])) {
            return nullptr;
        }

        ConstSyntaxChildList returnTypeChildren;
        returnTypeChildren.reserve(declaratorIndex + functionDeclaratorIndex);
        for (size_t index = 0; index < declaratorIndex; ++index) {
            returnTypeChildren.push_back(node.children[index]);
        }
        for (size_t index = 0; index < functionDeclaratorIndex; ++index) {
            returnTypeChildren.push_back(wrapper->children[index]);
        }

        ConstSyntaxChildList tailChildren;
        tailChildren.reserve(wrapper->children.size() - functionDeclaratorIndex - 1 + end);
        for (size_t index = functionDeclaratorIndex + 1; index < wrapper->children.size(); ++index) {
            tailChildren.push_back(wrapper->children[index]);
        }
        for (size_t index = declaratorIndex + 1; index < end; ++index) {
            tailChildren.push_back(node.children[index]);
        }

        FormatBreakNode* returnType = BuildSequenceFromPointers(returnTypeChildren, depth + 1);
        FormatBreakNode* declarator = BuildSequenceFromChildren(
            wrapper->children, functionDeclaratorIndex, functionDeclaratorIndex + 1, depth + 1
        );
        if (!returnType || !declarator) {
            return nullptr;
        }

        auto signature = MakeNode(FormatBreakNodeKind::FunctionSignature, depth);
        signature->functionSignatureHasBody =
            node.kind == SyntaxNodeKind::FunctionDefinition && end == node.children.size();
        NormalizeCallablePrefixBreakDepth(*returnType, *declarator);
        std::array<FormatBreakNode*, 3> signatureChildren{returnType, declarator, nullptr};
        size_t signatureChildCount = 2;
        if (!tailChildren.empty()) {
            FormatBreakNode* tail = BuildSequenceFromPointers(tailChildren, depth + 1);
            if (tail) {
                signatureChildren[signatureChildCount++] = tail;
            }
        }
        signature->children =
            StoreNodePointers(std::span<FormatBreakNode* const>{signatureChildren.data(), signatureChildCount});
        return signature;
    }

    FormatBreakNode* BuildFunctionSignature(const SyntaxNode& node, int depth, size_t end) {
        std::optional<size_t> declaratorIndex;
        std::optional<size_t> nestedFunctionDeclaratorIndex;
        for (size_t index = 0; index < end; ++index) {
            const SyntaxNode* child = node.children[index];
            if (child == nullptr) {
                continue;
            }
            if (child->kind == SyntaxNodeKind::FunctionDeclarator) {
                declaratorIndex = index;
                break;
            }
            if (CanWrapFunctionDeclarator(child->kind)) {
                if (std::optional<size_t> nested = DirectFunctionDeclaratorChildIndex(*child)) {
                    declaratorIndex = index;
                    nestedFunctionDeclaratorIndex = nested;
                    break;
                }
            }
        }
        if (!declaratorIndex || *declaratorIndex == 0) {
            return nullptr;
        }
        for (size_t index = 0; index < *declaratorIndex; ++index) {
            if (
                node.children[index] != nullptr &&
                ContainsSyntaxKind(*node.children[index], SyntaxNodeKind::KeywordExplicit)
            ) {
                return nullptr;
            }
        }
        if (nestedFunctionDeclaratorIndex) {
            return BuildNestedFunctionSignature(node, *declaratorIndex, *nestedFunctionDeclaratorIndex, depth, end);
        }
        if (!ContainsSelected(*node.children[*declaratorIndex])) {
            return nullptr;
        }

        FormatBreakNode* returnType = BuildSequenceFromChildren(node.children, 0, *declaratorIndex, depth + 1);
        FormatBreakNode* declarator =
            BuildSequenceFromChildren(node.children, *declaratorIndex, *declaratorIndex + 1, depth + 1);
        if (!returnType || !declarator) {
            return nullptr;
        }

        auto signature = MakeNode(FormatBreakNodeKind::FunctionSignature, depth);
        signature->functionSignatureHasBody =
            node.kind == SyntaxNodeKind::FunctionDefinition && end == node.children.size();
        NormalizeCallablePrefixBreakDepth(*returnType, *declarator);
        std::array<FormatBreakNode*, 3> signatureChildren{returnType, declarator, nullptr};
        size_t signatureChildCount = 2;
        if (*declaratorIndex + 1 < end) {
            FormatBreakNode* tail = BuildSequenceFromChildren(node.children, *declaratorIndex + 1, end, depth + 1);
            if (tail) {
                signatureChildren[signatureChildCount++] = tail;
            }
        }
        signature->children =
            StoreNodePointers(std::span<FormatBreakNode* const>{signatureChildren.data(), signatureChildCount});
        return signature;
    }

    FormatBreakNode* BuildFunctionSignature(const SyntaxNode& node, int depth) {
        return BuildFunctionSignature(node, depth, node.children.size());
    }

    static bool IsDirectInitializedDeclarator(const SyntaxNode& node) {
        if (node.kind != SyntaxNodeKind::InitDeclarator) {
            return false;
        }
        bool hasAssignment = false;
        bool hasInitializer = false;
        for (const SyntaxNode* child : node.children) {
            if (!child) {
                continue;
            }
            if (
                SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::Known) &&
                SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::AssignmentOperator)
            ) {
                hasAssignment = true;
            }
            if (child->kind == SyntaxNodeKind::InitializerList) {
                hasInitializer = true;
            }
            if (child->kind == SyntaxNodeKind::ArgumentList) {
                hasInitializer = true;
            }
        }
        return hasInitializer && !hasAssignment;
    }

    static bool IsAssignmentOperatorNode(const SyntaxNode& node) {
        return SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::Known) &&
            SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::AssignmentOperator);
    }

    FormatBreakNode* BuildCommaSeparatedDeclaration(const SyntaxNode& node, int depth) {
        std::vector<FormatBreakNode*> operands;
        std::vector<FormatBreakToken> operators;
        size_t operandBegin = 0;
        for (size_t index = 0; index < node.children.size(); ++index) {
            const SyntaxNode* child = node.children[index];
            if (child == nullptr || child->kind != SyntaxNodeKind::Comma) {
                continue;
            }
            const std::optional<FormatBreakToken> comma = TokenForNode(*child);
            if (!comma) {
                continue;
            }
            FormatBreakNode* operand = BuildSequenceFromChildren(node.children, operandBegin, index, depth + 1);
            if (operand == nullptr) {
                return nullptr;
            }
            operands.push_back(operand);
            operators.push_back(*comma);
            operandBegin = index + 1;
        }
        if (operators.empty()) {
            return nullptr;
        }
        FormatBreakNode* tail = BuildSequenceFromChildren(node.children, operandBegin, node.children.size(), depth + 1);
        if (tail == nullptr) {
            return nullptr;
        }
        operands.push_back(tail);

        auto chain = MakeNode(FormatBreakNodeKind::Chain, depth);
        chain->operands = StoreNodePointers(operands);
        chain->operators = StoreTokens(operators);
        return chain;
    }

    std::optional<size_t> DirectInitializedDeclaratorIndex(const SyntaxNode& node) const {
        bool hasAssignment = false;
        for (size_t index = 0; index < node.children.size(); ++index) {
            const SyntaxNode* child = node.children[index];
            if (child == nullptr) {
                continue;
            }
            hasAssignment = hasAssignment || IsAssignmentOperatorNode(*child);
            if (IsDirectInitializedDeclarator(*child)) {
                return index;
            }
            // Some direct initializer forms attach the initializer list as a declaration child after the declarator.
            if (child->kind == SyntaxNodeKind::InitializerList && index > 0 && !hasAssignment) {
                return index - 1;
            }
        }
        return std::nullopt;
    }

    FormatBreakNode* BuildDirectInitializedDeclaration(const SyntaxNode& node, int depth) {
        const std::optional<size_t> declaratorIndex = DirectInitializedDeclaratorIndex(node);
        if (!declaratorIndex || *declaratorIndex == 0) {
            return nullptr;
        }

        auto chain = MakeNode(FormatBreakNodeKind::Chain, depth);
        chain->declarationValueOwner = &node;
        FormatBreakNode* left = BuildSequenceFromChildren(node.children, 0, *declaratorIndex, depth + 1);
        FormatBreakNode* right =
            BuildSequenceFromChildren(node.children, *declaratorIndex, node.children.size(), depth + 1);
        if (right != nullptr) {
            MarkForceSplitAdjacentStringsFlat(*right);
            if (EndsWithBodyHeader(*right)) {
                chain->splitTrailingBodyHeaderAtParentIndent = true;
                MarkBodyHeaderSplitAtParentIndentWhenLineStarts(*right);
            }
        }
        chain->operands = StoreNodePointers({left, right});
        chain->operators = StoreTokens({{}});
        return chain;
    }

    FormatBreakNode* BuildAssignedDeclaration(const SyntaxNode& node, int depth) {
        std::optional<size_t> declaratorIndex;
        std::optional<size_t> operatorIndex;
        for (size_t index = 0; index < node.children.size(); ++index) {
            if (!node.children[index] || node.children[index]->kind != SyntaxNodeKind::InitDeclarator) {
                continue;
            }
            if (declaratorIndex) {
                return nullptr;
            }
            const std::optional<size_t> directOperator = DirectOperatorIndex(*node.children[index]);
            if (!directOperator || !node.children[index]->children[*directOperator]) {
                return nullptr;
            }
            declaratorIndex = index;
            operatorIndex = *directOperator;
        }
        if (!declaratorIndex || !operatorIndex) {
            return nullptr;
        }

        const SyntaxNode& declarator = *node.children[*declaratorIndex];
        const std::optional<FormatBreakToken> op = TokenForNode(*declarator.children[*operatorIndex]);
        if (!op || !IsAssignmentOperatorForNode(*op)) {
            return nullptr;
        }

        ConstSyntaxChildList leftChildren;
        leftChildren.reserve(*declaratorIndex + *operatorIndex);
        for (size_t index = 0; index < *declaratorIndex; ++index) {
            leftChildren.push_back(node.children[index]);
        }
        for (size_t index = 0; index < *operatorIndex; ++index) {
            leftChildren.push_back(declarator.children[index]);
        }

        auto chain = MakeNode(FormatBreakNodeKind::Chain, depth);
        chain->declarationValueOwner = &node;
        FormatBreakNode* left = BuildSequenceFromPointers(leftChildren, depth + 1);
        FormatBreakNode* right =
            BuildSequenceFromChildren(declarator.children, *operatorIndex + 1, declarator.children.size(), depth + 1);
        if (right != nullptr) {
            MarkForceSplitAdjacentStringsFlat(*right);
            if (EndsWithBodyHeader(*right)) {
                chain->splitTrailingBodyHeaderAtParentIndent = true;
                MarkBodyHeaderSplitAtParentIndentWhenLineStarts(*right);
            }
        }
        chain->operands = StoreNodePointers({left, right});
        chain->operators = StoreTokens({*op});
        std::vector<FormatBreakNode*> tailChildren;
        tailChildren.reserve(node.children.size() - *declaratorIndex - 1);
        for (size_t index = *declaratorIndex + 1; index < node.children.size(); ++index) {
            if (node.children[index] && ContainsSelected(*node.children[index])) {
                if (FormatBreakNode* built = BuildSyntaxNode(*node.children[index], depth + 1)) {
                    tailChildren.push_back(built);
                }
            }
        }
        if (tailChildren.empty()) {
            return chain;
        }
        auto sequence = MakeNode(FormatBreakNodeKind::Sequence, depth);
        std::vector<FormatBreakNode*> sequenceChildren;
        sequenceChildren.reserve(tailChildren.size() + 1);
        sequenceChildren.push_back(chain);
        sequenceChildren.insert(sequenceChildren.end(), tailChildren.begin(), tailChildren.end());
        sequence->children = StoreNodePointers(sequenceChildren);
        return sequence;
    }

    FormatBreakNode* BuildSequenceFromPointers(const ConstSyntaxChildList& children, int depth) {
        std::vector<FormatBreakNode*> builtChildren;
        builtChildren.reserve(children.size());
        for (const SyntaxNode* child : children) {
            if (child == nullptr) {
                continue;
            }
            if (FormatBreakNode* built = BuildSyntaxNode(*child, depth + 1)) {
                builtChildren.push_back(built);
            }
        }
        NormalizeNamedListBreakDepth(builtChildren);
        GroupMemberCallArguments(builtChildren, depth);
        if (FormatBreakNode* suffix = BuildRequiredTernarySuffix(builtChildren, depth)) {
            return suffix;
        }
        if (builtChildren.size() == 1) {
            return builtChildren.front();
        }
        auto sequence = MakeNode(FormatBreakNodeKind::Sequence, depth);
        sequence->children = StoreNodePointers(builtChildren);
        GroupAdjacentStrings(*sequence, depth);
        return sequence;
    }

    FormatBreakNode* BuildSequenceFromChildren(const ::SyntaxChildList& children, size_t begin, size_t end, int depth) {
        std::vector<FormatBreakNode*> builtChildren;
        builtChildren.reserve(end - begin);
        for (size_t index = begin; index < end;) {
            if (!children[index] || !ContainsSelected(*children[index])) {
                ++index;
                continue;
            }
            size_t afterTemplate = index;
            if (
                FormatBreakNode*
                    templated = BuildAdjacentTemplateDeclaration(children, index, end, depth + 1, afterTemplate)
            ) {
                builtChildren.push_back(templated);
                index = afterTemplate;
                continue;
            }
            size_t afterDelimited = index;
            if (FormatBreakNode* delimited = BuildDirectDelimited(children, index, end, depth + 1, afterDelimited)) {
                builtChildren.push_back(delimited);
                index = afterDelimited;
                continue;
            }
            if (FormatBreakNode* built = BuildSyntaxNode(*children[index], depth + 1)) {
                builtChildren.push_back(built);
            }
            ++index;
        }
        NormalizeNamedListBreakDepth(builtChildren);
        GroupMemberCallArguments(builtChildren, depth);
        if (FormatBreakNode* suffix = BuildRequiredTernarySuffix(builtChildren, depth)) {
            return suffix;
        }
        if (builtChildren.size() == 1) {
            return builtChildren.front();
        }
        auto sequence = MakeNode(FormatBreakNodeKind::Sequence, depth);
        sequence->children = StoreNodePointers(builtChildren);
        GroupAdjacentStrings(*sequence, depth);
        return sequence;
    }

    std::optional<size_t> DirectOperatorIndex(const SyntaxNode& node) const {
        for (size_t index = 0; index < node.children.size(); ++index) {
            if (!node.children[index]) {
                continue;
            }
            const std::optional<FormatBreakToken> token = TokenForNode(*node.children[index]);
            if (!token) {
                continue;
            }
            if (
                (node.kind == SyntaxNodeKind::BinaryExpression && IsBinaryOperatorForNode(*token)) ||
                (node.kind == SyntaxNodeKind::CommaExpression && IsCommaOperatorForNode(*token)) || (
                    (
                        node.kind == SyntaxNodeKind::AssignmentExpression ||
                        node.kind == SyntaxNodeKind::InitDeclarator ||
                        node.kind == SyntaxNodeKind::FieldDeclaration ||
                        node.kind == SyntaxNodeKind::AliasDeclaration
                    ) && IsAssignmentOperatorForNode(*token)
                )
            ) {
                return index;
            }
        }
        return std::nullopt;
    }

    std::optional<size_t> DirectMemberOperatorIndex(const SyntaxNode& node) const {
        if (node.kind != SyntaxNodeKind::FieldExpression) {
            return std::nullopt;
        }
        for (size_t index = 0; index < node.children.size(); ++index) {
            if (!node.children[index]) {
                continue;
            }
            const std::optional<FormatBreakToken> token = TokenForNode(*node.children[index]);
            if (token && IsMemberChainOperatorForNode(*token)) {
                return index;
            }
        }
        return std::nullopt;
    }

    FormatBreakNode* BuildMemberExpression(const SyntaxNode& node, int depth) {
        const std::optional<size_t> opIndex = DirectMemberOperatorIndex(node);
        if (!opIndex || !node.children[*opIndex]) {
            return nullptr;
        }
        const std::optional<FormatBreakToken> op = TokenForNode(*node.children[*opIndex]);
        if (!op) {
            return nullptr;
        }

        FormatBreakNode* left = BuildSequenceFromChildren(node.children, 0, *opIndex, depth + 1);
        FormatBreakNode* right =
            BuildSequenceFromChildren(node.children, *opIndex + 1, node.children.size(), depth + 1);
        if (left == nullptr || right == nullptr) {
            return nullptr;
        }

        std::vector<FormatBreakToken> commentsBeforeOperator = DetachTrailingStandaloneComments(left);
        FormatBreakNode* leftChain = MatchingChain(left, FormatBreakChainKind::MemberBeforeOperator);
        std::vector<FormatBreakNode*> operands;
        std::vector<FormatBreakToken> operators;
        std::vector<std::vector<FormatBreakToken>> commentsBeforeOperators;
        if (leftChain != nullptr) {
            operands.insert(operands.end(), leftChain->operands.begin(), leftChain->operands.end());
            operators.insert(operators.end(), leftChain->operators.begin(), leftChain->operators.end());
            commentsBeforeOperators = leftChain->commentsBeforeOperators;
        } else {
            operands.push_back(left);
        }
        operators.push_back(*op);
        commentsBeforeOperators.push_back(std::move(commentsBeforeOperator));
        operands.push_back(right);

        auto chain = MakeNode(FormatBreakNodeKind::Chain, depth);
        chain->chainKind = FormatBreakChainKind::MemberBeforeOperator;
        chain->chainPrefersSplitWhenCompactBreaks = std::any_of(
            commentsBeforeOperators.begin(),
            commentsBeforeOperators.end(),
            [](const std::vector<FormatBreakToken>& comments) { return !comments.empty(); }
        );
        chain->operands = StoreNodePointers(operands);
        chain->operators = StoreTokens(operators);
        chain->commentsBeforeOperators = std::move(commentsBeforeOperators);
        return chain;
    }

    bool HasDirectCommaOperator(const SyntaxNode& node) const {
        if (node.kind != SyntaxNodeKind::CommaExpression) {
            return false;
        }
        const std::optional<size_t> opIndex = DirectOperatorIndex(node);
        if (!opIndex || !node.children[*opIndex]) {
            return false;
        }
        const std::optional<FormatBreakToken> token = TokenForNode(*node.children[*opIndex]);
        return token && FormatBreakTokenSyntaxKind(*token) == SyntaxNodeKind::Comma;
    }

    bool AppendCommaExpressionListOperand(
        FormatBreakNode& list,
        const ::SyntaxChildList& children,
        size_t begin,
        size_t end,
        const FormatBreakToken& open,
        int depth,
        bool blankLineBefore
    ) {
        const SyntaxNode* selectedChild = nullptr;
        for (size_t index = begin; index < end; ++index) {
            const SyntaxNode* child = children[index];
            if (child == nullptr || !ContainsSelected(*child)) {
                continue;
            }
            if (selectedChild != nullptr) {
                selectedChild = nullptr;
                break;
            }
            selectedChild = child;
        }
        if (selectedChild != nullptr && HasDirectCommaOperator(*selectedChild)) {
            return AppendCommaExpressionListItems(list, *selectedChild, open, depth, blankLineBefore);
        }

        FormatBreakNode* item = BuildSequenceFromChildren(children, begin, end, depth + 1);
        if (item == nullptr) {
            return false;
        }
        AppendListItem(list, item, blankLineBefore);
        return true;
    }

    bool AppendCommaExpressionListItems(
        FormatBreakNode& list, const SyntaxNode& node, const FormatBreakToken& open, int depth, bool blankLineBefore
    ) {
        const std::optional<size_t> opIndex = DirectOperatorIndex(node);
        if (!opIndex || !node.children[*opIndex]) {
            return false;
        }
        const std::optional<FormatBreakToken> separator = TokenForNode(*node.children[*opIndex]);
        if (!separator || FormatBreakTokenSyntaxKind(*separator) != SyntaxNodeKind::Comma) {
            return false;
        }
        if (!AppendCommaExpressionListOperand(list, node.children, 0, *opIndex, open, depth, blankLineBefore)) {
            return false;
        }
        AttachSeparatorToPreviousItem(list, *separator);
        return AppendCommaExpressionListOperand(
            list, node.children, *opIndex + 1, node.children.size(), open, depth, false
        );
    }

    const SyntaxNode*
        DirectDelimitedCommaExpressionBody(const ::SyntaxChildList& children, size_t openIndex, size_t closeIndex) const
    {
        const SyntaxNode* body = nullptr;
        for (size_t index = openIndex + 1; index < closeIndex; ++index) {
            const SyntaxNode* child = children[index];
            if (child == nullptr || !ContainsSelected(*child)) {
                continue;
            }
            if (body != nullptr) {
                return nullptr;
            }
            body = child;
        }
        return body != nullptr && HasDirectCommaOperator(*body) ? body : nullptr;
    }

    std::optional<std::pair<size_t, size_t>> DirectConditionalOperatorIndices(const SyntaxNode& node) const {
        std::optional<size_t> question;
        std::optional<size_t> colon;
        for (size_t index = 0; index < node.children.size(); ++index) {
            if (!node.children[index]) {
                continue;
            }
            const std::optional<FormatBreakToken> token = TokenForNode(*node.children[index]);
            if (!token || !IsConditionalOperatorForNode(*token)) {
                continue;
            }
            if (FormatBreakTokenSyntaxKind(*token) == SyntaxNodeKind::Question) {
                question = index;
            } else if (FormatBreakTokenSyntaxKind(*token) == SyntaxNodeKind::Colon) {
                colon = index;
            }
        }
        if (!question || !colon || *question >= *colon) {
            return std::nullopt;
        }
        return std::pair<size_t, size_t>{*question, *colon};
    }

    bool HasSameDirectBinaryOperator(const SyntaxNode& node, SyntaxNodeKind op) const {
        if (node.kind != SyntaxNodeKind::BinaryExpression) {
            return false;
        }
        const std::optional<size_t> opIndex = DirectOperatorIndex(node);
        if (!opIndex || !node.children[*opIndex]) {
            return false;
        }
        const std::optional<FormatBreakToken> token = TokenForNode(*node.children[*opIndex]);
        return token && FormatBreakTokenSyntaxKind(*token) == op;
    }

    std::vector<FormatBreakToken> AppendBinaryChainOperand(
        std::vector<FormatBreakNode*>& operands,
        std::vector<FormatBreakToken>& operators,
        std::vector<std::vector<FormatBreakToken>>& commentsBeforeOperators,
        const ::SyntaxChildList& children,
        size_t begin,
        size_t end,
        SyntaxNodeKind op,
        int depth,
        bool detachTrailingComments
    ) {
        if (
            end == begin + 1 &&
            children[begin] &&
            SyntaxNodeKindHasClass(op, SyntaxNodeClass::ChainOperator) &&
            HasSameDirectBinaryOperator(*children[begin], op)
        ) {
            AppendBinaryChain(*children[begin], op, operands, operators, commentsBeforeOperators, depth);
            return {};
        }
        FormatBreakNode* operand = BuildSequenceFromChildren(children, begin, end, depth + 1);
        std::vector<FormatBreakToken> trailingComments;
        if (detachTrailingComments) {
            trailingComments = DetachTrailingStandaloneComments(operand);
        }
        const FormatBreakChainKind chainKind = op == SyntaxNodeKind::LessLess || op == SyntaxNodeKind::GreaterGreater ?
            FormatBreakChainKind::StreamBeforeOperator : FormatBreakChainKind::AfterOperator;
        FormatBreakNode* prefix = MatchingChain(operand, chainKind, op);
        if (prefix != nullptr) {
            operands.insert(operands.end(), prefix->operands.begin(), prefix->operands.end());
            operators.insert(operators.end(), prefix->operators.begin(), prefix->operators.end());
            commentsBeforeOperators.insert(
                commentsBeforeOperators.end(),
                prefix->commentsBeforeOperators.begin(),
                prefix->commentsBeforeOperators.end()
            );
            return trailingComments;
        }
        operands.push_back(operand);
        return trailingComments;
    }

    void AppendBinaryChain(
        const SyntaxNode& node,
        SyntaxNodeKind op,
        std::vector<FormatBreakNode*>& operands,
        std::vector<FormatBreakToken>& operators,
        std::vector<std::vector<FormatBreakToken>>& commentsBeforeOperators,
        int depth
    ) {
        const std::optional<size_t> opIndex = DirectOperatorIndex(node);
        if (!opIndex || !node.children[*opIndex]) {
            operands.push_back(BuildSyntaxNode(node, depth + 1));
            return;
        }

        const bool streamChain = op == SyntaxNodeKind::LessLess || op == SyntaxNodeKind::GreaterGreater;
        std::vector<FormatBreakToken> commentsBeforeOperator = AppendBinaryChainOperand(
            operands, operators, commentsBeforeOperators, node.children, 0, *opIndex, op, depth, streamChain
        );
        if (std::optional<FormatBreakToken> token = TokenForNode(*node.children[*opIndex])) {
            operators.push_back(*token);
            commentsBeforeOperators.push_back(std::move(commentsBeforeOperator));
        }
        AppendBinaryChainOperand(
            operands,
            operators,
            commentsBeforeOperators,
            node.children,
            *opIndex + 1,
            node.children.size(),
            op,
            depth,
            false
        );
    }

    FormatBreakNode* BuildLeadingStreamOperatorChain(const SyntaxNode& node, int depth) {
        std::vector<size_t> operatorIndices;
        std::vector<FormatBreakToken> directOperators;
        for (size_t index = 0; index < node.children.size(); ++index) {
            if (!node.children[index]) {
                continue;
            }
            const std::optional<FormatBreakToken> token = TokenForNode(*node.children[index]);
            if (!token) {
                continue;
            }
            const SyntaxNodeKind kind = FormatBreakTokenSyntaxKind(*token);
            if (kind != SyntaxNodeKind::LessLess && kind != SyntaxNodeKind::GreaterGreater) {
                continue;
            }
            operatorIndices.push_back(index);
            directOperators.push_back(*token);
        }
        if (operatorIndices.empty()) {
            return nullptr;
        }

        std::vector<FormatBreakNode*> operands;
        std::vector<FormatBreakToken> operators;
        std::vector<std::vector<FormatBreakToken>> commentsBeforeOperators;
        std::vector<FormatBreakToken> pendingComments;
        auto emptyReceiver = MakeNode(FormatBreakNodeKind::Sequence, depth + 1);
        operands.push_back(emptyReceiver);
        for (size_t index = 0; index < operatorIndices.size(); ++index) {
            operators.push_back(directOperators[index]);
            commentsBeforeOperators.push_back(std::move(pendingComments));
            const size_t operandBegin = operatorIndices[index] + 1;
            const size_t operandEnd =
                index + 1 < operatorIndices.size() ? operatorIndices[index + 1] : node.children.size();
            if (operandBegin >= operandEnd) {
                return nullptr;
            }
            pendingComments = AppendBinaryChainOperand(
                operands,
                operators,
                commentsBeforeOperators,
                node.children,
                operandBegin,
                operandEnd,
                FormatBreakTokenSyntaxKind(directOperators[index]),
                depth,
                index + 1 < operatorIndices.size()
            );
        }
        if (operands.size() != operators.size() + 1) {
            return nullptr;
        }

        auto chain = MakeNode(FormatBreakNodeKind::Chain, depth);
        chain->chainKind = FormatBreakChainKind::StreamBeforeOperator;
        chain->chainStartsWithOperator = true;
        chain->operands = StoreNodePointers(operands);
        chain->operators = StoreTokens(operators);
        chain->commentsBeforeOperators = std::move(commentsBeforeOperators);
        return chain;
    }

    bool SelectionStartsWithDirectStreamOperator(const SyntaxNode& node) const {
        for (const SyntaxNode* child : node.children) {
            if (child == nullptr || !ContainsSelected(*child)) {
                continue;
            }
            const std::optional<FormatBreakToken> token = TokenForNode(*child);
            return token && (
                FormatBreakTokenSyntaxKind(*token) == SyntaxNodeKind::LessLess ||
                FormatBreakTokenSyntaxKind(*token) == SyntaxNodeKind::GreaterGreater
            );
        }
        return false;
    }

    FormatBreakNode* BuildBinaryOrAssignmentExpression(const SyntaxNode& node, int depth) {
        if (SyntaxNodeHasLocalClass(node, SyntaxNodeClass::LeadingStreamOperatorChain) || (
            SyntaxNodeHasLocalClass(node, SyntaxNodeClass::ConditionalStreamOperatorChain) &&
            SelectionStartsWithDirectStreamOperator(node)
        )) {
            return BuildLeadingStreamOperatorChain(node, depth);
        }
        const std::optional<size_t> opIndex = DirectOperatorIndex(node);
        if (!opIndex || !node.children[*opIndex]) {
            return nullptr;
        }
        const std::optional<FormatBreakToken> token = TokenForNode(*node.children[*opIndex]);
        if (!token) {
            return nullptr;
        }

        auto chain = MakeNode(FormatBreakNodeKind::Chain, depth);
        if (
            node.kind == SyntaxNodeKind::Declaration ||
            node.kind == SyntaxNodeKind::FieldDeclaration ||
            node.kind == SyntaxNodeKind::AliasDeclaration ||
            node.kind == SyntaxNodeKind::FunctionPointerAliasDeclaration ||
            node.kind == SyntaxNodeKind::InitDeclarator
        ) {
            chain->declarationValueOwner = &node;
        }
        const SyntaxNodeKind operatorKind = FormatBreakTokenSyntaxKind(*token);
        chain->chainKind =
            (operatorKind == SyntaxNodeKind::LessLess || operatorKind == SyntaxNodeKind::GreaterGreater) ?
                FormatBreakChainKind::StreamBeforeOperator : FormatBreakChainKind::AfterOperator;
        chain->forceSplit = chain->chainKind == FormatBreakChainKind::StreamBeforeOperator &&
            context_.forceSplitStreamChain &&
            root_ == &node;
        if (
            node.kind == SyntaxNodeKind::BinaryExpression &&
            SyntaxNodeKindHasClass(operatorKind, SyntaxNodeClass::ChainOperator)
        ) {
            std::vector<FormatBreakNode*> operands;
            std::vector<FormatBreakToken> operators;
            std::vector<std::vector<FormatBreakToken>> commentsBeforeOperators;
            operands.reserve(2);
            operators.reserve(1);
            AppendBinaryChain(node, operatorKind, operands, operators, commentsBeforeOperators, depth);
            for (size_t index = 1; index < operands.size(); ++index) {
                if (operands[index] != nullptr) {
                    MarkForceSplitAdjacentStringsFlat(*operands[index]);
                }
            }
            chain->chainPrefersSplitWhenCompactBreaks = std::any_of(
                commentsBeforeOperators.begin(),
                commentsBeforeOperators.end(),
                [](const std::vector<FormatBreakToken>& comments) { return !comments.empty(); }
            );
            chain->operands = StoreNodePointers(operands);
            chain->operators = StoreTokens(operators);
            chain->commentsBeforeOperators = std::move(commentsBeforeOperators);
            return chain;
        }

        FormatBreakNode* left = BuildSequenceFromChildren(node.children, 0, *opIndex, depth + 1);
        FormatBreakNode* right =
            BuildSequenceFromChildren(node.children, *opIndex + 1, node.children.size(), depth + 1);
        if (right != nullptr) {
            MarkForceSplitAdjacentStringsFlat(*right);
            if (EndsWithBodyHeader(*right)) {
                chain->splitTrailingBodyHeaderAtParentIndent = true;
                MarkBodyHeaderSplitAtParentIndentWhenLineStarts(*right);
            }
        }
        chain->operands = StoreNodePointers({left, right});
        chain->operators = StoreTokens({*token});
        return chain;
    }

    void AppendConditionalChain(
        const SyntaxNode& node,
        std::vector<FormatBreakNode*>& operands,
        std::vector<FormatBreakToken>& operators,
        int depth
    ) {
        const std::optional<std::pair<size_t, size_t>> operatorIndices = DirectConditionalOperatorIndices(node);
        if (!operatorIndices) {
            operands.push_back(BuildSyntaxNode(node, depth + 1));
            return;
        }
        const size_t question = operatorIndices->first;
        const size_t colon = operatorIndices->second;
        operands.push_back(BuildSequenceFromChildren(node.children, 0, question, depth + 1));
        if (std::optional<FormatBreakToken> token = TokenForNode(*node.children[question])) {
            operators.push_back(*token);
        }
        AppendConditionalBranch(node.children, question + 1, colon, operands, operators, depth);
        if (std::optional<FormatBreakToken> token = TokenForNode(*node.children[colon])) {
            operators.push_back(*token);
        }
        AppendConditionalBranch(node.children, colon + 1, node.children.size(), operands, operators, depth);
    }

    void AppendConditionalBranch(
        const ::SyntaxChildList& children,
        size_t begin,
        size_t end,
        std::vector<FormatBreakNode*>& operands,
        std::vector<FormatBreakToken>& operators,
        int depth
    ) {
        if (
            end == begin + 1 &&
            begin < children.size() &&
            children[begin] != nullptr &&
            children[begin]->kind == SyntaxNodeKind::ConditionalExpression
        ) {
            AppendConditionalChain(*children[begin], operands, operators, depth);
        } else {
            operands.push_back(BuildSequenceFromChildren(children, begin, end, depth + 1));
        }
    }

    FormatBreakNode* BuildConditionalExpression(const SyntaxNode& node, int depth) {
        if (!DirectConditionalOperatorIndices(node)) {
            return nullptr;
        }
        auto chain = MakeNode(FormatBreakNodeKind::Chain, depth);
        chain->chainKind = FormatBreakChainKind::Ternary;
        std::vector<FormatBreakNode*> operands;
        std::vector<FormatBreakToken> operators;
        operands.reserve(3);
        operators.reserve(2);
        AppendConditionalChain(node, operands, operators, depth);
        for (size_t index = 1; index < operands.size(); ++index) {
            if (operands[index] != nullptr) {
                MarkForceSplitAdjacentStringsFlat(*operands[index]);
            }
        }
        chain->operands = StoreNodePointers(operands);
        chain->operators = StoreTokens(operators);
        return chain;
    }

    FormatBreakNode* BuildOperatorExpression(const SyntaxNode& node, int depth) {
        if (node.kind == SyntaxNodeKind::ConditionalExpression) {
            return BuildConditionalExpression(node, depth);
        }
        return BuildBinaryOrAssignmentExpression(node, depth);
    }

    FormatBreakNode* BuildDelimitedAssignmentItem(const ConstSyntaxChildList& children, int depth) {
        std::optional<size_t> operatorIndex;
        std::optional<FormatBreakToken> op;
        for (size_t index = 0; index < children.size(); ++index) {
            if (children[index] == nullptr) {
                continue;
            }
            const std::optional<FormatBreakToken> token = TokenForNode(*children[index]);
            if (!token || !IsAssignmentOperatorForNode(*token)) {
                continue;
            }
            if (operatorIndex) {
                return nullptr;
            }
            operatorIndex = index;
            op = token;
        }
        if (!operatorIndex || !op) {
            return nullptr;
        }

        ConstSyntaxChildList leftChildren(children.begin(), children.begin() + *operatorIndex);
        ConstSyntaxChildList rightChildren(children.begin() + *operatorIndex + 1, children.end());
        FormatBreakNode* left = BuildSequenceFromPointers(leftChildren, depth + 1);
        FormatBreakNode* right = BuildSequenceFromPointers(rightChildren, depth + 1);
        if (left == nullptr || right == nullptr) {
            return nullptr;
        }
        MarkForceSplitAdjacentStringsFlat(*right);

        auto chain = MakeNode(FormatBreakNodeKind::Chain, depth);
        if (EndsWithBodyHeader(*right)) {
            chain->splitTrailingBodyHeaderAtParentIndent = true;
            MarkBodyHeaderSplitAtParentIndentWhenLineStarts(*right);
        }
        chain->operands = StoreNodePointers({left, right});
        chain->operators = StoreTokens({*op});
        return chain;
    }

    FormatBreakNode* BuildPrefixList(const SyntaxNode& node, int depth) {
        std::optional<size_t> prefixIndex;
        for (size_t index = 0; index < node.children.size(); ++index) {
            if (!node.children[index]) {
                continue;
            }
            const std::optional<FormatBreakToken> token = TokenForNode(*node.children[index]);
            if (
                token &&
                FormatBreakTokenKind(*token) == PrintTokenKind::Known &&
                FormatBreakTokenSyntaxKind(*token) == SyntaxNodeKind::Colon
            ) {
                prefixIndex = index;
                break;
            }
        }
        if (!prefixIndex || !node.children[*prefixIndex]) {
            return nullptr;
        }
        const std::optional<FormatBreakToken> prefix = TokenForNode(*node.children[*prefixIndex]);
        if (!prefix) {
            return nullptr;
        }

        auto list = MakeNode(FormatBreakNodeKind::PrefixList, depth);
        list->children = StoreNodePointers({BuildToken(*prefix, depth + 1)});

        ConstSyntaxChildList itemChildren;
        bool pendingBlankLine = false;
        for (size_t index = *prefixIndex + 1; index < node.children.size(); ++index) {
            const SyntaxNode* child = node.children[index];
            if (child == nullptr) {
                continue;
            }
            if (SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::ListForceSplitMarker)) {
                list->forceSplit = true;
                if (child->kind == SyntaxNodeKind::BlankLine) {
                    pendingBlankLine = true;
                    continue;
                }
                const std::optional<FormatBreakToken> comment = TokenForNode(*child);
                if (!comment) {
                    continue;
                }
                if (child->kind == SyntaxNodeKind::TrailingComment) {
                    if (!itemChildren.empty()) {
                        const bool blankLineBefore = ShouldPreservePendingBlankLine(*list, pendingBlankLine, false);
                        AppendListItem(*list, BuildSequenceFromPointers(itemChildren, depth + 1), blankLineBefore);
                        itemChildren.clear();
                    }
                    AttachTrailingCommentToPreviousItem(*list, *comment);
                } else {
                    if (!itemChildren.empty()) {
                        const bool blankLineBefore = ShouldPreservePendingBlankLine(*list, pendingBlankLine, false);
                        AppendListItem(*list, BuildSequenceFromPointers(itemChildren, depth + 1), blankLineBefore);
                        itemChildren.clear();
                        pendingBlankLine = false;
                    }
                    const bool blankLineBefore = ShouldPreservePendingBlankLine(*list, pendingBlankLine, true);
                    AppendStandaloneCommentItem(*list, *comment, depth + 1, blankLineBefore);
                }
                pendingBlankLine = false;
                continue;
            }
            if (!ContainsSelected(*child)) {
                continue;
            }
            const std::optional<FormatBreakToken> token = TokenForNode(*child);
            if (
                token &&
                FormatBreakTokenKind(*token) == PrintTokenKind::Known &&
                FormatBreakTokenSyntaxKind(*token) == SyntaxNodeKind::Comma
            ) {
                if (!itemChildren.empty()) {
                    const bool blankLineBefore = ShouldPreservePendingBlankLine(*list, pendingBlankLine, false);
                    AppendListItem(*list, BuildSequenceFromPointers(itemChildren, depth + 1), blankLineBefore);
                    itemChildren.clear();
                    pendingBlankLine = false;
                }
                AttachSeparatorToPreviousItem(*list, *token);
                continue;
            }
            itemChildren.push_back(child);
        }
        if (!itemChildren.empty()) {
            const bool blankLineBefore = ShouldPreservePendingBlankLine(*list, pendingBlankLine, false);
            AppendListItem(*list, BuildSequenceFromPointers(itemChildren, depth + 1), blankLineBefore);
        }
        return list->items.empty() ? nullptr : list;
    }

    FormatBreakNode* BuildStatementSequence(const SyntaxNode& node, int depth) {
        auto sequence = MakeNode(FormatBreakNodeKind::StatementSequence, depth);
        sequence->forceSplit = true;

        const auto childEndsWithSemicolon = [](const SyntaxNode& child) {
            for (auto iterator = child.children.rbegin(); iterator != child.children.rend(); ++iterator) {
                const SyntaxNode* tail = *iterator;
                if (
                    tail == nullptr ||
                    tail->kind == SyntaxNodeKind::Comment ||
                    tail->kind == SyntaxNodeKind::TrailingComment ||
                    tail->kind == SyntaxNodeKind::BlankLine
                ) {
                    continue;
                }
                return tail->kind == SyntaxNodeKind::Semicolon;
            }
            return false;
        };

        ConstSyntaxChildList itemChildren;
        for (const SyntaxNode* child : node.children) {
            if (child == nullptr || !ContainsSelected(*child)) {
                continue;
            }
            const std::optional<FormatBreakToken> token = TokenForNode(*child);
            if (
                token &&
                FormatBreakTokenKind(*token) == PrintTokenKind::Known &&
                FormatBreakTokenSyntaxKind(*token) == SyntaxNodeKind::Semicolon
            ) {
                if (!itemChildren.empty()) {
                    AppendListItem(*sequence, BuildSequenceFromPointers(itemChildren, depth + 1), false);
                    itemChildren.clear();
                }
                AttachSeparatorToPreviousItem(*sequence, *token);
                continue;
            }
            itemChildren.push_back(child);
            if (childEndsWithSemicolon(*child)) {
                AppendListItem(*sequence, BuildSequenceFromPointers(itemChildren, depth + 1), false);
                itemChildren.clear();
            }
        }
        if (!itemChildren.empty()) {
            AppendListItem(*sequence, BuildSequenceFromPointers(itemChildren, depth + 1), false);
        }
        return sequence->items.empty() ? nullptr : sequence;
    }

    std::optional<std::pair<size_t, FormatBreakDelimiterKind>> FindDirectClose(
        const ::SyntaxChildList& children, size_t openIndex, size_t end, FormatBreakDelimiterKind delimiter
    ) const {
        for (size_t index = openIndex + 1; index < end; ++index) {
            if (!children[index]) {
                continue;
            }
            const std::optional<FormatBreakToken> token = TokenForNode(*children[index]);
            if (token && ClosingDelimiter(*token) == delimiter) {
                return std::pair<size_t, FormatBreakDelimiterKind>{index, delimiter};
            }
        }
        return std::nullopt;
    }

    FormatBreakNode* BuildDirectDelimited(
        const ::SyntaxChildList& children, size_t openIndex, size_t end, int depth, size_t& afterDelimited
    ) {
        afterDelimited = openIndex + 1;
        if (!children[openIndex]) {
            return nullptr;
        }
        const std::optional<FormatBreakToken> open = TokenForNode(*children[openIndex]);
        if (!open) {
            return nullptr;
        }
        const FormatBreakDelimiterKind delimiter = OpeningDelimiter(*open);
        if (delimiter == FormatBreakDelimiterKind::None) {
            return nullptr;
        }
        const std::optional<std::pair<size_t, FormatBreakDelimiterKind>> closeMatch =
            FindDirectClose(children, openIndex, end, delimiter);
        const auto virtualDelimiter = std::find_if(
            context_.virtualDelimiters.begin(),
            context_.virtualDelimiters.end(),
            [&](const FormatBreakVirtualDelimiter& candidate) {
                return candidate.open == FormatBreakTokenValue(*open).node &&
                    ClosingDelimiter(candidate.close) == delimiter;
            }
        );
        const bool hasVirtualClose = !closeMatch && virtualDelimiter != context_.virtualDelimiters.end();
        if (!closeMatch && !hasVirtualClose) {
            return nullptr;
        }
        const size_t closeIndex = closeMatch ? closeMatch->first : end;
        std::optional<FormatBreakToken> closeToken;
        const FormatBreakToken* close = hasVirtualClose ? &virtualDelimiter->close : nullptr;
        if (closeMatch) {
            closeToken = TokenForNode(*children[closeIndex]);
            if (!closeToken) {
                return nullptr;
            }
            close = &*closeToken;
        }
        if (close == nullptr) {
            return nullptr;
        }

        auto delimited = MakeNode(FormatBreakNodeKind::Delimited, depth);
        delimited->delimiterKind = delimiter;
        delimited->suppressCompactDelimiterPadding = delimiter == FormatBreakDelimiterKind::Brace &&
            FormatBreakTokenValue(*open).parentKind == SyntaxNodeKind::InitializerList;
        delimited->children = StoreNodePointers({BuildToken(*open, depth + 1), BuildToken(*close, depth + 1)});

        if (delimiter == FormatBreakDelimiterKind::Paren) {
            if (
                const SyntaxNode* commaExpression = DirectDelimitedCommaExpressionBody(children, openIndex, closeIndex)
            ) {
                if (AppendCommaExpressionListItems(*delimited, *commaExpression, *open, depth, false)) {
                    delimited->forceSplit = delimited->forceSplit || (hasVirtualClose && virtualDelimiter->forceSplit);
                    afterDelimited = hasVirtualClose ? end : closeIndex + 1;
                    return delimited;
                }
                delimited->items.clear();
            }
        }

        ConstSyntaxChildList itemChildren;
        bool pendingBlankLine = false;
        for (size_t index = openIndex + 1; index < closeIndex; ++index) {
            const SyntaxNode* child = children[index];
            if (child == nullptr) {
                continue;
            }
            if (SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::ListForceSplitMarker)) {
                delimited->forceSplit = true;
                if (child->kind == SyntaxNodeKind::BlankLine) {
                    pendingBlankLine = true;
                    continue;
                }
                const std::optional<FormatBreakToken> comment = TokenForNode(*child);
                if (!comment) {
                    continue;
                }
                if (child->kind == SyntaxNodeKind::TrailingComment) {
                    if (!itemChildren.empty()) {
                        const bool blankLineBefore =
                            ShouldPreservePendingBlankLine(*delimited, pendingBlankLine, false);
                        AppendDelimitedItem(*delimited, itemChildren, *open, depth, blankLineBefore);
                    }
                    AttachTrailingCommentToPreviousItem(*delimited, *comment);
                } else {
                    if (!itemChildren.empty()) {
                        const bool blankLineBefore =
                            ShouldPreservePendingBlankLine(*delimited, pendingBlankLine, false);
                        AppendDelimitedItem(*delimited, itemChildren, *open, depth, blankLineBefore);
                        pendingBlankLine = false;
                    }
                    const bool blankLineBefore = ShouldPreservePendingBlankLine(*delimited, pendingBlankLine, true);
                    AppendStandaloneCommentItem(*delimited, *comment, depth, blankLineBefore);
                }
                pendingBlankLine = false;
                continue;
            }
            if (!ContainsSelected(*child)) {
                continue;
            }
            const std::optional<FormatBreakToken> token = TokenForNode(*child);
            if (token && IsSelectedSeparator(*token)) {
                if (
                    itemChildren.empty() &&
                    IsForHeaderDelimiter(*open) &&
                    FormatBreakTokenKind(*token) == PrintTokenKind::Known &&
                    FormatBreakTokenSyntaxKind(*token) == SyntaxNodeKind::Semicolon
                ) {
                    const bool blankLineBefore = ShouldPreservePendingBlankLine(*delimited, pendingBlankLine, false);
                    AppendEmptyDelimitedItem(*delimited, depth, blankLineBefore);
                } else {
                    const bool blankLineBefore = ShouldPreservePendingBlankLine(*delimited, pendingBlankLine, false);
                    AppendDelimitedItem(*delimited, itemChildren, *open, depth, blankLineBefore);
                }
                pendingBlankLine = false;
                AttachSeparatorToPreviousItem(*delimited, *token);
                continue;
            }
            itemChildren.push_back(child);
            if (
                (
                    SyntaxNodeKindHasClass(FormatBreakTokenValue(*open).parentKind, SyntaxNodeClass::ControlHeader) ||
                    SyntaxNodeKindHasClass(FormatBreakTokenValue(*open).grandParentKind, SyntaxNodeClass::ControlHeader)
                ) &&
                itemChildren.size() == 1 &&
                (child->kind == SyntaxNodeKind::Declaration || child->kind == SyntaxNodeKind::InitStatement)
            ) {
                const bool blankLineBefore = ShouldPreservePendingBlankLine(*delimited, pendingBlankLine, false);
                AppendDelimitedItem(*delimited, itemChildren, *open, depth, blankLineBefore);
                pendingBlankLine = false;
            }
        }
        const bool blankLineBefore = ShouldPreservePendingBlankLine(*delimited, pendingBlankLine, false);
        AppendDelimitedItem(*delimited, itemChildren, *open, depth, blankLineBefore);
        if (
            itemChildren.empty() &&
            IsForHeaderDelimiter(*open) &&
            !delimited->items.empty() &&
            FormatBreakTokenKind(delimited->items.back().separator) == PrintTokenKind::Known
        ) {
            AppendEmptyDelimitedItem(*delimited, depth);
        }
        delimited->compactRequiresUnbrokenItems = IsMultiItemDesignatedInitializer(*delimited, *open);
        delimited->forceSplit = delimited->forceSplit || (hasVirtualClose && virtualDelimiter->forceSplit);
        afterDelimited = hasVirtualClose ? end : closeIndex + 1;
        return delimited;
    }
};

}  // namespace

FormatBreakModel BuildFormatBreakModel(std::span<const PrintToken> tokens) { return BuildFormatBreakModel(tokens, {}); }

FormatBreakModel BuildFormatBreakModel(std::span<const PrintToken> tokens, const FormatBreakModelContext& context) {
    return BreakModelBuilder(tokens, context).Build();
}

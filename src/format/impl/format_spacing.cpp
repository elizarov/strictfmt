#include "format/impl/format_spacing.h"

namespace {

bool IsReferenceToken(const PrintToken& token) {
    return token.kind == PrintTokenKind::Known &&
        PrintTokenSyntaxHasClass(token, SyntaxNodeClass::DeclaratorReferenceToken);
}

bool IsKeywordOwnedValueToken(const PrintToken& token) {
    return token.kind == PrintTokenKind::Known && PrintTokenSyntaxHasClass(token, SyntaxNodeClass::KeywordOwnedValue);
}

bool IsConditionDeclarationBindingToken(const PrintToken& token);
bool IsDeclaratorPackEllipsisToken(const PrintToken& token);

bool IsDeclaratorBindingToken(const PrintToken& token) {
    return IsDeclaratorPackEllipsisToken(token) || (IsReferenceToken(token) && (
        SyntaxNodeKindHasClass(token.parentKind, SyntaxNodeClass::DeclaratorReferenceParent) ||
        IsConditionDeclarationBindingToken(token)
    ));
}

bool IsUnaryContext(const PrintToken& token) { return token.parentKind == SyntaxNodeKind::UnaryExpression; }

bool IsBinaryContext(const PrintToken& token) {
    return token.parentKind == SyntaxNodeKind::BinaryExpression ||
        token.parentKind == SyntaxNodeKind::AssignmentExpression ||
        token.parentKind == SyntaxNodeKind::ConditionalExpression;
}

bool IsAlternativeBinaryOperatorToken(const PrintToken& token) {
    if (token.kind != PrintTokenKind::Text || !IsBinaryContext(token)) {
        return false;
    }
    return token.text == "and" ||
        token.text == "and_eq" ||
        token.text == "bitand" ||
        token.text == "bitor" ||
        token.text == "not_eq" ||
        token.text == "or" ||
        token.text == "or_eq" ||
        token.text == "xor" ||
        token.text == "xor_eq";
}

bool IsTriviaNode(const SyntaxNode* node) {
    return node == nullptr || SyntaxNodeKindHasClass(node->kind, SyntaxNodeClass::Trivia);
}

bool NodeOrAncestorHasClass(const SyntaxNode* node, SyntaxNodeClass syntaxNodeClass) {
    for (; node != nullptr; node = node->parent) {
        if (
            (node->classes & static_cast<std::uint64_t>(syntaxNodeClass)) != 0 ||
            SyntaxNodeKindHasClass(node->kind, syntaxNodeClass)
        ) {
            return true;
        }
    }
    return false;
}

const SyntaxNode* PreviousNonTriviaChild(const SyntaxNode& node, size_t before) {
    while (before > 0) {
        --before;
        if (!IsTriviaNode(node.children[before])) {
            return node.children[before];
        }
    }
    return nullptr;
}

const SyntaxNode* NextNonTriviaChild(const SyntaxNode& node, size_t after) {
    for (size_t index = after; index < node.children.size(); ++index) {
        if (!IsTriviaNode(node.children[index])) {
            return node.children[index];
        }
    }
    return nullptr;
}

bool IsDeclaratorPackEllipsisToken(const PrintToken& token) {
    if (
        token.kind != PrintTokenKind::Known ||
        token.syntaxKind != SyntaxNodeKind::Ellipsis ||
        token.node == nullptr ||
        token.node->parent == nullptr ||
        token.node->parent->parent == nullptr ||
        !SyntaxNodeKindHasClass(token.node->parent->parent->kind, SyntaxNodeClass::DeclaratorReferenceParent)
    ) {
        return false;
    }
    const SyntaxNode& pack = *token.node->parent;
    for (size_t index = 0; index < pack.children.size(); ++index) {
        if (pack.children[index] != token.node) {
            continue;
        }
        const SyntaxNode* identifier = NextNonTriviaChild(pack, index + 1);
        return identifier != nullptr && identifier->kind == SyntaxNodeKind::Identifier;
    }
    return false;
}

const SyntaxNode* SingleNonEllipsisChild(const SyntaxNode& node) {
    const SyntaxNode* result = nullptr;
    for (const SyntaxNode* child : node.children) {
        if (IsTriviaNode(child) || child->kind == SyntaxNodeKind::Ellipsis) {
            continue;
        }
        if (result != nullptr) {
            return nullptr;
        }
        result = child;
    }
    return result;
}

bool HasDirectTokenChild(const SyntaxNode& node, SyntaxNodeKind kind) {
    for (const SyntaxNode* child : node.children) {
        if (child != nullptr && child->kind == kind) {
            return true;
        }
    }
    return false;
}

size_t DirectTokenChildIndex(const SyntaxNode& node, const SyntaxNode* token) {
    for (size_t index = 0; index < node.children.size(); ++index) {
        if (node.children[index] == token) {
            return index;
        }
    }
    return node.children.size();
}

size_t DirectTokenKindIndex(const SyntaxNode& node, SyntaxNodeKind kind) {
    for (size_t index = 0; index < node.children.size(); ++index) {
        if (node.children[index] != nullptr && node.children[index]->kind == kind) {
            return index;
        }
    }
    return node.children.size();
}

bool IsConditionDeclarationBindingToken(const PrintToken& token) {
    if (!IsReferenceToken(token) || token.node == nullptr) {
        return false;
    }
    const SyntaxNode* expression = token.node->parent;
    if (
        expression == nullptr ||
        expression->kind != SyntaxNodeKind::BinaryExpression ||
        expression->parent == nullptr ||
        expression->parent->kind != SyntaxNodeKind::ConditionClause
    ) {
        return false;
    }
    const size_t tokenIndex = DirectTokenChildIndex(*expression, token.node);
    const SyntaxNode* declaratorAssignment = NextNonTriviaChild(*expression, tokenIndex + 1);
    return declaratorAssignment != nullptr && declaratorAssignment->kind == SyntaxNodeKind::AssignmentExpression;
}

bool HasCallableTemplateLessShape(const SyntaxNode& node) {
    if (node.kind == SyntaxNodeKind::BinaryExpression) {
        return DirectTokenKindIndex(node, SyntaxNodeKind::Less) < node.children.size();
    }
    if (node.kind != SyntaxNodeKind::Tree) {
        return false;
    }
    const SyntaxNode* child = SingleNonEllipsisChild(node);
    return child != nullptr && HasCallableTemplateLessShape(*child);
}

bool HasCallArgumentGroupShape(const SyntaxNode& node) {
    return
        HasDirectTokenChild(node, SyntaxNodeKind::LeftParen) && HasDirectTokenChild(node, SyntaxNodeKind::RightParen);
}

const SyntaxNode* LeadingCallArgumentGroup(const SyntaxNode& node) {
    if (HasCallArgumentGroupShape(node)) {
        return &node;
    }
    const SyntaxNode* first = NextNonTriviaChild(node, 0);
    return first == nullptr ? nullptr : LeadingCallArgumentGroup(*first);
}

const SyntaxNode* CallableTemplateCallArgumentGroup(const SyntaxNode& node, const SyntaxNode* greaterToken = nullptr) {
    if (node.kind != SyntaxNodeKind::BinaryExpression) {
        return nullptr;
    }
    const size_t greaterIndex = greaterToken == nullptr ? DirectTokenKindIndex(node, SyntaxNodeKind::Greater) :
        DirectTokenChildIndex(node, greaterToken);
    if (greaterIndex >= node.children.size()) {
        return nullptr;
    }
    const SyntaxNode* left = PreviousNonTriviaChild(node, greaterIndex);
    const SyntaxNode* right = NextNonTriviaChild(node, greaterIndex + 1);
    return left != nullptr && right != nullptr && HasCallableTemplateLessShape(*left) ?
        LeadingCallArgumentGroup(*right) : nullptr;
}

bool HasCallableTemplateGreaterShape(const SyntaxNode& node, const SyntaxNode* greaterToken = nullptr) {
    return CallableTemplateCallArgumentGroup(node, greaterToken) != nullptr;
}

bool HasConditionDeclarationTemplateGreaterShape(const SyntaxNode& node, const SyntaxNode* greaterToken = nullptr) {
    if (node.kind != SyntaxNodeKind::BinaryExpression || node.parent == nullptr) {
        return false;
    }
    const size_t greaterIndex = greaterToken == nullptr ? DirectTokenKindIndex(node, SyntaxNodeKind::Greater) :
        DirectTokenChildIndex(node, greaterToken);
    if (greaterIndex >= node.children.size()) {
        return false;
    }
    const SyntaxNode* left = PreviousNonTriviaChild(node, greaterIndex);
    const SyntaxNode* right = NextNonTriviaChild(node, greaterIndex + 1);
    return node.parent->kind == SyntaxNodeKind::ConditionClause &&
        left != nullptr &&
        right != nullptr &&
        HasCallableTemplateLessShape(*left) &&
        right->kind == SyntaxNodeKind::AssignmentExpression;
}

bool IsCallableTemplateLessToken(const PrintToken& token) {
    if (token.node == nullptr || token.syntaxKind != SyntaxNodeKind::Less) {
        return false;
    }
    const SyntaxNode* lessExpression = token.node->parent;
    if (lessExpression == nullptr || lessExpression->kind != SyntaxNodeKind::BinaryExpression) {
        return false;
    }
    const SyntaxNode* expression = lessExpression;
    const SyntaxNode* parent = expression->parent;
    while (parent != nullptr && parent->kind == SyntaxNodeKind::Tree && HasCallableTemplateLessShape(*parent)) {
        expression = parent;
        parent = parent->parent;
    }
    return parent != nullptr && HasCallableTemplateGreaterShape(*parent);
}

bool IsCallableTemplateGreaterToken(const PrintToken& token) {
    if (token.node == nullptr || token.syntaxKind != SyntaxNodeKind::Greater) {
        return false;
    }
    const SyntaxNode* expression = token.node->parent;
    return expression != nullptr && HasCallableTemplateGreaterShape(*expression, token.node);
}

bool IsConditionDeclarationTemplateLessToken(const PrintToken& token) {
    if (token.node == nullptr || token.syntaxKind != SyntaxNodeKind::Less) {
        return false;
    }
    const SyntaxNode* lessExpression = token.node->parent;
    if (lessExpression == nullptr || lessExpression->kind != SyntaxNodeKind::BinaryExpression) {
        return false;
    }
    const SyntaxNode* expression = lessExpression;
    const SyntaxNode* parent = expression->parent;
    while (parent != nullptr && parent->kind == SyntaxNodeKind::Tree && HasCallableTemplateLessShape(*parent)) {
        expression = parent;
        parent = parent->parent;
    }
    return parent != nullptr && HasConditionDeclarationTemplateGreaterShape(*parent);
}

bool IsConditionDeclarationTemplateGreaterToken(const PrintToken& token) {
    if (token.node == nullptr || token.syntaxKind != SyntaxNodeKind::Greater) {
        return false;
    }
    const SyntaxNode* expression = token.node->parent;
    return expression != nullptr && HasConditionDeclarationTemplateGreaterShape(*expression, token.node);
}

bool IsCallableTemplateCallOpenToken(const PrintToken& token) {
    if (token.node == nullptr || token.syntaxKind != SyntaxNodeKind::LeftParen) {
        return false;
    }
    const SyntaxNode* argumentGroup = token.node->parent;
    if (argumentGroup == nullptr) {
        return false;
    }
    for (const SyntaxNode* expression = argumentGroup->parent; expression != nullptr; expression = expression->parent) {
        if (CallableTemplateCallArgumentGroup(*expression) == argumentGroup) {
            return true;
        }
        if (LeadingCallArgumentGroup(*expression) != argumentGroup) {
            return false;
        }
    }
    return false;
}

bool IsTemplateDelimiterContext(const PrintToken& token) { return token.inTemplateList; }

bool IsOperatorSpellingContext(const PrintToken& token) {
    return token.parentKind == SyntaxNodeKind::OperatorName ||
        token.parentKind == SyntaxNodeKind::OperatorCast ||
        token.grandParentKind == SyntaxNodeKind::OperatorName ||
        token.grandParentKind == SyntaxNodeKind::OperatorCast;
}

bool IsLeadingGlobalScopeToken(const PrintToken& token) {
    if (token.syntaxKind != SyntaxNodeKind::ColonColon || token.node == nullptr || token.node->parent == nullptr) {
        return false;
    }
    return
        PreviousNonTriviaChild(*token.node->parent, DirectTokenChildIndex(*token.node->parent, token.node)) == nullptr;
}

bool IsBinaryOperatorSpacingContext(const PrintToken& token) {
    if (IsAlternativeBinaryOperatorToken(token)) {
        return true;
    }
    if (
        token.kind != PrintTokenKind::Known ||
        !PrintTokenSyntaxHasClass(token, SyntaxNodeClass::BinaryOperator) ||
        IsConditionDeclarationBindingToken(token) ||
        IsUnaryContext(token) || (
            (token.syntaxKind == SyntaxNodeKind::Less || token.syntaxKind == SyntaxNodeKind::Greater) &&
            IsTemplateAnglePrintToken(token)
        ) ||
        IsTemplateDelimiterContext(token) ||
        IsOperatorSpellingContext(token)
    ) {
        return false;
    }
    if (
        token.syntaxKind == SyntaxNodeKind::Less ||
        token.syntaxKind == SyntaxNodeKind::Greater ||
        token.syntaxKind == SyntaxNodeKind::Star ||
        token.syntaxKind == SyntaxNodeKind::Ampersand
    ) {
        return IsBinaryContext(token);
    }
    return true;
}

bool IsUserDefinedLiteralSuffix(const PrintToken& previous, const PrintToken& current) {
    return previous.parentKind == SyntaxNodeKind::UserDefinedLiteral &&
        current.parentKind == SyntaxNodeKind::UserDefinedLiteral &&
        previous.node != nullptr &&
        current.node != nullptr &&
        previous.node->parent == current.node->parent &&
        PrintTokenSyntaxHasClass(previous, SyntaxNodeClass::Literal) &&
        current.kind == PrintTokenKind::Text &&
        !current.text.empty() && (
            (current.text.front() >= 'A' && current.text.front() <= 'Z') ||
            (current.text.front() >= 'a' && current.text.front() <= 'z') ||
            current.text.front() == '_'
        );
}

bool IsWordBoundaryChar(char ch) {
    return (ch >= 'A' && ch <= 'Z') ||
        (ch >= 'a' && ch <= 'z') ||
        (ch >= '0' && ch <= '9') ||
        ch == '_' ||
        static_cast<unsigned char>(ch) >= 0x80;
}

bool StartsWithWordBoundary(const PrintToken& token) {
    if (token.kind == PrintTokenKind::Text) {
        return !token.text.empty() && IsWordBoundaryChar(token.text.front());
    }
    return token.kind == PrintTokenKind::Known && PrintTokenSyntaxHasClass(token, SyntaxNodeClass::Keyword);
}

bool KeywordOperatorNeedsSpaceAfter(const PrintToken& previous, const PrintToken& current) {
    if (previous.syntaxKind != SyntaxNodeKind::KeywordOperator) {
        return false;
    }
    return previous.parentKind == SyntaxNodeKind::OperatorCast ||
        current.parentKind == SyntaxNodeKind::OperatorCast ||
        current.syntaxKind == SyntaxNodeKind::ColonColon ||
        StartsWithWordBoundary(current);
}

const SyntaxNode* ParentNode(const PrintToken& token) { return token.node != nullptr ? token.node->parent : nullptr; }

const SyntaxNode* GrandParentNode(const PrintToken& token) {
    const SyntaxNode* parent = ParentNode(token);
    return parent != nullptr ? parent->parent : nullptr;
}

bool IsCompactEmptyBraceToken(const PrintToken& token) {
    return token.kind == PrintTokenKind::Text && token.text == "{}";
}

bool IsCompactSingleStatementBodyBrace(const PrintToken& token, SyntaxNodeKind kind) {
    return token.syntaxKind == kind &&
        token.inCompactSingleStatementBody &&
        token.parentKind == SyntaxNodeKind::CompoundStatement && (
            token.grandParentKind == SyntaxNodeKind::FunctionDefinition ||
            token.grandParentKind == SyntaxNodeKind::LambdaExpression
        );
}

bool IsAttributeCloseToken(const PrintToken& token) {
    return token.kind == PrintTokenKind::Text && token.text == "]]" && (
        token.parentKind == SyntaxNodeKind::AttributeSpecifier ||
        token.parentKind == SyntaxNodeKind::AttributeDeclaration
    );
}

bool IsAttributeOpenToken(const PrintToken& token) {
    return token.kind == PrintTokenKind::Text && token.text == "[[" && (
        token.parentKind == SyntaxNodeKind::AttributeSpecifier ||
        token.parentKind == SyntaxNodeKind::AttributeDeclaration
    );
}

bool IsFunctionSuffixMacro(const PrintToken& token) { return token.syntaxKind == SyntaxNodeKind::FunctionSuffixMacro; }

bool IsBlockCommentToken(const PrintToken& token) {
    return (token.kind == PrintTokenKind::Text || IsCommentToken(token.kind)) && token.text.starts_with("/*");
}

const SyntaxNode* LastNonTriviaLeaf(const SyntaxNode& node) {
    if (node.children.empty()) {
        return IsTriviaNode(&node) ? nullptr : &node;
    }
    for (size_t index = node.children.size(); index > 0; --index) {
        const SyntaxNode* child = node.children[index - 1];
        if (IsTriviaNode(child)) {
            continue;
        }
        const SyntaxNode* leaf = LastNonTriviaLeaf(*child);
        if (leaf != nullptr) {
            return leaf;
        }
    }
    return nullptr;
}

const SyntaxNode* PreviousNonTriviaLeaf(const SyntaxNode& node) {
    const SyntaxNode* branch = &node;
    for (const SyntaxNode* parent = node.parent; parent != nullptr; branch = parent, parent = parent->parent) {
        const size_t branchIndex = DirectTokenChildIndex(*parent, branch);
        const SyntaxNode* previous = PreviousNonTriviaChild(*parent, branchIndex);
        if (previous != nullptr) {
            return LastNonTriviaLeaf(*previous);
        }
    }
    return nullptr;
}

bool IsMemberPointerDeclaratorStar(const PrintToken& token) {
    if (token.syntaxKind != SyntaxNodeKind::Star || token.node == nullptr) {
        return false;
    }
    const SyntaxNode* previous = PreviousNonTriviaLeaf(*token.node);
    return previous != nullptr && previous->kind == SyntaxNodeKind::ColonColon;
}

bool IsTemplateArgumentExpressionOperator(const PrintToken& token) {
    if (token.templateArgumentExpressionOperator != 0) {
        return token.templateArgumentExpressionOperator == 2;
    }
    const bool result = token.kind == PrintTokenKind::Known &&
        IsTemplateDelimiterContext(token) &&
        PrintTokenSyntaxHasClass(token, SyntaxNodeClass::BinaryOperator) &&
        !IsDeclaratorBindingToken(token) &&
        !IsMemberPointerDeclaratorStar(token) &&
        !IsUnaryContext(token) &&
        !IsOperatorSpellingContext(token) &&
        token.syntaxKind != SyntaxNodeKind::Less &&
        token.syntaxKind != SyntaxNodeKind::Greater;
    token.templateArgumentExpressionOperator = result ? 2 : 1;
    return result;
}

bool HasCallModifierBeforeDeclaratorBinding(const PrintToken& token) {
    const SyntaxNode* declarator = ParentNode(token);
    const SyntaxNode* parenthesized = GrandParentNode(token);
    if (
        declarator == nullptr ||
        parenthesized == nullptr ||
        !SyntaxNodeKindHasClass(declarator->kind, SyntaxNodeClass::DeclaratorReferenceParent) ||
        !SyntaxNodeKindHasClass(parenthesized->kind, SyntaxNodeClass::ParenthesizedDeclarator)
    ) {
        return false;
    }
    for (const SyntaxNode* child : parenthesized->children) {
        if (child == declarator) {
            return false;
        }
        if (child != nullptr && child->kind == SyntaxNodeKind::MsCallModifier) {
            return true;
        }
    }
    return false;
}

bool IsFunctionPointerDeclaratorGroupOpen(const PrintToken& token) {
    if (token.kind != PrintTokenKind::Known || token.syntaxKind != SyntaxNodeKind::LeftParen || token.node == nullptr) {
        return false;
    }
    const SyntaxNode* parent = ParentNode(token);
    if (parent == nullptr) {
        return false;
    }

    bool inGroup = false;
    bool hasPointerMarker = false;
    bool sawClose = false;
    for (const SyntaxNode* child : parent->children) {
        if (child == nullptr) {
            continue;
        }
        if (child == token.node) {
            inGroup = true;
            continue;
        }
        if (!inGroup) {
            continue;
        }
        if (sawClose) {
            return child->kind == SyntaxNodeKind::ParameterList && hasPointerMarker;
        }
        if (child->kind == SyntaxNodeKind::RightParen) {
            sawClose = true;
            continue;
        }
        hasPointerMarker = hasPointerMarker ||
            child->kind == SyntaxNodeKind::Star ||
            (child->kind == SyntaxNodeKind::LexicalToken && child->text.find('*') != std::string_view::npos);
    }
    return false;
}

}  // namespace

bool IsPreprocessorPrintToken(PrintTokenKind kind) {
    return kind == PrintTokenKind::Preprocessor || kind == PrintTokenKind::IncludeRun;
}

bool IsPreprocessorLikeToken(const PrintToken& token) {
    return IsPreprocessorPrintToken(token.kind) || token.macroDefinition != nullptr;
}

bool IsCommentToken(PrintTokenKind kind) {
    return kind == PrintTokenKind::Comment || kind == PrintTokenKind::TrailingComment;
}

bool IsLineCommentToken(const PrintToken& token) { return IsCommentToken(token.kind) && token.text.starts_with("//"); }

bool IsWordLike(const PrintToken& token) {
    if (token.kind == PrintTokenKind::Text) {
        return !token.text.empty() && (IsWordBoundaryChar(token.text.front()) || IsWordBoundaryChar(token.text.back()));
    }
    return token.kind == PrintTokenKind::Known && PrintTokenSyntaxHasClass(token, SyntaxNodeClass::Keyword);
}

bool IsStringLike(const PrintToken& token) { return token.stringLike; }

bool IsAccessKeyword(const PrintToken& token) {
    return token.kind == PrintTokenKind::Known && PrintTokenSyntaxHasClass(token, SyntaxNodeClass::AccessKeyword);
}

bool IsCaseLabelKeyword(const PrintToken& token) {
    return token.kind == PrintTokenKind::Known &&
        token.parentKind == SyntaxNodeKind::CaseStatement &&
        (token.syntaxKind == SyntaxNodeKind::KeywordCase || token.syntaxKind == SyntaxNodeKind::KeywordDefault);
}

bool FormatTokensShareMacroDefinition(const PrintToken* left, const PrintToken* right) {
    return left != nullptr &&
        right != nullptr &&
        left->macroDefinition != nullptr &&
        left->macroDefinition == right->macroDefinition;
}

bool IsTemplateAnglePrintToken(const PrintToken& token) {
    if (
        token.kind != PrintTokenKind::Known ||
        (token.syntaxKind != SyntaxNodeKind::Less && token.syntaxKind != SyntaxNodeKind::Greater)
    ) {
        return false;
    }
    return (IsTemplateDelimiterContext(token) && !IsBinaryContext(token) && !IsOperatorSpellingContext(token)) || (
            token.syntaxKind == SyntaxNodeKind::Less ?
                IsCallableTemplateLessToken(token) : IsCallableTemplateGreaterToken(token)
        ) ||
        (
            token.syntaxKind == SyntaxNodeKind::Less ?
                IsConditionDeclarationTemplateLessToken(token) : IsConditionDeclarationTemplateGreaterToken(token)
        );
}

bool FormatTokenNeedsSpace(const PrintToken* previous, const PrintToken& current) {
    if (previous == nullptr) {
        return false;
    }
    if (IsBlockCommentToken(current)) {
        if (current.kind == PrintTokenKind::TrailingComment) {
            return true;
        }
        return !IsAttributeOpenToken(*previous) && !(
            previous->kind == PrintTokenKind::Known &&
            PrintTokenSyntaxHasClass(*previous, SyntaxNodeClass::OpeningDelimiter) &&
            (previous->syntaxKind != SyntaxNodeKind::Less || IsTemplateAnglePrintToken(*previous))
        );
    }
    if (IsBlockCommentToken(*previous)) {
        return !IsAttributeCloseToken(current) && !(current.kind == PrintTokenKind::Known && (
            current.syntaxKind == SyntaxNodeKind::RightParen ||
            current.syntaxKind == SyntaxNodeKind::RightBracket ||
            current.syntaxKind == SyntaxNodeKind::RightBrace ||
            current.syntaxKind == SyntaxNodeKind::Comma ||
            current.syntaxKind == SyntaxNodeKind::Semicolon ||
            (current.syntaxKind == SyntaxNodeKind::Greater && IsTemplateAnglePrintToken(current))
        ));
    }
    if (
        (IsPreprocessorLikeToken(*previous) && previous->macroDefinition == nullptr) ||
        (IsPreprocessorLikeToken(current) && current.macroDefinition == nullptr)
    ) {
        return false;
    }
    if (current.inMacroValue && !previous->inMacroValue && FormatTokensShareMacroDefinition(previous, &current)) {
        return true;
    }
    if (current.kind == PrintTokenKind::Text && current.text == "{}") {
        if (SyntaxNodeKindHasClass(current.parentKind, SyntaxNodeClass::CompoundBlock)) {
            return true;
        }
        return previous->kind == PrintTokenKind::Known && (
            PrintTokenSyntaxHasClass(*previous, SyntaxNodeClass::AssignmentOperator) ||
            previous->syntaxKind == SyntaxNodeKind::Comma ||
            IsKeywordOwnedValueToken(*previous) ||
            previous->syntaxKind == SyntaxNodeKind::Colon ||
            previous->syntaxKind == SyntaxNodeKind::Question
        );
    }
    if (IsStringLike(*previous) && IsStringLike(current)) {
        return true;
    }
    if (IsUserDefinedLiteralSuffix(*previous, current)) {
        return false;
    }
    if (current.kind == PrintTokenKind::Text && !current.text.empty() && current.text.front() == '=') {
        return true;
    }
    if (IsAttributeCloseToken(*previous)) {
        return current.kind != PrintTokenKind::Known || current.syntaxKind != SyntaxNodeKind::Semicolon;
    }
    if (IsAttributeOpenToken(current)) {
        if (IsWordLike(*previous)) {
            return true;
        }
        if (previous->kind == PrintTokenKind::Known && (
            previous->syntaxKind == SyntaxNodeKind::RightParen ||
            previous->syntaxKind == SyntaxNodeKind::RightBracket ||
            previous->syntaxKind == SyntaxNodeKind::RightBrace ||
            previous->syntaxKind == SyntaxNodeKind::Greater
        )) {
            return true;
        }
    }
    if ((IsStringLike(*previous) && IsWordLike(current)) || (IsWordLike(*previous) && IsStringLike(current))) {
        return true;
    }
    if (current.parentKind == SyntaxNodeKind::RefQualifier) {
        return true;
    }
    if (IsFunctionSuffixMacro(current)) {
        return true;
    }
    if (IsCompactEmptyBraceToken(*previous) && IsWordLike(current)) {
        return true;
    }
    if (IsCompactEmptyBraceToken(*previous) && current.kind == PrintTokenKind::Known) {
        if (
            PrintTokenSyntaxHasClass(current, SyntaxNodeClass::AssignmentOperator) ||
            IsBinaryOperatorSpacingContext(current)
        ) {
            return true;
        }
        if (
            current.parentKind == SyntaxNodeKind::ConditionalExpression &&
            (current.syntaxKind == SyntaxNodeKind::Question || current.syntaxKind == SyntaxNodeKind::Colon)
        ) {
            return true;
        }
        return PrintTokenSyntaxHasClass(current, SyntaxNodeClass::AttachAfterBlockKeyword);
    }
    if (previous->kind != PrintTokenKind::Known && current.kind != PrintTokenKind::Known) {
        return IsWordLike(*previous) && IsWordLike(current);
    }
    const SyntaxNodeKind prev =
        previous->kind == PrintTokenKind::Known ? previous->syntaxKind : SyntaxNodeKind::Unknown;
    const SyntaxNodeKind cur = current.kind == PrintTokenKind::Known ? current.syntaxKind : SyntaxNodeKind::Unknown;

    if (IsKeywordOwnedValueToken(*previous) && cur != SyntaxNodeKind::Semicolon) {
        return true;
    }

    if (
        cur == SyntaxNodeKind::LeftBrace &&
        NodeOrAncestorHasClass(current.node, SyntaxNodeClass::ConditionalFunctionHeader)
    ) {
        return true;
    }

    if (
        prev == SyntaxNodeKind::Comma &&
        cur == SyntaxNodeKind::RightParen &&
        ParentNode(current) != nullptr &&
        (ParentNode(current)->classes & static_cast<std::uint64_t>(SyntaxNodeClass::PreserveTrailingComma)) != 0
    ) {
        return true;
    }

    if (SyntaxNodeKindHasClass(prev, SyntaxNodeClass::PreprocessorDirective)) {
        return true;
    }
    if (IsTemplateArgumentExpressionOperator(*previous) || IsTemplateArgumentExpressionOperator(current)) {
        return true;
    }
    if (
        (cur == SyntaxNodeKind::Arrow && current.parentKind == SyntaxNodeKind::TrailingReturnType) ||
        (prev == SyntaxNodeKind::Arrow && previous->parentKind == SyntaxNodeKind::TrailingReturnType)
    ) {
        return true;
    }
    if (
        cur == SyntaxNodeKind::Dot &&
        current.parentKind == SyntaxNodeKind::FieldDesignator &&
        prev == SyntaxNodeKind::Comma
    ) {
        return true;
    }
    if (IsCompactSingleStatementBodyBrace(current, SyntaxNodeKind::RightBrace)) {
        return true;
    }
    if (
        cur == SyntaxNodeKind::RightParen ||
        cur == SyntaxNodeKind::RightBracket ||
        cur == SyntaxNodeKind::Comma ||
        cur == SyntaxNodeKind::Semicolon ||
        cur == SyntaxNodeKind::ColonColon ||
        cur == SyntaxNodeKind::Dot ||
        cur == SyntaxNodeKind::Arrow ||
        cur == SyntaxNodeKind::DotStar ||
        cur == SyntaxNodeKind::ArrowStar
    ) {
        if (
            cur == SyntaxNodeKind::ColonColon && IsCompactSingleStatementBodyBrace(*previous, SyntaxNodeKind::LeftBrace)
        ) {
            return true;
        }
        if (cur == SyntaxNodeKind::ColonColon && IsLeadingGlobalScopeToken(current) && (
            prev == SyntaxNodeKind::Comma ||
            prev == SyntaxNodeKind::Semicolon ||
            prev == SyntaxNodeKind::Question ||
            prev == SyntaxNodeKind::Colon ||
            SyntaxNodeKindHasClass(prev, SyntaxNodeClass::AssignmentOperator) ||
            IsBinaryOperatorSpacingContext(*previous)
        )) {
            return true;
        }
        if (cur == SyntaxNodeKind::ColonColon && (
            SyntaxNodeKindHasClass(prev, SyntaxNodeClass::Keyword) ||
            SyntaxNodeKindHasClass(prev, SyntaxNodeClass::AssignmentOperator)
        )) {
            return true;
        }
        return false;
    }
    if (previous->parentKind == SyntaxNodeKind::RefQualifier) {
        return true;
    }
    if (IsCompactSingleStatementBodyBrace(*previous, SyntaxNodeKind::LeftBrace)) {
        return true;
    }
    if (
        prev == SyntaxNodeKind::LeftParen ||
        prev == SyntaxNodeKind::LeftBracket ||
        prev == SyntaxNodeKind::ColonColon ||
        prev == SyntaxNodeKind::Dot ||
        prev == SyntaxNodeKind::Arrow ||
        prev == SyntaxNodeKind::DotStar ||
        prev == SyntaxNodeKind::ArrowStar ||
        prev == SyntaxNodeKind::Tilde
    ) {
        return false;
    }
    if (prev == SyntaxNodeKind::KeywordOperator && cur != SyntaxNodeKind::LeftParen) {
        return KeywordOperatorNeedsSpaceAfter(*previous, current);
    }
    if (prev == SyntaxNodeKind::KeywordVirtual && cur == SyntaxNodeKind::Tilde) {
        return true;
    }
    if (prev == SyntaxNodeKind::KeywordCase && current.parentKind == SyntaxNodeKind::CaseStatement) {
        return true;
    }
    if (prev == SyntaxNodeKind::KeywordRequires && previous->parentKind == SyntaxNodeKind::NestedRequirement) {
        return true;
    }
    if (IsAccessKeyword(*previous) && IsWordLike(current)) {
        return true;
    }
    if (prev == SyntaxNodeKind::Colon) {
        if (previous->parentKind == SyntaxNodeKind::SpliceSpecifier) {
            return false;
        }
        return current.parentKind != SyntaxNodeKind::CaseStatement &&
            !PrintTokenSyntaxHasClass(*previous, SyntaxNodeClass::AccessKeyword);
    }
    if (cur == SyntaxNodeKind::LeftParen) {
        if (
            previous->kind == PrintTokenKind::Known &&
            IsCallableTemplateCallOpenToken(current) &&
            IsTemplateAnglePrintToken(*previous)
        ) {
            return false;
        }
        if (
            current.parentKind == SyntaxNodeKind::MsCallModifier ||
            current.grandParentKind == SyntaxNodeKind::MsCallModifier
        ) {
            return false;
        }
        if (SyntaxNodeKindHasClass(current.parentKind, SyntaxNodeClass::ParenthesizedDeclarator)) {
            return true;
        }
        if (IsFunctionPointerDeclaratorGroupOpen(current)) {
            return true;
        }
        if (current.parentKind == SyntaxNodeKind::ForStatement) {
            return true;
        }
        if (
            previous->parentKind == SyntaxNodeKind::OperatorName || previous->parentKind == SyntaxNodeKind::OperatorCast
        ) {
            return false;
        }
        if (prev == SyntaxNodeKind::KeywordConstexpr && (
            previous->parentKind == SyntaxNodeKind::IfStatement ||
            current.parentKind == SyntaxNodeKind::ConditionClause ||
            current.grandParentKind == SyntaxNodeKind::IfStatement
        )) {
            return true;
        }
        if (IsBinaryOperatorSpacingContext(*previous) || (previous->kind == PrintTokenKind::Known && (
            SyntaxNodeKindHasClass(prev, SyntaxNodeClass::AssignmentOperator) ||
            prev == SyntaxNodeKind::Comma ||
            prev == SyntaxNodeKind::Semicolon ||
            prev == SyntaxNodeKind::Question
        ))) {
            return true;
        }
        return previous->kind == PrintTokenKind::Known && SyntaxNodeKindHasClass(prev, SyntaxNodeClass::ControlKeyword);
    }
    if (cur == SyntaxNodeKind::LeftBracket) {
        if (current.parentKind == SyntaxNodeKind::SpliceSpecifier) {
            return previous->kind == PrintTokenKind::Known && (
                SyntaxNodeKindHasClass(prev, SyntaxNodeClass::AssignmentOperator) ||
                prev == SyntaxNodeKind::Comma ||
                prev == SyntaxNodeKind::KeywordTypename ||
                prev == SyntaxNodeKind::Question
            );
        }
        if (current.parentKind == SyntaxNodeKind::StructuredBindingDeclarator) {
            return true;
        }
        if (current.parentKind == SyntaxNodeKind::LambdaCaptureSpecifier) {
            return previous->kind == PrintTokenKind::Known && (
                SyntaxNodeKindHasClass(prev, SyntaxNodeClass::AssignmentOperator) ||
                prev == SyntaxNodeKind::Comma ||
                prev == SyntaxNodeKind::Question
            );
        }
        return false;
    }
    if (cur == SyntaxNodeKind::LeftBrace) {
        if (current.parentKind == SyntaxNodeKind::RequirementSeq) {
            return true;
        }
        if (current.parentKind == SyntaxNodeKind::InitializerList) {
            return previous->kind == PrintTokenKind::Known && (
                SyntaxNodeKindHasClass(prev, SyntaxNodeClass::AssignmentOperator) ||
                prev == SyntaxNodeKind::Comma ||
                prev == SyntaxNodeKind::Question
            );
        }
        if (
            current.parentKind == SyntaxNodeKind::CompoundStatement ||
            current.parentKind == SyntaxNodeKind::FieldDeclarationList ||
            current.parentKind == SyntaxNodeKind::DeclarationList ||
            current.parentKind == SyntaxNodeKind::EnumeratorList
        ) {
            return true;
        }
        return false;
    }
    if (
        prev == SyntaxNodeKind::Semicolon &&
        cur == SyntaxNodeKind::Semicolon &&
        previous->parentKind == SyntaxNodeKind::ForStatement &&
        current.parentKind == SyntaxNodeKind::ForStatement
    ) {
        return false;
    }
    if (prev == SyntaxNodeKind::Comma || prev == SyntaxNodeKind::Semicolon || prev == SyntaxNodeKind::Question) {
        return true;
    }
    if (prev == SyntaxNodeKind::Ellipsis && IsWordLike(current)) {
        return true;
    }
    if (cur == SyntaxNodeKind::Question) {
        return true;
    }
    if (cur == SyntaxNodeKind::Less && prev == SyntaxNodeKind::KeywordTemplate) {
        return true;
    }
    if (cur == SyntaxNodeKind::Colon) {
        if (
            previous->syntaxKind == SyntaxNodeKind::KeywordDefault ||
            PrintTokenSyntaxHasClass(*previous, SyntaxNodeClass::AccessKeyword) ||
            current.parentKind == SyntaxNodeKind::LabeledStatement
        ) {
            return false;
        }
        return current.parentKind != SyntaxNodeKind::CaseStatement;
    }
    if (IsDeclaratorBindingToken(current)) {
        return false;
    }
    if (IsDeclaratorBindingToken(*previous)) {
        if (cur == SyntaxNodeKind::Greater) {
            return false;
        }
        if (SyntaxNodeKindHasClass(previous->grandParentKind, SyntaxNodeClass::ParenthesizedDeclarator)) {
            return HasCallModifierBeforeDeclaratorBinding(*previous) &&
                cur != SyntaxNodeKind::RightParen &&
                !IsDeclaratorBindingToken(current);
        }
        return !IsDeclaratorBindingToken(current);
    }
    if (
        (current.kind == PrintTokenKind::Known && SyntaxNodeKindHasClass(cur, SyntaxNodeClass::AssignmentOperator)) ||
        IsBinaryOperatorSpacingContext(current)
    ) {
        return true;
    }
    if (
        (
            previous->kind == PrintTokenKind::Known && SyntaxNodeKindHasClass(prev, SyntaxNodeClass::AssignmentOperator)
        ) || IsBinaryOperatorSpacingContext(*previous)
    ) {
        return true;
    }
    if (
        current.kind == PrintTokenKind::Known &&
        SyntaxNodeKindHasClass(cur, SyntaxNodeClass::UnaryOperator) &&
        IsUnaryContext(current)
    ) {
        return false;
    }
    if (
        previous->kind == PrintTokenKind::Known &&
        SyntaxNodeKindHasClass(prev, SyntaxNodeClass::UnaryOperator) &&
        IsUnaryContext(*previous)
    ) {
        return false;
    }
    if (
        SyntaxNodeKindHasClass(prev, SyntaxNodeClass::MemberOperator) ||
        SyntaxNodeKindHasClass(cur, SyntaxNodeClass::MemberOperator)
    ) {
        return false;
    }
    if (IsWordLike(*previous) && IsWordLike(current)) {
        return true;
    }
    if (
        previous->kind == PrintTokenKind::Known && (
            prev == SyntaxNodeKind::RightParen ||
            prev == SyntaxNodeKind::RightBracket ||
            prev == SyntaxNodeKind::RightBrace ||
            prev == SyntaxNodeKind::Greater
        ) &&
        IsWordLike(current)
    ) {
        if (prev == SyntaxNodeKind::RightParen && previous->parentKind == SyntaxNodeKind::CastExpression) {
            return false;
        }
        return true;
    }
    return false;
}

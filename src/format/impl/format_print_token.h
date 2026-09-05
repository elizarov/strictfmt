#pragma once

#include <string_view>

#include "format/impl/format_model.h"

// Tokens carry borrowed source/model references and materialized syntax facts.
// sourceIndex identifies original adjacency; buffered or reordered tokens must
// recompute spacing whenever that adjacency no longer holds.
enum class PrintTokenKind {
    Known,
    Text,
    Comment,
    TrailingComment,
    BlankLine,
    Preprocessor,
    IncludeRun,
};

struct PrintToken {
    PrintTokenKind kind = PrintTokenKind::Text;
    SyntaxNodeKind syntaxKind = SyntaxNodeKind::Unknown;
    std::string_view text;
    SyntaxNodeKind parentKind = SyntaxNodeKind::Unknown;
    SyntaxNodeKind grandParentKind = SyntaxNodeKind::Unknown;
    std::uint64_t syntaxClasses = 0;
    std::uint32_t sourceIndex = static_cast<std::uint32_t>(-1);
    bool inTemplateDeclaration : 1;
    bool inRequiresClause : 1;
    bool inCompilerCallModifier : 1;
    bool inCompactSingleStatementBody : 1;
    bool structuredPreprocessor : 1;
    bool inMacroValue : 1;
    bool stringLike : 1;
    bool containsSourceLineBreak : 1;
    bool inMacroStatementSequence : 1;
    bool inLeadingStreamOperatorChain : 1;
    bool inConditionalStreamOperatorChain : 1;
    bool inConditionalFunctionHeader : 1;
    bool inBareMacroItem : 1;
    bool inTemplateList : 1;
    bool inFieldInitializerList : 1;
    bool inTemplateDeclarationBlock : 1;
    bool inTemplateDeclarationHeader : 1;
    bool commentContinuation : 1;
    bool forcedLeadingPreprocessorListComma : 1;
    bool spaceBefore : 1;
    bool spaceBeforeKnown : 1;
    mutable unsigned templateArgumentExpressionOperator : 2;
    const SyntaxNode* node = nullptr;
    const SyntaxNode* declarationScopeItem = nullptr;
    const SyntaxNode* macroDefinition = nullptr;
};

inline bool PrintTokenSyntaxHasClass(const PrintToken& token, SyntaxNodeClass syntaxNodeClass) {
    return (token.syntaxClasses & static_cast<std::uint64_t>(syntaxNodeClass)) != 0;
}

inline bool PrintTokenSyntaxPathContains(const PrintToken& token, const SyntaxNode* node) {
    for (const SyntaxNode* cursor = token.node; cursor != nullptr; cursor = cursor->parent) {
        if (cursor == node) {
            return true;
        }
    }
    return false;
}

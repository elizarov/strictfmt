#pragma once

#include <string_view>

#include "format/impl/format_model.h"

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

bool IsPreprocessorPrintToken(PrintTokenKind kind);
bool IsPreprocessorLikeToken(const PrintToken& token);
bool IsCommentToken(PrintTokenKind kind);
bool IsLineCommentToken(const PrintToken& token);
bool IsWordLike(const PrintToken& token);
bool IsStringLike(const PrintToken& token);
bool IsAccessKeyword(const PrintToken& token);
bool IsCaseLabelKeyword(const PrintToken& token);
bool FormatTokensShareMacroDefinition(const PrintToken* left, const PrintToken* right);
bool IsTemplateAnglePrintToken(const PrintToken& token);
bool FormatTokenNeedsSpace(const PrintToken* previous, const PrintToken& current);
inline std::string_view FormatTokenText(const PrintToken& token) { return token.text; }
inline int FormatTokenWidth(const PrintToken& token) { return static_cast<int>(token.text.size()); }

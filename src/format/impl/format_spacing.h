#pragma once

#include <string_view>

#include "format/impl/format_print_token.h"
#include "util/utf8.h"

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
inline int FormatTokenWidth(const PrintToken& token) { return Utf8CharacterCount(token.text); }

inline bool IsStructuralTriviaToken(const PrintToken& token) {
    // Inline block comments are text tokens for emission, but remain trivia for grammar-neighbor decisions.
    return token.kind == PrintTokenKind::BlankLine ||
        IsCommentToken(token.kind) ||
        (token.node != nullptr && SyntaxNodeHasClass(*token.node, SyntaxNodeClass::Trivia));
}

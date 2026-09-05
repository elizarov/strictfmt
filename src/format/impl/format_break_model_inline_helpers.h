#pragma once

#include "format/impl/format_break_model.h"

inline const PrintToken& FormatBreakTokenValue(const FormatBreakToken& token) {
    static const PrintToken kEmptyToken{};
    return token.token == nullptr ? kEmptyToken : *token.token;
}

inline PrintTokenKind FormatBreakTokenKind(const FormatBreakToken& token) {
    return token.token == nullptr ? PrintTokenKind::Text : token.token->kind;
}

inline SyntaxNodeKind FormatBreakTokenSyntaxKind(const FormatBreakToken& token) {
    return token.token == nullptr ? SyntaxNodeKind::Unknown : token.token->syntaxKind;
}

inline bool FormatBreakHasTrailingComment(const FormatBreakNode& node, size_t index) {
    return index < node.items.size() && IsCommentToken(FormatBreakTokenKind(node.items[index].trailingComment));
}

inline bool FormatBreakHasLeadingTrailingComment(const FormatBreakNode& node) {
    return IsCommentToken(FormatBreakTokenKind(node.leadingTrailingComment));
}

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

inline const FormatBreakToken* FormatBreakNodeToken(const FormatBreakNode* node) {
    if (!node || node->kind != FormatBreakNodeKind::Token) {
        return nullptr;
    }
    return &node->token;
}

inline bool FormatBreakIsStandaloneCommentItem(const FormatBreakNode& node, size_t index) {
    if (index >= node.items.size()) {
        return false;
    }
    const FormatBreakToken* token = FormatBreakNodeToken(node.items[index].node);
    return token != nullptr && FormatBreakTokenKind(*token) == PrintTokenKind::Comment;
}

inline bool FormatBreakHasRealSeparators(const FormatBreakNode& node) {
    return std::any_of(node.items.begin(), node.items.end(), [](const FormatBreakListItem& item) {
        return FormatBreakTokenKind(item.separator) == PrintTokenKind::Known;
    });
}

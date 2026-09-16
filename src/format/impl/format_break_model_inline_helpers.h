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

inline bool FormatBreakHasSingleLineTrailingComma(const FormatBreakNode& node, size_t index) {
    return index < node.items.size() &&
        node.kind == FormatBreakNodeKind::Delimited &&
        node.children.size() == 2 &&
        !node.children.front()->token.contextOnly &&
        !node.children.back()->token.contextOnly &&
        PrintTokenIsSingleLineTrailingComma(FormatBreakTokenValue(node.items[index].separator)) &&
        FormatBreakTokenValue(node.children.back()->token).node != nullptr &&
        FormatBreakTokenValue(node.items[index].separator).node->parent ==
            FormatBreakTokenValue(node.children.back()->token).node->parent;
}

inline FormatBreakToken FormatBreakSingleLineCloseToken(const FormatBreakNode& node) {
    FormatBreakToken close = node.children.back()->token;
    if (!node.items.empty() && FormatBreakHasSingleLineTrailingComma(node, node.items.size() - 1)) {
        close.spaceBefore = node.singleLineCloseSpaceBefore;
    }
    return close;
}

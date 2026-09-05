#pragma once

#include "format/impl/format_model.h"

// Direct-child lexical queries shared by structural printing and continuation
// planning. These preserve the parser's nesting and do not search descendants.
inline const SyntaxNode* DirectTokenChild(const SyntaxNode& node, SyntaxNodeKind known) {
    for (const SyntaxNode* child : node.children) {
        if (child && child->kind == known) {
            return child;
        }
    }
    return nullptr;
}

inline bool HasDirectKnownChild(const SyntaxNode& node, SyntaxNodeKind known) {
    return DirectTokenChild(node, known) != nullptr;
}

inline SyntaxNodeKind MatchingListCloseToken(SyntaxNodeKind kind) {
    switch (kind) {
        case SyntaxNodeKind::LeftParen:
            return SyntaxNodeKind::RightParen;
        case SyntaxNodeKind::LeftBracket:
            return SyntaxNodeKind::RightBracket;
        case SyntaxNodeKind::LeftBrace:
            return SyntaxNodeKind::RightBrace;
        case SyntaxNodeKind::Less:
            return SyntaxNodeKind::Greater;
        default:
            return SyntaxNodeKind::Unknown;
    }
}

inline const SyntaxNode* DirectOpeningDelimiterChild(const SyntaxNode& node) {
    for (const SyntaxNode* child : node.children) {
        if (child && SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::OpeningDelimiter)) {
            return child;
        }
    }
    return nullptr;
}

inline const SyntaxNode* DirectMatchingClosingDelimiterChild(const SyntaxNode& node, const SyntaxNode* open) {
    const SyntaxNodeKind closingKind = open == nullptr ? SyntaxNodeKind::Unknown : MatchingListCloseToken(open->kind);
    return closingKind == SyntaxNodeKind::Unknown ? nullptr : DirectTokenChild(node, closingKind);
}

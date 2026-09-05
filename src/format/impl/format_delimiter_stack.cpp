#include "format/impl/format_delimiter_stack.h"

#include "format/impl/format_break_model_inline_helpers.h"

namespace {

bool IsTransparentSingleItemDelimiter(const FormatBreakNode& node, FormatDelimiterStackPolicy policy) {
    if (
        node.forceSplit ||
        node.delimiterKind != FormatBreakDelimiterKind::Paren ||
        node.children.size() < 2 ||
        node.items.size() != 1 ||
        FormatBreakHasRealSeparators(node) ||
        FormatBreakHasTrailingComment(node, 0) ||
        node.items.front().blankLineBefore ||
        (policy == FormatDelimiterStackPolicy::Emission && node.blankLineBeforeClose) ||
        node.items.front().node == nullptr
    ) {
        return false;
    }
    const FormatBreakNode* open = node.children[0];
    if (open == nullptr || open->kind != FormatBreakNodeKind::Token) {
        return false;
    }
    const PrintToken& token = FormatBreakTokenValue(open->token);
    return !SyntaxNodeKindHasClass(token.parentKind, SyntaxNodeClass::SemanticDelimitedParent) &&
        !SyntaxNodeKindHasClass(token.grandParentKind, SyntaxNodeClass::SemanticDelimitedParent);
}

const FormatBreakNode* SingleChildSequenceNode(const FormatBreakNode& node) {
    if (node.kind != FormatBreakNodeKind::Sequence || node.children.size() != 1) {
        return nullptr;
    }
    return node.children.front();
}

const FormatBreakNode* TransparentStackChild(const FormatBreakNode& node, FormatDelimiterStackPolicy policy) {
    if (!IsTransparentSingleItemDelimiter(node, policy)) {
        return nullptr;
    }
    const FormatBreakNode* item = node.items.front().node;
    while (item != nullptr) {
        if (IsTransparentSingleItemDelimiter(*item, policy)) {
            return item;
        }
        item = SingleChildSequenceNode(*item);
    }
    return nullptr;
}

bool IsDelimiterStackItem(const FormatBreakNode& node, FormatDelimiterStackPolicy policy) {
    return IsTransparentSingleItemDelimiter(node, policy) && TransparentStackChild(node, policy) != nullptr;
}

}  // namespace

std::optional<FormatDelimiterStack>
    CollectFormatDelimiterStack(const FormatBreakNode& node, FormatDelimiterStackPolicy policy)
{
    if (!IsDelimiterStackItem(node, policy)) {
        return std::nullopt;
    }
    FormatDelimiterStack stack;
    const FormatBreakNode* current = &node;
    while (current != nullptr) {
        stack.delimiters.push_back(current);
        const FormatBreakNode* child = TransparentStackChild(*current, policy);
        if (child == nullptr) {
            stack.leaf = current->items.front().node;
            break;
        }
        current = child;
    }
    return stack.leaf != nullptr && stack.delimiters.size() > 1 ? std::optional(stack) : std::nullopt;
}

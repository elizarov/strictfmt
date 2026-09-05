#include "format/impl/format_break_cost.h"

#include <cassert>
#include <optional>

#include "format/impl/format_break_model_inline_helpers.h"

namespace {

bool OwnsStructuralBreak(const FormatBreakNode& node) {
    return node.kind != FormatBreakNodeKind::Token && node.kind != FormatBreakNodeKind::Sequence;
}

std::optional<int> MinimumStructuralBreakDepth(const FormatBreakNode& node) {
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

std::optional<int> FirstParameterListBreakDepth(const FormatBreakNode& node) {
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

void ShiftStructuralDepth(FormatBreakNode& node, int delta, bool includeChainLinks = true) {
    if (includeChainLinks || !IsFormatBreakUniformChain(node)) {
        node.structuralDepth += delta;
        node.breakCost += delta;
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

FormatBreakNode* UnwrapSingleChildSequence(FormatBreakNode* node) {
    while (node != nullptr && node->kind == FormatBreakNodeKind::Sequence && node->children.size() == 1) {
        node = node->children.front();
    }
    return node;
}

void DiscountBreakCostsRecursively(FormatBreakNode& node, int discount) {
    assert(node.breakCost >= discount);
    node.breakCost -= discount;
    for (FormatBreakNode* child : node.children) {
        if (child != nullptr) {
            DiscountBreakCostsRecursively(*child, discount);
        }
    }
    for (FormatBreakListItem& item : node.items) {
        if (item.node != nullptr) {
            DiscountBreakCostsRecursively(*item.node, discount);
        }
    }
    for (FormatBreakNode* operand : node.operands) {
        if (operand != nullptr) {
            DiscountBreakCostsRecursively(*operand, discount);
        }
    }
}

FormatBreakNode* FinalLambdaBody(FormatBreakNode& list) {
    size_t end = list.items.size();
    while (end != 0 && FormatBreakIsStandaloneCommentItem(list, end - 1)) {
        --end;
    }
    FormatBreakNode* lambda = end == 0 ? nullptr : UnwrapSingleChildSequence(list.items[end - 1].node);
    if (lambda == nullptr || !lambda->bodyHeaderIsLambda || lambda->children.size() != 2) {
        return nullptr;
    }
    FormatBreakNode* body = UnwrapSingleChildSequence(lambda->children.back());
    return body != nullptr && body->kind == FormatBreakNodeKind::Delimited ? body : nullptr;
}

void DiscountFinalLambdaBody(FormatBreakNode& list) {
    if (FormatBreakNode* body = FinalLambdaBody(list)) {
        DiscountBreakCostsRecursively(*body, body->breakCost);
    }
}

void ApplyFinalLambdaDiscounts(FormatBreakNode& node) {
    if (node.kind == FormatBreakNodeKind::Delimited) {
        DiscountFinalLambdaBody(node);
    }
    for (FormatBreakNode* child : node.children) {
        if (child != nullptr) {
            ApplyFinalLambdaDiscounts(*child);
        }
    }
    for (FormatBreakListItem& item : node.items) {
        if (item.node != nullptr) {
            ApplyFinalLambdaDiscounts(*item.node);
        }
    }
    for (FormatBreakNode* operand : node.operands) {
        if (operand != nullptr) {
            ApplyFinalLambdaDiscounts(*operand);
        }
    }
}

void NormalizePrefixBreakDepth(FormatBreakNode& prefix, int competingDepth) {
    const std::optional<int> prefixDepth = MinimumStructuralBreakDepth(prefix);
    if (!prefixDepth || *prefixDepth > competingDepth) {
        return;
    }
    ShiftStructuralDepth(prefix, competingDepth + 1 - *prefixDepth);
}

}  // namespace

void FormatBreakCostNormalizer::NormalizeCallablePrefix(FormatBreakNode& prefix, const FormatBreakNode& declarator) {
    const std::optional<int> parameterDepth = FirstParameterListBreakDepth(declarator);
    if (parameterDepth) {
        NormalizePrefixBreakDepth(prefix, *parameterDepth);
    }
}

void FormatBreakCostNormalizer::NormalizeNamedListPrefixes(
    std::span<FormatBreakNode* const> children, FormatBreakNode* qualification
) {
    for (size_t index = 1; index < children.size(); ++index) {
        const FormatBreakNode* list = children[index];
        if (list->kind != FormatBreakNodeKind::Delimited || list->children.empty()) {
            continue;
        }
        const FormatBreakToken* open = FormatBreakNodeToken(list->children.front());
        if (
            open == nullptr ||
            !SyntaxNodeKindHasClass(FormatBreakTokenValue(*open).parentKind, SyntaxNodeClass::NamedList)
        ) {
            continue;
        }
        if (qualification != nullptr && qualification->structuralDepth <= list->structuralDepth) {
            // The qualifier precedes this list syntactically, but owns its right operand in the break tree.
            // Surcharge only the qualification decision and its left prefix, not the attached list.
            const int delta = list->structuralDepth + 1 - qualification->structuralDepth;
            qualification->structuralDepth += delta;
            qualification->breakCost += delta;
            ShiftStructuralDepth(*qualification->operands.front(), delta, false);
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

void FormatBreakCostNormalizer::ObserveDelimited(FormatBreakNode& node) {
    // Every completed delimiter is observed. Without a final lambda body, the
    // recursive discount pass is an identity operation and can be skipped.
    hasFinalLambdaBody_ = hasFinalLambdaBody_ || FinalLambdaBody(node) != nullptr;
}

void FormatBreakCostNormalizer::Finalize(FormatBreakNode& root) const {
    if (hasFinalLambdaBody_) {
        ApplyFinalLambdaDiscounts(root);
    }
}

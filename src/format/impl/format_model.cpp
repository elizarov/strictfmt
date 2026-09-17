#include "format/impl/format_model.h"

namespace {

bool BodyHasDisqualifier(
    const SyntaxNode& node,
    const SyntaxNode& body,
    const SyntaxNode* statement,
    bool inBody = false,
    bool inStatement = false
) {
    inBody = inBody || &node == &body;
    inStatement = inStatement || &node == statement;
    if (
        SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::PreprocessorDirective) ||
        (inBody && SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::Comment)) ||
        (inStatement && SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::CompoundBlock))
    ) {
        return true;
    }
    for (const SyntaxNode* child : node.children) {
        if (child != nullptr && BodyHasDisqualifier(*child, body, statement, inBody, inStatement)) {
            return true;
        }
    }
    return false;
}

bool IsNullItem(const SyntaxNode& node) {
    const SyntaxNode* item = &node;
    while (item->children.size() == 1 && item->children.front() != nullptr) {
        item = item->children.front();
    }
    return item->kind == SyntaxNodeKind::Semicolon;
}

const SyntaxNode* OnlyContentChild(const SyntaxNode& node) {
    const SyntaxNode* contentChild = nullptr;
    for (const SyntaxNode* child : node.children) {
        if (
            child == nullptr ||
            SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::Trivia) ||
            SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::Known) ||
            IsNullItem(*child)
        ) {
            continue;
        }
        if (contentChild != nullptr) {
            return nullptr;
        }
        contentChild = child;
    }
    return contentChild;
}

bool IsArgumentBody(const SyntaxNode& node) {
    const SyntaxNode* item = &node;
    while (item->parent != nullptr) {
        const SyntaxNode& parent = *item->parent;
        if (parent.kind == SyntaxNodeKind::ArgumentList) {
            return true;
        }
        if (
            (parent.kind != SyntaxNodeKind::Tree && parent.kind != SyntaxNodeKind::MacroStatementSequence) ||
            OnlyContentChild(parent) != item
        ) {
            return false;
        }
        item = &parent;
    }
    return false;
}

}  // namespace

SyntaxNode::SyntaxNode(std::pmr::memory_resource* childResource) : children(childResource) {}

FormatModel::FormatModel() : childStorage(std::make_unique<std::pmr::monotonic_buffer_resource>()) {}

bool BodyAllowsCompactSingleStatementForm(const SyntaxNode& node) {
    const SyntaxNodeKind parentKind = node.parent == nullptr ? SyntaxNodeKind::Unknown : node.parent->kind;
    const bool callableOwner =
        parentKind == SyntaxNodeKind::FunctionDefinition || parentKind == SyntaxNodeKind::LambdaExpression;
    if (node.kind != SyntaxNodeKind::CompoundStatement || (!callableOwner && !IsArgumentBody(node))) {
        return false;
    }
    if (node.compactBodyCache != 0) {
        return node.compactBodyCache == 2;
    }
    const SyntaxNode* statement = OnlyContentChild(node);
    // Spacing and break choices share eligibility. Callable headers also participate in the directive check.
    const SyntaxNode& searchRoot = callableOwner ? *node.parent : node;
    const bool result = statement != nullptr && !BodyHasDisqualifier(searchRoot, node, statement);
    node.compactBodyCache = result ? 2 : 1;
    return result;
}

bool IsConditionalPreprocessorHeaderChild(const SyntaxNode& node, size_t index) {
    const SyntaxNode& child = *node.children[index];
    if (
        index == 0 ||
        (child.kind == SyntaxNodeKind::LexicalToken && child.text.find_first_of("\r\n") != std::string_view::npos)
    ) {
        return true;
    }
    const SyntaxNodeKind directive = node.children.front()->kind;
    if (
        directive == SyntaxNodeKind::PreprocessorDirectiveIf || directive == SyntaxNodeKind::PreprocessorDirectiveElif
    ) {
        return child.isCondition;
    }
    if (
        directive == SyntaxNodeKind::PreprocessorDirectiveIfdef ||
        directive == SyntaxNodeKind::PreprocessorDirectiveIfndef ||
        directive == SyntaxNodeKind::PreprocessorDirectiveElifdef ||
        directive == SyntaxNodeKind::PreprocessorDirectiveElifndef
    ) {
        return child.isName;
    }
    return false;
}

SyntaxNode* MakeSyntaxNode(FormatModel& model, SyntaxNodeKind kind) {
    SyntaxNode* node = &model.nodes.emplace_back(model.childStorage.get());
    node->kind = kind;
    return node;
}

void ReparentSyntaxNode(SyntaxNode& node, const SyntaxNode* parent) {
    node.parent = parent;
    node.depth = parent == nullptr ? 0 : parent->depth + 1;
    for (SyntaxNode* child : node.children) {
        if (child != nullptr) {
            ReparentSyntaxNode(*child, &node);
        }
    }
}

void AppendSyntaxChild(SyntaxNode& parent, SyntaxNode* child) {
    if (child != nullptr) {
        ReparentSyntaxNode(*child, &parent);
    }
    parent.children.push_back(child);
}

#include "format/impl/format_declaration_layout.h"

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "format/impl/format_layout_tree.h"
#include "format/impl/format_break_model_inline_helpers.h"
#include "format/impl/format_syntax_helpers.h"

namespace {

enum class DeclarationGroupKind {
    None,
    Type,
    BodylessType,
    Callable,
    Object,
    Alias,
};

struct DeclarationGroupState {
    const SyntaxNode* previousItem = nullptr;
    const SyntaxNode* preparedItem = nullptr;
};

const SyntaxNode* DeclarationScopeItem(const SyntaxNode* node) {
    for (const SyntaxNode* cursor = node; cursor != nullptr && cursor->parent != nullptr; cursor = cursor->parent) {
        // DeclarationScope is a syntax-local normalized class, so its authoritative value is stored on the node.
        if ((cursor->parent->classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationScope)) != 0) {
            return cursor;
        }
    }
    return nullptr;
}

bool CanIsolateLargeValue(const SyntaxNode* item) {
    return item != nullptr && (
        SyntaxNodeHasClass(*item, SyntaxNodeClass::DeclarationGroupObject) ||
        SyntaxNodeHasClass(*item, SyntaxNodeClass::DeclarationGroupAlias)
    );
}

}  // namespace

struct FormatDeclarationLayout::Impl {
    std::span<const PrintToken> tokens_;
    std::unordered_map<const SyntaxNode*, DeclarationGroupState> declarationGroupStates_;
    std::vector<const SyntaxNode*> nextDeclarationItemsBySourceIndex_;

    explicit Impl(std::span<const PrintToken> tokens) : tokens_(tokens) {
        nextDeclarationItemsBySourceIndex_.resize(tokens.size());
        const SyntaxNode* next = nullptr;
        for (size_t index = tokens.size(); index-- > 0;) {
            nextDeclarationItemsBySourceIndex_[index] = next;
            const SyntaxNode* item = tokens[index].declarationScopeItem;
            if (DeclarationGroup(item) != DeclarationGroupKind::None) {
                next = item;
            }
        }
    }

    static DeclarationGroupKind DeclarationGroup(const SyntaxNode* item) {
        if (item == nullptr) {
            return DeclarationGroupKind::None;
        }
        // Declaration-group classes are normalized per node and intentionally excluded from static kind classes.
        // Reading the stored bits is therefore equivalent to five generic class queries and reuses that analysis.
        const std::uint64_t classes = item->classes;
        if ((classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupType)) != 0) {
            return DeclarationGroupKind::Type;
        }
        if ((classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupBodylessType)) != 0) {
            return DeclarationGroupKind::BodylessType;
        }
        if ((classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupCallable)) != 0) {
            return DeclarationGroupKind::Callable;
        }
        if ((classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupObject)) != 0) {
            return DeclarationGroupKind::Object;
        }
        if ((classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupAlias)) != 0) {
            return DeclarationGroupKind::Alias;
        }
        return DeclarationGroupKind::None;
    }

    bool RequiresDeclarationGroupSeparation(const SyntaxNode* left, const SyntaxNode* right) const {
        if (
            left == nullptr ||
            right == nullptr ||
            left->parent == nullptr ||
            left->parent != right->parent ||
            !SyntaxNodeHasClass(*left->parent, SyntaxNodeClass::DeclarationScope)
        ) {
            return false;
        }
        const DeclarationGroupKind leftGroup = DeclarationGroup(left);
        const DeclarationGroupKind rightGroup = DeclarationGroup(right);
        if (leftGroup == DeclarationGroupKind::None || rightGroup == DeclarationGroupKind::None) {
            return false;
        }
        if (leftGroup == DeclarationGroupKind::BodylessType && rightGroup == DeclarationGroupKind::BodylessType) {
            return false;
        }
        return leftGroup == DeclarationGroupKind::Type ||
            leftGroup == DeclarationGroupKind::BodylessType ||
            rightGroup == DeclarationGroupKind::Type ||
            rightGroup == DeclarationGroupKind::BodylessType ||
            leftGroup != rightGroup;
    }

    const SyntaxNode* NextDeclarationItem(size_t index) const {
        if (index >= nextDeclarationItemsBySourceIndex_.size()) {
            return nullptr;
        }
        // This is the exact result of the former forward scan: the immutable token order is summarized backward,
        // replacing repeated searches without changing which following declaration item is selected.
        return nextDeclarationItemsBySourceIndex_[index];
    }

    std::optional<FormatDeclarationBoundary> Boundary(const SyntaxNode* left, const SyntaxNode* right) const {
        if (
            left == nullptr ||
            right == nullptr ||
            left->parent == nullptr ||
            left->parent != right->parent ||
            !SyntaxNodeHasClass(*left->parent, SyntaxNodeClass::DeclarationScope) ||
            DeclarationGroup(left) == DeclarationGroupKind::None ||
            DeclarationGroup(right) == DeclarationGroupKind::None
        ) {
            return std::nullopt;
        }
        return FormatDeclarationBoundary{left, right, RequiresDeclarationGroupSeparation(left, right)};
    }

    std::optional<FormatDeclarationBoundary> BoundaryBefore(size_t index) {
        const PrintToken& token = tokens_[index];
        const SyntaxNode* item = token.declarationScopeItem;
        if (DeclarationGroup(item) != DeclarationGroupKind::None) {
            DeclarationGroupState& state = declarationGroupStates_[item->parent];
            if (item == state.previousItem) {
                return std::nullopt;
            }
            const auto boundary = item == state.preparedItem ? std::nullopt : Boundary(state.previousItem, item);
            state.previousItem = item;
            state.preparedItem = nullptr;
            return boundary;
        }
        if (
            item == nullptr ||
            item->parent == nullptr ||
            token.kind == PrintTokenKind::TrailingComment ||
            token.commentContinuation ||
            (token.node != nullptr && token.node->kind == SyntaxNodeKind::Semicolon)
        ) {
            return std::nullopt;
        }
        DeclarationGroupState& state = declarationGroupStates_[item->parent];
        const SyntaxNode* next = NextDeclarationItem(index);
        if (next == nullptr || next->parent != item->parent || next == state.preparedItem) {
            return std::nullopt;
        }
        const auto boundary = Boundary(state.previousItem, next);
        if (boundary) {
            state.preparedItem = next;
        }
        return boundary;
    }

};

FormatDeclarationLayout::FormatDeclarationLayout(std::span<const PrintToken> tokens) :
    impl_(std::make_unique<Impl>(tokens)) {}
FormatDeclarationLayout::~FormatDeclarationLayout() = default;
std::optional<FormatDeclarationBoundary> FormatDeclarationLayout::BoundaryBefore(size_t index) {
    return impl_->BoundaryBefore(index);
}

namespace {

struct TokenRange {
    size_t begin = std::numeric_limits<size_t>::max();
    size_t end = 0;

    void Add(size_t index) {
        begin = std::min(begin, index);
        end = std::max(end, index + 1);
    }
};

TokenRange SourceRange(const FormatBreakNode& node, const FormatLayoutTree& tree) {
    TokenRange range;
    auto token = [&](const FormatBreakToken& value) {
        if (value.token != nullptr) {
            range.Add(value.token->sourceIndex);
        }
    };
    auto child = [&](const FormatBreakNode* value) {
        if (value == nullptr) {
            return;
        }
        const auto nested = SourceRange(*value, tree);
        range.begin = std::min(range.begin, nested.begin);
        range.end = std::max(range.end, nested.end);
    };
    token(node.token);
    token(node.leadingTrailingComment);
    for (const auto* value : node.children) {
        child(value);
    }
    for (const auto* value : node.operands) {
        child(value);
    }
    for (const auto& value : node.operators) {
        token(value);
    }
    for (const auto& item : node.items) {
        child(item.node);
        token(item.separator);
        token(item.trailingComment);
    }
    if (node.bodySyntax != nullptr) {
        const auto& body = tree.Owner(tree.FindOwner(node.bodySyntax));
        if (body.begin < body.end) {
            range.Add(body.begin);
            range.Add(body.end - 1);
        }
    }
    return range;
}

TokenRange SyntaxRange(const SyntaxNode& node, const FormatLayoutTree& tree) {
    const auto& owner = tree.Owner(tree.FindOwner(&node));
    return {owner.begin, owner.end};
}

void AddRange(TokenRange& range, TokenRange part) {
    if (part.begin < part.end) {
        range.Add(part.begin);
        range.Add(part.end - 1);
    }
}

// A named declarator contributes its type constructors, but not the alias name.
// Follow declarator fields and transparent reference/parenthesis wrappers; lists
// inside the declarator retain their complete types and expressions.
TokenRange DeclaratorTypeRange(const SyntaxNode& node, const FormatLayoutTree& tree) {
    if (node.kind == SyntaxNodeKind::Identifier) {
        return {};
    }
    TokenRange range;
    const bool wrapper = SyntaxNodeHasClass(node, SyntaxNodeClass::DeclaratorReferenceParent) ||
        SyntaxNodeHasClass(node, SyntaxNodeClass::ParenthesizedDeclarator);
    for (const auto* child : node.children) {
        if (child == nullptr || SyntaxNodeHasClass(*child, SyntaxNodeClass::Trivia)) {
            continue;
        }
        const bool target = child->isDeclarator || (wrapper && !SyntaxNodeHasClass(*child, SyntaxNodeClass::Known));
        AddRange(range, target ? DeclaratorTypeRange(*child, tree) : SyntaxRange(*child, tree));
    }
    return range;
}

TokenRange AliasTargetRange(const SyntaxNode& node, const FormatLayoutTree& tree) {
    TokenRange range;
    bool target = false;
    for (const auto* child : node.children) {
        if (child == nullptr || SyntaxNodeHasClass(*child, SyntaxNodeClass::Trivia)) {
            continue;
        }
        target = target || child->isType;
        if (target && child->kind != SyntaxNodeKind::Semicolon) {
            AddRange(range, child->isDeclarator ? DeclaratorTypeRange(*child, tree) : SyntaxRange(*child, tree));
        } else if (SyntaxNodeHasClass(*child, SyntaxNodeClass::DeclarationGroupAlias)) {
            AddRange(range, AliasTargetRange(*child, tree));
        }
    }
    return range;
}

bool IsMeasured(const FormatLayoutTokenLines& lines) { return lines.first != std::numeric_limits<size_t>::max(); }

bool MayHaveLargeValue(const SyntaxNode* item, const FormatLayoutTree& tree, const FormatLayoutProgram& program) {
    const auto& owner = tree.Owner(tree.FindOwner(item));
    size_t first = std::numeric_limits<size_t>::max();
    size_t last = 0;
    for (size_t index = owner.begin; index < std::min(owner.end, program.tokenLines.size()); ++index) {
        const auto& lines = program.tokenLines[index];
        if (IsMeasured(lines)) {
            first = std::min(first, lines.first);
            last = std::max(last, lines.last);
            if (last - first > 1) {
                return true;
            }
        }
    }
    // The value's continuation range cannot exceed the whole declaration's range.
    return false;
}

// Counts the selected value's physical continuation lines while treating every
// nested compound body as opaque. Source ranges retain bodies omitted from cost
// regions; interval union prevents nested scopes from being subtracted twice.
bool LargeValue(TokenRange prefix, TokenRange value, const FormatLayoutTree& tree, const FormatLayoutProgram& program) {
    if (prefix.begin >= prefix.end || value.begin >= value.end) {
        return false;
    }
    auto lastLine = [&](TokenRange range) -> std::optional<size_t> {
        for (size_t index = range.end; index-- > range.begin;) {
            if (index < program.tokenLines.size() && IsMeasured(program.tokenLines[index])) {
                return program.tokenLines[index].last;
            }
        }
        return std::nullopt;
    };
    const auto start = lastLine(prefix);
    const auto end = lastLine(value);
    if (!start || !end || *end <= *start + 1) {
        return false;
    }
    std::vector<std::pair<size_t, size_t>> bodies;
    const auto tokens = tree.Tokens();
    for (size_t index = value.begin; index < value.end && index < tokens.size(); ++index) {
        const auto* open = tokens[index].node;
        if (
            open == nullptr ||
            open->kind != SyntaxNodeKind::LeftBrace ||
            open->parent == nullptr ||
            !SyntaxNodeHasClass(*open->parent, SyntaxNodeClass::CompoundBlock)
        ) {
            continue;
        }
        const auto* close = DirectMatchingClosingDelimiterChild(*open->parent, open);
        if (close == nullptr) {
            continue;
        }
        const auto& closeOwner = tree.Owner(tree.FindOwner(close));
        if (closeOwner.begin >= program.tokenLines.size()) {
            continue;
        }
        const auto& openLines = program.tokenLines[index];
        const auto& closeLines = program.tokenLines[closeOwner.begin];
        if (IsMeasured(openLines) && IsMeasured(closeLines)) {
            bodies.emplace_back(std::max(*start, openLines.last), std::min(*end, closeLines.first));
        }
    }
    std::sort(bodies.begin(), bodies.end());
    size_t excluded = 0;
    size_t covered = *start;
    for (const auto& [begin, end] : bodies) {
        const size_t from = std::max(covered, begin);
        if (end > from) {
            excluded += end - from;
        }
        covered = std::max(covered, end);
    }
    return *end - *start - excluded > 1;
}

void CollectLargeValues(
    const FormatBreakNode& node,
    const FormatLayoutTree& tree,
    const FormatLayoutProgram& program,
    std::unordered_set<const SyntaxNode*>& isolated,
    std::unordered_set<const SyntaxNode*>& examined
) {
    if (node.declarationValueOwner != nullptr && examined.insert(node.declarationValueOwner).second) {
        const auto* item = DeclarationScopeItem(node.declarationValueOwner);
        if (CanIsolateLargeValue(item) && !isolated.contains(item) && node.operands.size() >= 2) {
            const auto prefix = SourceRange(*node.operands[node.operands.size() - 2], tree);
            const auto value = SourceRange(*node.operands.back(), tree);
            if (LargeValue(prefix, value, tree, program)) {
                isolated.insert(item);
            }
        }
    }
    for (const auto* child : node.children) {
        if (child != nullptr) {
            CollectLargeValues(*child, tree, program, isolated, examined);
        }
    }
    for (const auto* child : node.operands) {
        if (child != nullptr) {
            CollectLargeValues(*child, tree, program, isolated, examined);
        }
    }
    for (const auto& item : node.items) {
        if (item.node != nullptr) {
            CollectLargeValues(*item.node, tree, program, isolated, examined);
        }
    }
}

}  // namespace

void FormatDeclarationLayout::Resolve(FormatLayoutTree& tree, FormatLayoutProgram& program) const {
    // Only object and alias values can add an optional boundary. If none of
    // those boundaries remains, traversing complete models cannot change output.
    if (std::none_of(program.groupBoundaries.begin(), program.groupBoundaries.end(), [](const auto& boundary) {
        return !boundary.required && (CanIsolateLargeValue(boundary.left) || CanIsolateLargeValue(boundary.right));
    })) {
        return;
    }
    std::unordered_set<const SyntaxNode*> isolated;
    std::unordered_set<const SyntaxNode*> examined;
    std::unordered_set<const SyntaxNode*> declarations;
    for (const auto& boundary : program.groupBoundaries) {
        if (boundary.required) {
            continue;
        }
        for (const auto* item : {boundary.left, boundary.right}) {
            if (
                !CanIsolateLargeValue(item) ||
                !declarations.insert(item).second ||
                !MayHaveLargeValue(item, tree, program)
            ) {
                continue;
            }
            if (SyntaxNodeHasClass(*item, SyntaxNodeClass::DeclarationGroupAlias)) {
                const auto target = AliasTargetRange(*item, tree);
                const TokenRange prefix{SyntaxRange(*item, tree).begin, target.begin};
                if (LargeValue(prefix, target, tree, program)) {
                    isolated.insert(item);
                }
                continue;
            }
            const auto& model = tree.CompleteModel(tree.FindOwner(item));
            if (model.root != nullptr) {
                CollectLargeValues(*model.root, tree, program, isolated, examined);
            }
        }
    }
    for (auto& boundary : program.groupBoundaries) {
        boundary.required = boundary.required || isolated.contains(boundary.left) || isolated.contains(boundary.right);
    }
}

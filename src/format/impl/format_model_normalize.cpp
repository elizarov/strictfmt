#include "format/impl/format_model_normalize.h"

#include <algorithm>
#include <optional>
#include <vector>

namespace {

std::optional<size_t> PreviousNonTriviaChildIndex(const SyntaxChildList& children, size_t before) {
    while (before > 0) {
        --before;
        const SyntaxNode* child = children[before];
        if (child == nullptr || !SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::Trivia)) {
            return before;
        }
    }
    return std::nullopt;
}

std::optional<size_t> NextNonTriviaChildIndex(const SyntaxChildList& children, size_t after) {
    for (size_t index = after; index < children.size(); ++index) {
        const SyntaxNode* child = children[index];
        if (child == nullptr || !SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::Trivia)) {
            return index;
        }
    }
    return std::nullopt;
}

// Structural lookup uses effective classes because inline block comments print as lexical tokens while retaining
// their trivia class. Token-rewriting lookups above intentionally use only static kind classes to preserve position.
std::optional<size_t> PreviousStructuralChildIndex(const SyntaxChildList& children, size_t before) {
    while (before > 0) {
        --before;
        const SyntaxNode* child = children[before];
        if (child == nullptr || !SyntaxNodeHasClass(*child, SyntaxNodeClass::Trivia)) {
            return before;
        }
    }
    return std::nullopt;
}

std::optional<size_t> NextStructuralChildIndex(const SyntaxChildList& children, size_t after) {
    for (size_t index = after; index < children.size(); ++index) {
        const SyntaxNode* child = children[index];
        if (child == nullptr || !SyntaxNodeHasClass(*child, SyntaxNodeClass::Trivia)) {
            return index;
        }
    }
    return std::nullopt;
}

void RemovePreviousComma(SyntaxChildList& children, size_t before) {
    const std::optional<size_t> previous = PreviousNonTriviaChildIndex(children, before);
    if (previous && children[*previous]->kind == SyntaxNodeKind::Comma) {
        children.erase(children.begin() + static_cast<std::ptrdiff_t>(*previous));
    }
}

void RemoveTerminalConditionalListCommas(SyntaxNode& node) {
    SyntaxChildList& children = node.children;
    for (size_t index = children.size(); index > 0; --index) {
        SyntaxNode* child = children[index - 1];
        if (child != nullptr && (
            SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::ConditionalPreprocessorTree) ||
            SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::EndifDirective)
        )) {
            RemovePreviousComma(children, index - 1);
        }
    }
    for (SyntaxNode* child : children) {
        if (child != nullptr && SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::ConditionalPreprocessorTree)) {
            RemoveTerminalConditionalListCommas(*child);
        }
    }
    if (
        SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::ConditionalPreprocessorTree) &&
        !SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::ConditionalPreprocessorOpen)
    ) {
        RemovePreviousComma(children, children.size());
    }
}

void InsertCommaAfter(FormatModel& model, SyntaxNode& node, size_t index) {
    SyntaxNode* comma = MakeSyntaxNode(model, SyntaxNodeKind::Comma);
    comma->parent = &node;
    comma->depth = node.depth + 1;
    node.children.insert(node.children.begin() + static_cast<std::ptrdiff_t>(index + 1), comma);
}

void AddTerminalConditionalListCommas(FormatModel& model, SyntaxNode& node) {
    SyntaxChildList& children = node.children;
    const size_t headerChildren = node.kind == SyntaxNodeKind::PreprocElse ? 1 : 2;
    for (size_t index = 0; index < children.size(); ++index) {
        SyntaxNode* child = children[index];
        if (child == nullptr) {
            continue;
        }
        if (
            SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::ConditionalBranchSeparatorDirective) ||
            SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::EndifDirective)
        ) {
            const std::optional<size_t> previous = PreviousNonTriviaChildIndex(children, index);
            if (
                previous &&
                *previous >= headerChildren &&
                children[*previous]->kind != SyntaxNodeKind::Comma &&
                !SyntaxNodeKindHasClass(children[*previous]->kind, SyntaxNodeClass::ConditionalPreprocessorTree)
            ) {
                InsertCommaAfter(model, node, *previous);
                ++index;
            }
        }
        if (SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::ConditionalPreprocessorTree)) {
            AddTerminalConditionalListCommas(model, *child);
        }
    }
    if (
        SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::ConditionalPreprocessorTree) &&
        !SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::ConditionalPreprocessorOpen)
    ) {
        const std::optional<size_t> previous = PreviousNonTriviaChildIndex(children, children.size());
        if (
            previous &&
            *previous >= headerChildren &&
            children[*previous]->kind != SyntaxNodeKind::Comma &&
            !SyntaxNodeKindHasClass(children[*previous]->kind, SyntaxNodeClass::ConditionalPreprocessorTree)
        ) {
            InsertCommaAfter(model, node, *previous);
        }
    }
}

void NormalizeTrailingCommas(FormatModel& model, SyntaxNode& node) {
    SyntaxChildList& children = node.children;
    for (size_t index = 0; index < children.size(); ++index) {
        if (children[index] == nullptr) {
            continue;
        }
        if (
            children[index]->kind != SyntaxNodeKind::RightBrace &&
            children[index]->kind != SyntaxNodeKind::RightParen &&
            children[index]->kind != SyntaxNodeKind::RightBracket &&
            children[index]->kind != SyntaxNodeKind::Greater
        ) {
            continue;
        }
        const std::optional<size_t> previous = PreviousNonTriviaChildIndex(children, index);
        if (!previous) {
            continue;
        }
        const bool braceList = children[index]->kind == SyntaxNodeKind::RightBrace &&
            SyntaxNodeHasClass(node, SyntaxNodeClass::AllowedListPreprocessorContainer);
        if (
            !braceList &&
            SyntaxNodeKindHasClass(children[*previous]->kind, SyntaxNodeClass::ConditionalPreprocessorTree)
        ) {
            RemoveTerminalConditionalListCommas(*children[*previous]);
        }
        if (braceList) {
            if (SyntaxNodeKindHasClass(children[*previous]->kind, SyntaxNodeClass::ConditionalPreprocessorTree)) {
                AddTerminalConditionalListCommas(model, *children[*previous]);
                continue;
            }
            if (
                children[*previous]->kind != SyntaxNodeKind::Comma &&
                children[*previous]->kind != SyntaxNodeKind::LeftBrace
            ) {
                InsertCommaAfter(model, node, *previous);
                ++index;
            }
            continue;
        }
        if (
            children[*previous]->kind == SyntaxNodeKind::Comma &&
            !SyntaxNodeHasClass(node, SyntaxNodeClass::PreserveTrailingComma)
        ) {
            children.erase(children.begin() + static_cast<std::ptrdiff_t>(*previous));
            --index;
        }
    }
}

bool IsEmptyStatementNode(const SyntaxNode& node) {
    if (node.kind == SyntaxNodeKind::Semicolon) {
        return true;
    }
    if (node.kind != SyntaxNodeKind::Tree) {
        return false;
    }
    const SyntaxNode* content = nullptr;
    for (const SyntaxNode* child : node.children) {
        if (child == nullptr || SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::Trivia)) {
            continue;
        }
        if (content != nullptr) {
            return false;
        }
        content = child;
    }
    return content != nullptr && content->kind == SyntaxNodeKind::Semicolon;
}

bool IsStatementKindThroughAttributes(const SyntaxNode& node, SyntaxNodeKind kind) {
    if (node.kind == kind) {
        return true;
    }
    if (node.kind != SyntaxNodeKind::AttributedStatement) {
        return false;
    }
    const std::optional<size_t> statementIndex = PreviousNonTriviaChildIndex(node.children, node.children.size());
    return statementIndex &&
        node.children[*statementIndex] != nullptr &&
        IsStatementKindThroughAttributes(*node.children[*statementIndex], kind);
}

bool IsBracedControlBody(const SyntaxNode& node) {
    return IsStatementKindThroughAttributes(node, SyntaxNodeKind::CompoundStatement);
}

void WrapControlBody(FormatModel& model, SyntaxNode& node, size_t childIndex) {
    if (
        childIndex >= node.children.size() ||
        (node.children[childIndex] != nullptr && IsBracedControlBody(*node.children[childIndex]))
    ) {
        return;
    }
    const bool emptyStatementBody =
        node.children[childIndex] != nullptr && IsEmptyStatementNode(*node.children[childIndex]);
    size_t firstBodyIndex = childIndex;
    while (
        firstBodyIndex > 0 &&
        node.children[firstBodyIndex - 1] != nullptr &&
        SyntaxNodeKindHasClass(node.children[firstBodyIndex - 1]->kind, SyntaxNodeClass::Comment)
    ) {
        --firstBodyIndex;
    }
    size_t lastBodyIndex = childIndex;
    while (
        lastBodyIndex + 1 < node.children.size() &&
        node.children[lastBodyIndex + 1] != nullptr &&
        node.children[lastBodyIndex + 1]->kind == SyntaxNodeKind::TrailingComment
    ) {
        ++lastBodyIndex;
    }

    SyntaxNode* compound = MakeSyntaxNode(model);
    compound->kind = SyntaxNodeKind::CompoundStatement;
    compound->parent = &node;
    compound->depth = node.depth + 1;
    compound->children.reserve(lastBodyIndex - firstBodyIndex + 3);
    AppendSyntaxChild(*compound, MakeSyntaxNode(model, SyntaxNodeKind::LeftBrace));
    for (size_t index = firstBodyIndex; index <= lastBodyIndex; ++index) {
        if (emptyStatementBody && index == childIndex) {
            continue;
        }
        AppendSyntaxChild(*compound, node.children[index]);
    }
    AppendSyntaxChild(*compound, MakeSyntaxNode(model, SyntaxNodeKind::RightBrace));

    node.children.erase(
        node.children.begin() + static_cast<std::ptrdiff_t>(firstBodyIndex),
        node.children.begin() + static_cast<std::ptrdiff_t>(lastBodyIndex + 1)
    );
    node.children.insert(node.children.begin() + static_cast<std::ptrdiff_t>(firstBodyIndex), compound);
}

std::optional<size_t> FindOnlyIfInBraceBlock(const SyntaxNode& node) {
    if (node.kind != SyntaxNodeKind::CompoundStatement) {
        return std::nullopt;
    }
    std::optional<size_t> ifIndex;
    for (size_t index = 0; index < node.children.size(); ++index) {
        const SyntaxNode* child = node.children[index];
        if (child == nullptr) {
            return std::nullopt;
        }
        if (child->kind == SyntaxNodeKind::LeftBrace || child->kind == SyntaxNodeKind::RightBrace) {
            continue;
        }
        if (IsStatementKindThroughAttributes(*child, SyntaxNodeKind::IfStatement) && !ifIndex) {
            ifIndex = index;
            continue;
        }
        return std::nullopt;
    }
    return ifIndex;
}

void NormalizeElseClauseBody(FormatModel& model, SyntaxNode& node) {
    for (size_t index = 0; index < node.children.size(); ++index) {
        if (node.children[index] == nullptr || node.children[index]->kind != SyntaxNodeKind::KeywordElse) {
            continue;
        }
        const std::optional<size_t> bodyIndex = NextStructuralChildIndex(node.children, index + 1);
        if (!bodyIndex) {
            return;
        }
        if (
            node.children[*bodyIndex] != nullptr &&
            IsStatementKindThroughAttributes(*node.children[*bodyIndex], SyntaxNodeKind::IfStatement)
        ) {
            return;
        }
        if (
            node.children[*bodyIndex] != nullptr && node.children[*bodyIndex]->kind == SyntaxNodeKind::CompoundStatement
        ) {
            std::optional<size_t> ifIndex = FindOnlyIfInBraceBlock(*node.children[*bodyIndex]);
            if (ifIndex) {
                node.children[*bodyIndex] = node.children[*bodyIndex]->children[*ifIndex];
                ReparentSyntaxNode(*node.children[*bodyIndex], &node);
                return;
            }
            return;
        }
        WrapControlBody(model, node, *bodyIndex);
        return;
    }
}

void NormalizeIfStatementBody(FormatModel& model, SyntaxNode& node) {
    size_t before = node.children.size();
    for (size_t index = 0; index < node.children.size(); ++index) {
        if (node.children[index] != nullptr && node.children[index]->kind == SyntaxNodeKind::ElseClause) {
            before = index;
            break;
        }
    }
    const std::optional<size_t> consequenceIndex = PreviousStructuralChildIndex(node.children, before);
    if (consequenceIndex) {
        WrapControlBody(model, node, *consequenceIndex);
    }
}

void NormalizeDoStatementBody(FormatModel& model, SyntaxNode& node) {
    for (size_t index = 0; index < node.children.size(); ++index) {
        if (node.children[index] == nullptr || node.children[index]->kind != SyntaxNodeKind::KeywordWhile) {
            continue;
        }
        const std::optional<size_t> bodyIndex = PreviousStructuralChildIndex(node.children, index);
        if (bodyIndex) {
            WrapControlBody(model, node, *bodyIndex);
        }
        return;
    }
}

void NormalizeLastControlBody(FormatModel& model, SyntaxNode& node) {
    const std::optional<size_t> bodyIndex = PreviousStructuralChildIndex(node.children, node.children.size());
    if (bodyIndex) {
        WrapControlBody(model, node, *bodyIndex);
    }
}

void NormalizeControlBodies(FormatModel& model, SyntaxNode& node) {
    switch (node.kind) {
        case SyntaxNodeKind::IfStatement:
            NormalizeIfStatementBody(model, node);
            return;
        case SyntaxNodeKind::ElseClause:
            NormalizeElseClauseBody(model, node);
            return;
        case SyntaxNodeKind::ForStatement:
        case SyntaxNodeKind::WhileStatement:
        case SyntaxNodeKind::SwitchStatement:
            NormalizeLastControlBody(model, node);
            return;
        case SyntaxNodeKind::DoStatement:
            NormalizeDoStatementBody(model, node);
            return;
        default:
            return;
    }
}

void NormalizeColonPrefixedListComments(SyntaxNode& node) {
    for (size_t initializerIndex = 0; initializerIndex < node.children.size(); ++initializerIndex) {
        SyntaxNode* initializerList = node.children[initializerIndex];
        if (initializerList == nullptr || (
            initializerList->kind != SyntaxNodeKind::FieldInitializerList &&
            initializerList->kind != SyntaxNodeKind::BaseClassClause
        )) {
            continue;
        }
        size_t commentBegin = initializerIndex;
        while (commentBegin > 0) {
            const SyntaxNode* previous = node.children[commentBegin - 1];
            if (
                previous == nullptr ||
                (previous->kind != SyntaxNodeKind::Comment && previous->kind != SyntaxNodeKind::TrailingComment)
            ) {
                break;
            }
            --commentBegin;
        }
        if (commentBegin == initializerIndex) {
            return;
        }
        const size_t colonIndex = static_cast<size_t>(
            std::find_if(
                initializerList->children.begin(), initializerList->children.end(), [](const SyntaxNode* child) {
                    return child != nullptr && child->kind == SyntaxNodeKind::Colon;
                }
            ) - initializerList->children.begin()
        );
        if (colonIndex >= initializerList->children.size()) {
            return;
        }
        std::vector<SyntaxNode*> comments;
        comments.insert(
            comments.end(),
            node.children.begin() + static_cast<std::ptrdiff_t>(commentBegin),
            node.children.begin() + static_cast<std::ptrdiff_t>(initializerIndex)
        );
        node.children.erase(
            node.children.begin() + static_cast<std::ptrdiff_t>(commentBegin),
            node.children.begin() + static_cast<std::ptrdiff_t>(initializerIndex)
        );
        initializerList->children.insert(
            initializerList->children.begin() + static_cast<std::ptrdiff_t>(colonIndex + 1),
            comments.begin(),
            comments.end()
        );
        for (SyntaxNode* comment : comments) {
            if (comment != nullptr) {
                ReparentSyntaxNode(*comment, initializerList);
            }
        }
        return;
    }
}

void NormalizeLeadingStreamComments(SyntaxNode& node) {
    for (size_t index = 0; index < node.children.size(); ++index) {
        SyntaxNode* chain = node.children[index];
        if (chain == nullptr || !SyntaxNodeHasClass(*chain, SyntaxNodeClass::LeadingStreamOperatorChain)) {
            continue;
        }
        size_t commentBegin = index;
        while (commentBegin > 0) {
            const SyntaxNode* previous = node.children[commentBegin - 1];
            if (
                previous == nullptr ||
                (previous->kind != SyntaxNodeKind::Comment && previous->kind != SyntaxNodeKind::BlankLine)
            ) {
                break;
            }
            --commentBegin;
        }
        if (commentBegin == index) {
            continue;
        }
        const auto begin = node.children.begin() + static_cast<std::ptrdiff_t>(commentBegin);
        const auto end = node.children.begin() + static_cast<std::ptrdiff_t>(index);
        chain->children.insert(chain->children.begin(), begin, end);
        for (auto comment = begin; comment != end; ++comment) {
            ReparentSyntaxNode(**comment, chain);
        }
        node.children.erase(begin, end);
        index = commentBegin;
    }
}

void NormalizeAttachedTrailingBlockComment(SyntaxNode& node) {
    for (size_t index = 0; index < node.children.size(); ++index) {
        SyntaxNode* comment = node.children[index];
        if (
            comment == nullptr || comment->kind != SyntaxNodeKind::TrailingComment || !comment->text.starts_with("/*")
        ) {
            continue;
        }
        const std::optional<size_t> nextIndex = NextNonTriviaChildIndex(node.children, index + 1);
        if (nextIndex && node.children[*nextIndex] != nullptr && (
            SyntaxNodeKindHasClass(node.children[*nextIndex]->kind, SyntaxNodeClass::CompoundBlock) ||
            node.children[*nextIndex]->kind == SyntaxNodeKind::RequiresClause
        )) {
            comment->kind = SyntaxNodeKind::LexicalToken;
        }
    }
}

constexpr std::uint64_t kDeclarationGroupClasses = static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupType) |
    static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupForwardType) |
    static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupCallable) |
    static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupObject) |
    static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupAlias);

bool ContainsDeclarationSyntaxKind(const SyntaxNode& node, SyntaxNodeKind kind, bool root = true) {
    if (node.kind == kind) {
        return true;
    }
    if (!root && SyntaxNodeHasClass(node, SyntaxNodeClass::DeclarationScope)) {
        return false;
    }
    return std::any_of(node.children.begin(), node.children.end(), [kind](const SyntaxNode* child) {
        return child != nullptr && ContainsDeclarationSyntaxKind(*child, kind, false);
    });
}

const SyntaxNode* FirstNonTriviaChild(const SyntaxNode& node) {
    for (const SyntaxNode* child : node.children) {
        if (child != nullptr && !SyntaxNodeHasClass(*child, SyntaxNodeClass::Trivia)) {
            return child;
        }
    }
    return nullptr;
}

void ClassifyKeywordOwnedValue(SyntaxNode& node) {
    const SyntaxNode* first = FirstNonTriviaChild(node);
    if (first != nullptr && SyntaxNodeKindHasClass(first->kind, SyntaxNodeClass::KeywordOwnedValue)) {
        node.classes |= static_cast<std::uint64_t>(SyntaxNodeClass::KeywordOwnedValue);
    }
}

bool ContainsCallableDeclarator(const SyntaxNode& node, bool root = true) {
    if (node.kind == SyntaxNodeKind::OperatorCast) {
        return true;
    }
    if (node.kind == SyntaxNodeKind::FunctionDeclarator) {
        const SyntaxNode* target = FirstNonTriviaChild(node);
        if (
            target != nullptr &&
            target->kind != SyntaxNodeKind::ParenthesizedDeclarator &&
            target->kind != SyntaxNodeKind::AbstractParenthesizedDeclarator
        ) {
            return true;
        }
    }
    if (!root && SyntaxNodeHasClass(node, SyntaxNodeClass::DeclarationScope)) {
        return false;
    }
    return std::any_of(node.children.begin(), node.children.end(), [](const SyntaxNode* child) {
        return child != nullptr && ContainsCallableDeclarator(*child, false);
    });
}

bool IsTypeSpecifier(const SyntaxNode& node) {
    return SyntaxNodeHasClass(node, SyntaxNodeClass::DeclaredTypeSpecifier);
}

bool TypeSpecifierHasDefinitionBody(const SyntaxNode& node) {
    return std::any_of(node.children.begin(), node.children.end(), [](const SyntaxNode* child) {
        return child != nullptr && (
            SyntaxNodeHasClass(*child, SyntaxNodeClass::DeclarationScope) ||
            child->kind == SyntaxNodeKind::EnumeratorList
        );
    });
}

enum class DirectTypeDeclarationKind {
    None,
    Forward,
    Definition,
};

DirectTypeDeclarationKind DirectTypeDeclaration(const SyntaxNode& declaration) {
    for (size_t index = 0; index < declaration.children.size(); ++index) {
        const SyntaxNode* child = declaration.children[index];
        if (child == nullptr || !IsTypeSpecifier(*child)) {
            continue;
        }
        if (TypeSpecifierHasDefinitionBody(*child)) {
            return DirectTypeDeclarationKind::Definition;
        }
        const bool hasDeclarator = std::any_of(
            declaration.children.begin() + static_cast<std::ptrdiff_t>(index + 1),
            declaration.children.end(),
            [](const SyntaxNode* suffix) {
                return suffix != nullptr &&
                    !SyntaxNodeHasClass(*suffix, SyntaxNodeClass::Trivia) &&
                    suffix->kind != SyntaxNodeKind::Semicolon;
            }
        );
        return hasDeclarator ? DirectTypeDeclarationKind::None : DirectTypeDeclarationKind::Forward;
    }
    return DirectTypeDeclarationKind::None;
}

std::uint64_t SingleIntroducedDeclarationGroup(const SyntaxNode& node, bool root = true) {
    const std::uint64_t nodeGroup = node.classes & kDeclarationGroupClasses;
    if (nodeGroup != 0) {
        return nodeGroup;
    }
    if (node.kind == SyntaxNodeKind::MacroDefinition) {
        return 0;
    }
    if (!root && (
        SyntaxNodeHasClass(node, SyntaxNodeClass::DeclarationScope) ||
        SyntaxNodeHasClass(node, SyntaxNodeClass::ConditionalPreprocessorTree)
    )) {
        return 0;
    }
    std::uint64_t result = 0;
    for (const SyntaxNode* child : node.children) {
        if (child == nullptr) {
            continue;
        }
        const std::uint64_t childGroup = SingleIntroducedDeclarationGroup(*child, false);
        if (childGroup == 0) {
            continue;
        }
        if (result != 0) {
            return 0;
        }
        result = childGroup;
    }
    return result;
}

void ClassifyDeclarationGroup(SyntaxNode& node) {
    if ((node.classes & kDeclarationGroupClasses) != 0) {
        return;
    }
    if (IsTypeSpecifier(node)) {
        const SyntaxNodeClass group = TypeSpecifierHasDefinitionBody(node) ? SyntaxNodeClass::DeclarationGroupType :
            SyntaxNodeClass::DeclarationGroupForwardType;
        node.classes |= static_cast<std::uint64_t>(group);
        return;
    }
    if (node.kind == SyntaxNodeKind::AliasDeclaration || node.kind == SyntaxNodeKind::FunctionPointerAliasDeclaration) {
        node.classes |= static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupAlias);
        return;
    }
    if (node.kind == SyntaxNodeKind::FunctionDefinition) {
        node.classes |= static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupCallable);
        return;
    }
    if (node.kind == SyntaxNodeKind::Declaration || node.kind == SyntaxNodeKind::FieldDeclaration) {
        SyntaxNodeClass group = SyntaxNodeClass::DeclarationGroupObject;
        const DirectTypeDeclarationKind typeDeclaration = DirectTypeDeclaration(node);
        if (typeDeclaration == DirectTypeDeclarationKind::Definition) {
            group = SyntaxNodeClass::DeclarationGroupType;
        } else if (typeDeclaration == DirectTypeDeclarationKind::Forward) {
            group = SyntaxNodeClass::DeclarationGroupForwardType;
        } else if (ContainsDeclarationSyntaxKind(node, SyntaxNodeKind::KeywordTypedef)) {
            group = SyntaxNodeClass::DeclarationGroupAlias;
        } else if (ContainsCallableDeclarator(node)) {
            group = SyntaxNodeClass::DeclarationGroupCallable;
        }
        node.classes |= static_cast<std::uint64_t>(group);
        return;
    }
    if (
        node.parent == nullptr ||
        !SyntaxNodeHasClass(*node.parent, SyntaxNodeClass::DeclarationScope) ||
        SyntaxNodeHasClass(node, SyntaxNodeClass::ConditionalPreprocessorTree)
    ) {
        return;
    }
    std::uint64_t group = SingleIntroducedDeclarationGroup(node);
    if (
        node.kind == SyntaxNodeKind::TemplateInstantiation &&
        group == static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupForwardType)
    ) {
        group = static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationGroupType);
    }
    node.classes |= group;
}

bool ContainsConditionalPreprocessor(const SyntaxNode& node) {
    if (SyntaxNodeHasClass(node, SyntaxNodeClass::ConditionalPreprocessorTree)) {
        return true;
    }
    return std::any_of(node.children.begin(), node.children.end(), [](const SyntaxNode* child) {
        return child != nullptr && ContainsConditionalPreprocessor(*child);
    });
}

}  // namespace

void NormalizeSyntaxNode(FormatModel& model, SyntaxNode& node) {
    if (SyntaxNodeHasClass(node, SyntaxNodeClass::PreprocessorSplitList) && ContainsConditionalPreprocessor(node)) {
        // Only preprocessor-split lists query this immutable descendant predicate. Materializing it on the owning
        // node is equivalent to the printer's former recursive memoization and avoids its hash table.
        node.classes |= static_cast<std::uint64_t>(SyntaxNodeClass::ContainsConditionalPreprocessor);
    }
    if (
        SyntaxNodeHasClass(node, SyntaxNodeClass::ConditionalPreprocessorTree) &&
        std::any_of(node.children.begin(), node.children.end(), [](const SyntaxNode* child) {
            return child != nullptr && SyntaxNodeHasClass(*child, SyntaxNodeClass::DeclarationModifierPreprocessor);
        })
    ) {
        node.classes |= static_cast<std::uint64_t>(SyntaxNodeClass::AtomicPreprocessor) |
            static_cast<std::uint64_t>(SyntaxNodeClass::SupportedPreprocessorPlacement) |
            static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationModifierPreprocessor);
    }
    if (node.kind == SyntaxNodeKind::BinaryExpression) {
        const bool startsConditionalStream =
            std::any_of(node.children.begin(), node.children.end(), [](const SyntaxNode* child) {
                return child != nullptr && SyntaxNodeHasClass(*child, SyntaxNodeClass::ConditionalPreprocessorTree);
            });
        const bool continuesConditionalStream =
            std::any_of(node.children.begin(), node.children.end(), [](const SyntaxNode* child) {
                return child != nullptr && SyntaxNodeHasClass(*child, SyntaxNodeClass::ConditionalStreamOperatorChain);
            }) && std::any_of(node.children.begin(), node.children.end(), [](const SyntaxNode* child) {
                return child != nullptr &&
                    (child->kind == SyntaxNodeKind::LessLess || child->kind == SyntaxNodeKind::GreaterGreater);
            });
        if (startsConditionalStream || continuesConditionalStream) {
            node.classes |= static_cast<std::uint64_t>(SyntaxNodeClass::ConditionalStreamOperatorChain);
        }
    }
    ClassifyKeywordOwnedValue(node);
    ClassifyDeclarationGroup(node);
    NormalizeTrailingCommas(model, node);
    NormalizeControlBodies(model, node);
    NormalizeColonPrefixedListComments(node);
    NormalizeLeadingStreamComments(node);
    NormalizeAttachedTrailingBlockComment(node);
}

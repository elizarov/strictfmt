#include "format/impl/format_model_builder.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "tools/tools_common.h"

namespace {

std::string_view NodeText(TSNode node, std::string_view source) {
    const uint32_t start = ts_node_start_byte(node);
    const uint32_t end = ts_node_end_byte(node);
    if (start > end || end > source.size()) {
        return {};
    }
    return source.substr(start, end - start);
}

bool ContainsBlankLine(std::string_view source, uint32_t firstEnd, uint32_t secondStart) {
    if (firstEnd >= secondStart || secondStart > source.size()) {
        return false;
    }
    int lineBreaks = 0;
    bool sawNonWhitespace = false;
    for (size_t index = firstEnd; index < secondStart; ++index) {
        const char ch = source[index];
        if (ch == '\r' || ch == '\n') {
            ++lineBreaks;
            if (ch == '\r' && index + 1 < secondStart && source[index + 1] == '\n') {
                ++index;
            }
            if (lineBreaks >= 2 && !sawNonWhitespace) {
                return true;
            }
            continue;
        }
        if (ch != ' ' && ch != '\t' && ch != '\v' && ch != '\f') {
            sawNonWhitespace = true;
        }
    }
    return lineBreaks >= 2 && !sawNonWhitespace;
}

std::string_view TrimLeadingWhitespace(std::string_view value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
        value.remove_prefix(1);
    }
    return value;
}

SyntaxNode* MakeNode(FormatModel& model) { return &model.nodes.emplace_back(model.childStorage.get()); }

void SetParentRecursive(SyntaxNode& node, const SyntaxNode* parent) {
    node.parent = parent;
    node.depth = parent == nullptr ? 0 : parent->depth + 1;
    for (SyntaxNode* child : node.children) {
        if (child != nullptr) {
            SetParentRecursive(*child, &node);
        }
    }
}

void AppendChild(SyntaxNode& parent, SyntaxNode* child) {
    if (child != nullptr) {
        SetParentRecursive(*child, &parent);
    }
    parent.children.push_back(child);
}

SyntaxNode* MakeBlankLine(FormatModel& model) {
    SyntaxNode* node = MakeNode(model);
    node->kind = SyntaxNodeKind::BlankLine;
    return node;
}

SyntaxNode* MakeTokenNode(FormatModel& model, SyntaxNodeKind token) {
    SyntaxNode* node = MakeNode(model);
    node->kind = token;
    return node;
}

void SetKnownTokenNode(SyntaxNode& node, SyntaxNodeKind token, std::string_view text) {
    node.kind = token;
    if (
        text != SyntaxNodeKindTokenText(token) && !SyntaxNodeKindHasClass(token, SyntaxNodeClass::PreprocessorDirective)
    ) {
        node.text = text;
    }
}

bool CommentConsumesLineTail(std::string_view source, uint32_t commentStart, uint32_t commentEnd) {
    if (commentStart + 1 >= source.size()) {
        return true;
    }
    if (source[commentStart] == '/' && source[commentStart + 1] == '/') {
        return true;
    }
    for (size_t index = commentEnd; index < source.size(); ++index) {
        const char ch = source[index];
        if (ch == '\r' || ch == '\n') {
            return true;
        }
        if (ch == '\\') {
            for (size_t tail = index + 1; tail < source.size(); ++tail) {
                const char tailCh = source[tail];
                if (tailCh == '\r' || tailCh == '\n') {
                    return true;
                }
                if (tailCh != ' ' && tailCh != '\t') {
                    return false;
                }
            }
            return true;
        }
        if (ch != ' ' && ch != '\t') {
            return false;
        }
    }
    return true;
}

bool IsBlockComment(std::string_view source, uint32_t commentStart) {
    return commentStart + 1 < source.size() && source[commentStart] == '/' && source[commentStart + 1] == '*';
}

bool SyntaxNodeHasClass(const SyntaxNode& node, SyntaxNodeClass syntaxNodeClass) {
    return (node.classes & static_cast<std::uint64_t>(syntaxNodeClass)) != 0 ||
        SyntaxNodeKindHasClass(node.kind, syntaxNodeClass);
}

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
    SyntaxNode* comma = MakeTokenNode(model, SyntaxNodeKind::Comma);
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

bool IsBracedControlBody(const SyntaxNode& node) {
    if (node.kind == SyntaxNodeKind::CompoundStatement) {
        return true;
    }
    if (node.kind != SyntaxNodeKind::AttributedStatement) {
        return false;
    }
    const std::optional<size_t> statementIndex = PreviousNonTriviaChildIndex(node.children, node.children.size());
    return statementIndex &&
        node.children[*statementIndex] != nullptr &&
        IsBracedControlBody(*node.children[*statementIndex]);
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

    SyntaxNode* compound = MakeNode(model);
    compound->kind = SyntaxNodeKind::CompoundStatement;
    compound->parent = &node;
    compound->depth = node.depth + 1;
    compound->children.reserve(lastBodyIndex - firstBodyIndex + 3);
    AppendChild(*compound, MakeTokenNode(model, SyntaxNodeKind::LeftBrace));
    for (size_t index = firstBodyIndex; index <= lastBodyIndex; ++index) {
        if (emptyStatementBody && index == childIndex) {
            continue;
        }
        AppendChild(*compound, node.children[index]);
    }
    AppendChild(*compound, MakeTokenNode(model, SyntaxNodeKind::RightBrace));

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
        if (child->kind == SyntaxNodeKind::IfStatement && !ifIndex) {
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
        const std::optional<size_t> bodyIndex = NextNonTriviaChildIndex(node.children, index + 1);
        if (!bodyIndex) {
            return;
        }
        if (node.children[*bodyIndex] != nullptr && node.children[*bodyIndex]->kind == SyntaxNodeKind::IfStatement) {
            return;
        }
        if (
            node.children[*bodyIndex] != nullptr && node.children[*bodyIndex]->kind == SyntaxNodeKind::CompoundStatement
        ) {
            std::optional<size_t> ifIndex = FindOnlyIfInBraceBlock(*node.children[*bodyIndex]);
            if (ifIndex) {
                node.children[*bodyIndex] = node.children[*bodyIndex]->children[*ifIndex];
                SetParentRecursive(*node.children[*bodyIndex], &node);
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
    const std::optional<size_t> consequenceIndex = PreviousNonTriviaChildIndex(node.children, before);
    if (consequenceIndex) {
        WrapControlBody(model, node, *consequenceIndex);
    }
}

void NormalizeDoStatementBody(FormatModel& model, SyntaxNode& node) {
    for (size_t index = 0; index < node.children.size(); ++index) {
        if (node.children[index] == nullptr || node.children[index]->kind != SyntaxNodeKind::KeywordWhile) {
            continue;
        }
        const std::optional<size_t> bodyIndex = PreviousNonTriviaChildIndex(node.children, index);
        if (bodyIndex) {
            WrapControlBody(model, node, *bodyIndex);
        }
        return;
    }
}

void NormalizeLastControlBody(FormatModel& model, SyntaxNode& node) {
    const std::optional<size_t> bodyIndex = PreviousNonTriviaChildIndex(node.children, node.children.size());
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
                SetParentRecursive(*comment, initializerList);
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
            SetParentRecursive(**comment, chain);
        }
        node.children.erase(begin, end);
        index = commentBegin;
    }
}

void NormalizeBlockHeaderTrailingComment(SyntaxNode& node) {
    for (size_t index = 0; index < node.children.size(); ++index) {
        SyntaxNode* comment = node.children[index];
        if (
            comment == nullptr || comment->kind != SyntaxNodeKind::TrailingComment || !comment->text.starts_with("/*")
        ) {
            continue;
        }
        const std::optional<size_t> nextIndex = NextNonTriviaChildIndex(node.children, index + 1);
        if (
            nextIndex &&
            node.children[*nextIndex] != nullptr &&
            SyntaxNodeKindHasClass(node.children[*nextIndex]->kind, SyntaxNodeClass::CompoundBlock)
        ) {
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

std::uint64_t SingleIntroducedDeclarationGroup(const SyntaxNode& node) {
    std::uint64_t result = 0;
    for (const SyntaxNode* child : node.children) {
        if (child == nullptr) {
            continue;
        }
        const std::uint64_t childGroup = child->classes & kDeclarationGroupClasses;
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

bool HasBinaryExpressionAncestorThroughWrappers(const SyntaxNode& node) {
    const SyntaxNode* ancestor = node.parent;
    while (ancestor != nullptr && ancestor->kind == SyntaxNodeKind::Tree) {
        ancestor = ancestor->parent;
    }
    return ancestor != nullptr && ancestor->kind == SyntaxNodeKind::BinaryExpression;
}

void CollectNonTriviaLeaves(SyntaxNode& node, std::vector<SyntaxNode*>& leaves) {
    if (node.children.empty()) {
        if (!SyntaxNodeHasClass(node, SyntaxNodeClass::Trivia)) {
            leaves.push_back(&node);
        }
        return;
    }
    for (SyntaxNode* child : node.children) {
        if (child != nullptr) {
            CollectNonTriviaLeaves(*child, leaves);
        }
    }
}

bool HasQualifiedNameAncestor(const SyntaxNode& node, const SyntaxNode& root) {
    for (const SyntaxNode* ancestor = node.parent; ancestor != nullptr; ancestor = ancestor->parent) {
        if (SyntaxNodeHasClass(*ancestor, SyntaxNodeClass::QualifiedName)) {
            return true;
        }
        if (ancestor == &root) {
            return false;
        }
    }
    return false;
}

bool HasDistinctiveTemplateArgument(const std::vector<SyntaxNode*>& leaves, size_t first, size_t last) {
    for (size_t index = first; index < last; ++index) {
        const SyntaxNodeKind kind = leaves[index]->kind;
        if (
            kind == SyntaxNodeKind::NumberLiteral ||
            kind == SyntaxNodeKind::CharacterLiteral ||
            kind == SyntaxNodeKind::KeywordTrue ||
            kind == SyntaxNodeKind::KeywordFalse ||
            kind == SyntaxNodeKind::KeywordNullptr ||
            kind == SyntaxNodeKind::KeywordSizeof ||
            kind == SyntaxNodeKind::KeywordAlignof ||
            kind == SyntaxNodeKind::Ellipsis ||
            kind == SyntaxNodeKind::Comma ||
            kind == SyntaxNodeKind::ColonColon
        ) {
            return true;
        }
    }
    return false;
}

bool IsRecoveredTemplateSuffix(SyntaxNodeKind suffix, bool hasDeclarationTemplateEvidence) {
    if (suffix == SyntaxNodeKind::ColonColon || suffix == SyntaxNodeKind::LeftParen) {
        return true;
    }
    if (!hasDeclarationTemplateEvidence) {
        return false;
    }
    return suffix == SyntaxNodeKind::Identifier ||
        suffix == SyntaxNodeKind::LeftBrace ||
        suffix == SyntaxNodeKind::Star ||
        suffix == SyntaxNodeKind::Ampersand ||
        suffix == SyntaxNodeKind::AmpersandAmpersand ||
        suffix == SyntaxNodeKind::KeywordConst ||
        suffix == SyntaxNodeKind::KeywordVolatile;
}

void RecoverAmbiguousTemplateDelimiters(SyntaxNode& node) {
    if (node.kind != SyntaxNodeKind::BinaryExpression || HasBinaryExpressionAncestorThroughWrappers(node)) {
        return;
    }
    std::vector<SyntaxNode*> leaves;
    CollectNonTriviaLeaves(node, leaves);
    struct LessDelimiter {
        size_t index;
        int groupDepth;
    };
    std::vector<LessDelimiter> lessDelimiters;
    int groupDepth = 0;
    for (size_t index = 0; index < leaves.size(); ++index) {
        if (
            leaves[index]->kind == SyntaxNodeKind::LeftParen ||
            leaves[index]->kind == SyntaxNodeKind::LeftBracket ||
            leaves[index]->kind == SyntaxNodeKind::LeftBrace
        ) {
            ++groupDepth;
            continue;
        }
        if (
            leaves[index]->kind == SyntaxNodeKind::RightParen ||
            leaves[index]->kind == SyntaxNodeKind::RightBracket ||
            leaves[index]->kind == SyntaxNodeKind::RightBrace
        ) {
            while (!lessDelimiters.empty() && lessDelimiters.back().groupDepth >= groupDepth) {
                lessDelimiters.pop_back();
            }
            --groupDepth;
            continue;
        }
        if (
            leaves[index]->kind == SyntaxNodeKind::AmpersandAmpersand || leaves[index]->kind == SyntaxNodeKind::PipePipe
        ) {
            while (!lessDelimiters.empty() && lessDelimiters.back().groupDepth == groupDepth) {
                lessDelimiters.pop_back();
            }
            continue;
        }
        if (leaves[index]->kind == SyntaxNodeKind::Less) {
            lessDelimiters.push_back({.index = index, .groupDepth = groupDepth});
            continue;
        }
        if (
            leaves[index]->kind != SyntaxNodeKind::Greater ||
            lessDelimiters.empty() ||
            lessDelimiters.back().groupDepth != groupDepth
        ) {
            continue;
        }
        const size_t lessIndex = lessDelimiters.back().index;
        lessDelimiters.pop_back();
        if (lessIndex == 0 || index + 1 >= leaves.size()) {
            continue;
        }
        const bool hasQualifiedName = HasQualifiedNameAncestor(*leaves[lessIndex - 1], node);
        const bool hasDeclarationTemplateEvidence =
            hasQualifiedName || HasDistinctiveTemplateArgument(leaves, lessIndex + 1, index);
        if (!IsRecoveredTemplateSuffix(leaves[index + 1]->kind, hasDeclarationTemplateEvidence)) {
            continue;
        }
        leaves[lessIndex]->classes |= static_cast<std::uint64_t>(SyntaxNodeClass::RecoveredTemplateDelimiter);
        leaves[index]->classes |= static_cast<std::uint64_t>(SyntaxNodeClass::RecoveredTemplateDelimiter);
    }
}

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
    RecoverAmbiguousTemplateDelimiters(node);
    ClassifyKeywordOwnedValue(node);
    ClassifyDeclarationGroup(node);
    NormalizeTrailingCommas(model, node);
    NormalizeControlBodies(model, node);
    NormalizeColonPrefixedListComments(node);
    NormalizeLeadingStreamComments(node);
    NormalizeBlockHeaderTrailingComment(node);
}

struct TsNodeSyntax {
    TSSymbol symbol = 0;
    SyntaxNodeKind kind = SyntaxNodeKind::Unknown;
    SyntaxNodeKind tokenKind = SyntaxNodeKind::Unknown;
    std::uint64_t classes = 0;
    SyntaxWrapperRole wrapperRole = SyntaxWrapperRole::None;
};

inline TsNodeSyntax GetTsNodeSyntax(TSNode tsNode) {
    const TSSymbol symbol = ts_node_symbol(tsNode);
    const SyntaxSymbolInfo info = SyntaxSymbolInfoForSymbol(symbol);
    return {
        .symbol = symbol,
        .kind = info.treeKind,
        .tokenKind = info.tokenKind,
        .classes = info.classes,
        .wrapperRole = info.wrapperRole,
    };
}

bool TsNodeSyntaxHasClass(TsNodeSyntax syntax, SyntaxNodeClass syntaxNodeClass) {
    return (syntax.classes & static_cast<std::uint64_t>(syntaxNodeClass)) != 0;
}

void AppendTsChildren(
    FormatModel& model, TSNode tsNode, std::string_view source, SyntaxNode& parent, uint32_t childCount
);

SyntaxNode*
    BuildNode(FormatModel& model, TSNode tsNode, std::string_view source, const SyntaxNode* parent, TsNodeSyntax syntax)
{
    SyntaxNode* node = MakeNode(model);
    node->parent = parent;
    node->depth = parent == nullptr ? 0 : parent->depth + 1;
    node->classes = syntax.classes;

    if (ts_node_is_missing(tsNode)) {
        node->kind = SyntaxNodeKind::Missing;
        node->text = ts_node_type(tsNode);
        return node;
    }

    if (std::string_view(ts_node_type(tsNode)) == "ERROR") {
        node->kind = SyntaxNodeKind::Error;
        node->text = NodeText(tsNode, source);
        const uint32_t childCount = ts_node_child_count(tsNode);
        node->children.reserve(childCount);
        AppendTsChildren(model, tsNode, source, *node, childCount);
        return node;
    }

    if (syntax.kind == SyntaxNodeKind::Comment) {
        node->kind = SyntaxNodeKind::Comment;
        std::string_view commentText = NodeText(tsNode, source);
        while (!commentText.empty() && (commentText.back() == '\r' || commentText.back() == '\n')) {
            commentText.remove_suffix(1);
        }
        node->text = commentText;
        return node;
    }

    if (syntax.wrapperRole == SyntaxWrapperRole::LexicalWrapper) {
        const std::string_view text = NodeText(tsNode, source);
        const SyntaxNodeKind known = SyntaxNodeKindFromTokenText(text);
        if (known != SyntaxNodeKind::Unknown) {
            SetKnownTokenNode(*node, known, text);
            return node;
        }
    }
    if (
        TsNodeSyntaxHasClass(syntax, SyntaxNodeClass::OpaqueSource) ||
        TsNodeSyntaxHasClass(syntax, SyntaxNodeClass::LexicalAtom)
    ) {
        node->kind = syntax.kind;
        node->text = NodeText(tsNode, source);
        return node;
    }

    const uint32_t childCount = ts_node_child_count(tsNode);
    if (syntax.tokenKind != SyntaxNodeKind::Unknown && childCount == 0) {
        SetKnownTokenNode(*node, syntax.tokenKind, NodeText(tsNode, source));
        return node;
    }

    if (childCount == 0) {
        const std::string_view text = NodeText(tsNode, source);
        const SyntaxNodeKind knownFromText = SyntaxNodeKindFromTokenText(text);
        if (knownFromText != SyntaxNodeKind::Unknown) {
            SetKnownTokenNode(*node, knownFromText, text);
            return node;
        }
        node->kind = syntax.kind == SyntaxNodeKind::Unknown ? SyntaxNodeKind::LexicalToken : syntax.kind;
        node->text = text;
        return node;
    }

    node->kind = syntax.kind == SyntaxNodeKind::Unknown ? SyntaxNodeKind::Tree : syntax.kind;
    if (
        TsNodeSyntaxHasClass(syntax, SyntaxNodeClass::AtomicPreprocessor) ||
        SyntaxNodeKindHasClass(node->kind, SyntaxNodeClass::ConditionalPreprocessorTree)
    ) {
        node->text = NodeText(tsNode, source);
    }
    node->children.reserve(childCount);
    AppendTsChildren(model, tsNode, source, *node, childCount);
    NormalizeSyntaxNode(model, *node);
    return node;
}

inline void AppendTsNode(
    FormatModel& model,
    TSNode tsNode,
    std::string_view source,
    SyntaxNode& parent,
    TsNodeSyntax syntax,
    bool isTrailingComment,
    bool isInlineBlockComment
) {
    if (syntax.wrapperRole == SyntaxWrapperRole::Flatten) {
        AppendTsChildren(model, tsNode, source, parent, ts_node_child_count(tsNode));
        return;
    }
    SyntaxNode* childNode = BuildNode(model, tsNode, source, &parent, syntax);
    if (isTrailingComment && childNode->kind == SyntaxNodeKind::Comment) {
        childNode->kind = SyntaxNodeKind::TrailingComment;
    } else if (isInlineBlockComment && childNode->kind == SyntaxNodeKind::Comment) {
        childNode->kind = SyntaxNodeKind::LexicalToken;
    }
    parent.children.push_back(childNode);
}

inline void AppendTsChild(
    FormatModel& model,
    TSNode child,
    uint32_t childEnd,
    uint32_t childEndRow,
    uint32_t childEndColumn,
    std::string_view source,
    SyntaxNode& parent,
    uint32_t& previousEnd,
    uint32_t& previousEndRow,
    uint32_t& previousEndColumn,
    bool& hasPreviousSibling
) {
    const TsNodeSyntax childSyntax = GetTsNodeSyntax(child);
    const uint32_t childStart = ts_node_start_byte(child);
    const uint32_t childStartRow = ts_node_start_point(child).row;
    if (hasPreviousSibling && (
        ContainsBlankLine(source, previousEnd, childStart) || (previousEndColumn == 0 && childStartRow > previousEndRow)
    )) {
        AppendChild(parent, MakeBlankLine(model));
    }
    const bool isComment = childSyntax.kind == SyntaxNodeKind::Comment;
    const bool isBlock = isComment && IsBlockComment(source, childStart);
    const bool consumesLineTail = !isComment || CommentConsumesLineTail(source, childStart, childEnd);
    const bool isTrailingComment =
        isComment && hasPreviousSibling && previousEndRow == childStartRow && previousEndColumn > 0 && consumesLineTail;
    const bool isInlineBlockComment = isBlock && !consumesLineTail;
    AppendTsNode(model, child, source, parent, childSyntax, isTrailingComment, isInlineBlockComment);
    previousEnd = childEnd;
    previousEndRow = childEndRow;
    previousEndColumn = childEndColumn;
    hasPreviousSibling = true;
}

void AppendTsChildren(
    FormatModel& model, TSNode tsNode, std::string_view source, SyntaxNode& parent, uint32_t childCount
) {
    if (childCount == 0) {
        return;
    }

    uint32_t previousEnd = ts_node_start_byte(tsNode);
    uint32_t previousEndRow = ts_node_start_point(tsNode).row;
    uint32_t previousEndColumn = ts_node_start_point(tsNode).column;
    bool hasPreviousSibling = !parent.children.empty();
    for (uint32_t index = 0; index < childCount; ++index) {
        TSNode child = ts_node_child(tsNode, index);
        const TSPoint childEndPoint = ts_node_end_point(child);
        AppendTsChild(
            model,
            child,
            ts_node_end_byte(child),
            childEndPoint.row,
            childEndPoint.column,
            source,
            parent,
            previousEnd,
            previousEndRow,
            previousEndColumn,
            hasPreviousSibling
        );
    }
}

struct ProblemNode {
    bool missing = false;
    TSNode node = {};
};

void CollectProblemNodes(TSNode node, std::vector<ProblemNode>& problems) {
    if (ts_node_is_missing(node)) {
        problems.push_back({.missing = true, .node = node});
        return;
    }
    if (std::string_view(ts_node_type(node)) == "ERROR") {
        problems.push_back({.missing = false, .node = node});
        return;
    }

    const uint32_t childCount = ts_node_child_count(node);
    for (uint32_t index = 0; index < childCount; ++index) {
        TSNode child = ts_node_child(node, index);
        if (!ts_node_has_error(child) && !ts_node_is_missing(child)) {
            continue;
        }
        CollectProblemNodes(child, problems);
    }
}

void AppendIncludeRun(
    SyntaxChildList& sourceChildren,
    size_t& index,
    SyntaxChildList& groupedChildren,
    FormatModel& model,
    SyntaxNode& root
) {
    SyntaxNode* includeRun = MakeNode(model);
    includeRun->kind = SyntaxNodeKind::IncludeRun;
    includeRun->parent = &root;
    includeRun->depth = root.depth + 1;

    for (; index < sourceChildren.size(); ++index) {
        if (
            sourceChildren[index] != nullptr &&
            SyntaxNodeKindHasClass(sourceChildren[index]->kind, SyntaxNodeClass::IncludeDirective)
        ) {
            AppendChild(*includeRun, sourceChildren[index]);
            continue;
        }
        if (sourceChildren[index] != nullptr && sourceChildren[index]->kind == SyntaxNodeKind::BlankLine) {
            size_t nextIndex = index + 1;
            while (
                nextIndex < sourceChildren.size() &&
                sourceChildren[nextIndex] != nullptr &&
                sourceChildren[nextIndex]->kind == SyntaxNodeKind::BlankLine
            ) {
                ++nextIndex;
            }
            if (
                nextIndex < sourceChildren.size() &&
                sourceChildren[nextIndex] != nullptr &&
                SyntaxNodeKindHasClass(sourceChildren[nextIndex]->kind, SyntaxNodeClass::IncludeDirective)
            ) {
                AppendChild(*includeRun, sourceChildren[index]);
                index = nextIndex - 1;
                continue;
            }
            index = nextIndex;
            break;
        }
        break;
    }

    groupedChildren.push_back(includeRun);
}

bool IsPragmaNode(const SyntaxNode& node) {
    return node.kind == SyntaxNodeKind::PreprocCall &&
        SyntaxNodeKindFromPreprocessorDirectiveLine(TrimLeadingWhitespace(node.text)) ==
            SyntaxNodeKind::PreprocessorDirectivePragma;
}

bool IsPreprocessorConditionHeaderNode(const SyntaxNode& node) {
    return node.kind == SyntaxNodeKind::LexicalToken ||
        node.kind == SyntaxNodeKind::Identifier ||
        SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::PreprocessorDirective);
}

bool CanRemainInOpeningIncludeArea(const SyntaxNode& owner, const SyntaxNode& child, bool sawInclude) {
    if (SyntaxNodeKindHasClass(child.kind, SyntaxNodeClass::Trivia)) {
        return true;
    }
    if (sawInclude) {
        return false;
    }
    if (owner.kind == SyntaxNodeKind::TranslationUnit) {
        return IsPragmaNode(child);
    }
    if (SyntaxNodeKindHasClass(owner.kind, SyntaxNodeClass::ConditionalPreprocessorOpen)) {
        return child.kind == SyntaxNodeKind::MacroDefinition || IsPreprocessorConditionHeaderNode(child);
    }
    return false;
}

void GroupOpeningIncludeRuns(FormatModel& model, SyntaxNode& root) {
    for (SyntaxNode* child : root.children) {
        if (child != nullptr) {
            GroupOpeningIncludeRuns(model, *child);
        }
    }

    if (
        root.kind != SyntaxNodeKind::TranslationUnit &&
        !SyntaxNodeKindHasClass(root.kind, SyntaxNodeClass::ConditionalPreprocessorOpen)
    ) {
        return;
    }

    SyntaxChildList groupedChildren(root.children.get_allocator());
    groupedChildren.reserve(root.children.size());
    bool sawInclude = false;
    bool inOpeningArea = true;
    for (size_t index = 0; index < root.children.size();) {
        if (
            inOpeningArea &&
            root.children[index] != nullptr &&
            SyntaxNodeKindHasClass(root.children[index]->kind, SyntaxNodeClass::IncludeDirective)
        ) {
            AppendIncludeRun(root.children, index, groupedChildren, model, root);
            sawInclude = true;
            continue;
        }
        const bool canRemainInOpeningArea = inOpeningArea &&
            root.children[index] != nullptr &&
            CanRemainInOpeningIncludeArea(root, *root.children[index], sawInclude);
        if (canRemainInOpeningArea) {
            groupedChildren.push_back(root.children[index]);
            ++index;
            continue;
        }

        inOpeningArea = false;
        groupedChildren.push_back(root.children[index]);
        ++index;
    }

    root.children = std::move(groupedChildren);
}

std::string ParseProblemMessage(const ProblemNode& problem) {
    const TSPoint point = ts_node_start_point(problem.node);
    const std::string nodeType = problem.missing ? "missing " + std::string(ts_node_type(problem.node)) :
        std::string(ts_node_type(problem.node));
    return "parse failed at " +
        std::to_string(static_cast<int>(point.row) + 1) +
        ":" +
        std::to_string(static_cast<int>(point.column) + 1) +
        " near " +
        nodeType;
}

ParseResult ParseFailure(TSNode root) {
    std::vector<ProblemNode> problems;
    CollectProblemNodes(root, problems);
    if (problems.empty()) {
        problems.push_back({.missing = false, .node = root});
    }

    ParseResult parse;
    parse.ok = false;
    for (const ProblemNode& problem : problems) {
        if (!parse.error.empty()) {
            parse.error.push_back('\n');
        }
        parse.error += ParseProblemMessage(problem);
    }
    return parse;
}

}  // namespace

FormatModel BuildFormatModel(TSNode root, std::unique_ptr<std::string> sourceText) {
    FormatModel model;
    model.sourceText = std::move(sourceText);
    if (!model.sourceText) {
        model.parse.error = "formatter source ownership setup failed";
        return model;
    }

    const std::string_view source(*model.sourceText);
    model.nodes.reserve(source.size() * 2 + 64);

    const bool hasParseProblems = ts_node_has_error(root) || ts_node_is_missing(root);
    if (hasParseProblems) {
        model.parse = ParseFailure(root);
    }

    model.root = BuildNode(model, root, source, nullptr, GetTsNodeSyntax(root));
    GroupOpeningIncludeRuns(model, *model.root);
    if (!hasParseProblems) {
        model.parse.ok = true;
    }
    return model;
}

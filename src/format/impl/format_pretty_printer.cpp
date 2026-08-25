#include "format/impl/format_pretty_printer.h"

#include <algorithm>
#include <chrono>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "format/impl/format_break_model_builder.h"
#include "format/impl/format_break_model_inline_helpers.h"
#include "format/impl/format_break_solver.h"
#include "format/impl/format_include_sort.h"
#include "format/impl/format_raw_macro.h"
#include "format/impl/format_spacing.h"
#include "tools/tools_common.h"

namespace {

enum class BraceRole {
    Compact,
    Block,
    Enum,
    NamespaceLike,
    CaseBlock,
};

enum class DeclarationGroupKind {
    None,
    Type,
    Callable,
    Object,
    Alias,
};

enum class PreprocessorGroupKind {
    Other,
    Pragma,
    Undef,
};

struct PreprocessorGroup {
    const SyntaxNode* item = nullptr;
    PreprocessorGroupKind kind = PreprocessorGroupKind::Other;
};

struct BraceFrame {
    BraceRole role = BraceRole::Compact;
    int parenDepth = 0;
    int indentRestore = 0;
    int closeIndent = 0;
};

bool SyntaxNodeHasClass(const SyntaxNode& node, SyntaxNodeClass syntaxNodeClass) {
    return (node.classes & static_cast<std::uint64_t>(syntaxNodeClass)) != 0 ||
        SyntaxNodeKindHasClass(node.kind, syntaxNodeClass);
}

bool IsPreprocessorNode(const SyntaxNode& node) {
    return SyntaxNodeHasClass(node, SyntaxNodeClass::AtomicPreprocessor);
}

std::string_view TrimSourceLine(std::string_view line) {
    while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
        line.remove_prefix(1);
    }
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
        line.remove_suffix(1);
    }
    return line;
}

BraceRole RoleForBraceParent(SyntaxNodeKind parentKind) {
    switch (parentKind) {
        case SyntaxNodeKind::CompoundStatement:
        case SyntaxNodeKind::FieldDeclarationList:
        case SyntaxNodeKind::DeclarationList:
        case SyntaxNodeKind::RequirementSeq:
            return BraceRole::Block;
        case SyntaxNodeKind::EnumeratorList:
            return BraceRole::Enum;
        default:
            return BraceRole::Compact;
    }
}

bool IsNamespaceDefinitionDeclarationList(const PrintToken& token) {
    return token.parentKind == SyntaxNodeKind::DeclarationList &&
        token.grandParentKind == SyntaxNodeKind::NamespaceDefinition;
}

bool IsLinkageSpecificationDeclarationList(const PrintToken& token) {
    return token.parentKind == SyntaxNodeKind::DeclarationList &&
        token.grandParentKind == SyntaxNodeKind::LinkageSpecification;
}

bool IsNamespaceLikeBrace(const PrintToken& token) {
    return token.parentKind == SyntaxNodeKind::LinkageSpecification ||
        IsNamespaceDefinitionDeclarationList(token) ||
        IsLinkageSpecificationDeclarationList(token);
}

BraceRole RoleForBrace(const PrintToken& token) {
    if (
        token.inSingleStatementLambdaBody &&
        token.parentKind == SyntaxNodeKind::CompoundStatement &&
        token.grandParentKind == SyntaxNodeKind::LambdaExpression
    ) {
        return BraceRole::Compact;
    }
    if (IsNamespaceLikeBrace(token)) {
        return BraceRole::NamespaceLike;
    }
    return RoleForBraceParent(token.parentKind);
}

bool IsWithinConditionalFunctionHeader(const PrintToken& token) {
    for (const SyntaxNode* node = token.node; node != nullptr; node = node->parent) {
        if (SyntaxNodeHasClass(*node, SyntaxNodeClass::ConditionalFunctionHeader)) {
            return true;
        }
    }
    return false;
}

bool IsDeclarationModifierPreprocessorToken(const PrintToken& token) {
    return token.node != nullptr &&
        (token.node->classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationModifierPreprocessor)) != 0;
}

bool IsConditionalRhsPreprocessorToken(const PrintToken& token) {
    return token.node != nullptr &&
        (token.node->classes & static_cast<std::uint64_t>(SyntaxNodeClass::ConditionalRhsPreprocessor)) != 0;
}

bool IsStandalonePreprocessorBranchToken(const SyntaxNode& node, SyntaxNodeKind parentKind) {
    return parentKind == SyntaxNodeKind::PreprocElse && node.kind == SyntaxNodeKind::PreprocessorDirectiveElse;
}

bool PreprocessorLineHasClass(std::string_view line, SyntaxNodeClass syntaxNodeClass) {
    return SyntaxNodeKindHasClass(SyntaxNodeKindFromPreprocessorDirectiveLine(line), syntaxNodeClass);
}

bool IsPreprocessorDirectiveNameChar(char ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_';
}

std::string CanonicalizePreprocessorDirectiveLine(std::string_view line) {
    const SyntaxNodeKind directiveKind = SyntaxNodeKindFromPreprocessorDirectiveLine(line);
    if (!SyntaxNodeKindHasClass(directiveKind, SyntaxNodeClass::PreprocessorDirective)) {
        return std::string(line);
    }

    size_t cursor = 0;
    while (cursor < line.size() && (line[cursor] == ' ' || line[cursor] == '\t')) {
        ++cursor;
    }
    const size_t prefixEnd = cursor;
    if (cursor >= line.size() || line[cursor] != '#') {
        return std::string(line);
    }
    ++cursor;
    while (cursor < line.size() && (line[cursor] == ' ' || line[cursor] == '\t')) {
        ++cursor;
    }
    while (cursor < line.size() && IsPreprocessorDirectiveNameChar(line[cursor])) {
        ++cursor;
    }

    std::string result;
    result.reserve(line.size());
    result.append(line.data(), prefixEnd);
    result.append(SyntaxNodeKindTokenText(directiveKind));
    while (cursor < line.size() && (line[cursor] == ' ' || line[cursor] == '\t')) {
        ++cursor;
    }
    if (cursor < line.size()) {
        result.push_back(' ');
        result.append(line.data() + cursor, line.size() - cursor);
    }
    return NormalizeTrailingLineCommentSpacing(result);
}

std::string CanonicalizePreprocessorDirectiveLines(std::string_view text) {
    std::string result;
    size_t start = 0;
    while (start <= text.size()) {
        const size_t end = text.find('\n', start);
        const std::string_view line = end == std::string::npos ? text.substr(start) : text.substr(start, end - start);
        if (!result.empty()) {
            result.push_back('\n');
        }
        result.append(CanonicalizePreprocessorDirectiveLine(line));
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return result;
}

bool IsStructuredConditionalPreprocessorNode(const SyntaxNode& node) {
    return SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::ConditionalPreprocessorTree) && !IsPreprocessorNode(node);
}

bool IsSourceLineBreak(char ch) {
    return ch == '\r' || ch == '\n';
}

size_t FindSourceLineBreak(std::string_view text, size_t start = 0) {
    for (size_t index = start; index < text.size(); ++index) {
        if (IsSourceLineBreak(text[index])) {
            return index;
        }
    }
    return std::string_view::npos;
}

bool ContainsSourceLineBreak(std::string_view text) {
    return FindSourceLineBreak(text) != std::string_view::npos;
}

std::string_view FirstSourceLine(std::string_view text) {
    const size_t end = FindSourceLineBreak(text);
    return end == std::string_view::npos ? text : text.substr(0, end);
}

bool LineEndsWithContinuation(std::string_view line) {
    while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
        line.remove_suffix(1);
    }
    return !line.empty() && line.back() == '\\';
}

std::string_view ContinuedPreprocessorHeader(std::string_view text) {
    size_t lineStart = 0;
    while (lineStart < text.size()) {
        const size_t lineEnd = FindSourceLineBreak(text, lineStart);
        if (lineEnd == std::string_view::npos) {
            return text;
        }
        const std::string_view line = text.substr(lineStart, lineEnd - lineStart);
        if (!LineEndsWithContinuation(line)) {
            return text.substr(0, lineEnd);
        }
        lineStart = lineEnd + 1;
        if (text[lineEnd] == '\r' && lineStart < text.size() && text[lineStart] == '\n') {
            ++lineStart;
        }
    }
    return text;
}

bool IsPreprocHeaderSeparator(const SyntaxNode& node) {
    return node.kind == SyntaxNodeKind::FreeToken && ContainsSourceLineBreak(node.text);
}

bool IsPreprocIfdefHeaderChild(const SyntaxNode& node, size_t index) {
    if (index < 2) {
        return true;
    }
    return index == 2 && IsPreprocHeaderSeparator(node);
}

bool IsPreprocElseHeaderChild(const SyntaxNode& node, size_t index) {
    if (index == 0) {
        return true;
    }
    return index == 1 && IsPreprocHeaderSeparator(node);
}

bool IsPreprocIfHeaderChild(const SyntaxNode& node, size_t index, bool& inHeader) {
    if (!inHeader) {
        return false;
    }
    if (IsPreprocHeaderSeparator(node)) {
        inHeader = false;
        return true;
    }
    if (index < 2) {
        return true;
    }
    inHeader = false;
    return false;
}

bool IsPreprocEndifToken(const SyntaxNode& node) {
    return SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::EndifDirective);
}

bool IsCommentAlreadyInPreprocessorHeader(const SyntaxNode& parent, const SyntaxNode& child) {
    if (child.kind != SyntaxNodeKind::Comment && child.kind != SyntaxNodeKind::TrailingComment) {
        return false;
    }
    return FirstSourceLine(parent.text).find(child.text) != std::string_view::npos;
}

bool IsFirstConditionalBranchChild(const SyntaxNode& node) {
    const SyntaxNode* parent = node.parent;
    if (
        parent == nullptr ||
        !SyntaxNodeKindHasClass(parent->kind, SyntaxNodeClass::ConditionalPreprocessorTree)
    ) {
        return false;
    }

    bool inPreprocIfHeader = true;
    for (size_t index = 0; index < parent->children.size(); ++index) {
        const SyntaxNode* child = parent->children[index];
        if (child == nullptr) {
            continue;
        }
        if (IsCommentAlreadyInPreprocessorHeader(*parent, *child)) {
            continue;
        }
        if (parent->kind == SyntaxNodeKind::PreprocIfdef && IsPreprocIfdefHeaderChild(*child, index)) {
            continue;
        }
        if (parent->kind == SyntaxNodeKind::PreprocElse && IsPreprocElseHeaderChild(*child, index)) {
            continue;
        }
        if (
            (parent->kind == SyntaxNodeKind::PreprocIf || parent->kind == SyntaxNodeKind::PreprocElif) &&
            IsPreprocIfHeaderChild(*child, index, inPreprocIfHeader)
        ) {
            continue;
        }
        if (SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::Trivia)) {
            continue;
        }
        return child == &node;
    }
    return false;
}

std::string_view PreprocEndifLine(const SyntaxNode& node) {
    return node.text.empty() ? SyntaxNodeKindTokenText(node.kind) : TrimSourceLine(FirstSourceLine(node.text));
}

bool IsRawStatementToken(const PrintToken& token) {
    if (token.kind != PrintTokenKind::Free || ContainsSourceLineBreak(token.text)) {
        return false;
    }
    const SyntaxNodeKind parent = token.parentKind;
    return (
        parent == SyntaxNodeKind::TranslationUnit ||
        parent == SyntaxNodeKind::DeclarationList ||
        parent == SyntaxNodeKind::FieldDeclarationList ||
        parent == SyntaxNodeKind::CompoundStatement
    ) && EndsWith(TrimSourceLine(token.text), ";");
}

SyntaxNodeKind MatchingListCloseToken(SyntaxNodeKind kind) {
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

bool HasDirectKnownChild(const SyntaxNode& node, SyntaxNodeKind kind) {
    for (const SyntaxNode* child : node.children) {
        if (child != nullptr && child->kind == kind) {
            return true;
        }
    }
    return false;
}

bool HasDirectListDelimiterPair(const SyntaxNode& node) {
    for (const SyntaxNode* child : node.children) {
        if (child == nullptr || !SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::OpeningDelimiter)) {
            continue;
        }
        const SyntaxNodeKind close = MatchingListCloseToken(child->kind);
        if (close != SyntaxNodeKind::Unknown && HasDirectKnownChild(node, close)) {
            return true;
        }
    }
    return false;
}

bool HasDirectDelimiterPair(const SyntaxNode& node, SyntaxNodeKind openKind) {
    return HasDirectKnownChild(node, openKind) && HasDirectKnownChild(node, MatchingListCloseToken(openKind));
}

bool HasDirectListPrefix(const SyntaxNode& node) {
    return HasDirectKnownChild(node, SyntaxNodeKind::Colon);
}

bool IsSeparatedListContainer(const SyntaxNode& node) {
    if (HasDirectKnownChild(node, SyntaxNodeKind::Comma)) {
        return HasDirectListDelimiterPair(node) || HasDirectListPrefix(node);
    }
    return HasDirectKnownChild(node, SyntaxNodeKind::Semicolon) &&
        HasDirectDelimiterPair(node, SyntaxNodeKind::LeftParen);
}

bool HasDirectCommentChild(const SyntaxNode& node) {
    for (const SyntaxNode* child : node.children) {
        if (child != nullptr && (
            child->kind == SyntaxNodeKind::Comment || child->kind == SyntaxNodeKind::TrailingComment
        )) {
            return true;
        }
    }
    return false;
}

bool HasSeparatedListAncestor(const SyntaxNode* node) {
    for (
        const SyntaxNode* cursor = node == nullptr ? nullptr : node->parent;
        cursor != nullptr;
        cursor = cursor->parent
    ) {
        if (IsSeparatedListContainer(*cursor)) {
            return true;
        }
    }
    return false;
}

bool KeepsListCommentInBreakModel(const PrintToken& token) {
    if (!IsCommentToken(token.kind)) {
        return false;
    }
    return HasSeparatedListAncestor(token.node);
}

void AppendPreprocessorPrintToken(
    const SyntaxNode& node,
    std::string_view text,
    SyntaxNodeKind parentKind,
    SyntaxNodeKind grandParentKind,
    bool inTemplateDeclaration,
    bool inRequiresClause,
    bool inCompilerCallModifier,
    bool inSingleStatementLambdaBody,
    bool inMacroValue,
    bool structuredPreprocessor,
    const SyntaxNode* macroDefinition,
    std::vector<PrintToken>& tokens
) {
    tokens.push_back({
        .kind = PrintTokenKind::Preprocessor,
        .syntaxKind = node.kind,
        .text = text,
        .parentKind = parentKind,
        .grandParentKind = grandParentKind,
        .inTemplateDeclaration = inTemplateDeclaration,
        .inRequiresClause = inRequiresClause,
        .inCompilerCallModifier = inCompilerCallModifier,
        .inSingleStatementLambdaBody = inSingleStatementLambdaBody,
        .structuredPreprocessor = structuredPreprocessor,
        .inMacroValue = inMacroValue,
        .node = &node,
        .macroDefinition = macroDefinition
    });
}

void AppendTokens(
    const SyntaxNode& node,
    SyntaxNodeKind parentKind,
    SyntaxNodeKind grandParentKind,
    bool inTemplateDeclaration,
    bool inRequiresClause,
    bool inCompilerCallModifier,
    bool inSingleStatementLambdaBody,
    const SyntaxNode* macroDefinition,
    bool inMacroValue,
    std::vector<PrintToken>& tokens
) {
    const SyntaxNodeKind nodeKind = node.kind;
    const bool childInTemplateDeclaration = inTemplateDeclaration || nodeKind == SyntaxNodeKind::TemplateDeclaration;
    const bool childInRequiresClause = inRequiresClause || nodeKind == SyntaxNodeKind::RequiresClause;
    const bool childInCompilerCallModifier = inCompilerCallModifier ||
        nodeKind == SyntaxNodeKind::MsCallModifier ||
        nodeKind == SyntaxNodeKind::MsDeclspecModifier;
    const bool childInSingleStatementLambdaBody =
        inSingleStatementLambdaBody || LambdaBodyAllowsCompactSingleStatementForm(node, parentKind);
    const SyntaxNode* childMacroDefinition = macroDefinition != nullptr ? macroDefinition :
        (SyntaxNodeKindHasClass(nodeKind, SyntaxNodeClass::MacroDefinition) ? &node : nullptr);
    const bool childInMacroValue = inMacroValue || nodeKind == SyntaxNodeKind::MacroReplacementList;

    if (nodeKind == SyntaxNodeKind::BlankLine) {
        if (!SyntaxNodeKindHasClass(parentKind, SyntaxNodeClass::PreserveBlankLineParent)) {
            return;
        }
        tokens.push_back({
            .kind = PrintTokenKind::BlankLine,
            .inMacroValue = childInMacroValue,
            .node = &node,
            .macroDefinition = childMacroDefinition
        });
        return;
    }
    if (nodeKind == SyntaxNodeKind::Comment || nodeKind == SyntaxNodeKind::TrailingComment) {
        tokens.push_back({
            .kind = nodeKind == SyntaxNodeKind::TrailingComment ? PrintTokenKind::TrailingComment :
                PrintTokenKind::Comment,
            .syntaxKind = nodeKind,
            .text = node.text,
            .parentKind = parentKind,
            .grandParentKind = grandParentKind,
            .inTemplateDeclaration = childInTemplateDeclaration,
            .inRequiresClause = childInRequiresClause,
            .inCompilerCallModifier = childInCompilerCallModifier,
            .inSingleStatementLambdaBody = childInSingleStatementLambdaBody,
            .inMacroValue = childInMacroValue,
            .node = &node,
            .macroDefinition = childMacroDefinition
        });
        return;
    }
    if (IsStandalonePreprocessorBranchToken(node, parentKind)) {
        tokens.push_back({
            .kind = PrintTokenKind::Preprocessor,
            .syntaxKind = nodeKind,
            .text = node.text,
            .parentKind = parentKind,
            .grandParentKind = grandParentKind,
            .inTemplateDeclaration = childInTemplateDeclaration,
            .inRequiresClause = childInRequiresClause,
            .inCompilerCallModifier = childInCompilerCallModifier,
            .inSingleStatementLambdaBody = childInSingleStatementLambdaBody,
            .inMacroValue = childInMacroValue,
            .node = &node,
            .macroDefinition = childMacroDefinition
        });
        return;
    }
    if (IsStructuredConditionalPreprocessorNode(node)) {
        AppendPreprocessorPrintToken(
            node,
            ContinuedPreprocessorHeader(node.text),
            parentKind,
            grandParentKind,
            childInTemplateDeclaration,
            childInRequiresClause,
            childInCompilerCallModifier,
            childInSingleStatementLambdaBody,
            childInMacroValue,
            true,
            childMacroDefinition,
            tokens
        );

        bool inPreprocIfHeader = true;
        for (size_t index = 0; index < node.children.size(); ++index) {
            const SyntaxNode* child = node.children[index];
            if (child == nullptr) {
                continue;
            }
            if (IsPreprocEndifToken(*child)) {
                AppendPreprocessorPrintToken(
                    *child,
                    PreprocEndifLine(*child),
                    parentKind,
                    grandParentKind,
                    childInTemplateDeclaration,
                    childInRequiresClause,
                    childInCompilerCallModifier,
                    childInSingleStatementLambdaBody,
                    childInMacroValue,
                    true,
                    childMacroDefinition,
                    tokens
                );
                continue;
            }
            if (IsCommentAlreadyInPreprocessorHeader(node, *child)) {
                continue;
            }
            if (nodeKind == SyntaxNodeKind::PreprocIfdef && IsPreprocIfdefHeaderChild(*child, index)) {
                continue;
            }
            if (nodeKind == SyntaxNodeKind::PreprocElse && IsPreprocElseHeaderChild(*child, index)) {
                continue;
            }
            if (
                (nodeKind == SyntaxNodeKind::PreprocIf || nodeKind == SyntaxNodeKind::PreprocElif) &&
                IsPreprocIfHeaderChild(*child, index, inPreprocIfHeader)
            ) {
                continue;
            }
            AppendTokens(
                *child,
                nodeKind,
                parentKind,
                childInTemplateDeclaration,
                childInRequiresClause,
                childInCompilerCallModifier,
                childInSingleStatementLambdaBody,
                childMacroDefinition,
                childInMacroValue,
                tokens
            );
        }
        return;
    }
    if (SyntaxNodeKindHasClass(nodeKind, SyntaxNodeClass::Known)) {
        tokens.push_back({
            .kind = PrintTokenKind::Known,
            .syntaxKind = nodeKind,
            .text = node.text.empty() ? SyntaxNodeKindTokenText(nodeKind) : node.text,
            .parentKind = parentKind,
            .grandParentKind = grandParentKind,
            .inTemplateDeclaration = childInTemplateDeclaration,
            .inRequiresClause = childInRequiresClause,
            .inCompilerCallModifier = childInCompilerCallModifier,
            .inSingleStatementLambdaBody = childInSingleStatementLambdaBody,
            .inMacroValue = childInMacroValue,
            .node = &node,
            .macroDefinition = childMacroDefinition
        });
        return;
    }
    if (nodeKind == SyntaxNodeKind::IncludeRun) {
        tokens.push_back({
            .kind = PrintTokenKind::IncludeRun,
            .syntaxKind = nodeKind,
            .parentKind = parentKind,
            .grandParentKind = grandParentKind,
            .inTemplateDeclaration = childInTemplateDeclaration,
            .inRequiresClause = childInRequiresClause,
            .inCompilerCallModifier = childInCompilerCallModifier,
            .inSingleStatementLambdaBody = childInSingleStatementLambdaBody,
            .inMacroValue = childInMacroValue,
            .node = &node,
            .macroDefinition = childMacroDefinition
        });
        return;
    }
    if (IsPreprocessorNode(node)) {
        tokens.push_back({
            .kind = PrintTokenKind::Preprocessor,
            .syntaxKind = nodeKind,
            .text = node.text,
            .parentKind = parentKind,
            .grandParentKind = grandParentKind,
            .inTemplateDeclaration = childInTemplateDeclaration,
            .inRequiresClause = childInRequiresClause,
            .inCompilerCallModifier = childInCompilerCallModifier,
            .inSingleStatementLambdaBody = childInSingleStatementLambdaBody,
            .inMacroValue = childInMacroValue,
            .node = &node,
            .macroDefinition = childMacroDefinition
        });
        return;
    }
    if (nodeKind == SyntaxNodeKind::FreeToken || node.children.empty()) {
        tokens.push_back({
            .kind = PrintTokenKind::Free,
            .syntaxKind = nodeKind,
            .text = node.text,
            .parentKind = parentKind,
            .grandParentKind = grandParentKind,
            .inTemplateDeclaration = childInTemplateDeclaration,
            .inRequiresClause = childInRequiresClause,
            .inCompilerCallModifier = childInCompilerCallModifier,
            .inSingleStatementLambdaBody = childInSingleStatementLambdaBody,
            .inMacroValue = childInMacroValue,
            .node = &node,
            .macroDefinition = childMacroDefinition
        });
        return;
    }
    if (nodeKind == SyntaxNodeKind::MacroReplacementList) {
        for (const SyntaxNode* child : node.children) {
            AppendTokens(
                *child,
                nodeKind,
                parentKind,
                childInTemplateDeclaration,
                childInRequiresClause,
                childInCompilerCallModifier,
                childInSingleStatementLambdaBody,
                childMacroDefinition,
                true,
                tokens
            );
        }
        return;
    }
    if (SyntaxNodeKindHasClass(nodeKind, SyntaxNodeClass::Tree)) {
        for (const SyntaxNode* child : node.children) {
            AppendTokens(
                *child,
                nodeKind,
                parentKind,
                childInTemplateDeclaration,
                childInRequiresClause,
                childInCompilerCallModifier,
                childInSingleStatementLambdaBody,
                childMacroDefinition,
                childInMacroValue,
                tokens
            );
        }
    }
}

size_t FindLineCommentStart(std::string_view line) {
    bool inString = false;
    bool inChar = false;
    for (size_t index = 0; index + 1 < line.size(); ++index) {
        const char ch = line[index];
        const char next = line[index + 1];
        if (ch == '\\' && (inString || inChar)) {
            ++index;
            continue;
        }
        if (ch == '"' && !inChar) {
            inString = !inString;
            continue;
        }
        if (ch == '\'' && !inString) {
            inChar = !inChar;
            continue;
        }
        if (!inString && !inChar && ch == '/' && next == '/') {
            return index;
        }
    }
    return std::string_view::npos;
}

std::string RemoveTrailingListComma(std::string_view line) {
    const size_t commentStart = FindLineCommentStart(line);
    const size_t codeEnd = commentStart == std::string_view::npos ? line.size() : commentStart;
    size_t trimmedCodeEnd = codeEnd;
    while (trimmedCodeEnd > 0 && (line[trimmedCodeEnd - 1] == ' ' || line[trimmedCodeEnd - 1] == '\t')) {
        --trimmedCodeEnd;
    }
    if (trimmedCodeEnd == 0 || line[trimmedCodeEnd - 1] != ',') {
        return std::string(line);
    }

    std::string result;
    result.reserve(line.size() - 1);
    result.append(line.substr(0, trimmedCodeEnd - 1));
    if (commentStart != std::string_view::npos) {
        result.append("  ");
        result.append(line.substr(commentStart));
    }
    return result;
}

std::string FormatListPreprocessorLines(std::string_view text, int itemIndent, int indentWidth, bool finalListItem) {
    const std::string normalized = PreserveSourceLines(text);
    std::vector<std::string> lines;
    size_t start = 0;
    while (start <= normalized.size()) {
        const size_t end = normalized.find('\n', start);
        const std::string_view rawLine = end == std::string::npos ? std::string_view(normalized).substr(start) :
            std::string_view(normalized).substr(start, end - start);
        const std::string line = NormalizeTrailingLineCommentSpacing(TrimSourceLine(rawLine));
        lines.push_back(CanonicalizePreprocessorDirectiveLine(line));
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    if (finalListItem) {
        size_t finalConditionalStart = lines.size();
        for (size_t index = lines.size(); index > 0; --index) {
            if (PreprocessorLineHasClass(lines[index - 1], SyntaxNodeClass::ConditionalOpeningDirective)) {
                finalConditionalStart = index - 1;
                break;
            }
        }
        const size_t firstLineToNormalize = finalConditionalStart == lines.size() ? 0 : finalConditionalStart + 1;
        for (size_t index = firstLineToNormalize; index < lines.size(); ++index) {
            if (!lines[index].empty() && lines[index].front() != '#') {
                lines[index] = RemoveTrailingListComma(lines[index]);
            }
        }
    }

    std::string result;
    for (const std::string& line : lines) {
        if (!result.empty()) {
            result.push_back('\n');
        }
        if (!line.empty() && line.front() != '#') {
            result.append(static_cast<size_t>(std::max(0, itemIndent) * indentWidth), ' ');
        }
        result.append(line);
    }
    return result;
}

std::string FormatDeclarationModifierPreprocessorLines(std::string_view text, int declarationIndent, int indentWidth) {
    const std::string normalized = PreserveSourceLines(text);
    std::string result;
    size_t start = 0;
    while (start <= normalized.size()) {
        const size_t end = normalized.find('\n', start);
        const std::string_view rawLine = end == std::string::npos ? std::string_view(normalized).substr(start) :
            std::string_view(normalized).substr(start, end - start);
        const std::string line =
            CanonicalizePreprocessorDirectiveLine(NormalizeTrailingLineCommentSpacing(TrimSourceLine(rawLine)));
        if (!result.empty()) {
            result.push_back('\n');
        }
        if (!line.empty() && line.front() != '#') {
            result.append(static_cast<size_t>(std::max(0, declarationIndent) * indentWidth), ' ');
        }
        result.append(line);
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return result;
}

std::string FormatConditionalRhsPreprocessorLines(std::string_view text, int continuationIndent, int indentWidth) {
    const std::string normalized = PreserveSourceLines(text);
    std::string result;
    size_t start = 0;
    while (start <= normalized.size()) {
        const size_t end = normalized.find('\n', start);
        const std::string_view rawLine = end == std::string::npos ? std::string_view(normalized).substr(start) :
            std::string_view(normalized).substr(start, end - start);
        const std::string line =
            CanonicalizePreprocessorDirectiveLine(NormalizeTrailingLineCommentSpacing(TrimSourceLine(rawLine)));
        if (!result.empty()) {
            result.push_back('\n');
        }
        if (!line.empty() && line.front() != '#') {
            result.append(static_cast<size_t>(std::max(0, continuationIndent) * indentWidth), ' ');
        }
        result.append(line);
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return result;
}

std::string FormatConditionalAssignmentPrefix(std::string_view text) {
    const size_t equals = text.rfind('=');
    if (equals == std::string_view::npos) {
        return NormalizeTrailingLineCommentSpacing(CollapseSourceWhitespace(TrimSourceLine(text)));
    }
    std::string_view left = text.substr(0, equals);
    while (!left.empty() && (left.back() == ' ' || left.back() == '\t' || left.back() == '\r' || left.back() == '\n')) {
        left.remove_suffix(1);
    }
    std::string result = CollapseSourceWhitespace(TrimSourceLine(left));
    result += " =";
    return result;
}

struct DeferredSplitListContext {
    const SyntaxNode* list = nullptr;
    const SyntaxNode* lambdaRightBrace = nullptr;
    const SyntaxNode* closeToken = nullptr;
    int itemIndent = 0;
    int closeIndent = 0;
    bool afterLambdaClose = false;
};

struct LambdaSplitListPlan {
    FormatBreakModelContext breakContext;
    DeferredSplitListContext deferredContext;
};

struct PreprocessorSplitListContext {
    const SyntaxNode* list = nullptr;
    const SyntaxNode* closeToken = nullptr;
    int itemIndent = 0;
    int closeIndent = 0;
};

struct PreprocessorSplitListPlan {
    FormatBreakModelContext breakContext;
    PreprocessorSplitListContext deferredContext;
};

class Printer {
public:
    Printer(const FormatterConfig& config, std::string_view sourcePath, FormatModelTextStats* stats) :
        config_(config),
        sourcePath_(sourcePath),
        stats_(stats),
        indentWidth_(std::max(1, config.indentWidth)),
        tabWidth_(std::max(1, config.tabWidth)) {}

    std::string Print(const std::vector<PrintToken>& tokens) {
        activeTokens_ = &tokens;
        AnalyzeDeclarationGroups(tokens);
        output_.reserve(tokens.size() * 8);
        pendingTokens_.reserve(64);
        for (size_t index = 0; index < tokens.size(); ++index) {
            currentTokenIndex_ = index;
            const PrintToken* previous = PreviousToken(tokens, index);
            const PrintToken* rawPrevious = index == 0 ? nullptr : &tokens[index - 1];
            const PrintToken* next = NextToken(tokens, index);
            const PrintToken* rawNext = RawNextToken(tokens, index);
            PrintOne(tokens[index], previous, rawPrevious, next, rawNext);
        }
        activeTokens_ = nullptr;
        FlushPendingTokens();
        FinishLine();
        TrimTrailingBlankLines();
        if (!output_.empty() && output_.back() != '\n') {
            output_.push_back('\n');
        }
        return output_;
    }

private:
    const FormatterConfig& config_;
    std::string_view sourcePath_;
    FormatModelTextStats* stats_ = nullptr;
    int indentWidth_ = 4;
    int tabWidth_ = 4;
    std::string output_;
    std::vector<PrintToken> pendingTokens_;
    int indentLevel_ = 0;
    bool atLineStart_ = true;
    bool lineHasText_ = false;
    bool suppressNextBreakTokenSpace_ = false;
    int currentColumn_ = 0;
    bool macroContinuationLine_ = false;
    bool forceColumnZeroLine_ = false;
    bool emittingMacroDefinition_ = false;
    const std::vector<PrintToken>* activeTokens_ = nullptr;
    size_t currentTokenIndex_ = 0;
    std::optional<int> pendingIndentLevel_;
    int compactRightBraceSkips_ = 0;
    int switchDepth_ = 0;
    int parenDepth_ = 0;
    int bracketDepth_ = 0;
    std::vector<BraceFrame> braceStack_;
    std::vector<int> activeCaseBodySwitchDepths_;
    std::vector<DeferredSplitListContext> deferredSplitListContexts_;
    std::vector<PreprocessorSplitListContext> preprocessorSplitListContexts_;
    std::vector<int> conditionalFunctionIndents_;
    std::optional<int> pendingIndentRestoreAfterFlush_;
    std::unordered_set<const SyntaxNode*> isolatedDeclarationItems_;
    const SyntaxNode* previousDeclarationItem_ = nullptr;
    const SyntaxNode* preparedDeclarationItem_ = nullptr;
    const PrintToken* preparedPreprocessorGroupToken_ = nullptr;

    static const PrintToken* PreviousToken(const std::vector<PrintToken>& tokens, size_t index) {
        while (index > 0) {
            --index;
            if (tokens[index].kind != PrintTokenKind::BlankLine && !IsCommentToken(tokens[index].kind)) {
                return &tokens[index];
            }
        }
        return nullptr;
    }

    static const PrintToken* NextToken(const std::vector<PrintToken>& tokens, size_t index) {
        for (++index; index < tokens.size(); ++index) {
            if (tokens[index].kind != PrintTokenKind::BlankLine && !IsCommentToken(tokens[index].kind)) {
                return &tokens[index];
            }
        }
        return nullptr;
    }

    static const PrintToken* RawNextToken(const std::vector<PrintToken>& tokens, size_t index) {
        return index + 1 < tokens.size() ? &tokens[index + 1] : nullptr;
    }

    static const SyntaxNode* DeclarationScopeItem(const SyntaxNode* node) {
        for (const SyntaxNode* cursor = node; cursor != nullptr && cursor->parent != nullptr; cursor = cursor->parent) {
            if (SyntaxNodeHasClass(*cursor->parent, SyntaxNodeClass::DeclarationScope)) {
                return cursor;
            }
        }
        return nullptr;
    }

    static DeclarationGroupKind DeclarationGroup(const SyntaxNode* item) {
        if (item == nullptr) {
            return DeclarationGroupKind::None;
        }
        if (SyntaxNodeHasClass(*item, SyntaxNodeClass::DeclarationGroupType)) {
            return DeclarationGroupKind::Type;
        }
        if (SyntaxNodeHasClass(*item, SyntaxNodeClass::DeclarationGroupCallable)) {
            return DeclarationGroupKind::Callable;
        }
        if (SyntaxNodeHasClass(*item, SyntaxNodeClass::DeclarationGroupObject)) {
            return DeclarationGroupKind::Object;
        }
        if (SyntaxNodeHasClass(*item, SyntaxNodeClass::DeclarationGroupAlias)) {
            return DeclarationGroupKind::Alias;
        }
        return DeclarationGroupKind::None;
    }

    static const SyntaxNode* DirectChildContaining(const SyntaxNode& ancestor, const SyntaxNode* descendant) {
        const SyntaxNode* child = descendant;
        while (child != nullptr && child->parent != &ancestor) {
            child = child->parent;
        }
        return child;
    }

    static bool DirectChildFollowsToken(
        const SyntaxNode& owner,
        const SyntaxNode* descendant,
        SyntaxNodeKind tokenKind
    ) {
        const SyntaxNode* target = DirectChildContaining(owner, descendant);
        bool sawToken = false;
        for (const SyntaxNode* child : owner.children) {
            if (child == nullptr) {
                continue;
            }
            if (child == target) {
                return sawToken;
            }
            sawToken = sawToken || child->kind == tokenKind;
        }
        return false;
    }

    static bool DelimiterBelongsToDeclarationIsolationTarget(
        const SyntaxNode& declaration,
        const SyntaxNode* delimiter
    ) {
        if (delimiter == nullptr) {
            return false;
        }
        if (
            declaration.kind == SyntaxNodeKind::AliasDeclaration ||
            declaration.kind == SyntaxNodeKind::FunctionPointerAliasDeclaration
        ) {
            return DirectChildFollowsToken(declaration, delimiter, SyntaxNodeKind::Equal);
        }
        if (declaration.kind != SyntaxNodeKind::FieldDeclaration) {
            return false;
        }
        for (const SyntaxNode* cursor = delimiter; cursor != nullptr && cursor != &declaration; cursor = cursor->parent) {
            if (cursor->kind != SyntaxNodeKind::InitDeclarator) {
                continue;
            }
            if (DirectChildFollowsToken(*cursor, delimiter, SyntaxNodeKind::Equal)) {
                return true;
            }
            const SyntaxNode* target = DirectChildContaining(*cursor, delimiter);
            return target != nullptr && (
                target->kind == SyntaxNodeKind::InitializerList || target->kind == SyntaxNodeKind::ArgumentList
            );
        }
        return DirectChildFollowsToken(declaration, delimiter, SyntaxNodeKind::Equal);
    }

    static bool IsNestedDeclarationIsolationBoundary(const SyntaxNode& node, bool root) {
        return !root && (
            SyntaxNodeHasClass(node, SyntaxNodeClass::DeclarationScope) ||
            SyntaxNodeHasClass(node, SyntaxNodeClass::CompoundBlock)
        );
    }

    static bool ContainsDeclarationIsolationDelimiter(
        const SyntaxNode& declaration,
        const SyntaxNode& node,
        bool root = true
    ) {
        if (
            SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::OpeningDelimiter) &&
            DelimiterBelongsToDeclarationIsolationTarget(declaration, &node)
        ) {
            return true;
        }
        if (IsNestedDeclarationIsolationBoundary(node, root)) {
            return false;
        }
        return std::any_of(node.children.begin(), node.children.end(), [&](const SyntaxNode* child) {
            return child != nullptr && ContainsDeclarationIsolationDelimiter(declaration, *child, false);
        });
    }

    static bool IsSelectedDelimiterSplit(FormatBreakChoice choice) {
        return choice == FormatBreakChoice::Split ||
            choice == FormatBreakChoice::SplitAttachedOpen ||
            choice == FormatBreakChoice::SplitDelimiterStack ||
            choice == FormatBreakChoice::SplitDelimiterStackDetachedLeaf ||
            choice == FormatBreakChoice::SplitDelimiterStackRun;
    }

    bool HasDeclarationIndentDelimiterSplit(
        const FormatBreakModel& model,
        const FormatBreakSolution& solution,
        const SyntaxNode& declaration,
        int declarationIndent
    ) const {
        if (model.nodes == nullptr) {
            return false;
        }
        return std::any_of(model.nodes->begin(), model.nodes->end(), [&](const FormatBreakNode& node) {
            const size_t index = static_cast<size_t>(node.id);
            return node.kind == FormatBreakNodeKind::Delimited &&
                index < solution.choices.size() &&
                index < solution.indentLevels.size() &&
                IsSelectedDelimiterSplit(solution.choices[index]) &&
                solution.indentLevels[index] == declarationIndent &&
                !node.children.empty() &&
                node.children.front() != nullptr &&
                DelimiterBelongsToDeclarationIsolationTarget(
                    declaration,
                    FormatBreakTokenValue(node.children.front()->token).node
                );
        });
    }

    static int DeclarationIndent(const SyntaxNode& item) {
        int indent = 0;
        for (const SyntaxNode* cursor = item.parent; cursor != nullptr; cursor = cursor->parent) {
            if (cursor->kind == SyntaxNodeKind::FieldDeclarationList) {
                ++indent;
            }
        }
        return indent;
    }

    static const SyntaxNode* DeclarationIsolationOwner(const SyntaxNode& item, bool root = true) {
        if (
            item.kind == SyntaxNodeKind::FieldDeclaration ||
            item.kind == SyntaxNodeKind::AliasDeclaration ||
            item.kind == SyntaxNodeKind::FunctionPointerAliasDeclaration
        ) {
            return &item;
        }
        if (IsNestedDeclarationIsolationBoundary(item, root)) {
            return nullptr;
        }
        for (const SyntaxNode* child : item.children) {
            if (child != nullptr) {
                if (const SyntaxNode* owner = DeclarationIsolationOwner(*child, false)) {
                    return owner;
                }
            }
        }
        return nullptr;
    }

    void AnalyzeDeclarationGroups(const std::vector<PrintToken>& tokens) {
        // A delimiter-owned declaration must be known before its first token is emitted so the mandatory blank
        // line can precede it. Pre-solve only those declaration items with the ordinary break model and solver;
        // isolation therefore follows the selected layout, not a width estimate or a printer-side break choice.
        isolatedDeclarationItems_.clear();
        std::unordered_set<const SyntaxNode*> analyzedItems;
        for (size_t index = 0; index < tokens.size();) {
            const SyntaxNode* item = DeclarationScopeItem(tokens[index].node);
            if (item == nullptr || !analyzedItems.insert(item).second) {
                ++index;
                continue;
            }
            const SyntaxNode* isolationOwner =
                DeclarationIsolationOwner(*item);
            if (
                isolationOwner == nullptr ||
                !ContainsDeclarationIsolationDelimiter(*isolationOwner, *isolationOwner)
            ) {
                ++index;
                continue;
            }
            size_t end = index + 1;
            while (end < tokens.size() && SyntaxPathContains(tokens[end], item)) {
                ++end;
            }
            FormatBreakModel model = BuildFormatBreakModel(std::span<const PrintToken>{tokens.data() + index, end - index});
            const int declarationIndent = DeclarationIndent(*item);
            FormatBreakSolution solution = SolveFormatBreaks(
                config_,
                model,
                declarationIndent * indentWidth_,
                declarationIndent,
                indentWidth_,
                0
            );
            if (
                model.root != nullptr &&
                HasDeclarationIndentDelimiterSplit(model, solution, *isolationOwner, declarationIndent)
            ) {
                isolatedDeclarationItems_.insert(item);
            }
            index = end;
        }
    }

    bool RequiresDeclarationGroupSeparation(const SyntaxNode* left, const SyntaxNode* right) const {
        if (
            left == nullptr || right == nullptr ||
            left->parent == nullptr || left->parent != right->parent ||
            !SyntaxNodeHasClass(*left->parent, SyntaxNodeClass::DeclarationScope)
        ) {
            return false;
        }
        if (isolatedDeclarationItems_.contains(left) || isolatedDeclarationItems_.contains(right)) {
            return true;
        }
        const DeclarationGroupKind leftGroup = DeclarationGroup(left);
        const DeclarationGroupKind rightGroup = DeclarationGroup(right);
        if (leftGroup == DeclarationGroupKind::None || rightGroup == DeclarationGroupKind::None) {
            return false;
        }
        return leftGroup == DeclarationGroupKind::Type ||
            rightGroup == DeclarationGroupKind::Type ||
            leftGroup != rightGroup;
    }

    const SyntaxNode* NextDeclarationItem(size_t index) const {
        if (activeTokens_ == nullptr) {
            return nullptr;
        }
        for (++index; index < activeTokens_->size(); ++index) {
            const SyntaxNode* item = DeclarationScopeItem((*activeTokens_)[index].node);
            if (DeclarationGroup(item) != DeclarationGroupKind::None) {
                return item;
            }
        }
        return nullptr;
    }

    bool PrepareDeclarationGroupBoundary(const PrintToken& token) {
        const SyntaxNode* item = DeclarationScopeItem(token.node);
        const DeclarationGroupKind group = DeclarationGroup(item);
        if (group != DeclarationGroupKind::None) {
            if (item != previousDeclarationItem_) {
                if (item != preparedDeclarationItem_ && RequiresDeclarationGroupSeparation(
                    previousDeclarationItem_,
                    item
                )) {
                    FlushPendingTokens();
                    BlankLine();
                }
                previousDeclarationItem_ = item;
                preparedDeclarationItem_ = nullptr;
            }
            return false;
        }
        if (item == nullptr || item->parent == nullptr || !SyntaxNodeHasClass(
            *item->parent,
            SyntaxNodeClass::DeclarationScope
        )) {
            return false;
        }
        // A declaration terminator may be a declaration-scope sibling when the parser flattens a bare
        // class, struct, or enum declaration. It completes the preceding group item; it is not a prefix
        // of the next declaration.
        if (
            token.kind == PrintTokenKind::TrailingComment ||
            (token.node != nullptr && token.node->kind == SyntaxNodeKind::Semicolon)
        ) {
            return false;
        }

        const SyntaxNode* nextItem = NextDeclarationItem(currentTokenIndex_);
        const bool prefixesNextItem = nextItem != nullptr && nextItem->parent == item->parent;
        const bool separates = prefixesNextItem &&
            RequiresDeclarationGroupSeparation(previousDeclarationItem_, nextItem);
        if (separates && preparedDeclarationItem_ != nextItem) {
            FlushPendingTokens();
            BlankLine();
            preparedDeclarationItem_ = nextItem;
        }
        return false;
    }

    static const SyntaxNode* DirectSourceItem(const SyntaxNode* node) {
        for (
            const SyntaxNode* cursor = node;
            cursor != nullptr && cursor->parent != nullptr;
            cursor = cursor->parent
        ) {
            if (SyntaxNodeHasClass(*cursor->parent, SyntaxNodeClass::PreserveBlankLineParent)) {
                return cursor;
            }
        }
        return nullptr;
    }

    static const SyntaxNode* PreprocessorGroupingSourceItem(const PrintToken* token) {
        if (token == nullptr || token->node == nullptr) {
            return nullptr;
        }
        const SyntaxNode* item = DirectSourceItem(token->node);
        if (item == nullptr) {
            return nullptr;
        }

        // Delimiters belong to the source item that owns their structural container. Conditional
        // branch delimiters likewise belong to the complete conditional item in the surrounding
        // source-item scope. Mapping them to their owner prevents a group separator at the inside
        // edge of `{ ... }` or `#if ... #endif`, while still exposing that complete item to a
        // neighboring pragma or undef group in the enclosing scope.
        if (item->kind == SyntaxNodeKind::LeftBrace || item->kind == SyntaxNodeKind::RightBrace) {
            return DirectSourceItem(item->parent);
        }
        if (SyntaxNodeHasClass(*item, SyntaxNodeClass::ConditionalPreprocessorDirective)) {
            for (const SyntaxNode* cursor = item; cursor != nullptr; cursor = cursor->parent) {
                if (SyntaxNodeHasClass(*cursor, SyntaxNodeClass::ConditionalPreprocessorOpen)) {
                    return DirectSourceItem(cursor);
                }
            }
            return nullptr;
        }
        return item;
    }

    static std::optional<PreprocessorGroup> GroupForPreprocessorSeparation(const PrintToken* token) {
        const SyntaxNode* item = PreprocessorGroupingSourceItem(token);
        if (item == nullptr) {
            return std::nullopt;
        }
        const SyntaxNodeKind directiveKind = item->kind == SyntaxNodeKind::PreprocCall ?
            SyntaxNodeKindFromPreprocessorDirectiveLine(FirstSourceLine(item->text)) :
            SyntaxNodeKind::Unknown;
        if (directiveKind == SyntaxNodeKind::PreprocessorDirectivePragma) {
            return PreprocessorGroup{.item = item, .kind = PreprocessorGroupKind::Pragma};
        }
        if (directiveKind == SyntaxNodeKind::PreprocessorDirectiveUndef) {
            return PreprocessorGroup{.item = item, .kind = PreprocessorGroupKind::Undef};
        }
        return PreprocessorGroup{.item = item, .kind = PreprocessorGroupKind::Other};
    }

    static bool RequiresPreprocessorGroupSeparation(
        PreprocessorGroupKind left,
        PreprocessorGroupKind right
    ) {
        return left != right && (
            left != PreprocessorGroupKind::Other || right != PreprocessorGroupKind::Other
        );
    }

    void PreparePreprocessorGroupBoundary(
        const PrintToken& token,
        const PrintToken* previous,
        const PrintToken* next
    ) {
        if (preparedPreprocessorGroupToken_ == &token) {
            preparedPreprocessorGroupToken_ = nullptr;
            return;
        }
        const bool prefixToken = token.kind == PrintTokenKind::BlankLine || token.kind == PrintTokenKind::Comment;
        const PrintToken* right = prefixToken ? next : &token;
        const std::optional<PreprocessorGroup> leftGroup = GroupForPreprocessorSeparation(previous);
        const std::optional<PreprocessorGroup> rightGroup = GroupForPreprocessorSeparation(right);
        if (
            !leftGroup || !rightGroup ||
            leftGroup->item == rightGroup->item ||
            leftGroup->item->parent != rightGroup->item->parent ||
            !RequiresPreprocessorGroupSeparation(leftGroup->kind, rightGroup->kind)
        ) {
            return;
        }
        if (preparedPreprocessorGroupToken_ != right) {
            FlushPendingTokens();
            BlankLine();
        }
        if (prefixToken) {
            preparedPreprocessorGroupToken_ = right;
        }
    }

    static bool AreNeighboringSourceItems(const PrintToken& left, const PrintToken& right) {
        const SyntaxNode* leftItem = PreprocessorGroupingSourceItem(&left);
        const SyntaxNode* rightItem = PreprocessorGroupingSourceItem(&right);
        return leftItem != nullptr && rightItem != nullptr && leftItem != rightItem &&
            leftItem->parent == rightItem->parent;
    }

    static bool SyntaxPathContains(const PrintToken& token, const SyntaxNode* node) {
        for (const SyntaxNode* cursor = token.node; cursor != nullptr; cursor = cursor->parent) {
            if (cursor == node) {
                return true;
            }
        }
        return false;
    }

    static bool SyntaxPathContainsKind(const PrintToken& token, SyntaxNodeKind kind) {
        for (const SyntaxNode* cursor = token.node; cursor != nullptr; cursor = cursor->parent) {
            if (cursor->kind == kind) {
                return true;
            }
        }
        return false;
    }

    static bool SyntaxPathContainsClass(const PrintToken& token, SyntaxNodeClass syntaxNodeClass) {
        for (const SyntaxNode* cursor = token.node; cursor != nullptr; cursor = cursor->parent) {
            if (SyntaxNodeHasClass(*cursor, syntaxNodeClass)) {
                return true;
            }
        }
        return false;
    }

    static const SyntaxNode* DirectTokenChild(const SyntaxNode& node, SyntaxNodeKind known) {
        for (const SyntaxNode* child : node.children) {
            if (child && child->kind == known) {
                return child;
            }
        }
        return nullptr;
    }

    static bool HasDirectTokenChild(const SyntaxNode& node, SyntaxNodeKind known) {
        return DirectTokenChild(node, known) != nullptr;
    }

    static bool RightParenClosesCompoundExpression(const PrintToken& token, const PrintToken& next) {
        if (token.parentKind != SyntaxNodeKind::CompoundStatement || next.syntaxKind != SyntaxNodeKind::RightParen) {
            return false;
        }
        const SyntaxNode* compound = token.node != nullptr ? token.node->parent : nullptr;
        const SyntaxNode* closeParent = next.node != nullptr ? next.node->parent : nullptr;
        if (compound == nullptr || compound->kind != SyntaxNodeKind::CompoundStatement || closeParent == nullptr) {
            return false;
        }
        for (const SyntaxNode* cursor = compound->parent; cursor != nullptr; cursor = cursor->parent) {
            if (cursor == closeParent) {
                return true;
            }
        }
        return false;
    }

    static SyntaxNodeKind MatchingClosingDelimiterToken(SyntaxNodeKind kind) {
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

    static const SyntaxNode* DirectOpeningDelimiterChild(const SyntaxNode& node) {
        for (const SyntaxNode* child : node.children) {
            if (child && SyntaxNodeKindHasClass(child->kind, SyntaxNodeClass::OpeningDelimiter)) {
                return child;
            }
        }
        return nullptr;
    }

    static const SyntaxNode* DirectMatchingClosingDelimiterChild(const SyntaxNode& node, const SyntaxNode* open) {
        const SyntaxNodeKind closingKind =
            open == nullptr ? SyntaxNodeKind::Unknown : MatchingClosingDelimiterToken(open->kind);
        return closingKind == SyntaxNodeKind::Unknown ? nullptr : DirectTokenChild(node, closingKind);
    }

    static const SyntaxNode* NearestDelimitedListAncestorBefore(const PrintToken& token, const SyntaxNode* before) {
        for (const SyntaxNode* cursor = token.node; cursor != nullptr; cursor = cursor->parent) {
            if (cursor == before) {
                for (cursor = cursor->parent; cursor != nullptr; cursor = cursor->parent) {
                    const SyntaxNode* open = DirectOpeningDelimiterChild(*cursor);
                    if (
                        open != nullptr &&
                        DirectMatchingClosingDelimiterChild(*cursor, open) != nullptr &&
                        HasDirectTokenChild(*cursor, SyntaxNodeKind::Comma)
                    ) {
                        return cursor;
                    }
                }
                return nullptr;
            }
        }
        return nullptr;
    }

    static const SyntaxNode* NearestAncestor(const PrintToken& token, SyntaxNodeKind kind) {
        for (const SyntaxNode* cursor = token.node; cursor != nullptr; cursor = cursor->parent) {
            if (cursor->kind == kind) {
                return cursor;
            }
        }
        return nullptr;
    }

    static bool IsStatementItemContainer(SyntaxNodeKind kind) {
        switch (kind) {
            case SyntaxNodeKind::TranslationUnit:
            case SyntaxNodeKind::DeclarationList:
            case SyntaxNodeKind::FieldDeclarationList:
            case SyntaxNodeKind::CompoundStatement:
            case SyntaxNodeKind::CaseStatement:
            case SyntaxNodeKind::PreprocIf:
            case SyntaxNodeKind::PreprocIfdef:
            case SyntaxNodeKind::PreprocElse:
            case SyntaxNodeKind::PreprocElif:
                return true;
            default:
                return false;
        }
    }

    static bool ClosesStatementPositionMacroCallItem(const PrintToken& token) {
        if (
            token.kind != PrintTokenKind::Known ||
            token.syntaxKind != SyntaxNodeKind::RightParen ||
            token.node == nullptr
        ) {
            return false;
        }
        const SyntaxNode* arguments = token.node->parent;
        if (arguments == nullptr || arguments->kind != SyntaxNodeKind::ArgumentList) {
            return false;
        }
        const SyntaxNode* macroCall = arguments->parent;
        if (macroCall == nullptr || macroCall->kind != SyntaxNodeKind::MacroCallItem) {
            return false;
        }
        const SyntaxNode* macroCallParent = macroCall->parent;
        return macroCallParent != nullptr && IsStatementItemContainer(macroCallParent->kind);
    }

    static bool StartsPreprocessorSplitList(const PrintToken& token) {
        return token.kind == PrintTokenKind::Preprocessor &&
            SyntaxNodeKindHasClass(token.syntaxKind, SyntaxNodeClass::ConditionalPreprocessorOpen);
    }

    static const SyntaxNode* NearestPreprocessorSplitListAncestor(const PrintToken& token) {
        for (const SyntaxNode* cursor = token.node; cursor != nullptr; cursor = cursor->parent) {
            if (SyntaxNodeKindHasClass(cursor->kind, SyntaxNodeClass::PreprocessorSplitList)) {
                return cursor;
            }
        }
        return nullptr;
    }

    static bool NodeOrDescendantHasConditionalPreprocessor(const SyntaxNode& node) {
        if (SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::ConditionalPreprocessorTree)) {
            return true;
        }
        for (const SyntaxNode* child : node.children) {
            if (child != nullptr && NodeOrDescendantHasConditionalPreprocessor(*child)) {
                return true;
            }
        }
        return false;
    }

    static const SyntaxNode* ImmediateConditionalPreprocessorListParent(const PrintToken& token) {
        const SyntaxNode* parent = token.node == nullptr ? nullptr : token.node->parent;
        if (
            parent == nullptr ||
            !SyntaxNodeKindHasClass(parent->kind, SyntaxNodeClass::PreprocessorSplitList) ||
            !NodeOrDescendantHasConditionalPreprocessor(*parent)
        ) {
            return nullptr;
        }
        return parent;
    }

    bool AppendCompactWidthToken(
        const PrintToken& token,
        int& width,
        bool& hasText,
        const PrintToken*& previous,
        bool& previousStringLike,
        bool allowFieldInitializerList = false
    ) const {
        if (token.kind != PrintTokenKind::Known && token.kind != PrintTokenKind::Free) {
            return false;
        }
        const bool stringLike = IsStringLike(token);
        const bool inFieldInitializerList = token.parentKind == SyntaxNodeKind::FieldInitializerList ||
            token.grandParentKind == SyntaxNodeKind::FieldInitializerList;
        if (
            token.inMacroValue ||
            token.macroDefinition != nullptr ||
            (stringLike && previousStringLike) ||
            (!allowFieldInitializerList && inFieldInitializerList)
        ) {
            return false;
        }
        if (FormatTokenNeedsSpace(previous, token) && hasText) {
            ++width;
        }
        const int tokenWidth = FormatTokenWidth(token);
        width += tokenWidth;
        hasText = hasText || tokenWidth > 0;
        previous = &token;
        previousStringLike = stringLike;
        return true;
    }

    static bool IsAccessLabel(const PrintToken& token, const PrintToken* next) {
        return IsAccessKeyword(token) &&
            next != nullptr &&
            next->kind == PrintTokenKind::Known &&
            next->syntaxKind == SyntaxNodeKind::Colon;
    }

    std::optional<size_t> FindTokenIndex(const SyntaxNode* node, size_t begin) const {
        if (node == nullptr || activeTokens_ == nullptr) {
            return std::nullopt;
        }
        for (size_t index = begin; index < activeTokens_->size(); ++index) {
            if ((*activeTokens_)[index].node == node) {
                return index;
            }
        }
        return std::nullopt;
    }

    const SyntaxNode* FindPendingOpeningDelimiterFor(const SyntaxNode* list) const {
        for (auto token = pendingTokens_.rbegin(); token != pendingTokens_.rend(); ++token) {
            if (
                token->kind == PrintTokenKind::Known &&
                SyntaxNodeKindHasClass(token->syntaxKind, SyntaxNodeClass::OpeningDelimiter) &&
                SyntaxPathContains(*token, list)
            ) {
                return token->node;
            }
        }
        return nullptr;
    }

    std::optional<size_t> FindFutureClosingDelimiterFor(const SyntaxNode* list, SyntaxNodeKind openKind) const {
        if (activeTokens_ == nullptr) {
            return std::nullopt;
        }
        const SyntaxNodeKind closeKind = MatchingClosingDelimiterToken(openKind);
        if (closeKind == SyntaxNodeKind::Unknown) {
            return std::nullopt;
        }
        for (size_t index = currentTokenIndex_ + 1; index < activeTokens_->size(); ++index) {
            const PrintToken& candidate = (*activeTokens_)[index];
            if (candidate.kind == PrintTokenKind::Known && candidate.syntaxKind == closeKind && SyntaxPathContains(
                candidate,
                list
            )) {
                return index;
            }
        }
        return std::nullopt;
    }

    void TrimTrailingSpaces() {
        while (!output_.empty() && output_.back() == ' ') {
            output_.pop_back();
            currentColumn_ = std::max(0, currentColumn_ - 1);
        }
    }

    bool HasOutputContent() const {
        for (char ch : output_) {
            if (ch != '\n') {
                return true;
            }
        }
        return false;
    }

    void FinishLine() {
        TrimTrailingSpaces();
    }

    void TrimTrailingBlankLines() {
        while (output_.size() >= 2 && output_.back() == '\n' && output_[output_.size() - 2] == '\n') {
            output_.pop_back();
        }
    }

    void NewLine(bool macroContinuation = false) {
        FinishLine();
        if (macroContinuation && lineHasText_) {
            output_.append(" \\");
        }
        if (output_.empty() || output_.back() != '\n') {
            output_.push_back('\n');
        }
        atLineStart_ = true;
        lineHasText_ = false;
        currentColumn_ = 0;
        macroContinuationLine_ = macroContinuation;
        forceColumnZeroLine_ = false;
        pendingIndentLevel_.reset();
    }

    void NewLineWithIndent(int indentLevel) {
        NewLine(emittingMacroDefinition_);
        pendingIndentLevel_ = std::max(0, indentLevel);
    }

    void BlankLineWithIndent(int indentLevel) {
        BlankLine();
        pendingIndentLevel_ = std::max(0, indentLevel);
    }

    void BreakListLine(int indentLevel, bool blankLine) {
        if (blankLine) {
            BlankLineWithIndent(indentLevel);
            return;
        }
        NewLineWithIndent(indentLevel);
    }

    void BlankLine() {
        if (!HasOutputContent() && !lineHasText_) {
            atLineStart_ = true;
            currentColumn_ = 0;
            macroContinuationLine_ = false;
            forceColumnZeroLine_ = false;
            pendingIndentLevel_.reset();
            return;
        }
        NewLine(false);
        if (output_.size() < 2 || output_[output_.size() - 2] != '\n') {
            output_.push_back('\n');
        }
        atLineStart_ = true;
        lineHasText_ = false;
        currentColumn_ = 0;
        macroContinuationLine_ = false;
        forceColumnZeroLine_ = false;
        pendingIndentLevel_.reset();
    }

    void ReopenLastOutputLine() {
        if (!output_.empty() && output_.back() == '\n') {
            output_.pop_back();
        }
        const size_t lineStart = output_.find_last_of('\n');
        currentColumn_ = lineStart == std::string::npos ? static_cast<int>(output_.size()) :
            static_cast<int>(output_.size() - lineStart - 1);
        atLineStart_ = false;
        lineHasText_ = currentColumn_ > 0;
        macroContinuationLine_ = false;
        forceColumnZeroLine_ = false;
        pendingIndentLevel_.reset();
    }

    void WriteIndentIfNeeded() {
        if (!atLineStart_) {
            return;
        }
        const int macroOffset = macroContinuationLine_ ? 1 : 0;
        const int indentLevel = pendingIndentLevel_.value_or(forceColumnZeroLine_ ? 0 : indentLevel_ + macroOffset);
        currentColumn_ = std::max(0, indentLevel) * indentWidth_;
        output_.append(static_cast<size_t>(currentColumn_), ' ');
        atLineStart_ = false;
        macroContinuationLine_ = false;
        forceColumnZeroLine_ = false;
        pendingIndentLevel_.reset();
    }

    void WriteWithIndentOffset(std::string_view text, int indentOffset) {
        if (!atLineStart_) {
            output_.append(text);
            AdvanceCurrentColumn(text);
            lineHasText_ = lineHasText_ || !text.empty();
            return;
        }
        const int adjustedIndent = std::max(0, indentLevel_ + indentOffset);
        currentColumn_ = adjustedIndent * indentWidth_;
        output_.append(static_cast<size_t>(currentColumn_), ' ');
        atLineStart_ = false;
        macroContinuationLine_ = false;
        forceColumnZeroLine_ = false;
        pendingIndentLevel_.reset();
        output_.append(text);
        AdvanceCurrentColumn(text);
        lineHasText_ = lineHasText_ || !text.empty();
    }

    void Write(std::string_view text) {
        WriteIndentIfNeeded();
        output_.append(text);
        AdvanceCurrentColumn(text);
        lineHasText_ = lineHasText_ || !text.empty();
    }

    void CloseCaseBodyIndentIfNeeded() {
        if (!activeCaseBodySwitchDepths_.empty() && activeCaseBodySwitchDepths_.back() == switchDepth_) {
            indentLevel_ = std::max(0, indentLevel_ - 1);
            activeCaseBodySwitchDepths_.pop_back();
        }
    }

    void Space() {
        if (!atLineStart_ && !output_.empty() && output_.back() != ' ' && output_.back() != '\n') {
            output_.push_back(' ');
            ++currentColumn_;
        }
    }

    int CurrentColumn() const {
        if (atLineStart_) {
            const int macroOffset = macroContinuationLine_ ? 1 : 0;
            const int indentLevel = pendingIndentLevel_.value_or(forceColumnZeroLine_ ? 0 : indentLevel_ + macroOffset);
            return std::max(0, indentLevel) * indentWidth_;
        }
        return currentColumn_;
    }

    void AdvanceCurrentColumn(std::string_view text) {
        const size_t newline = text.find_last_of('\n');
        if (newline == std::string_view::npos) {
            currentColumn_ += static_cast<int>(text.size());
            return;
        }
        currentColumn_ = static_cast<int>(text.size() - newline - 1);
    }

    int CurrentLineIndentLevel() const {
        const size_t lineStart = output_.find_last_of('\n');
        size_t cursor = lineStart == std::string::npos ? 0 : lineStart + 1;
        int spaces = 0;
        while (cursor < output_.size() && output_[cursor] == ' ') {
            ++spaces;
            ++cursor;
        }
        return spaces / indentWidth_;
    }

    int PendingCompactWidth() const {
        int width = 0;
        bool hasText = lineHasText_;
        const PrintToken* previous = nullptr;
        for (const PrintToken& token : pendingTokens_) {
            if (FormatTokenNeedsSpace(previous, token) && hasText) {
                ++width;
            }
            width += FormatTokenWidth(token);
            hasText = hasText || FormatTokenWidth(token) > 0;
            previous = &token;
        }
        return width;
    }

    bool CanFlushPendingTokensCompact(const FormatBreakModelContext& context) const {
        if (
            context.virtualDelimiterOpen != nullptr ||
            context.virtualDelimiterClose.token != nullptr ||
            context.forceSplitVirtualDelimiter
        ) {
            return false;
        }
        int width = 0;
        bool hasText = lineHasText_;
        const PrintToken* previous = nullptr;
        bool previousStringLike = false;
        for (const PrintToken& token : pendingTokens_) {
            if (token.kind != PrintTokenKind::Known && token.kind != PrintTokenKind::Free) {
                return false;
            }
            const bool stringLike = IsStringLike(token);
            if (
                token.inMacroValue ||
                token.macroDefinition != nullptr ||
                SyntaxPathContainsKind(token, SyntaxNodeKind::MacroStatementSequence) ||
                SyntaxPathContainsClass(token, SyntaxNodeClass::LeadingStreamOperatorChain) ||
                SyntaxPathContainsClass(token, SyntaxNodeClass::ConditionalStreamOperatorChain) ||
                token.inTemplateDeclaration ||
                (stringLike && previousStringLike) ||
                token.parentKind == SyntaxNodeKind::FieldInitializerList ||
                token.grandParentKind == SyntaxNodeKind::FieldInitializerList
            ) {
                return false;
            }
            if (FormatTokenNeedsSpace(previous, token) && hasText) {
                ++width;
            }
            const int tokenWidth = FormatTokenWidth(token);
            width += tokenWidth;
            hasText = hasText || tokenWidth > 0;
            previous = &token;
            previousStringLike = stringLike;
        }
        return CurrentColumn() + width <= config_.columnLimit;
    }

    void FlushPendingTokensCompact() {
        const PrintToken* previous = nullptr;
        for (const PrintToken& token : pendingTokens_) {
            if (FormatTokenNeedsSpace(previous, token) && !atLineStart_) {
                Space();
            }
            Write(FormatTokenText(token));
            previous = &token;
        }
        pendingTokens_.clear();
    }

    void BufferToken(const PrintToken& token) {
        pendingTokens_.push_back(token);
    }

    FormatBreakChoice ChoiceFor(const FormatBreakSolution& solution, int nodeId) const {
        if (nodeId < 0 || static_cast<size_t>(nodeId) >= solution.choices.size()) {
            return FormatBreakChoice::Compact;
        }
        return solution.choices[static_cast<size_t>(nodeId)];
    }

    static bool IsSplitChoice(FormatBreakChoice choice) {
        return choice == FormatBreakChoice::Split ||
            choice == FormatBreakChoice::BodyHeaderSplitAtParentIndent ||
            choice == FormatBreakChoice::BodyHeaderDetachedBody ||
            choice == FormatBreakChoice::SplitAttachedOpen ||
            choice == FormatBreakChoice::SplitDelimiterStack ||
            choice == FormatBreakChoice::SplitDelimiterStackDetachedLeaf;
    }

    static bool IsBodyHeaderSplitChoice(FormatBreakChoice choice) {
        return choice == FormatBreakChoice::Split ||
            choice == FormatBreakChoice::BodyHeaderSplitAtParentIndent ||
            choice == FormatBreakChoice::BodyHeaderDetachedBody;
    }

    void WriteBreakToken(const FormatBreakToken& token) {
        const bool suppressSpace = suppressNextBreakTokenSpace_;
        suppressNextBreakTokenSpace_ = false;
        if (token.contextOnly) {
            return;
        }
        const PrintToken& printToken = FormatBreakTokenValue(token);
        if (
            printToken.macroDefinition != nullptr &&
            !printToken.inMacroValue &&
            atLineStart_ &&
            !macroContinuationLine_
        ) {
            forceColumnZeroLine_ = true;
            pendingIndentLevel_.reset();
        }
        if (IsCommentToken(printToken.kind)) {
            if (!atLineStart_) {
                Space();
                output_.push_back(' ');
            }
            Write(FormatTokenText(printToken));
            NewLine(false);
            return;
        }
        if (token.spaceBefore && !suppressSpace && !atLineStart_) {
            Space();
        }
        Write(FormatTokenText(printToken));
    }

    void EmitBreakNode(const FormatBreakNode& node, const FormatBreakSolution& solution, int baseIndent) {
        switch (node.kind) {
            case FormatBreakNodeKind::Token:
                WriteBreakToken(node.token);
                return;
            case FormatBreakNodeKind::Sequence:
                for (const FormatBreakNode* child : node.children) {
                    EmitBreakNode(*child, solution, baseIndent);
                }
                return;
            case FormatBreakNodeKind::Delimited:
                EmitDelimitedNode(node, solution, baseIndent);
                return;
            case FormatBreakNodeKind::PrefixList:
                EmitPrefixListNode(node, solution, baseIndent);
                return;
            case FormatBreakNodeKind::StatementSequence:
                EmitStatementSequenceNode(node, solution, baseIndent);
                return;
            case FormatBreakNodeKind::FunctionSignature:
                EmitFunctionSignatureNode(node, solution, baseIndent);
                return;
            case FormatBreakNodeKind::BodyHeader:
                EmitBodyHeaderNode(node, solution, baseIndent);
                return;
            case FormatBreakNodeKind::Chain:
                EmitChainNode(node, solution, baseIndent);
                return;
            case FormatBreakNodeKind::AdjacentStrings:
                EmitAdjacentStringsNode(node, solution, baseIndent);
                return;
        }
    }

    bool IsDirectSplitDelimitedItem(const FormatBreakNode& node, const FormatBreakSolution& solution) const {
        return node.kind == FormatBreakNodeKind::Delimited && IsSplitChoice(ChoiceFor(solution, node.id));
    }

    bool ShouldCombineSplitDelimitedItemBoundary(
        const FormatBreakNode& node,
        const FormatBreakSolution& solution,
        size_t index
    ) const {
        if (index + 1 >= node.items.size()) {
            return false;
        }
        const FormatBreakListItem& item = node.items[index];
        const FormatBreakListItem& nextItem = node.items[index + 1];
        return item.node != nullptr &&
            nextItem.node != nullptr &&
            FormatBreakTokenKind(item.separator) == PrintTokenKind::Known &&
            FormatBreakTokenSyntaxKind(item.separator) == SyntaxNodeKind::Comma &&
            IsDirectSplitDelimitedItem(*item.node, solution) &&
            IsDirectSplitDelimitedItem(*nextItem.node, solution) &&
            !HasTrailingComment(node, index) &&
            !HasBlankLineBeforeItem(node, index + 1);
    }

    static bool HasRealSeparators(const FormatBreakNode& node) {
        return std::any_of(node.items.begin(), node.items.end(), [](const FormatBreakListItem& item) {
            return FormatBreakTokenKind(item.separator) == PrintTokenKind::Known;
        });
    }

    struct DelimiterStackEmitView {
        std::vector<const FormatBreakNode*> delimiters;
        const FormatBreakNode* leaf = nullptr;
    };

    struct DelimiterStackRun {
        size_t begin = 0;
        size_t end = 0;
        int indentLevel = 0;
    };

    static bool IsTransparentSingleItemDelimiter(const FormatBreakNode& node) {
        if (
            node.forceSplit ||
            node.delimiterKind != FormatBreakDelimiterKind::Paren ||
            node.children.size() < 2 ||
            node.items.size() != 1 ||
            HasRealSeparators(node) ||
            HasTrailingComment(node, 0) ||
            HasBlankLineBeforeItem(node, 0) ||
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

    static const FormatBreakNode* SingleChildSequenceNode(const FormatBreakNode& node) {
        if (node.kind != FormatBreakNodeKind::Sequence || node.children.size() != 1) {
            return nullptr;
        }
        return node.children.front();
    }

    static const FormatBreakNode* TransparentStackChild(const FormatBreakNode& node) {
        if (!IsTransparentSingleItemDelimiter(node)) {
            return nullptr;
        }
        const FormatBreakNode* item = node.items.front().node;
        while (item != nullptr) {
            if (IsTransparentSingleItemDelimiter(*item)) {
                return item;
            }
            item = SingleChildSequenceNode(*item);
        }
        return nullptr;
    }

    static std::optional<DelimiterStackEmitView> CollectDelimiterStack(const FormatBreakNode& node) {
        if (!IsTransparentSingleItemDelimiter(node) || TransparentStackChild(node) == nullptr) {
            return std::nullopt;
        }
        DelimiterStackEmitView stack;
        const FormatBreakNode* current = &node;
        while (current != nullptr) {
            stack.delimiters.push_back(current);
            const FormatBreakNode* child = TransparentStackChild(*current);
            if (child == nullptr) {
                stack.leaf = current->items.front().node;
                break;
            }
            current = child;
        }
        return stack.leaf != nullptr && stack.delimiters.size() > 1 ? std::optional(stack) : std::nullopt;
    }

    void EmitDelimiterStackNode(const FormatBreakNode& node, const FormatBreakSolution& solution, int baseIndent) {
        const std::optional<DelimiterStackEmitView> stack = CollectDelimiterStack(node);
        if (!stack) {
            return;
        }
        int currentLineIndent = baseIndent;
        int nextOpenIndent = baseIndent + 1;
        std::vector<DelimiterStackRun> delimiterRuns;
        delimiterRuns.reserve(stack->delimiters.size());
        for (size_t index = 0; index < stack->delimiters.size(); ++index) {
            const FormatBreakNode* delimiter = stack->delimiters[index];
            const FormatBreakToken& open = delimiter->children.front()->token;
            if (
                ChoiceFor(solution, delimiter->children.front()->id) ==
                FormatBreakChoice::SplitDelimiterStackRun
            ) {
                currentLineIndent = nextOpenIndent;
                NewLineWithIndent(currentLineIndent);
                ++nextOpenIndent;
            }
            if (delimiterRuns.empty() || delimiterRuns.back().indentLevel != currentLineIndent) {
                delimiterRuns.push_back(
                    DelimiterStackRun{.begin = index, .end = index, .indentLevel = currentLineIndent}
                );
            }
            delimiterRuns.back().end = index + 1;
            WriteBreakToken(open);
        }
        const bool detachLeaf = ChoiceFor(solution, node.id) == FormatBreakChoice::SplitDelimiterStackDetachedLeaf;
        if (detachLeaf && lineHasText_) {
            NewLineWithIndent(nextOpenIndent);
        }
        EmitBreakNode(*stack->leaf, solution, nextOpenIndent);
        for (size_t runIndex = delimiterRuns.size(); runIndex-- > 0;) {
            const DelimiterStackRun& run = delimiterRuns[runIndex];
            const bool firstClosingRun = runIndex + 1 == delimiterRuns.size();
            if (lineHasText_ && (detachLeaf || !firstClosingRun)) {
                NewLineWithIndent(run.indentLevel);
            }
            for (size_t index = run.end; index-- > run.begin;) {
                WriteBreakToken(stack->delimiters[index]->children.back()->token);
            }
        }
    }

    static bool HasTrailingComment(const FormatBreakNode& node, size_t index) {
        return index < node.items.size() && IsCommentToken(FormatBreakTokenKind(node.items[index].trailingComment));
    }

    static bool HasLeadingTrailingComment(const FormatBreakNode& node) {
        return IsCommentToken(FormatBreakTokenKind(node.leadingTrailingComment));
    }

    static bool HasBlankLineBeforeItem(const FormatBreakNode& node, size_t index) {
        return index < node.items.size() && node.items[index].blankLineBefore;
    }

    void EmitDelimitedNode(const FormatBreakNode& node, const FormatBreakSolution& solution, int baseIndent) {
        const FormatBreakChoice choice = ChoiceFor(solution, node.id);
        if (
            choice == FormatBreakChoice::SplitDelimiterStack ||
            choice == FormatBreakChoice::SplitDelimiterStackDetachedLeaf
        ) {
            EmitDelimiterStackNode(node, solution, baseIndent);
            return;
        }
        if (!IsSplitChoice(choice) || node.items.empty()) {
            EmitBreakNode(*node.children[0], solution, baseIndent);
            if (HasLeadingTrailingComment(node)) {
                WriteBreakToken(node.leadingTrailingComment);
            }
            for (size_t index = 0; index < node.items.size(); ++index) {
                const FormatBreakListItem& item = node.items[index];
                if (node.suppressCompactDelimiterPadding && index == 0) {
                    suppressNextBreakTokenSpace_ = true;
                }
                EmitBreakNode(*item.node, solution, baseIndent);
                if (FormatBreakTokenKind(item.separator) == PrintTokenKind::Known) {
                    WriteBreakToken(item.separator);
                }
                if (HasTrailingComment(node, index)) {
                    WriteBreakToken(item.trailingComment);
                }
            }
            if (node.suppressCompactDelimiterPadding) {
                suppressNextBreakTokenSpace_ = true;
            }
            EmitBreakNode(*node.children[1], solution, baseIndent);
            return;
        }

        EmitBreakNode(*node.children[0], solution, baseIndent);
        if (HasLeadingTrailingComment(node)) {
            WriteBreakToken(node.leadingTrailingComment);
        }
        const bool closesInContext = node.children.size() > 1 &&
            node.children[1]->kind == FormatBreakNodeKind::Token &&
            node.children[1]->token.contextOnly;
        BreakListLine(baseIndent + 1, HasBlankLineBeforeItem(node, 0));
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& item = node.items[index];
            EmitBreakNode(*item.node, solution, baseIndent + 1);
            if (FormatBreakTokenKind(item.separator) == PrintTokenKind::Known) {
                WriteBreakToken(item.separator);
            }
            if (HasTrailingComment(node, index)) {
                WriteBreakToken(item.trailingComment);
            }
            if (ShouldCombineSplitDelimitedItemBoundary(node, solution, index)) {
                Space();
            } else if (closesInContext && index + 1 == node.items.size()) {
                continue;
            } else {
                const bool hasNextItem = index + 1 < node.items.size();
                if (hasNextItem) {
                    BreakListLine(baseIndent + 1, HasBlankLineBeforeItem(node, index + 1));
                } else {
                    BreakListLine(baseIndent, false);
                }
            }
        }
        EmitBreakNode(*node.children[1], solution, baseIndent);
    }

    void EmitPrefixListNode(const FormatBreakNode& node, const FormatBreakSolution& solution, int baseIndent) {
        const FormatBreakChoice choice = ChoiceFor(solution, node.id);
        if (choice != FormatBreakChoice::Split) {
            EmitBreakNode(*node.children[0], solution, baseIndent);
            if (HasLeadingTrailingComment(node)) {
                WriteBreakToken(node.leadingTrailingComment);
            }
            for (size_t index = 0; index < node.items.size(); ++index) {
                const FormatBreakListItem& item = node.items[index];
                EmitBreakNode(*item.node, solution, baseIndent);
                if (FormatBreakTokenKind(item.separator) == PrintTokenKind::Known) {
                    WriteBreakToken(item.separator);
                }
                if (HasTrailingComment(node, index)) {
                    WriteBreakToken(item.trailingComment);
                }
            }
            return;
        }

        EmitBreakNode(*node.children[0], solution, baseIndent);
        if (HasLeadingTrailingComment(node)) {
            WriteBreakToken(node.leadingTrailingComment);
        }
        BreakListLine(baseIndent + 1, HasBlankLineBeforeItem(node, 0));
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& item = node.items[index];
            EmitBreakNode(*item.node, solution, baseIndent + 1);
            if (FormatBreakTokenKind(item.separator) == PrintTokenKind::Known) {
                WriteBreakToken(item.separator);
            }
            if (HasTrailingComment(node, index)) {
                WriteBreakToken(item.trailingComment);
            }
            if (index + 1 < node.items.size()) {
                BreakListLine(baseIndent + 1, HasBlankLineBeforeItem(node, index + 1));
            }
        }
    }

    void EmitStatementSequenceNode(const FormatBreakNode& node, const FormatBreakSolution& solution, int baseIndent) {
        const FormatBreakChoice choice = ChoiceFor(solution, node.id);
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& item = node.items[index];
            if (choice == FormatBreakChoice::Split && index > 0) {
                BreakListLine(baseIndent, HasBlankLineBeforeItem(node, index));
            }
            EmitBreakNode(*item.node, solution, baseIndent);
            if (FormatBreakTokenKind(item.separator) == PrintTokenKind::Known) {
                WriteBreakToken(item.separator);
            }
            if (HasTrailingComment(node, index)) {
                WriteBreakToken(item.trailingComment);
            }
        }
    }

    void EmitDelimitedNodeAfterAttachedOpen(
        const FormatBreakNode& node,
        const FormatBreakSolution& solution,
        int baseIndent
    ) {
        EmitBreakNode(*node.children[0], solution, baseIndent);
        if (HasLeadingTrailingComment(node)) {
            WriteBreakToken(node.leadingTrailingComment);
        }
        const bool closesInContext = node.children.size() > 1 &&
            node.children[1]->kind == FormatBreakNodeKind::Token &&
            node.children[1]->token.contextOnly;
        BreakListLine(baseIndent + 1, HasBlankLineBeforeItem(node, 0));
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& item = node.items[index];
            EmitBreakNode(*item.node, solution, baseIndent + 1);
            if (FormatBreakTokenKind(item.separator) == PrintTokenKind::Known) {
                WriteBreakToken(item.separator);
            }
            if (HasTrailingComment(node, index)) {
                WriteBreakToken(item.trailingComment);
            }
            if (ShouldCombineSplitDelimitedItemBoundary(node, solution, index)) {
                Space();
            } else if (closesInContext && index + 1 == node.items.size()) {
                continue;
            } else {
                const bool hasNextItem = index + 1 < node.items.size();
                BreakListLine(
                    hasNextItem ? baseIndent + 1 : baseIndent,
                    hasNextItem && HasBlankLineBeforeItem(node, index + 1)
                );
            }
        }
        EmitBreakNode(*node.children[1], solution, baseIndent);
    }

    void EmitFunctionSignatureNode(const FormatBreakNode& node, const FormatBreakSolution& solution, int baseIndent) {
        const FormatBreakChoice choice = ChoiceFor(solution, node.id);
        if (choice != FormatBreakChoice::Split || node.children.size() < 2) {
            for (const FormatBreakNode* child : node.children) {
                EmitBreakNode(*child, solution, baseIndent);
            }
            return;
        }
        EmitBreakNode(*node.children[0], solution, baseIndent);
        NewLineWithIndent(baseIndent + 1);
        EmitBreakNode(*node.children[1], solution, baseIndent + 1);
        if (node.children.size() > 2) {
            if (node.functionSignatureHasBody) {
                NewLineWithIndent(baseIndent);
                EmitBreakNode(*node.children[2], solution, baseIndent);
            } else {
                EmitBreakNode(*node.children[2], solution, baseIndent + 1);
            }
        }
    }

    void EmitBodyHeaderNode(const FormatBreakNode& node, const FormatBreakSolution& solution, int baseIndent) {
        const FormatBreakChoice choice = ChoiceFor(solution, node.id);
        if (node.bodyHeaderRequiresDetachedBody && node.children.size() >= 2) {
            EmitBreakNode(*node.children[0], solution, baseIndent);
            NewLineWithIndent(baseIndent);
            EmitBreakNode(*node.children[1], solution, baseIndent);
            return;
        }
        if (!IsBodyHeaderSplitChoice(choice) || node.children.size() < 2) {
            for (const FormatBreakNode* child : node.children) {
                EmitBreakNode(*child, solution, baseIndent);
            }
            return;
        }
        EmitBreakNode(*node.children[0], solution, baseIndent);
        const int bodyIndent =
            choice == FormatBreakChoice::BodyHeaderSplitAtParentIndent ? std::max(0, baseIndent - 1) : baseIndent;
        if (
            choice == FormatBreakChoice::BodyHeaderSplitAtParentIndent ||
            choice == FormatBreakChoice::BodyHeaderDetachedBody
        ) {
            NewLineWithIndent(bodyIndent);
        }
        EmitBreakNode(*node.children[1], solution, bodyIndent);
    }

    void EmitChainNode(const FormatBreakNode& node, const FormatBreakSolution& solution, int baseIndent) {
        const FormatBreakChoice choice = ChoiceFor(solution, node.id);
        if (choice == FormatBreakChoice::Compact) {
            for (size_t index = 0; index < node.operands.size(); ++index) {
                EmitBreakNode(*node.operands[index], solution, baseIndent);
                if (index < node.operators.size()) {
                    WriteBreakToken(node.operators[index]);
                }
            }
            return;
        }

        if (node.chainKind == FormatBreakChainKind::StreamBeforeOperator) {
            if (!node.chainStartsWithOperator) {
                EmitBreakNode(*node.operands.front(), solution, baseIndent);
            }
            if (node.chainStartsWithOperator && atLineStart_) {
                pendingIndentLevel_ = baseIndent + 1;
            } else {
                NewLineWithIndent(baseIndent + 1);
            }
            for (size_t index = 0; index < node.operators.size(); ++index) {
                WriteBreakToken(node.operators[index]);
                EmitBreakNode(*node.operands[index + 1], solution, baseIndent + 1);
                if (
                    choice == FormatBreakChoice::Split &&
                    index + 1 < node.operators.size() &&
                    !IsFormatBreakStreamConfigurationOperand(
                        *node.operands[index + 1],
                        config_.streamShiftConfigurationMethods
                    )
                ) {
                    NewLineWithIndent(baseIndent + 1);
                }
            }
            return;
        }

        if (node.chainKind == FormatBreakChainKind::MemberBeforeOperator) {
            EmitBreakNode(*node.operands.front(), solution, baseIndent);
            if (choice == FormatBreakChoice::MemberCompactTail) {
                NewLineWithIndent(baseIndent + 1);
                for (size_t index = 0; index < node.operators.size(); ++index) {
                    WriteBreakToken(node.operators[index]);
                    EmitBreakNode(*node.operands[index + 1], solution, baseIndent + 1);
                }
                return;
            }
            for (size_t index = 0; index < node.operators.size(); ++index) {
                NewLineWithIndent(baseIndent + 1);
                WriteBreakToken(node.operators[index]);
                EmitBreakNode(*node.operands[index + 1], solution, baseIndent + 1);
            }
            return;
        }

        if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() > 2) {
            for (size_t index = 0; index < node.operands.size(); ++index) {
                EmitBreakNode(*node.operands[index], solution, index == 0 ? baseIndent : baseIndent + 1);
                if (index < node.operators.size()) {
                    WriteBreakToken(node.operators[index]);
                    if (
                        FormatBreakTokenKind(node.operators[index]) == PrintTokenKind::Known &&
                        FormatBreakTokenSyntaxKind(node.operators[index]) == SyntaxNodeKind::Colon
                    ) {
                        NewLineWithIndent(baseIndent + 1);
                    }
                }
            }
            return;
        }

        if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() == 2) {
            const int continuationIndent = node.flatSplitIndent ? baseIndent : baseIndent + 1;
            const bool breakAfterQuestion =
                choice == FormatBreakChoice::TernaryBreakAfterQuestion || choice == FormatBreakChoice::Split;
            const bool breakAfterColon =
                choice == FormatBreakChoice::TernaryBreakAfterColon || choice == FormatBreakChoice::Split;
            for (size_t index = 0; index < node.operands.size(); ++index) {
                EmitBreakNode(*node.operands[index], solution, index == 0 ? baseIndent : continuationIndent);
                if (index < node.operators.size()) {
                    WriteBreakToken(node.operators[index]);
                    if ((index == 0 && breakAfterQuestion) || (index == 1 && breakAfterColon)) {
                        NewLineWithIndent(continuationIndent);
                    }
                }
            }
            return;
        }

        const int continuationIndent = node.flatSplitIndent ? baseIndent : baseIndent + 1;
        for (size_t index = 0; index < node.operands.size(); ++index) {
            EmitBreakNode(*node.operands[index], solution, index == 0 ? baseIndent : continuationIndent);
            if (index < node.operators.size()) {
                WriteBreakToken(node.operators[index]);
                if (
                    index + 1 < node.operands.size() &&
                    node.operands[index + 1]->kind == FormatBreakNodeKind::Delimited &&
                    ChoiceFor(solution, node.operands[index + 1]->id) == FormatBreakChoice::SplitAttachedOpen
                ) {
                    const size_t attachedOperandIndex = index + 1;
                    EmitDelimitedNodeAfterAttachedOpen(
                        *node.operands[attachedOperandIndex],
                        solution,
                        continuationIndent
                    );
                    if (attachedOperandIndex < node.operators.size()) {
                        WriteBreakToken(node.operators[attachedOperandIndex]);
                        NewLineWithIndent(continuationIndent);
                    }
                    ++index;
                    continue;
                }
                NewLineWithIndent(continuationIndent);
            }
        }
    }

    void EmitAdjacentStringsNode(const FormatBreakNode& node, const FormatBreakSolution& solution, int baseIndent) {
        const FormatBreakChoice choice = ChoiceFor(solution, node.id);
        const int continuationIndent = node.flatSplitIndent ? baseIndent : baseIndent + 1;
        for (size_t index = 0; index < node.operands.size(); ++index) {
            if (choice == FormatBreakChoice::Split && index > 0) {
                NewLineWithIndent(continuationIndent);
            }
            EmitBreakNode(*node.operands[index], solution, index == 0 ? baseIndent : continuationIndent);
        }
    }

    bool UsesSplitContextClose(const FormatBreakNode& node, const FormatBreakSolution& solution) const {
        if (
            node.kind == FormatBreakNodeKind::Delimited &&
            node.children.size() > 1 &&
            node.children[1]->kind == FormatBreakNodeKind::Token &&
            node.children[1]->token.contextOnly &&
            IsSplitChoice(ChoiceFor(solution, node.id))
        ) {
            return true;
        }
        for (const FormatBreakNode* child : node.children) {
            if (child && UsesSplitContextClose(*child, solution)) {
                return true;
            }
        }
        for (const FormatBreakListItem& item : node.items) {
            if (item.node && UsesSplitContextClose(*item.node, solution)) {
                return true;
            }
        }
        for (const FormatBreakNode* operand : node.operands) {
            if (operand && UsesSplitContextClose(*operand, solution)) {
                return true;
            }
        }
        return false;
    }

    bool FlushPendingTokens(const FormatBreakModelContext& context = {}) {
        if (pendingTokens_.empty()) {
            return false;
        }
        if (CanFlushPendingTokensCompact(context)) {
            FlushPendingTokensCompact();
            if (pendingIndentRestoreAfterFlush_) {
                indentLevel_ = *pendingIndentRestoreAfterFlush_;
                pendingIndentRestoreAfterFlush_.reset();
            }
            return false;
        }
        const auto modelStart = std::chrono::steady_clock::now();
        FormatBreakModelContext effectiveContext = context;
        effectiveContext.forceSplitStreamChain = effectiveContext.forceSplitStreamChain || std::any_of(
            pendingTokens_.begin(),
            pendingTokens_.end(),
            [](const PrintToken& token) {
                return SyntaxPathContainsClass(token, SyntaxNodeClass::ConditionalStreamOperatorChain);
            }
        );
        FormatBreakModel model = BuildFormatBreakModel(pendingTokens_, effectiveContext);
        if (stats_ != nullptr) {
            stats_->breakModel += std::chrono::steady_clock::now() - modelStart;
        }
        const bool previousEmittingMacroDefinition = emittingMacroDefinition_;
        emittingMacroDefinition_ = std::any_of(
            pendingTokens_.begin(),
            pendingTokens_.end(),
            [](const PrintToken& token) {
                return token.macroDefinition != nullptr;
            }
        );
        if (emittingMacroDefinition_ && pendingTokens_.front().inMacroValue && atLineStart_) {
            pendingIndentLevel_ = std::max(pendingIndentLevel_.value_or(0), indentLevel_ + 1);
        }
        const int baseIndentLevel = pendingIndentLevel_.value_or(indentLevel_);
        const auto solveStart = std::chrono::steady_clock::now();
        FormatBreakSolution solution = SolveFormatBreaks(
            config_,
            model,
            CurrentColumn(),
            baseIndentLevel,
            indentWidth_,
            emittingMacroDefinition_ ? 2 : 0
        );
        if (stats_ != nullptr) {
            stats_->solve += std::chrono::steady_clock::now() - solveStart;
        }
        bool splitContextClose = false;
        if (model.root) {
            splitContextClose = UsesSplitContextClose(*model.root, solution);
            const auto emitStart = std::chrono::steady_clock::now();
            EmitBreakNode(*model.root, solution, baseIndentLevel);
            if (stats_ != nullptr) {
                stats_->emit += std::chrono::steady_clock::now() - emitStart;
            }
        }
        emittingMacroDefinition_ = previousEmittingMacroDefinition;
        pendingTokens_.clear();
        if (pendingIndentRestoreAfterFlush_) {
            indentLevel_ = *pendingIndentRestoreAfterFlush_;
            pendingIndentRestoreAfterFlush_.reset();
        }
        return splitContextClose;
    }

    bool HasBufferedLineText() const {
        return lineHasText_ || !pendingTokens_.empty();
    }

    std::optional<LambdaSplitListPlan> BuildLambdaSplitListPlan(const PrintToken& token) const {
        if (
            activeTokens_ == nullptr ||
            token.kind != PrintTokenKind::Known ||
            token.syntaxKind != SyntaxNodeKind::LeftBrace ||
            token.inSingleStatementLambdaBody ||
            token.parentKind != SyntaxNodeKind::CompoundStatement ||
            token.grandParentKind != SyntaxNodeKind::LambdaExpression
        ) {
            return std::nullopt;
        }
        const SyntaxNode* compound = NearestAncestor(token, SyntaxNodeKind::CompoundStatement);
        const SyntaxNode* lambda = NearestAncestor(token, SyntaxNodeKind::LambdaExpression);
        if (compound == nullptr || lambda == nullptr) {
            return std::nullopt;
        }
        const SyntaxNode* list = NearestDelimitedListAncestorBefore(token, lambda);
        if (list == nullptr) {
            return std::nullopt;
        }
        const SyntaxNode* listOpen = DirectOpeningDelimiterChild(*list);
        const SyntaxNode* listClose = DirectMatchingClosingDelimiterChild(*list, listOpen);
        const SyntaxNode* lambdaClose = DirectTokenChild(*compound, SyntaxNodeKind::RightBrace);
        if (listOpen == nullptr || listClose == nullptr || lambdaClose == nullptr) {
            return std::nullopt;
        }
        const std::optional<size_t> closeIndex = FindTokenIndex(listClose, currentTokenIndex_ + 1);
        const std::optional<size_t> lambdaCloseIndex = FindTokenIndex(lambdaClose, currentTokenIndex_ + 1);
        if (!closeIndex || !lambdaCloseIndex || *lambdaCloseIndex >= *closeIndex) {
            return std::nullopt;
        }
        FormatBreakToken virtualClose{&(*activeTokens_)[*closeIndex], false, true};
        const int baseIndentLevel = pendingIndentLevel_.value_or(indentLevel_);
        bool hasCommaAfterLambda = false;
        for (size_t index = *lambdaCloseIndex + 1; index < *closeIndex; ++index) {
            const PrintToken& candidate = (*activeTokens_)[index];
            if (
                candidate.kind == PrintTokenKind::Known &&
                candidate.syntaxKind == SyntaxNodeKind::Comma &&
                SyntaxPathContains(candidate, list)
            ) {
                hasCommaAfterLambda = true;
                break;
            }
        }

        return LambdaSplitListPlan{
            .breakContext = {
                .virtualDelimiterOpen = listOpen,
                .virtualDelimiterClose = virtualClose,
                .forceSplitVirtualDelimiter = hasCommaAfterLambda || HasDirectCommentChild(*list)
            },
            .deferredContext = {
                .list = list,
                .lambdaRightBrace = lambdaClose,
                .closeToken = listClose,
                .itemIndent = baseIndentLevel + 1,
                .closeIndent = baseIndentLevel
            }
        };
    }

    std::optional<PreprocessorSplitListPlan> BuildPreprocessorSplitListPlan(const PrintToken& token) const {
        if (activeTokens_ == nullptr || token.node == nullptr || !StartsPreprocessorSplitList(token)) {
            return std::nullopt;
        }
        const SyntaxNode* list = NearestPreprocessorSplitListAncestor(token);
        if (list == nullptr) {
            return std::nullopt;
        }
        const SyntaxNode* listOpen = DirectOpeningDelimiterChild(*list);
        const SyntaxNode* pendingOpen = FindPendingOpeningDelimiterFor(list);
        if (pendingOpen != nullptr) {
            listOpen = pendingOpen;
        }
        if (listOpen == nullptr) {
            return std::nullopt;
        }
        const SyntaxNode* listClose = DirectMatchingClosingDelimiterChild(*list, listOpen);
        std::optional<size_t> closeIndex = FindTokenIndex(listClose, currentTokenIndex_ + 1);
        if (!closeIndex) {
            closeIndex = FindFutureClosingDelimiterFor(list, listOpen->kind);
        }
        if (!closeIndex) {
            return std::nullopt;
        }
        listClose = (*activeTokens_)[*closeIndex].node;
        FormatBreakToken virtualClose{&(*activeTokens_)[*closeIndex], false, true};
        const int itemIndentLevel = pendingIndentLevel_.value_or(indentLevel_ + 1);
        return PreprocessorSplitListPlan{
            .breakContext = {
                .virtualDelimiterOpen = listOpen,
                .virtualDelimiterClose = virtualClose,
                .forceSplitVirtualDelimiter = true
            },
            .deferredContext = {
                .list = list,
                .closeToken = listClose,
                .itemIndent = itemIndentLevel,
                .closeIndent = std::max(0, itemIndentLevel - 1)
            }
        };
    }

    bool ShouldBreakAfterSemicolon() const {
        if (braceStack_.empty()) {
            return parenDepth_ == 0;
        }
        return parenDepth_ <= braceStack_.back().parenDepth;
    }

    bool ShouldContinueMacroLine(const PrintToken& token, const PrintToken* next) const {
        return token.inMacroValue && next != nullptr && next->macroDefinition == token.macroDefinition;
    }

    const PrintToken* RawTokenAfterCurrent(size_t offset) const {
        if (activeTokens_ == nullptr || currentTokenIndex_ + offset >= activeTokens_->size()) {
            return nullptr;
        }
        return &(*activeTokens_)[currentTokenIndex_ + offset];
    }

    bool ShouldBreakAfterCompactEmptyBlock(const PrintToken& token, const PrintToken* afterClose) const {
        if (RoleForBrace(token) != BraceRole::Block) {
            return false;
        }
        if (afterClose == nullptr) {
            return false;
        }
        if (afterClose->kind != PrintTokenKind::Known) {
            return true;
        }
        const bool closesLambdaArgument = token.parentKind == SyntaxNodeKind::CompoundStatement &&
            token.grandParentKind == SyntaxNodeKind::LambdaExpression &&
            afterClose->syntaxKind == SyntaxNodeKind::RightParen;
        return afterClose->syntaxKind != SyntaxNodeKind::Semicolon &&
            afterClose->syntaxKind != SyntaxNodeKind::Comma &&
            !closesLambdaArgument;
    }

    static bool ClosesImmediatelyInvokedLambda(const PrintToken& token, const PrintToken& next) {
        return token.parentKind == SyntaxNodeKind::CompoundStatement &&
            token.grandParentKind == SyntaxNodeKind::LambdaExpression &&
            next.kind == PrintTokenKind::Known &&
            next.syntaxKind == SyntaxNodeKind::LeftParen &&
            next.parentKind == SyntaxNodeKind::ArgumentList;
    }

    DeferredSplitListContext* ActiveDeferredSplitListContext() {
        return deferredSplitListContexts_.empty() ? nullptr : &deferredSplitListContexts_.back();
    }

    PreprocessorSplitListContext* ActivePreprocessorSplitListContextFor(const PrintToken& token) {
        for (
            auto context = preprocessorSplitListContexts_.rbegin();
            context != preprocessorSplitListContexts_.rend();
            ++context
        ) {
            if (SyntaxPathContains(token, context->list)) {
                return &*context;
            }
        }
        return nullptr;
    }

    static bool IsPreprocessorSplitListComma(const PrintToken& token, const PreprocessorSplitListContext& context) {
        if (token.kind != PrintTokenKind::Known || token.syntaxKind != SyntaxNodeKind::Comma || token.node == nullptr) {
            return false;
        }
        if (token.node->parent == context.list) {
            return true;
        }
        return token.node->parent != nullptr &&
            SyntaxNodeKindHasClass(token.node->parent->kind, SyntaxNodeClass::ConditionalPreprocessorTree) &&
            SyntaxPathContains(token, context.list);
    }

    static bool IsForcedLeadingPreprocessorListComma(const PrintToken& token) {
        return token.kind == PrintTokenKind::Known &&
            token.syntaxKind == SyntaxNodeKind::Comma &&
            token.node != nullptr &&
            IsFirstConditionalBranchChild(*token.node);
    }

    bool IsFinalPreprocessorSplitListItem(const PrintToken& token) const {
        if (activeTokens_ == nullptr || token.node == nullptr) {
            return false;
        }
        const SyntaxNode* list = NearestPreprocessorSplitListAncestor(token);
        const SyntaxNode* open = list == nullptr ? nullptr : DirectOpeningDelimiterChild(*list);
        const SyntaxNode* close = list == nullptr ? nullptr : DirectMatchingClosingDelimiterChild(*list, open);
        if (close == nullptr) {
            return false;
        }

        for (size_t index = currentTokenIndex_ + 1; index < activeTokens_->size(); ++index) {
            const PrintToken& candidate = (*activeTokens_)[index];
            if (candidate.kind == PrintTokenKind::BlankLine || IsCommentToken(candidate.kind)) {
                continue;
            }
            if (!SyntaxPathContains(candidate, list)) {
                continue;
            }
            return candidate.kind == PrintTokenKind::Known && candidate.node == close;
        }
        return false;
    }

    bool TryPrintPreprocessorSplitListComma(const PrintToken& token) {
        PreprocessorSplitListContext* context = ActivePreprocessorSplitListContextFor(token);
        if (context == nullptr || !IsPreprocessorSplitListComma(token, *context)) {
            return false;
        }
        BufferToken(token);
        if (IsForcedLeadingPreprocessorListComma(token)) {
            return true;
        }
        FlushPendingTokens();
        NewLineWithIndent(context->itemIndent);
        return true;
    }

    bool TryPrintPreprocessorSplitListClose(const PrintToken& token) {
        PreprocessorSplitListContext* context = ActivePreprocessorSplitListContextFor(token);
        if (context == nullptr || token.kind != PrintTokenKind::Known || context->closeToken != token.node) {
            return false;
        }
        if (HasBufferedLineText()) {
            FlushPendingTokens();
        }
        NewLineWithIndent(context->closeIndent);
        BufferToken(token);
        preprocessorSplitListContexts_.pop_back();
        return true;
    }

    bool TryPrintConditionalPreprocessorListOpen(const PrintToken& token) {
        if (
            token.kind != PrintTokenKind::Known ||
            !SyntaxNodeKindHasClass(token.syntaxKind, SyntaxNodeClass::OpeningDelimiter) ||
            ImmediateConditionalPreprocessorListParent(token) == nullptr
        ) {
            return false;
        }
        BufferToken(token);
        FlushPendingTokens();
        NewLineWithIndent(CurrentLineIndentLevel() + 1);
        return true;
    }

    bool TryPrintConditionalPreprocessorListComma(const PrintToken& token) {
        if (
            token.kind != PrintTokenKind::Known ||
            token.syntaxKind != SyntaxNodeKind::Comma ||
            ImmediateConditionalPreprocessorListParent(token) == nullptr
        ) {
            return false;
        }
        BufferToken(token);
        FlushPendingTokens();
        NewLineWithIndent(CurrentLineIndentLevel());
        return true;
    }

    bool TryPrintConditionalPreprocessorListClose(const PrintToken& token) {
        if (
            token.kind != PrintTokenKind::Known ||
            MatchingClosingDelimiterToken(token.syntaxKind) != SyntaxNodeKind::Unknown ||
            ImmediateConditionalPreprocessorListParent(token) == nullptr
        ) {
            return false;
        }
        if (HasBufferedLineText()) {
            FlushPendingTokens();
        }
        NewLineWithIndent(indentLevel_);
        BufferToken(token);
        return true;
    }

    void MarkDeferredSplitLambdaClosed(const PrintToken& token) {
        DeferredSplitListContext* context = ActiveDeferredSplitListContext();
        if (context != nullptr && context->lambdaRightBrace == token.node) {
            context->afterLambdaClose = true;
        }
    }

    bool TryPrintDeferredSplitListComma(const PrintToken& token) {
        DeferredSplitListContext* context = ActiveDeferredSplitListContext();
        if (
            context == nullptr ||
            !context->afterLambdaClose ||
            token.kind != PrintTokenKind::Known ||
            token.syntaxKind != SyntaxNodeKind::Comma ||
            !SyntaxPathContains(token, context->list)
        ) {
            return false;
        }
        BufferToken(token);
        FlushPendingTokens();
        NewLineWithIndent(context->itemIndent);
        return true;
    }

    bool TryPrintDeferredSplitListClose(const PrintToken& token) {
        DeferredSplitListContext* context = ActiveDeferredSplitListContext();
        if (
            context == nullptr ||
            !context->afterLambdaClose ||
            token.kind != PrintTokenKind::Known ||
            context->closeToken != token.node
        ) {
            return false;
        }
        if (HasBufferedLineText()) {
            FlushPendingTokens();
        }
        NewLineWithIndent(context->closeIndent);
        BufferToken(token);
        deferredSplitListContexts_.pop_back();
        return true;
    }

    void PrepareMacroBoundary(const PrintToken* previous, const PrintToken& current) {
        if (current.macroDefinition != nullptr && !current.inMacroValue && atLineStart_) {
            forceColumnZeroLine_ = true;
            pendingIndentLevel_.reset();
        }
        if (
            previous != nullptr &&
            previous->macroDefinition != nullptr &&
            previous->macroDefinition != current.macroDefinition
        ) {
            if (HasBufferedLineText()) {
                FlushPendingTokens();
                NewLine(false);
            }
            if (current.macroDefinition != nullptr && !current.inMacroValue) {
                forceColumnZeroLine_ = true;
                pendingIndentLevel_.reset();
            }
            if (!IsPreprocessorLikeToken(current)) {
                BlankLine();
            }
        }
        if (
            current.macroDefinition != nullptr &&
            (previous == nullptr || previous->macroDefinition != current.macroDefinition) &&
            HasBufferedLineText()
        ) {
            FlushPendingTokens();
            NewLine(false);
            if (!current.inMacroValue) {
                forceColumnZeroLine_ = true;
                pendingIndentLevel_.reset();
            }
        }
    }

    bool CanAttachToPreviousPreprocessorLine(const PrintToken& token, const PrintToken* rawPrevious) const {
        return token.kind == PrintTokenKind::TrailingComment &&
            rawPrevious != nullptr &&
            rawPrevious->kind == PrintTokenKind::Preprocessor &&
            SyntaxNodeKindHasClass(rawPrevious->syntaxKind, SyntaxNodeClass::EndifDirective);
    }

    void PrintOne(
        const PrintToken& token,
        const PrintToken* previous,
        const PrintToken* rawPrevious,
        const PrintToken* next,
        const PrintToken* rawNext
    ) {
        if (PrepareDeclarationGroupBoundary(token)) {
            return;
        }
        PreparePreprocessorGroupBoundary(token, previous, next);
        PrepareMacroBoundary(rawPrevious, token);
        if (token.kind == PrintTokenKind::BlankLine) {
            FlushPendingTokens();
            BlankLine();
            return;
        }
        if (IsCommentToken(token.kind)) {
            if (KeepsListCommentInBreakModel(token)) {
                BufferToken(token);
                return;
            }
            if (
                token.kind == PrintTokenKind::TrailingComment &&
                !CanAttachToPreviousPreprocessorLine(token, rawPrevious)
            ) {
                BufferToken(token);
                FlushPendingTokens();
                return;
            }
            FlushPendingTokens();
            if (CanAttachToPreviousPreprocessorLine(token, rawPrevious)) {
                ReopenLastOutputLine();
            }
            PrintComment(token, next);
            return;
        }
        if (token.kind == PrintTokenKind::Preprocessor) {
            FlushPendingTokens();
            PrintPreprocessor(token, next);
            return;
        }
        if (token.kind == PrintTokenKind::IncludeRun) {
            FlushPendingTokens();
            PrintIncludeRun(token, next);
            return;
        }
        if (
            token.kind == PrintTokenKind::Free &&
            token.syntaxKind == SyntaxNodeKind::PreprocAssignmentStatement
        ) {
            FlushPendingTokens();
            PrintPreprocessorAssignmentStatement(token);
            return;
        }
        if (token.kind == PrintTokenKind::Known) {
            PrintKnown(token, previous, next, rawNext);
            return;
        }
        if (token.syntaxKind == SyntaxNodeKind::RawMacroReplacement) {
            FlushPendingTokens();
            Write(FormatRawMacroReplacement(token.text, CurrentLineIndentLevel() + 1, indentWidth_, tabWidth_));
            NewLine();
            return;
        }
        if (IsRawStatementToken(token)) {
            FlushPendingTokens();
            if (lineHasText_) {
                NewLine();
            }
            Write(CollapseSourceWhitespace(token.text));
            if (!(rawNext != nullptr && rawNext->kind == PrintTokenKind::TrailingComment)) {
                NewLine(ShouldContinueMacroLine(token, next));
            }
            return;
        }
        BufferToken(token);
    }

    void PrintPreprocessorAssignmentStatement(const PrintToken& token) {
        if (lineHasText_) {
            NewLine();
        }
        const std::string normalized = PreserveSourceLines(token.text);
        const size_t conditionalStart = normalized.find('#');
        if (conditionalStart == std::string::npos) {
            Write(NormalizeTrailingLineCommentSpacing(CollapseSourceWhitespace(TrimSourceLine(normalized))));
            NewLine();
            return;
        }
        Write(FormatConditionalAssignmentPrefix(std::string_view(normalized).substr(0, conditionalStart)));
        const int continuationIndent = CurrentLineIndentLevel() + 1;
        NewLine();
        const std::string outputLine = FormatConditionalRhsPreprocessorLines(
            std::string_view(normalized).substr(conditionalStart),
            continuationIndent,
            indentWidth_
        );
        output_.append(outputLine);
        AdvanceCurrentColumn(outputLine);
        lineHasText_ = true;
        atLineStart_ = false;
        NewLine();
    }

    void PrintComment(const PrintToken& token, const PrintToken* next) {
        if (token.kind == PrintTokenKind::TrailingComment && lineHasText_) {
            Space();
            output_.push_back(' ');
            ++currentColumn_;
            Write(token.text);
            NewLine(ShouldContinueMacroLine(token, next));
            return;
        }
        if (lineHasText_) {
            NewLine(ShouldContinueMacroLine(token, next));
        }
        Write(token.text);
        NewLine(ShouldContinueMacroLine(token, next));
    }

    void PrintIncludeRun(const PrintToken& token, const PrintToken* next) {
        if (token.node == nullptr) {
            return;
        }
        if (lineHasText_) {
            NewLine();
        }
        const std::string text = FormatIncludeRunText(config_, *token.node, sourcePath_);
        output_.append(text);
        currentColumn_ = 0;
        atLineStart_ = true;
        lineHasText_ = false;
        if (
            !text.empty() &&
            next != nullptr &&
            !SyntaxNodeKindHasClass(next->syntaxKind, SyntaxNodeClass::ConditionalBranchSeparatorDirective)
        ) {
            BlankLine();
        }
    }

    void PrintPreprocessor(const PrintToken& token, const PrintToken* next) {
        const bool hasLineBreak = ContainsSourceLineBreak(token.text);
        const std::string line = CanonicalizePreprocessorDirectiveLines(
            hasLineBreak ? PreservePreprocessorLines(token.text) :
            NormalizeTrailingLineCommentSpacing(CollapseSourceWhitespace(token.text))
        );
        const bool isInclude = SyntaxNodeKindHasClass(token.syntaxKind, SyntaxNodeClass::IncludeDirective) ||
            PreprocessorLineHasClass(line, SyntaxNodeClass::IncludeDirective);
        const bool listConditional =
            StartsPreprocessorSplitList(token) && NearestPreprocessorSplitListAncestor(token) != nullptr;
        const SyntaxNodeKind lineDirectiveKind = SyntaxNodeKindFromPreprocessorDirectiveLine(line);
        const bool closesConditionalFunctionHeader =
            ((token.node != nullptr && IsPreprocEndifToken(*token.node)) ||
                token.syntaxKind == SyntaxNodeKind::PreprocessorDirectiveEndif ||
                lineDirectiveKind == SyntaxNodeKind::PreprocessorDirectiveEndif) &&
            IsWithinConditionalFunctionHeader(token);
        if (IsConditionalRhsPreprocessorToken(token)) {
            if (HasBufferedLineText()) {
                FlushPendingTokens();
            }
            const int continuationIndent = (lineHasText_ ? CurrentLineIndentLevel() : indentLevel_) + 1;
            if (lineHasText_) {
                NewLine();
            }
            const std::string outputLine =
                FormatConditionalRhsPreprocessorLines(token.text, continuationIndent, indentWidth_);
            output_.append(outputLine);
            AdvanceCurrentColumn(outputLine);
            lineHasText_ = true;
            atLineStart_ = false;
            NewLine();
            return;
        }
        if (IsDeclarationModifierPreprocessorToken(token)) {
            if (HasBufferedLineText()) {
                FlushPendingTokens();
            }
            if (lineHasText_) {
                NewLine();
            }
            const int declarationIndent = pendingIndentLevel_.value_or(indentLevel_);
            const std::string outputLine =
                FormatDeclarationModifierPreprocessorLines(token.text, declarationIndent, indentWidth_);
            output_.append(outputLine);
            AdvanceCurrentColumn(outputLine);
            lineHasText_ = true;
            atLineStart_ = false;
            NewLine();
            pendingIndentLevel_ = declarationIndent;
            return;
        }
        if (token.structuredPreprocessor || isInclude || listConditional) {
            PreprocessorSplitListContext* splitContext = ActivePreprocessorSplitListContextFor(token);
            const std::optional<PreprocessorSplitListPlan> splitListPlan =
                splitContext == nullptr ? BuildPreprocessorSplitListPlan(token) : std::nullopt;
            std::optional<int> listItemIndent;
            if (splitListPlan) {
                FlushPendingTokens(splitListPlan->breakContext);
                preprocessorSplitListContexts_.push_back(splitListPlan->deferredContext);
                splitContext = &preprocessorSplitListContexts_.back();
            } else if (splitContext != nullptr && HasBufferedLineText()) {
                FlushPendingTokens();
            } else if (token.structuredPreprocessor && HasBufferedLineText()) {
                FlushPendingTokens();
            }
            if (splitContext != nullptr) {
                listItemIndent = splitContext->itemIndent;
            } else if (listConditional) {
                listItemIndent = pendingIndentLevel_.value_or(indentLevel_ + 1);
            }
            if (lineHasText_) {
                NewLine();
            }
            const std::string outputLine = listConditional && !token.structuredPreprocessor && listItemIndent ?
                FormatListPreprocessorLines(token.text, *listItemIndent, indentWidth_, IsFinalPreprocessorSplitListItem(
                    token
                )) : line;
            output_.append(outputLine);
            AdvanceCurrentColumn(outputLine);
            lineHasText_ = true;
            atLineStart_ = false;
            NewLine();
            if (closesConditionalFunctionHeader) {
                conditionalFunctionIndents_.push_back(indentLevel_);
                ++indentLevel_;
            }
            if (listItemIndent) {
                pendingIndentLevel_ = *listItemIndent;
            }
            return;
        }
        const bool inlineFragment = token.parentKind == SyntaxNodeKind::ArgumentList ||
            token.parentKind == SyntaxNodeKind::BinaryExpression ||
            token.parentKind == SyntaxNodeKind::ConditionClause ||
            token.grandParentKind == SyntaxNodeKind::ArgumentList;
        const bool isConditionalDirective =
            SyntaxNodeKindHasClass(token.syntaxKind, SyntaxNodeClass::ConditionalPreprocessorDirective) ||
            SyntaxNodeKindHasClass(lineDirectiveKind, SyntaxNodeClass::ConditionalPreprocessorDirective);
        if (lineHasText_) {
            NewLine();
        }
        output_.append(line);
        AdvanceCurrentColumn(line);
        lineHasText_ = true;
        atLineStart_ = false;
        NewLine();
        if (closesConditionalFunctionHeader) {
            conditionalFunctionIndents_.push_back(indentLevel_);
            ++indentLevel_;
            return;
        }
        if (
            !inlineFragment && !isInclude && !isConditionalDirective && next != nullptr &&
            !IsPreprocessorLikeToken(*next) && AreNeighboringSourceItems(token, *next)
        ) {
            BlankLine();
        }
    }

    void PrintKnown(
        const PrintToken& token,
        const PrintToken* previous,
        const PrintToken* next,
        const PrintToken* rawNext
    ) {
        switch (token.syntaxKind) {
            case SyntaxNodeKind::LeftParen:
                if (TryPrintConditionalPreprocessorListOpen(token)) {
                    ++parenDepth_;
                    return;
                }
                BufferToken(token);
                ++parenDepth_;
                return;
            case SyntaxNodeKind::RightParen:
                if (TryPrintPreprocessorSplitListClose(token)) {
                    if (parenDepth_ > 0) {
                        --parenDepth_;
                    }
                    return;
                }
                if (TryPrintConditionalPreprocessorListClose(token)) {
                    if (parenDepth_ > 0) {
                        --parenDepth_;
                    }
                    return;
                }
                if (TryPrintDeferredSplitListClose(token)) {
                    if (parenDepth_ > 0) {
                        --parenDepth_;
                    }
                    return;
                }
                BufferToken(token);
                if (parenDepth_ > 0) {
                    --parenDepth_;
                }
                if (ClosesStatementPositionMacroCallItem(token) && !(
                    rawNext != nullptr && rawNext->kind == PrintTokenKind::TrailingComment
                )) {
                    FlushPendingTokens();
                    NewLine(ShouldContinueMacroLine(token, next));
                    return;
                }
                if (token.parentKind == SyntaxNodeKind::RequiresClause && token.inTemplateDeclaration) {
                    FlushPendingTokens();
                    NewLine(ShouldContinueMacroLine(token, next));
                }
                return;
            case SyntaxNodeKind::LeftBracket:
                if (TryPrintConditionalPreprocessorListOpen(token)) {
                    ++bracketDepth_;
                    return;
                }
                BufferToken(token);
                ++bracketDepth_;
                return;
            case SyntaxNodeKind::RightBracket:
                if (TryPrintPreprocessorSplitListClose(token)) {
                    if (bracketDepth_ > 0) {
                        --bracketDepth_;
                    }
                    return;
                }
                if (TryPrintConditionalPreprocessorListClose(token)) {
                    if (bracketDepth_ > 0) {
                        --bracketDepth_;
                    }
                    return;
                }
                if (TryPrintDeferredSplitListClose(token)) {
                    if (bracketDepth_ > 0) {
                        --bracketDepth_;
                    }
                    return;
                }
                BufferToken(token);
                if (bracketDepth_ > 0) {
                    --bracketDepth_;
                }
                return;
            case SyntaxNodeKind::Less:
                if (TryPrintConditionalPreprocessorListOpen(token)) {
                    return;
                }
                BufferToken(token);
                return;
            case SyntaxNodeKind::Greater:
                if (TryPrintPreprocessorSplitListClose(token)) {
                    return;
                }
                if (TryPrintConditionalPreprocessorListClose(token)) {
                    return;
                }
                BufferToken(token);
                if (token.parentKind == SyntaxNodeKind::TemplateParameterList && token.inTemplateDeclaration && !(
                    next != nullptr &&
                    next->kind == PrintTokenKind::Known &&
                    next->syntaxKind == SyntaxNodeKind::KeywordRequires
                )) {
                    FlushPendingTokens();
                    NewLine(ShouldContinueMacroLine(token, next));
                }
                return;
            case SyntaxNodeKind::LeftBrace:
                if (TryPrintConditionalPreprocessorListOpen(token)) {
                    return;
                }
                PrintLeftBrace(token, previous, rawNext);
                return;
            case SyntaxNodeKind::RightBrace:
                if (TryPrintPreprocessorSplitListClose(token)) {
                    return;
                }
                if (TryPrintConditionalPreprocessorListClose(token)) {
                    return;
                }
                if (TryPrintDeferredSplitListClose(token)) {
                    return;
                }
                PrintRightBrace(token, next, rawNext);
                return;
            case SyntaxNodeKind::Semicolon:
                BufferToken(token);
                if (!token.inSingleStatementLambdaBody && ShouldBreakAfterSemicolon() && !(
                    rawNext != nullptr && rawNext->kind == PrintTokenKind::TrailingComment
                )) {
                    FlushPendingTokens();
                    NewLine(ShouldContinueMacroLine(token, next));
                }
                return;
            case SyntaxNodeKind::Comma:
                if (TryPrintConditionalPreprocessorListComma(token)) {
                    return;
                }
                if (TryPrintPreprocessorSplitListComma(token)) {
                    return;
                }
                if (TryPrintDeferredSplitListComma(token)) {
                    return;
                }
                BufferToken(token);
                if (token.parentKind == SyntaxNodeKind::EnumeratorList && !(
                    rawNext != nullptr && rawNext->kind == PrintTokenKind::TrailingComment
                )) {
                    FlushPendingTokens();
                    NewLine(ShouldContinueMacroLine(token, next));
                }
                return;
            case SyntaxNodeKind::Colon:
                BufferToken(token);
                if (token.parentKind == SyntaxNodeKind::CaseStatement) {
                    FlushPendingTokens();
                    ++indentLevel_;
                    activeCaseBodySwitchDepths_.push_back(switchDepth_);
                    if (
                        rawNext != nullptr &&
                        rawNext->kind == PrintTokenKind::Known &&
                        rawNext->syntaxKind == SyntaxNodeKind::LeftBrace
                    ) {
                        return;
                    }
                    if (rawNext != nullptr && rawNext->kind == PrintTokenKind::TrailingComment) {
                        return;
                    }
                    NewLine(ShouldContinueMacroLine(token, next));
                    return;
                }
                if (previous != nullptr && previous->kind == PrintTokenKind::Known && (
                    previous->syntaxKind == SyntaxNodeKind::KeywordDefault ||
                    SyntaxNodeKindHasClass(previous->syntaxKind, SyntaxNodeClass::AccessKeyword)
                )) {
                    FlushPendingTokens();
                    NewLine(ShouldContinueMacroLine(token, next));
                }
                return;
            default:
                if (IsCaseLabelKeyword(token) && atLineStart_) {
                    CloseCaseBodyIndentIfNeeded();
                }
                if (IsAccessLabel(token, next)) {
                    FlushPendingTokens();
                    WriteWithIndentOffset(FormatTokenText(token), -1);
                    return;
                }
                BufferToken(token);
                return;
        }
    }

    void PrintLeftBrace(const PrintToken& token, const PrintToken* previous, const PrintToken* rawNext) {
        if (IsWithinConditionalFunctionHeader(token)) {
            BufferToken(token);
            FlushPendingTokens();
            NewLine(ShouldContinueMacroLine(token, rawNext));
            return;
        }
        const bool isEmptyBracePair = rawNext != nullptr &&
            rawNext->kind == PrintTokenKind::Known &&
            rawNext->syntaxKind == SyntaxNodeKind::RightBrace;
        const bool isCaseBlock = previous != nullptr &&
            previous->kind == PrintTokenKind::Known &&
            previous->syntaxKind == SyntaxNodeKind::Colon &&
            previous->parentKind == SyntaxNodeKind::CaseStatement;
        const BraceRole role = isCaseBlock ? BraceRole::CaseBlock : RoleForBrace(token);
        if (role == BraceRole::CaseBlock && lineHasText_) {
            Space();
        }
        if (isEmptyBracePair) {
            PrintToken compact = token;
            compact.kind = PrintTokenKind::Free;
            compact.syntaxKind = SyntaxNodeKind::Unknown;
            compact.text = "{}";
            BufferToken(compact);
            ++compactRightBraceSkips_;
            const PrintToken* afterClose = RawTokenAfterCurrent(2);
            if (ShouldBreakAfterCompactEmptyBlock(token, afterClose)) {
                FlushPendingTokens();
                NewLine(ShouldContinueMacroLine(token, afterClose));
            }
            return;
        }
        const std::optional<LambdaSplitListPlan> splitListPlan = BuildLambdaSplitListPlan(token);
        BufferToken(token);
        if (role == BraceRole::Compact) {
            return;
        }
        const bool splitList =
            FlushPendingTokens(splitListPlan ? splitListPlan->breakContext : FormatBreakModelContext{});
        if (splitListPlan && splitList) {
            deferredSplitListContexts_.push_back(splitListPlan->deferredContext);
        }
        const bool functionBlock = token.parentKind == SyntaxNodeKind::CompoundStatement &&
            token.grandParentKind == SyntaxNodeKind::FunctionDefinition;
        int openLineIndent = splitListPlan && splitList ? splitListPlan->deferredContext.itemIndent : (
            token.inMacroValue || functionBlock ?
                indentLevel_ : (lineHasText_ ? CurrentLineIndentLevel() : indentLevel_)
        );
        if (
            token.parentKind == SyntaxNodeKind::RequirementSeq &&
            token.inTemplateDeclaration &&
            token.inRequiresClause
        ) {
            openLineIndent = std::max(openLineIndent, indentLevel_ + 1);
        }
        braceStack_.push_back({
            .role = role,
            .parenDepth = parenDepth_,
            .indentRestore = indentLevel_,
            .closeIndent = role == BraceRole::Block || role == BraceRole::Enum ? openLineIndent : indentLevel_
        });
        if (
            token.parentKind == SyntaxNodeKind::CompoundStatement &&
            token.grandParentKind == SyntaxNodeKind::SwitchStatement
        ) {
            ++switchDepth_;
        }
        if (role == BraceRole::Block || role == BraceRole::Enum) {
            indentLevel_ = std::max(indentLevel_, openLineIndent) + 1;
            NewLine(ShouldContinueMacroLine(token, rawNext));
        } else if (role == BraceRole::NamespaceLike || role == BraceRole::CaseBlock) {
            NewLine(ShouldContinueMacroLine(token, rawNext));
            if (role == BraceRole::NamespaceLike) {
                BlankLine();
            }
        }
    }

    void PrintRightBrace(const PrintToken& token, const PrintToken* next, const PrintToken* rawNext) {
        if (compactRightBraceSkips_ > 0) {
            --compactRightBraceSkips_;
            return;
        }
        if (
            token.parentKind == SyntaxNodeKind::CompoundStatement &&
            token.grandParentKind == SyntaxNodeKind::FunctionDefinition &&
            token.node != nullptr &&
            token.node->parent != nullptr &&
            !HasDirectTokenChild(*token.node->parent, SyntaxNodeKind::LeftBrace) &&
            !conditionalFunctionIndents_.empty()
        ) {
            FlushPendingTokens();
            if (lineHasText_) {
                NewLine(token.inMacroValue);
            }
            indentLevel_ = conditionalFunctionIndents_.back();
            conditionalFunctionIndents_.pop_back();
            BufferToken(token);
            FlushPendingTokens();
            if (next != nullptr && next->kind == PrintTokenKind::TrailingComment) {
                return;
            }
            NewLine(ShouldContinueMacroLine(token, next));
            return;
        }
        const BraceRole tokenRole = RoleForBrace(token);
        if (tokenRole == BraceRole::Compact) {
            BufferToken(token);
            return;
        }
        const BraceRole role = braceStack_.empty() ? tokenRole : braceStack_.back().role;
        FlushPendingTokens();
        // A non-compact closing brace is positioned by its brace frame, never by a pending
        // declaration or list-item indent left by the preceding construct.
        pendingIndentLevel_.reset();
        std::optional<int> restoreIndent;
        std::optional<int> closeIndent;
        if (!braceStack_.empty()) {
            restoreIndent = braceStack_.back().indentRestore;
            closeIndent = braceStack_.back().closeIndent;
            braceStack_.pop_back();
        }
        if (role == BraceRole::NamespaceLike) {
            if (lineHasText_) {
                NewLine(token.inMacroValue);
            }
            BlankLine();
            BufferToken(token);
            FlushPendingTokens();
            if (rawNext != nullptr && rawNext->kind == PrintTokenKind::TrailingComment) {
                return;
            }
            NewLine(ShouldContinueMacroLine(token, next));
            return;
        }
        if (role == BraceRole::CaseBlock) {
            if (lineHasText_) {
                NewLine(token.inMacroValue);
            }
            WriteWithIndentOffset("}", -1);
            NewLine(ShouldContinueMacroLine(token, next));
            return;
        }
        if (role != BraceRole::Compact) {
            if (lineHasText_) {
                NewLine(token.inMacroValue);
            }
            const bool isSwitchBody = token.parentKind == SyntaxNodeKind::CompoundStatement &&
                token.grandParentKind == SyntaxNodeKind::SwitchStatement;
            if (isSwitchBody) {
                CloseCaseBodyIndentIfNeeded();
            }
            indentLevel_ = closeIndent.value_or(restoreIndent.value_or(std::max(0, indentLevel_ - 1)));
            if (restoreIndent && *restoreIndent != indentLevel_) {
                pendingIndentRestoreAfterFlush_ = *restoreIndent;
            }
            BufferToken(token);
            MarkDeferredSplitLambdaClosed(token);
            if (isSwitchBody) {
                switchDepth_ = std::max(0, switchDepth_ - 1);
            }
            if (next != nullptr && next->kind == PrintTokenKind::Known) {
                const bool closesLambdaArgument = token.parentKind == SyntaxNodeKind::CompoundStatement &&
                    token.grandParentKind == SyntaxNodeKind::LambdaExpression &&
                    next->syntaxKind == SyntaxNodeKind::RightParen;
                const bool closesCompoundExpression = RightParenClosesCompoundExpression(token, *next);
                const bool attachesToFollowingKeyword =
                    SyntaxNodeKindHasClass(next->syntaxKind, SyntaxNodeClass::AttachAfterBlockKeyword) &&
                        next->syntaxKind != SyntaxNodeKind::KeywordWhile;
                const bool closesDoWhile =
                    next->syntaxKind == SyntaxNodeKind::KeywordWhile && next->parentKind == SyntaxNodeKind::DoStatement;
                if (
                    next->syntaxKind == SyntaxNodeKind::Semicolon ||
                    next->syntaxKind == SyntaxNodeKind::Comma || (
                        token.parentKind == SyntaxNodeKind::RequirementSeq &&
                        SyntaxNodeKindHasClass(next->syntaxKind, SyntaxNodeClass::BinaryOperator)
                    ) ||
                    closesLambdaArgument ||
                    ClosesImmediatelyInvokedLambda(token, *next) ||
                    closesCompoundExpression ||
                    attachesToFollowingKeyword ||
                    closesDoWhile
                ) {
                    return;
                }
            }
            if (next != nullptr) {
                const bool closesStructOrClassBody = token.parentKind == SyntaxNodeKind::FieldDeclarationList && (
                    token.grandParentKind == SyntaxNodeKind::StructSpecifier ||
                    token.grandParentKind == SyntaxNodeKind::ClassSpecifier
                );
                const bool closesEnumBody =
                    token.parentKind == SyntaxNodeKind::EnumeratorList &&
                    token.grandParentKind == SyntaxNodeKind::EnumSpecifier;
                if (closesStructOrClassBody || closesEnumBody) {
                    return;
                }
            }
            FlushPendingTokens();
            NewLine(ShouldContinueMacroLine(token, next));
            return;
        }
    }
};

}  // namespace

std::string FormatModelText(const FormatterConfig& config, const FormatModel& model, std::string_view sourcePath) {
    FormatModelTextStats stats;
    return FormatModelText(config, model, sourcePath, stats);
}

std::string FormatModelText(
    const FormatterConfig& config,
    const FormatModel& model,
    std::string_view sourcePath,
    FormatModelTextStats& stats
) {
    if (!model.root) {
        return {};
    }
    const auto tokenizeStart = std::chrono::steady_clock::now();
    std::vector<PrintToken> tokens;
    const size_t sourceSize = model.sourceText != nullptr ? model.sourceText->size() : 0;
    tokens.reserve(std::max<size_t>(256, sourceSize / 4));
    AppendTokens(
        *model.root,
        SyntaxNodeKind::Unknown,
        SyntaxNodeKind::Unknown,
        false,
        false,
        false,
        false,
        nullptr,
        false,
        tokens
    );
    stats.tokenize += std::chrono::steady_clock::now() - tokenizeStart;
    const auto printStart = std::chrono::steady_clock::now();
    std::string result = Printer(config, sourcePath, &stats).Print(tokens);
    stats.print += std::chrono::steady_clock::now() - printStart;
    return result;
}

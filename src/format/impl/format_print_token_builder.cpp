#include "format/impl/format_print_token_builder.h"

#include <algorithm>
#include <optional>

#include "format/impl/format_spacing.h"
#include "util/strings.h"

namespace {

bool IsPreprocessorNode(const SyntaxNode& node) {
    return SyntaxNodeHasClass(node, SyntaxNodeClass::AtomicPreprocessor);
}

bool ContainsSourceLineBreak(std::string_view text);

void InitializePrintTokenTraits(PrintToken& token) {
    if (token.kind == PrintTokenKind::Known && token.text.empty()) {
        token.text = SyntaxNodeKindTokenText(token.syntaxKind);
    }
    token.syntaxClasses = SyntaxNodeKindClasses(token.syntaxKind);
    token.stringLike = PrintTokenSyntaxHasClass(token, SyntaxNodeClass::StringLike) ||
        (token.kind == PrintTokenKind::Text && token.text.find('"') != std::string_view::npos);
    token.containsSourceLineBreak = ContainsSourceLineBreak(FormatTokenText(token));
    token.inFieldInitializerList = token.parentKind == SyntaxNodeKind::FieldInitializerList ||
        token.grandParentKind == SyntaxNodeKind::FieldInitializerList;
}

enum PrintTokenAncestryFlag : std::uint8_t {
    InMacroStatementSequence = 1u << 0,
    InLeadingStreamOperatorChain = 1u << 1,
    InConditionalStreamOperatorChain = 1u << 2,
    InConditionalFunctionHeader = 1u << 3,
    InBareMacroItem = 1u << 4,
    InTemplateList = 1u << 5,
};

void ApplyPrintTokenAncestryTraits(
    PrintToken& token,
    std::uint8_t flags,
    const SyntaxNode* declarationScopeItem,
    bool inTemplateDeclarationBlock,
    bool inTemplateDeclarationHeader
) {
    token.inMacroStatementSequence = (flags & InMacroStatementSequence) != 0;
    token.inLeadingStreamOperatorChain = (flags & InLeadingStreamOperatorChain) != 0;
    token.inConditionalStreamOperatorChain = (flags & InConditionalStreamOperatorChain) != 0;
    token.inConditionalFunctionHeader = (flags & InConditionalFunctionHeader) != 0;
    token.inBareMacroItem = (flags & InBareMacroItem) != 0;
    token.inTemplateList = (flags & InTemplateList) != 0;
    token.inTemplateDeclarationBlock = inTemplateDeclarationBlock;
    token.inTemplateDeclarationHeader = inTemplateDeclarationHeader;
    token.declarationScopeItem = declarationScopeItem;
}

bool IsStandalonePreprocessorBranchToken(const SyntaxNode& node, SyntaxNodeKind parentKind) {
    return parentKind == SyntaxNodeKind::PreprocElse && node.kind == SyntaxNodeKind::PreprocessorDirectiveElse;
}

bool IsStructuredConditionalPreprocessorNode(const SyntaxNode& node) {
    return SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::ConditionalPreprocessorTree) && !IsPreprocessorNode(node);
}

bool IsSourceLineBreak(char ch) { return ch == '\r' || ch == '\n'; }

size_t FindSourceLineBreak(std::string_view text, size_t start = 0) {
    for (size_t index = start; index < text.size(); ++index) {
        if (IsSourceLineBreak(text[index])) {
            return index;
        }
    }
    return std::string_view::npos;
}

bool ContainsSourceLineBreak(std::string_view text) { return FindSourceLineBreak(text) != std::string_view::npos; }

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
    return node.kind == SyntaxNodeKind::LexicalToken && ContainsSourceLineBreak(node.text);
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
    if (parent == nullptr || !SyntaxNodeKindHasClass(parent->kind, SyntaxNodeClass::ConditionalPreprocessorTree)) {
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
        if (SyntaxNodeHasClass(*child, SyntaxNodeClass::Trivia)) {
            continue;
        }
        return child == &node;
    }
    return false;
}

std::string_view PreprocEndifLine(const SyntaxNode& node) {
    return node.text.empty() ? SyntaxNodeKindTokenText(node.kind) : TrimWhitespaceView(FirstSourceLine(node.text));
}

void AppendPreprocessorPrintToken(
    const SyntaxNode& node,
    std::string_view text,
    SyntaxNodeKind parentKind,
    SyntaxNodeKind grandParentKind,
    bool inTemplateDeclaration,
    bool inRequiresClause,
    bool inCompilerCallModifier,
    bool inCompactSingleStatementBody,
    bool inMacroValue,
    bool structuredPreprocessor,
    const SyntaxNode* macroDefinition,
    std::uint8_t ancestryFlags,
    const SyntaxNode* declarationScopeItem,
    bool inTemplateDeclarationBlock,
    bool inTemplateDeclarationHeader,
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
        .inCompactSingleStatementBody = inCompactSingleStatementBody,
        .structuredPreprocessor = structuredPreprocessor,
        .inMacroValue = inMacroValue,
        .node = &node,
        .macroDefinition = macroDefinition,
    });
    ApplyPrintTokenAncestryTraits(
        tokens.back(), ancestryFlags, declarationScopeItem, inTemplateDeclarationBlock, inTemplateDeclarationHeader
    );
}

void AppendTokens(
    const SyntaxNode& node,
    SyntaxNodeKind parentKind,
    SyntaxNodeKind grandParentKind,
    bool inTemplateDeclaration,
    bool inRequiresClause,
    bool inCompilerCallModifier,
    bool inCompactSingleStatementBody,
    const SyntaxNode* macroDefinition,
    bool inMacroValue,
    std::uint8_t ancestryFlags,
    const SyntaxNode* declarationScopeItem,
    bool inTemplateDeclarationBlock,
    bool inTemplateDeclarationHeader,
    std::vector<PrintToken>& tokens
) {
    const SyntaxNodeKind nodeKind = node.kind;
    const bool childInTemplateDeclaration = inTemplateDeclaration || nodeKind == SyntaxNodeKind::TemplateDeclaration;
    const bool childInRequiresClause = inRequiresClause || nodeKind == SyntaxNodeKind::RequiresClause;
    const bool childInCompilerCallModifier = inCompilerCallModifier ||
        nodeKind == SyntaxNodeKind::MsCallModifier ||
        nodeKind == SyntaxNodeKind::MsDeclspecModifier;
    const bool childInCompactSingleStatementBody =
        inCompactSingleStatementBody || CallableBodyAllowsCompactSingleStatementForm(node, parentKind);
    const SyntaxNode* childMacroDefinition = macroDefinition != nullptr ? macroDefinition :
        (SyntaxNodeKindHasClass(nodeKind, SyntaxNodeClass::MacroDefinition) ? &node : nullptr);
    const bool childInMacroValue = inMacroValue || nodeKind == SyntaxNodeKind::MacroReplacementList;
    std::uint8_t childAncestryFlags = ancestryFlags;
    childAncestryFlags |= nodeKind == SyntaxNodeKind::MacroStatementSequence ? InMacroStatementSequence : 0;
    childAncestryFlags |=
        (node.classes & static_cast<std::uint64_t>(SyntaxNodeClass::LeadingStreamOperatorChain)) != 0 ?
            InLeadingStreamOperatorChain : 0;
    childAncestryFlags |=
        (node.classes & static_cast<std::uint64_t>(SyntaxNodeClass::ConditionalStreamOperatorChain)) != 0 ?
            InConditionalStreamOperatorChain : 0;
    childAncestryFlags |= (node.classes & static_cast<std::uint64_t>(SyntaxNodeClass::ConditionalFunctionHeader)) != 0 ?
        InConditionalFunctionHeader : 0;
    childAncestryFlags |= nodeKind == SyntaxNodeKind::BareMacroItem ? InBareMacroItem : 0;
    childAncestryFlags |=
        (nodeKind == SyntaxNodeKind::TemplateArgumentList || nodeKind == SyntaxNodeKind::TemplateParameterList) ?
            InTemplateList : 0;
    const SyntaxNode* childDeclarationScopeItem = node.parent != nullptr &&
        (node.parent->classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationScope)) != 0 ? &node :
        declarationScopeItem;
    bool childInTemplateDeclarationBlock = inTemplateDeclarationBlock;
    bool childInTemplateDeclarationHeader = inTemplateDeclarationHeader;
    if (nodeKind == SyntaxNodeKind::TemplateDeclaration) {
        childInTemplateDeclarationBlock = false;
        childInTemplateDeclarationHeader = false;
    } else {
        childInTemplateDeclarationBlock =
            childInTemplateDeclarationBlock || SyntaxNodeKindHasClass(nodeKind, SyntaxNodeClass::CompoundBlock);
        childInTemplateDeclarationHeader = childInTemplateDeclarationHeader ||
            nodeKind == SyntaxNodeKind::KeywordTemplate ||
            nodeKind == SyntaxNodeKind::TemplateParameterList ||
            nodeKind == SyntaxNodeKind::RequiresClause;
    }
    const auto applyAncestryTraits = [&]() {
        ApplyPrintTokenAncestryTraits(
            tokens.back(),
            childAncestryFlags,
            childDeclarationScopeItem,
            childInTemplateDeclarationBlock,
            childInTemplateDeclarationHeader
        );
    };

    if (nodeKind == SyntaxNodeKind::BlankLine) {
        tokens.push_back({
            .kind = PrintTokenKind::BlankLine,
            .inMacroValue = childInMacroValue,
            .node = &node,
            .macroDefinition = childMacroDefinition,
        });
        applyAncestryTraits();
        return;
    }
    if (nodeKind == SyntaxNodeKind::Comment || nodeKind == SyntaxNodeKind::TrailingComment) {
        tokens.push_back({
            .kind =
                nodeKind == SyntaxNodeKind::TrailingComment ? PrintTokenKind::TrailingComment : PrintTokenKind::Comment,
            .syntaxKind = nodeKind,
            .text = node.text,
            .parentKind = parentKind,
            .grandParentKind = grandParentKind,
            .inTemplateDeclaration = childInTemplateDeclaration,
            .inRequiresClause = childInRequiresClause,
            .inCompilerCallModifier = childInCompilerCallModifier,
            .inCompactSingleStatementBody = childInCompactSingleStatementBody,
            .inMacroValue = childInMacroValue,
            .node = &node,
            .macroDefinition = childMacroDefinition,
        });
        applyAncestryTraits();
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
            .inCompactSingleStatementBody = childInCompactSingleStatementBody,
            .inMacroValue = childInMacroValue,
            .node = &node,
            .macroDefinition = childMacroDefinition,
        });
        applyAncestryTraits();
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
            childInCompactSingleStatementBody,
            childInMacroValue,
            true,
            childMacroDefinition,
            childAncestryFlags,
            childDeclarationScopeItem,
            childInTemplateDeclarationBlock,
            childInTemplateDeclarationHeader,
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
                    childInCompactSingleStatementBody,
                    childInMacroValue,
                    true,
                    childMacroDefinition,
                    childAncestryFlags,
                    childDeclarationScopeItem,
                    childInTemplateDeclarationBlock,
                    childInTemplateDeclarationHeader,
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
                childInCompactSingleStatementBody,
                childMacroDefinition,
                childInMacroValue,
                childAncestryFlags,
                childDeclarationScopeItem,
                childInTemplateDeclarationBlock,
                childInTemplateDeclarationHeader,
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
            .inCompactSingleStatementBody = childInCompactSingleStatementBody,
            .inMacroValue = childInMacroValue,
            .node = &node,
            .macroDefinition = childMacroDefinition,
        });
        applyAncestryTraits();
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
            .inCompactSingleStatementBody = childInCompactSingleStatementBody,
            .inMacroValue = childInMacroValue,
            .node = &node,
            .macroDefinition = childMacroDefinition,
        });
        applyAncestryTraits();
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
            .inCompactSingleStatementBody = childInCompactSingleStatementBody,
            .inMacroValue = childInMacroValue,
            .node = &node,
            .macroDefinition = childMacroDefinition,
        });
        applyAncestryTraits();
        return;
    }
    if (nodeKind == SyntaxNodeKind::LexicalToken || node.children.empty()) {
        tokens.push_back({
            .kind = PrintTokenKind::Text,
            .syntaxKind = nodeKind,
            .text = node.text,
            .parentKind = parentKind,
            .grandParentKind = grandParentKind,
            .inTemplateDeclaration = childInTemplateDeclaration,
            .inRequiresClause = childInRequiresClause,
            .inCompilerCallModifier = childInCompilerCallModifier,
            .inCompactSingleStatementBody = childInCompactSingleStatementBody,
            .inMacroValue = childInMacroValue,
            .node = &node,
            .macroDefinition = childMacroDefinition,
        });
        applyAncestryTraits();
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
                childInCompactSingleStatementBody,
                childMacroDefinition,
                true,
                childAncestryFlags,
                childDeclarationScopeItem,
                childInTemplateDeclarationBlock,
                childInTemplateDeclarationHeader,
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
                childInCompactSingleStatementBody,
                childMacroDefinition,
                childInMacroValue,
                childAncestryFlags,
                childDeclarationScopeItem,
                childInTemplateDeclarationBlock,
                childInTemplateDeclarationHeader,
                tokens
            );
        }
    }
}

std::optional<size_t> SourceTextOffset(std::string_view source, std::string_view tokenText) {
    if (source.empty() || tokenText.empty()) {
        return std::nullopt;
    }
    const std::uintptr_t sourceBegin = reinterpret_cast<std::uintptr_t>(source.data());
    const std::uintptr_t sourceEnd = sourceBegin + source.size();
    const std::uintptr_t tokenBegin = reinterpret_cast<std::uintptr_t>(tokenText.data());
    const std::uintptr_t tokenEnd = tokenBegin + tokenText.size();
    if (tokenBegin < sourceBegin || tokenEnd > sourceEnd) {
        return std::nullopt;
    }
    return static_cast<size_t>(tokenBegin - sourceBegin);
}

size_t SourceLineStart(std::string_view source, size_t offset) {
    if (offset == 0) {
        return 0;
    }
    const size_t newline = source.rfind('\n', offset - 1);
    return newline == std::string_view::npos ? 0 : newline + 1;
}

int SourceDisplayColumn(std::string_view source, size_t offset, int tabWidth) {
    const size_t lineStart = SourceLineStart(source, offset);
    const std::string_view prefix = source.substr(lineStart, offset - lineStart);
    int column = 0;
    size_t begin = 0;
    while (begin < prefix.size()) {
        const size_t tab = prefix.find('\t', begin);
        if (tab == std::string_view::npos) {
            column += Utf8CharacterCount(prefix.substr(begin));
            break;
        }
        column += Utf8CharacterCount(prefix.substr(begin, tab - begin));
        column += tabWidth - column % tabWidth;
        begin = tab + 1;
    }
    return column;
}

bool IsNextSourceLine(std::string_view source, size_t previousEnd, size_t currentStart) {
    if (currentStart < previousEnd) {
        return false;
    }
    std::string_view between = source.substr(previousEnd, currentStart - previousEnd);
    if (between.empty() || !IsSourceLineBreak(between.front())) {
        return false;
    }
    const char lineBreak = between.front();
    between.remove_prefix(1);
    if (lineBreak == '\r' && !between.empty() && between.front() == '\n') {
        between.remove_prefix(1);
    }
    return std::all_of(between.begin(), between.end(), [](char value) { return value == ' ' || value == '\t'; });
}

void MarkCommentContinuations(std::vector<PrintToken>& tokens, std::string_view source, int tabWidth) {
    for (size_t index = 1; index < tokens.size(); ++index) {
        PrintToken& comment = tokens[index];
        const PrintToken& previous = tokens[index - 1];
        if (
            comment.kind != PrintTokenKind::Comment ||
            !IsLineCommentToken(comment) ||
            !IsLineCommentToken(previous) ||
            (previous.kind != PrintTokenKind::TrailingComment && !previous.commentContinuation)
        ) {
            continue;
        }
        const std::optional<size_t> previousOffset = SourceTextOffset(source, previous.text);
        const std::optional<size_t> commentOffset = SourceTextOffset(source, comment.text);
        if (!previousOffset || !commentOffset) {
            continue;
        }
        const size_t previousEnd = *previousOffset + previous.text.size();
        comment.commentContinuation = IsNextSourceLine(source, previousEnd, *commentOffset) &&
            SourceDisplayColumn(source, *previousOffset, tabWidth) ==
                SourceDisplayColumn(source, *commentOffset, tabWidth);
    }
}

}  // namespace

std::vector<PrintToken> BuildPrintTokens(const FormatModel& model, int tabWidth) {
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
        0,
        nullptr,
        false,
        false,
        tokens
    );
    if (model.sourceText != nullptr) {
        MarkCommentContinuations(tokens, *model.sourceText, std::max(1, tabWidth));
    }
    const PrintToken* previous = nullptr;
    for (size_t index = 0; index < tokens.size(); ++index) {
        PrintToken& token = tokens[index];
        // The syntax tree is immutable during printing. Cache traits reused by compact checks and break-model
        // construction; they are exact projections of the original token and its ancestry.
        InitializePrintTokenTraits(token);
        token.forcedLeadingPreprocessorListComma = token.kind == PrintTokenKind::Known &&
            token.syntaxKind == SyntaxNodeKind::Comma &&
            token.node != nullptr &&
            IsFirstConditionalBranchChild(*token.node);
        token.sourceIndex = static_cast<std::uint32_t>(index);
        token.spaceBefore = FormatTokenNeedsSpace(previous, token);
        token.spaceBeforeKnown = true;
        previous = &token;
    }
    return tokens;
}

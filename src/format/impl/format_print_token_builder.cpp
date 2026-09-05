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

bool IsPreprocEndifToken(const SyntaxNode& node) {
    return SyntaxNodeKindHasClass(node.kind, SyntaxNodeClass::EndifDirective);
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

bool IsCommentAlreadyInPreprocessorHeader(const SyntaxNode& parent, const SyntaxNode& child) {
    return SyntaxNodeHasClass(child, SyntaxNodeClass::Trivia) &&
        SourceTextOffset(ContinuedPreprocessorHeader(parent.text), child.text).has_value();
}

bool IsFirstConditionalBranchChild(const SyntaxNode& node) {
    const SyntaxNode* parent = node.parent;
    if (parent == nullptr || !SyntaxNodeKindHasClass(parent->kind, SyntaxNodeClass::ConditionalPreprocessorTree)) {
        return false;
    }

    for (size_t index = 0; index < parent->children.size(); ++index) {
        const SyntaxNode* child = parent->children[index];
        if (child == nullptr) {
            continue;
        }
        if (IsCommentAlreadyInPreprocessorHeader(*parent, *child)) {
            continue;
        }
        if (IsConditionalPreprocessorHeaderChild(*parent, index)) {
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

// Inherited syntax context is copied per recursion branch. Parent kinds describe
// the emitted node; semantic flags include that node before token construction.
struct TokenContext {
    SyntaxNodeKind parentKind = SyntaxNodeKind::Unknown;
    SyntaxNodeKind grandParentKind = SyntaxNodeKind::Unknown;
    bool inTemplateDeclaration = false;
    bool inRequiresClause = false;
    bool inCompilerCallModifier = false;
    bool inCompactSingleStatementBody = false;
    const SyntaxNode* macroDefinition = nullptr;
    bool inMacroValue = false;
    std::uint8_t ancestryFlags = 0;
    const SyntaxNode* declarationScopeItem = nullptr;
    bool inTemplateDeclarationBlock = false;
    bool inTemplateDeclarationHeader = false;

    void Enter(const SyntaxNode& node) {
        const SyntaxNodeKind kind = node.kind;
        inTemplateDeclaration |= kind == SyntaxNodeKind::TemplateDeclaration;
        inRequiresClause |= kind == SyntaxNodeKind::RequiresClause;
        inCompilerCallModifier |= kind == SyntaxNodeKind::MsCallModifier || kind == SyntaxNodeKind::MsDeclspecModifier;
        inCompactSingleStatementBody =
            inCompactSingleStatementBody || CallableBodyAllowsCompactSingleStatementForm(node, parentKind);
        if (macroDefinition == nullptr && SyntaxNodeKindHasClass(kind, SyntaxNodeClass::MacroDefinition)) {
            macroDefinition = &node;
        }
        inMacroValue |= kind == SyntaxNodeKind::MacroReplacementList;
        ancestryFlags |= kind == SyntaxNodeKind::MacroStatementSequence ? InMacroStatementSequence : 0;
        ancestryFlags |= (node.classes & static_cast<std::uint64_t>(SyntaxNodeClass::LeadingStreamOperatorChain)) != 0 ?
            InLeadingStreamOperatorChain : 0;
        ancestryFlags |=
            (node.classes & static_cast<std::uint64_t>(SyntaxNodeClass::ConditionalStreamOperatorChain)) != 0 ?
                InConditionalStreamOperatorChain : 0;
        ancestryFlags |= (node.classes & static_cast<std::uint64_t>(SyntaxNodeClass::ConditionalFunctionHeader)) != 0 ?
            InConditionalFunctionHeader : 0;
        ancestryFlags |= kind == SyntaxNodeKind::BareMacroItem ? InBareMacroItem : 0;
        ancestryFlags |=
            (kind == SyntaxNodeKind::TemplateArgumentList || kind == SyntaxNodeKind::TemplateParameterList) ?
                InTemplateList : 0;
        if (
            node.parent != nullptr &&
            (node.parent->classes & static_cast<std::uint64_t>(SyntaxNodeClass::DeclarationScope)) != 0
        ) {
            declarationScopeItem = &node;
        }
        if (kind == SyntaxNodeKind::TemplateDeclaration) {
            inTemplateDeclarationBlock = false;
            inTemplateDeclarationHeader = false;
        } else {
            inTemplateDeclarationBlock |= SyntaxNodeKindHasClass(kind, SyntaxNodeClass::CompoundBlock);
            inTemplateDeclarationHeader |= kind == SyntaxNodeKind::KeywordTemplate ||
                kind == SyntaxNodeKind::TemplateParameterList ||
                kind == SyntaxNodeKind::RequiresClause;
        }
    }

    TokenContext ForChildren(SyntaxNodeKind kind) const {
        TokenContext child = *this;
        child.grandParentKind = parentKind;
        child.parentKind = kind;
        return child;
    }
};

PrintToken
    MakePrintToken(const SyntaxNode& node, PrintTokenKind kind, const TokenContext& context, std::string_view text = {})
{
    PrintToken token{
        .kind = kind, .inMacroValue = context.inMacroValue, .node = &node, .macroDefinition = context.macroDefinition
    };
    token.inMacroStatementSequence = (context.ancestryFlags & InMacroStatementSequence) != 0;
    token.inLeadingStreamOperatorChain = (context.ancestryFlags & InLeadingStreamOperatorChain) != 0;
    token.inConditionalStreamOperatorChain = (context.ancestryFlags & InConditionalStreamOperatorChain) != 0;
    token.inConditionalFunctionHeader = (context.ancestryFlags & InConditionalFunctionHeader) != 0;
    token.inBareMacroItem = (context.ancestryFlags & InBareMacroItem) != 0;
    token.inTemplateList = (context.ancestryFlags & InTemplateList) != 0;
    token.inTemplateDeclarationBlock = context.inTemplateDeclarationBlock;
    token.inTemplateDeclarationHeader = context.inTemplateDeclarationHeader;
    token.declarationScopeItem = context.declarationScopeItem;
    // Blank lines inherit scope/macro facts, but carry no lexical syntax context.
    if (kind != PrintTokenKind::BlankLine) {
        token.syntaxKind = node.kind;
        token.text = text;
        token.parentKind = context.parentKind;
        token.grandParentKind = context.grandParentKind;
        token.inTemplateDeclaration = context.inTemplateDeclaration;
        token.inRequiresClause = context.inRequiresClause;
        token.inCompilerCallModifier = context.inCompilerCallModifier;
        token.inCompactSingleStatementBody = context.inCompactSingleStatementBody;
    }
    return token;
}

void AppendTokens(const SyntaxNode& node, TokenContext context, std::vector<PrintToken>& tokens) {
    context.Enter(node);
    const SyntaxNodeKind nodeKind = node.kind;
    if (nodeKind == SyntaxNodeKind::BlankLine) {
        tokens.push_back(MakePrintToken(node, PrintTokenKind::BlankLine, context));
        return;
    }
    if (nodeKind == SyntaxNodeKind::Comment || nodeKind == SyntaxNodeKind::TrailingComment) {
        const PrintTokenKind kind =
            nodeKind == SyntaxNodeKind::TrailingComment ? PrintTokenKind::TrailingComment : PrintTokenKind::Comment;
        tokens.push_back(MakePrintToken(node, kind, context, node.text));
        return;
    }
    if (IsStandalonePreprocessorBranchToken(node, context.parentKind)) {
        tokens.push_back(MakePrintToken(node, PrintTokenKind::Preprocessor, context, node.text));
        return;
    }
    if (IsStructuredConditionalPreprocessorNode(node)) {
        const auto appendDirective = [&](const SyntaxNode& directive, std::string_view text) {
            PrintToken token = MakePrintToken(directive, PrintTokenKind::Preprocessor, context, text);
            token.structuredPreprocessor = true;
            tokens.push_back(token);
        };
        appendDirective(node, ContinuedPreprocessorHeader(node.text));
        for (size_t index = 0; index < node.children.size(); ++index) {
            const SyntaxNode* child = node.children[index];
            if (child == nullptr) {
                continue;
            }
            if (IsPreprocEndifToken(*child)) {
                appendDirective(*child, PreprocEndifLine(*child));
                continue;
            }
            if (IsCommentAlreadyInPreprocessorHeader(node, *child)) {
                continue;
            }
            if (IsConditionalPreprocessorHeaderChild(node, index)) {
                continue;
            }
            AppendTokens(*child, context.ForChildren(nodeKind), tokens);
        }
        return;
    }
    if (SyntaxNodeKindHasClass(nodeKind, SyntaxNodeClass::Known)) {
        tokens.push_back(MakePrintToken(
            node, PrintTokenKind::Known, context, node.text.empty() ? SyntaxNodeKindTokenText(nodeKind) : node.text
        ));
        return;
    }
    if (nodeKind == SyntaxNodeKind::IncludeRun) {
        tokens.push_back(MakePrintToken(node, PrintTokenKind::IncludeRun, context));
        return;
    }
    if (IsPreprocessorNode(node)) {
        tokens.push_back(MakePrintToken(node, PrintTokenKind::Preprocessor, context, node.text));
        return;
    }
    if (nodeKind == SyntaxNodeKind::LexicalToken || node.children.empty()) {
        tokens.push_back(MakePrintToken(node, PrintTokenKind::Text, context, node.text));
        return;
    }
    if (nodeKind == SyntaxNodeKind::MacroReplacementList || SyntaxNodeKindHasClass(nodeKind, SyntaxNodeClass::Tree)) {
        for (const SyntaxNode* child : node.children) {
            AppendTokens(*child, context.ForChildren(nodeKind), tokens);
        }
    }
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
    AppendTokens(*model.root, {}, tokens);
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

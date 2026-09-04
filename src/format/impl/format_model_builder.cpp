#include "format/impl/format_model_builder.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "tools/tools_common.h"
#include "format/impl/format_model_normalize.h"

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
    SyntaxNode* node = MakeSyntaxNode(model);
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
        AppendSyntaxChild(parent, MakeSyntaxNode(model, SyntaxNodeKind::BlankLine));
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
    TSTreeCursor cursor = ts_tree_cursor_new(tsNode);
    ts_tree_cursor_goto_first_child(&cursor);
    for (uint32_t index = 0; index < childCount; ++index) {
        TSNode child = ts_tree_cursor_current_node(&cursor);
        const TSPoint childEndPoint = ts_node_end_point(child);
        const size_t childBegin = parent.children.size();
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
        const char* fieldName = ts_tree_cursor_current_field_name(&cursor);
        if (fieldName != nullptr && std::string_view(fieldName) == "declarator") {
            for (size_t childIndex = childBegin; childIndex < parent.children.size(); ++childIndex) {
                SyntaxNode* childNode = parent.children[childIndex];
                if (childNode != nullptr && !SyntaxNodeHasClass(*childNode, SyntaxNodeClass::Trivia)) {
                    childNode->isDeclarator = true;
                }
            }
        }
        ts_tree_cursor_goto_next_sibling(&cursor);
    }
    ts_tree_cursor_delete(&cursor);
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
    SyntaxNode* includeRun = MakeSyntaxNode(model);
    includeRun->kind = SyntaxNodeKind::IncludeRun;
    includeRun->parent = &root;
    includeRun->depth = root.depth + 1;

    for (; index < sourceChildren.size(); ++index) {
        if (
            sourceChildren[index] != nullptr &&
            SyntaxNodeKindHasClass(sourceChildren[index]->kind, SyntaxNodeClass::IncludeDirective)
        ) {
            AppendSyntaxChild(*includeRun, sourceChildren[index]);
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
                AppendSyntaxChild(*includeRun, sourceChildren[index]);
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
    return "parse failed at " + std::to_string(static_cast<int>(point.row) + 1) +
        ":" + std::to_string(static_cast<int>(point.column) + 1) +
        " near " + nodeType;
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

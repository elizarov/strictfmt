#include "format/impl/format_layout_writer.h"

#include <algorithm>

#include "format/impl/format_break_model_inline_helpers.h"
#include "format/impl/format_layout_program.h"
#include "format/impl/format_layout_tree.h"
#include "format/impl/format_syntax_helpers.h"

namespace {

bool SyntaxSubtreeEndsWith(const SyntaxNode& node, SyntaxNodeKind kind) {
    if (node.kind == kind) {
        return true;
    }
    for (auto child = node.children.rbegin(); child != node.children.rend(); ++child) {
        if (*child == nullptr || SyntaxNodeHasClass(**child, SyntaxNodeClass::Trivia)) {
            continue;
        }
        return SyntaxSubtreeEndsWith(**child, kind);
    }
    return false;
}

bool TrailingCommentReturnsToStructuralIndent(const PrintToken& token) {
    if (token.node == nullptr || token.node->parent == nullptr) {
        return false;
    }
    const SyntaxNode* scope = token.node->parent;
    while (
        scope != nullptr &&
        (scope->kind == SyntaxNodeKind::MacroCallItem || scope->kind == SyntaxNodeKind::BareMacroItem)
    ) {
        scope = scope->parent;
    }
    if (scope != nullptr && SyntaxNodeHasClass(*scope, SyntaxNodeClass::SourceItemScope)) {
        return true;
    }
    const SyntaxNode* previous = nullptr;
    for (const SyntaxNode* child : token.node->parent->children) {
        if (child == token.node) {
            break;
        }
        if (child != nullptr && !SyntaxNodeHasClass(*child, SyntaxNodeClass::Trivia)) {
            previous = child;
        }
    }
    return previous != nullptr && (
        previous->kind == SyntaxNodeKind::LeftBrace || (
            previous->kind == SyntaxNodeKind::TemplateParameterList &&
            token.node->parent->kind == SyntaxNodeKind::TemplateDeclaration
        ) ||
        SyntaxSubtreeEndsWith(*previous, SyntaxNodeKind::RightBrace) ||
        (previous->kind == SyntaxNodeKind::Colon && token.node->parent->kind == SyntaxNodeKind::CaseStatement)
    );
}

const SyntaxNode* LineCommentAlignmentGroup(const PrintToken& token) {
    const SyntaxNode* group = token.node == nullptr ? nullptr : token.node->parent;
    while (
        group != nullptr &&
        SyntaxNodeHasClass(*group, SyntaxNodeClass::Expression) &&
        !HasDirectListDelimiterPair(*group)
    ) {
        group = group->parent;
    }
    return group;
}

}  // namespace

FormatLayoutWriter::FormatLayoutWriter(
    FormatLayoutProgramBuilder& program, const FormatLayoutTree& tree, FormatLayoutWriteContext context
) : program_(program), tree_(tree), context_(context) {}

void FormatLayoutWriter::EnterNode(const FormatBreakNode& node) {
    program_.PushOwner(tree_.FindOwner(node.syntaxOwner));
}
void FormatLayoutWriter::LeaveNode() { program_.PopOwner(); }
const FormatOutputState& FormatLayoutWriter::State() const { return program_.State(); }
void FormatLayoutWriter::Write(std::string_view text) { program_.Write(text, context_.structuralIndent); }
void FormatLayoutWriter::Space() { program_.Space(); }
void FormatLayoutWriter::SetPendingIndent(int indentLevel) { program_.SetPendingIndent(indentLevel); }

void FormatLayoutWriter::BreakLine(int indentLevel, bool blankLine) {
    if (blankLine) {
        program_.BlankLine(context_.macroContinuation);
    } else {
        program_.NewLine(context_.macroContinuation);
    }
    program_.SetPendingIndent(std::max(0, indentLevel));
}

void FormatLayoutWriter::WriteComment(
    const PrintToken& token, std::string_view text, FormatOutputComment placement, bool spaceBefore
) {
    program_.WriteComment(
        text,
        context_.structuralIndent,
        LineCommentAlignmentGroup(token),
        placement,
        IsLineCommentToken(token),
        spaceBefore
    );
}

void FormatLayoutWriter::WriteToken(
    const FormatBreakToken& token, std::string_view text, std::optional<int> continuationBaseIndent, bool suppressSpace
) {
    if (token.contextOnly) {
        return;
    }
    const PrintToken& printToken = FormatBreakTokenValue(token);
    auto tokenScope = program_.TokenScope(printToken, tree_.FindOwner(printToken.node));
    if (
        printToken.macroDefinition != nullptr &&
        !printToken.inMacroValue &&
        State().atLineStart &&
        !State().macroContinuation
    ) {
        program_.ForceColumnZero();
    }
    if (IsCommentToken(printToken.kind)) {
        const int commentIndent = State().atLineStart ?
            program_.CurrentColumn(context_.structuralIndent) / std::max(1, context_.indentWidth) :
            program_.CurrentLineIndentLevel();
        if (printToken.kind == PrintTokenKind::Comment) {
            if (!State().atLineStart) {
                BreakLine(commentIndent, false);
            }
            if (printToken.commentContinuation) {
                WriteComment(printToken, text, FormatOutputComment::Continuation);
            } else {
                Write(text);
            }
            BreakLine(commentIndent, false);
            return;
        }
        const int breakModelContinuationIndent = continuationBaseIndent ?
            *continuationBaseIndent + (*continuationBaseIndent == context_.structuralIndent ? 1 : 0) : 0;
        const int continuationIndent = std::max(commentIndent, breakModelContinuationIndent);
        if (!State().atLineStart) {
            WriteComment(printToken, text, FormatOutputComment::Trailing, token.spaceBefore);
        } else {
            WriteComment(printToken, text, FormatOutputComment::Standalone);
        }
        const PrintToken* nextToken = printToken.sourceIndex + 1 < context_.sourceTokens.size() ?
            &context_.sourceTokens[printToken.sourceIndex + 1] : nullptr;
        const bool continuesMacro = PrintTokenContinuesMacroLine(printToken, nextToken);
        if (
            TrailingCommentReturnsToStructuralIndent(printToken) ||
            (printToken.macroDefinition != nullptr && !continuesMacro)
        ) {
            program_.NewLine(continuesMacro);
        } else {
            BreakLine(continuationIndent, false);
        }
        return;
    }
    if (token.spaceBefore && !suppressSpace && !State().atLineStart) {
        Space();
    }
    Write(text);
}

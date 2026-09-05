#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include "format/impl/format_model.h"
#include "format/impl/format_output.h"
#include "format/impl/format_break_emitter.h"
#include "format/impl/format_break_model.h"
#include "format/impl/format_list_continuation.h"

namespace {

void Check(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

void TestOutput() {
    {
        FormatOutput output(2, 80);
        output.SetPendingIndent(3);
        output.Write("é", 1);
        Check(output.CurrentColumn(1) == 7, "columns count Unicode characters after pending indent");
        Check(!output.State().pendingIndentLevel, "writing consumes pending indentation");
        output.Space();
        output.NewLine(true);
        Check(output.State().macroContinuation && output.CurrentColumn(1) == 4, "macro continuation adds one indent");
        output.Write("z", 1);
        Check(output.Finish() == "      é \\\n    z\n", "macro suffix follows trimmed content");
    }
    {
        FormatOutput output(2, 80);
        output.SetPendingIndent(3);
        output.ForceColumnZero();
        output.Write("#define X", 2);
        output.NewLine(true);
        output.SetPendingIndent(3);
        output.WriteAtIndent("x", 1);
        Check(output.Finish() == "#define X \\\n  x\n", "explicit indentation overrides pending and macro indentation");
    }
    {
        SyntaxNode group;
        FormatOutput output(2, 20);
        output.Write("a;", 0);
        output.WriteComment("// x", 0, &group, FormatOutputComment::Trailing, true);
        output.NewLine();
        output.Write("long;", 0);
        output.WriteComment("// y", 0, &group, FormatOutputComment::Trailing, true);
        output.NewLine();
        output.SetPendingIndent(4);
        output.WriteComment("// more", 0, &group, FormatOutputComment::Continuation, true);
        Check(output.Finish() == "a;     // x\nlong;  // y\n       // more\n", "comment continuation follows its aligned anchor");
    }
    {
        SyntaxNode group;
        FormatOutput output(2, 8);
        output.Write("a;", 0);
        output.WriteComment("// x", 0, &group, FormatOutputComment::Trailing, true);
        output.NewLine();
        output.Write("long;", 0);
        output.WriteComment("// y", 0, &group, FormatOutputComment::Trailing, true);
        Check(output.Finish() == "a;  // x\nlong;  // y\n", "alignment is skipped when the run does not fit");
    }
    {
        FormatOutput output(2, 80);
        output.BlankLine();
        output.Write("x", 0);
        output.BlankLine();
        output.ReopenLastLine(true);
        output.Space();
        output.Write("y", 0);
        Check(output.Finish() == "x y\n", "reopening discards requested blank lines and restores columns");
    }
    {
        FormatOutput output(2, 80);
        output.SetPendingIndent(2);
        output.AppendCompleteLines("#include <x>\n");
        output.Write("x", 0);
        output.NewLine();
        output.WriteVerbatim("#if X\n  y");
        Check(output.CurrentColumn(0) == 3, "verbatim multiline text tracks its final physical column");
        Check(output.Finish() == "#include <x>\n    x\n#if X\n  y\n", "complete lines preserve pending indentation for following text");
    }
}


void TestListContinuation() {
    SyntaxNode list;
    list.kind = SyntaxNodeKind::ArgumentList;
    SyntaxNode lambda;
    lambda.kind = SyntaxNodeKind::LambdaExpression;
    lambda.parent = &list;
    SyntaxNode body;
    body.kind = SyntaxNodeKind::CompoundStatement;
    body.parent = &lambda;
    SyntaxNode open;
    open.kind = SyntaxNodeKind::LeftParen;
    open.parent = &list;
    SyntaxNode blockOpen;
    blockOpen.kind = SyntaxNodeKind::LeftBrace;
    blockOpen.parent = &body;
    SyntaxNode blockClose;
    blockClose.kind = SyntaxNodeKind::RightBrace;
    blockClose.parent = &body;
    SyntaxNode comma;
    comma.kind = SyntaxNodeKind::Comma;
    comma.parent = &list;
    SyntaxNode close;
    close.kind = SyntaxNodeKind::RightParen;
    close.parent = &list;
    list.children = {&open, &lambda, &comma, &close};
    lambda.children = {&body};
    body.children = {&blockOpen, &blockClose};
    const auto token = [](const SyntaxNode& node) {
        return PrintToken{.kind = PrintTokenKind::Known, .syntaxKind = node.kind, .node = &node};
    };
    const std::array tokens{token(open), token(blockOpen), token(blockClose), token(comma), token(close)};
    FormatListContinuation continuation(tokens);
    const auto* plan = continuation.PlanBlock(1);
    Check(plan != nullptr && plan->virtualDelimiters.size() == 1, "block plan retains its enclosing list delimiter");
    Check(plan->virtualDelimiters.front().forceSplit, "following list item requires the virtual list to split");
    const std::array selected{FormatBreakSplitList{&open, 3, 1}};
    Check(continuation.AcceptBlock(selected) == 3, "selected list indentation is retained across the block");
    Check(!continuation.TakeBoundary(tokens[3], FormatListContinuationKind::Block), "list continuation waits for its block to close");
    continuation.CloseBlock(tokens[2], &tokens[3]);

    SyntaxNode nested;
    nested.kind = SyntaxNodeKind::ArgumentList;
    nested.parent = &list;
    SyntaxNode nestedOpen;
    nestedOpen.kind = SyntaxNodeKind::LeftParen;
    nestedOpen.parent = &nested;
    SyntaxNode nestedComma;
    nestedComma.kind = SyntaxNodeKind::Comma;
    nestedComma.parent = &nested;
    nested.children = {&nestedOpen, &nestedComma};
    Check(!continuation.TakeBoundary(token(nestedComma), FormatListContinuationKind::Block), "nested list separator cannot consume its enclosing continuation");
    const auto separator = continuation.TakeBoundary(tokens[3], FormatListContinuationKind::Block);
    Check(separator && !separator->beforeToken && separator->indent == 3, "separator breaks after itself at the selected item indent");
    const auto closer = continuation.TakeBoundary(tokens[4], FormatListContinuationKind::Block);
    Check(closer && closer->beforeToken && closer->indent == 1, "closer breaks before itself at the selected close indent");
    Check(!continuation.TakeBoundary(tokens[4], FormatListContinuationKind::Block), "closer consumes its continuation once");
}

}  // namespace

int main() {
    try {
        TestOutput();
        TestListContinuation();
    } catch (const std::exception& error) {
        std::cerr << "Layout contract test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Passed layout contract tests.\n";
}

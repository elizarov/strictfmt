#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include "format/impl/format_model.h"
#include "format/impl/format_output.h"
#include "format/impl/format_choice_history.h"
#include "format/impl/format_candidates.h"
#include "format/impl/format_delimiter_stack.h"
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
        output.Write("\xc3\xa9", 1);
        Check(output.CurrentColumn(1) == 7, "columns count Unicode characters after pending indent");
        Check(!output.State().pendingIndentLevel, "writing consumes pending indentation");
        output.Space();
        output.NewLine(true);
        Check(output.State().macroContinuation && output.CurrentColumn(1) == 4, "macro continuation adds one indent");
        output.Write("z", 1);
        Check(output.Finish() == "      \xc3\xa9 \\\n    z\n", "macro suffix follows trimmed content");
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


void TestChoiceHistory() {
    FormatChoiceHistory history;
    const auto first = history.AddChoice(nullptr, 1, FormatBreakChoice::Split, 2);
    const auto latest = history.AddChoice(first, 1, FormatBreakChoice::Compact, 9);
    Check(history.Concat(nullptr, latest) == latest && history.Concat(latest, nullptr) == latest,
        "empty history is a concatenation identity");
    Check(FormatChoiceHistory::Find(latest, 1) == FormatBreakChoice::Compact, "lookup gives the latest matching record");
    auto records = history.AddContinuationLines(latest, 1, 4);
    records = history.AddContinuationLines(records, 1, 7);
    records = history.AddAttachedOperator(records, 9);
    records = history.AddAttachedOperator(records, 4);
    records = history.AddAttachedOperator(records, 9);
    const auto solution = FormatChoiceHistory::Materialize(records, 4);
    Check(solution.choices[1] == FormatBreakChoice::Split && solution.indentLevels[1] == 2,
        "materialization retains the first choice and render base");
    Check(solution.declarationValueContinuationLines[1] == 7, "materialization retains the last continuation count");
    Check(solution.attachedChainOperators == std::vector<std::uint32_t>({4, 9}), "attached operator indexes are sorted and unique");
    Check(solution.choices[3] == FormatBreakChoice::Compact && solution.indentLevels[3] == -1,
        "unassigned nodes retain materialization defaults");
    auto branch = history.AddContinuationLines(first, 2, 3);
    branch = history.AddChoice(branch, 2, FormatBreakChoice::Split, 5);
    const auto branchSolution = FormatChoiceHistory::Materialize(branch, 4);
    Check(branchSolution.choices[2] == FormatBreakChoice::Compact && branchSolution.indentLevels[2] == -1,
        "metadata records preserve their existing position in choice precedence");
    for (int index = 0; index < 1024; ++index) {
        records = history.AddChoice(records, 99, FormatBreakChoice::Split, index);
    }
    Check(FormatChoiceHistory::Find(first, 1) == FormatBreakChoice::Split && !FormatChoiceHistory::Find(first, 2),
        "arena growth and branch appends leave earlier handles unchanged");
    Check(FormatChoiceHistory::Materialize(records, 4).choices.size() == 4,
        "records outside the model index range do not grow the solution");
}


void TestCandidates() {
    FormatCandidateOrder order(10);
    FormatLayoutCandidate unfinished{.valid = true, .endColumn = 12, .endLineHasText = true};
    Check(order.CurrentLineOverflow(unfinished) == 2, "unfinished physical line contributes virtual overflow");
    order.FinishCurrentLine(unfinished, 2);
    const auto finishedProfile = unfinished.overflowSizeProfile;
    order.FinishCurrentLine(unfinished, 2);
    Check(order.CurrentLineOverflow(unfinished) == 0 && order.MaximumOverflow(unfinished) == 4 &&
        CompareFormatValueProfiles(finishedProfile, unfinished.overflowSizeProfile) == 0,
        "completed line overflow includes the suffix exactly once");

    FormatLayoutCandidate shortLine{.valid = true, .endColumn = 5, .endLineHasText = true, .extraLines = 1};
    auto longLine = shortLine;
    longLine.endColumn = 6;
    longLine.extraLines = 2;
    Check(order.Dominates(shortLine, longLine), "no-worse costs and shorter continuation permit dominance");
    for (bool FormatLayoutCandidate::* flag : {&FormatLayoutCandidate::currentLineOverflowRecorded,
             &FormatLayoutCandidate::ownExpansionCharged, &FormatLayoutCandidate::compactNextStreamOperand}) {
        auto distinct = longLine;
        distinct.*flag = true;
        Check(!FormatCandidateOrder::SameState(longLine, distinct) && !order.Dominates(shortLine, distinct),
            "continuation-sensitive flags prevent state merging and dominance");
    }
    auto expensive = shortLine;
    expensive.expansionDepthProfile.AddValue(100);
    auto overflowing = shortLine;
    overflowing.endColumn = 11;
    Check(order.Better(expensive, overflowing), "overflow has priority over expansion cost");

    FormatChoiceHistory history;
    shortLine.choices = history.AddChoice(nullptr, 1, FormatBreakChoice::Compact);
    auto equal = shortLine;
    equal.choices = history.AddChoice(nullptr, 1, FormatBreakChoice::Split);
    FormatLayoutCandidates frontier;
    order.AddPruned(frontier, longLine);
    order.AddPruned(frontier, shortLine);
    order.AddPruned(frontier, equal);
    Check(frontier.size() == 1 && frontier[0].choices == shortLine.choices,
        "frontier removes dominated candidates and retains the first equal-cost history");
    auto different = shortLine;
    different.compactNextStreamOperand = true;
    order.AddPruned(frontier, different);
    FormatCandidateOrder::Sort(frontier);
    Check(frontier.size() == 2 && !frontier[0].compactNextStreamOperand && frontier[1].compactNextStreamOperand,
        "frontier retains distinct continuation states in deterministic order");

    FormatLayoutCandidates values;
    for (int index = 0; index < 12; ++index) {
        auto value = shortLine;
        value.endColumn = index;
        for (int depth = 1; depth <= 6; ++depth) {
            value.expansionDepthProfile.AddValue(depth);
        }
        values.push_back(std::move(value));
    }
    auto copy = values;
    copy.erase(copy.begin() + 3);
    copy[0].expansionDepthProfile.AddValue(20);
    Check(values.size() == 12 && copy.size() == 11 && copy[3].endColumn == 4 &&
        values[0].expansionDepthProfile.GreatestValue() == 6, "spilled candidate copies own profiles and preserve erase order");
    auto moved = std::move(copy);
    Check(copy.empty() && moved.size() == 11, "spilled candidate move transfers storage");
    moved = frontier;
    Check(moved.size() == 2, "copy assignment transitions heap storage back to inline values");
    moved.erase(moved.begin());
    FormatLayoutCandidates inlineMove = std::move(moved);
    Check(moved.empty() && inlineMove.size() == 1 && inlineMove[0].compactNextStreamOperand,
        "inline move and erase preserve candidate state");
}


void TestDelimiterStack() {
    PrintToken opener{.kind = PrintTokenKind::Known, .syntaxKind = SyntaxNodeKind::LeftParen};
    FormatBreakNode open;
    open.kind = FormatBreakNodeKind::Token;
    open.token = {&opener};
    FormatBreakNode close;
    close.kind = FormatBreakNodeKind::Token;
    FormatBreakNode leaf;
    leaf.kind = FormatBreakNodeKind::Token;
    std::array delimiters{&open, &close};
    FormatBreakNode inner;
    inner.kind = FormatBreakNodeKind::Delimited;
    inner.delimiterKind = FormatBreakDelimiterKind::Paren;
    inner.children = delimiters;
    inner.items = {{.node = &leaf}};
    FormatBreakNode wrapper;
    wrapper.kind = FormatBreakNodeKind::Sequence;
    std::array wrapped{&inner};
    wrapper.children = wrapped;
    FormatBreakNode outer;
    outer.kind = FormatBreakNodeKind::Delimited;
    outer.delimiterKind = FormatBreakDelimiterKind::Paren;
    outer.children = delimiters;
    outer.items = {{.node = &wrapper}};
    const auto view = CollectFormatDelimiterStack(outer, FormatDelimiterStackPolicy::Solving);
    Check(view && view->delimiters.size() == 2 && view->delimiters[0] == &outer &&
        view->delimiters[1] == &inner && view->leaf == &leaf, "stack recognition unwraps only transparent sequences in order");
    inner.blankLineBeforeClose = true;
    Check(CollectFormatDelimiterStack(outer, FormatDelimiterStackPolicy::Solving).has_value() &&
        !CollectFormatDelimiterStack(outer, FormatDelimiterStackPolicy::Emission), "closing blank lines retain distinct solver and emitter contracts");
    inner.blankLineBeforeClose = false;
    inner.forceSplit = true;
    Check(!CollectFormatDelimiterStack(outer, FormatDelimiterStackPolicy::Solving), "required split interrupts transparent stacks");
    inner.forceSplit = false;
    opener.parentKind = SyntaxNodeKind::ArgumentList;
    Check(!CollectFormatDelimiterStack(outer, FormatDelimiterStackPolicy::Solving), "semantic argument lists are not transparent parentheses");
}

}  // namespace

int main() {
    try {
        TestOutput();
        TestListContinuation();
        TestChoiceHistory();
        TestCandidates();
        TestDelimiterStack();
    } catch (const std::exception& error) {
        std::cerr << "Layout contract test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Passed layout contract tests.\n";
}

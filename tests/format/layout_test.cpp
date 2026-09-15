#include <array>
#include <cstdlib>
#include <memory>
#include <tree_sitter/api.h>
#include <tree_sitter_cpp.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <unordered_set>

#include "format/impl/format_model.h"
#include "format/impl/format_output.h"
#include "format/impl/format_choice_history.h"
#include "format/impl/format_candidates.h"
#include "format/impl/format_delimiter_stack.h"
#include "format/impl/format_layout_program.h"
#include "format/impl/format_layout_writer.h"
#include "format/impl/format_layout_lowerer.h"
#include "format/impl/format_break_solver.h"
#include "format/impl/format_break_model.h"
#include "format/impl/format_list_continuation.h"
#include "format/impl/format_chain_continuation.h"
#include "format/impl/format_layout_tree.h"
#include "format/impl/format_syntax_map.h"
#include "format/impl/format_config.h"
#include "format/impl/format_model_parse.h"
#include "format/impl/format_print_token_builder.h"

namespace {

void Check(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

size_t CountSyntaxNodes(TSNode node, std::string_view type) {
    size_t count = std::string_view(ts_node_type(node)) == type ? 1 : 0;
    for (uint32_t index = 0; index < ts_node_child_count(node); ++index) {
        count += CountSyntaxNodes(ts_node_child(node, index), type);
    }
    return count;
}

void TestIncrementalMacroParsing() {
    using Parser = std::unique_ptr<TSParser, decltype(&ts_parser_delete)>;
    using Tree = std::unique_ptr<TSTree, decltype(&ts_tree_delete)>;
    Parser parser(ts_parser_new(), ts_parser_delete);
    Parser freshParser(ts_parser_new(), ts_parser_delete);
    Check(ts_parser_set_language(parser.get(), tree_sitter_cpp()), "incremental parser language");
    Check(ts_parser_set_language(freshParser.get(), tree_sitter_cpp()), "fresh parser language");
    auto parse = [](TSParser* parser, const TSTree* oldTree, std::string_view text) {
        return Tree(ts_parser_parse_string(parser, oldTree, text.data(), static_cast<uint32_t>(text.size())), ts_tree_delete);
    };
    auto pointAt = [](std::string_view text, size_t offset) {
        TSPoint point{};
        for (size_t index = 0; index < offset; ++index) {
            if (text[index] == '\n') { ++point.row; point.column = 0; }
            else { ++point.column; }
        }
        return point;
    };
    const std::string prefix = "int before;\n#define VALUE ";
    const std::string suffix = "\nint values[]={1,\n#define MEMBER )\n2};\n#define NEXT(x) ((x)+1)\n";
    const std::array replacements{
        std::pair{"namespace outer {", true},
        std::pair{"namespace outer { int value; }", false},
        std::pair{")", true},
        std::pair{"1+2", false},
        std::pair{"", false},
        std::pair{"R\"tag(one\ntwo)tag\"", false},
        std::pair{") R\"tag(one\ntwo)tag\"", true},
    };
    std::string source;
    Tree tree(nullptr, ts_tree_delete);
    for (const auto& [replacement, raw] : replacements) {
        const std::string updated = prefix + replacement + suffix;
        if (tree) {
            const size_t oldEnd = source.size() - suffix.size();
            const size_t newEnd = updated.size() - suffix.size();
            const TSInputEdit edit{
                static_cast<uint32_t>(prefix.size()), static_cast<uint32_t>(oldEnd), static_cast<uint32_t>(newEnd),
                pointAt(source, prefix.size()), pointAt(source, oldEnd), pointAt(updated, newEnd),
            };
            ts_tree_edit(tree.get(), &edit);
        }
        tree = parse(parser.get(), tree.get(), updated);
        Check(tree && !ts_node_has_error(ts_tree_root_node(tree.get())), "incremental macro parse succeeds");
        Check(CountSyntaxNodes(ts_tree_root_node(tree.get()), "raw_macro_replacement") == (raw ? 2u : 1u),
            "incremental edit selects the correct replacement kind and preserves adjacent definitions");
        Tree fresh = parse(freshParser.get(), nullptr, updated);
        Check(fresh && !ts_node_has_error(ts_tree_root_node(fresh.get())), "fresh macro parse succeeds");
        std::unique_ptr<char, decltype(&std::free)> actual(ts_node_string(ts_tree_root_node(tree.get())), std::free);
        std::unique_ptr<char, decltype(&std::free)> expected(ts_node_string(ts_tree_root_node(fresh.get())), std::free);
        Check(std::string_view(actual.get()) == expected.get(), "incremental macro tree matches a fresh parse");
        source = updated;
    }
}

void TestLayoutProgram() {
    FormatLayoutProgramBuilder builder(4, 80);
    builder.SetTokenCount(2);
    PrintToken first{.sourceIndex = 0};
    PrintToken second{.sourceIndex = 1};
    {
        auto scope = builder.TokenScope(first, 17);
        builder.Write("value", 1);
        builder.NewLine();
        builder.SetPendingIndent(2);
    }
    builder.ForceColumnZero();
    builder.Write("#define VALUE 2", 0);
    builder.NewLine();
    {
        auto scope = builder.TokenScope(second, 17);
        builder.SetPendingIndent(2);
        builder.Write("+ VALUE", 0);
        builder.NewLine();
        builder.BlankLine();
        builder.ReopenLastLine(true);
        builder.Write(";", 0);
    }
    const auto program = builder.Finish();
    const std::string expected = "    value\n#define VALUE 2\n        + VALUE;\n";
    Check(EmitFormatLayoutProgram(program, 4, 80) == expected, "selected anchors survive directives and reopen");
    Check(EmitFormatLayoutProgram(program, 4, 80) == expected, "completed layout replays without mutation");
    Check(program.tokenLines[0].first == 0 && program.tokenLines[0].last == 0 &&
        program.tokenLines[1].first == 2 && program.tokenLines[1].last == 2, "selected source-token line ranges");
    Check(std::any_of(program.anchors.begin(), program.anchors.end(), [](const auto& anchor) {
        return anchor.owner == 17 && anchor.indent == 2;
    }), "continuation anchor retains its syntax owner");
    bool rejected = false;
    try { builder.NewLine(); } catch (const std::logic_error&) { rejected = true; }
    Check(rejected, "completed program rejects further planning");

    FormatOutput lines(4, 80);
    lines.Write("one\ntwo", 0);
    Check(lines.CurrentLineIndex() == 1, "embedded newline is measured");
    lines.BlankLine();
    Check(lines.CurrentLineIndex() == 3, "blank line is measured");
    lines.ReopenLastLine(true);
    Check(lines.CurrentLineIndex() == 1, "reopen removes both boundary newlines");
    lines.NewLine();
    lines.AppendCompleteLines("three\nfour\n");
    Check(lines.CurrentLineIndex() == 4, "complete lines are measured");
}

void TestResolvedLayoutIndentation() {
    FormatLayoutProgramBuilder builder(4, 80);
    builder.Write("first;", 0);
    builder.NewLine();
    builder.SetPendingIndent(2);
    builder.GroupBoundary({}, false);
    builder.Write("next;", 0);
    auto program = builder.Finish();
    program.groupBoundaries[0].required = true;
    Check(EmitFormatLayoutProgram(program, 4, 80) == "first;\n\n        next;\n",
        "late blank-line insertion cannot reset a resolved write indentation");

    SyntaxNode comments;
    auto write = [&](auto& output) {
        output.ResetCommentContinuation();
        output.Write("#define ACTION", 0);
        output.ResetCommentContinuation();
        output.NewLine(true);
        output.Write("one;", 0);
        output.BlankLine(true);
        output.Write("longer;", 0);
        output.NewLine();
        output.Write("int a;", 0);
        output.WriteComment("// first", 0, &comments, FormatOutputComment::Trailing, true);
        output.NewLine();
        output.Write("int longer;", 0);
        output.WriteComment("// second", 0, &comments, FormatOutputComment::Trailing, true);
        output.NewLine();
        output.WriteComment("// continued", 1, &comments, FormatOutputComment::Continuation, true);
        output.NewLine();
        output.ResetCommentContinuation();
        output.Write("next;", 0);
        output.ResetCommentContinuation();
        output.NewLine();
        output.WriteComment("// detached", 1, &comments, FormatOutputComment::Continuation, true);
    };
    FormatOutput reference(4, 80);
    write(reference);
    FormatLayoutProgramBuilder recorded(4, 80);
    write(recorded);
    const auto selected = recorded.Finish();
    Check(std::count_if(selected.commands.begin(), selected.commands.end(), [](const auto& command) {
        return command.kind == FormatLayoutCommandKind::ResetComments;
    }) == 1, "only an active comment continuation requires a reset command");
    Check(EmitFormatLayoutProgram(selected, 4, 80) == reference.Finish(),
        "resolved anchors preserve macro suffix and comment alignment semantics");
}

void TestIndependentLayoutLowering() {
    auto lower = [](std::string_view source, int structuralIndent, bool macroContinuation) {
        FormatterConfig config;
        config.columnLimit = 40;
        auto syntax = ParseFormatModel(source, config);
        Check(syntax.parse.ok, "independent lowering input parses");
        const auto tokens = BuildPrintTokens(syntax, config.tabWidth);
        FormatLayoutTree tree(tokens);
        auto& region = tree.AddRegion(tokens, {});
        region.solution = SolveFormatBreaks(
            config, region.model, structuralIndent * config.indentWidth, structuralIndent, config.indentWidth,
            macroContinuation ? 2 : 0);
        FormatLayoutProgramBuilder program(config.indentWidth, config.columnLimit);
        program.SetTokenCount(tokens.size());
        program.SetPendingIndent(structuralIndent);
        FormatLayoutWriteContext context{
            .sourceTokens = tokens,
            .structuralIndent = structuralIndent,
            .indentWidth = config.indentWidth,
            .macroContinuation = macroContinuation,
        };
        FormatLayoutWriter writer(program, tree, context);
        context.structuralIndent = 99;
        context.macroContinuation = !macroContinuation;
        LowerFormatLayout(config, region.model, region.solution, structuralIndent, tree, writer);
        return program.Finish();
    };
    // Neither a planner nor the original syntax/models survive into replay.
    const auto list = lower("auto value=Pack{first, // first\nsecond};\n", 1, false);
    const std::string expected = "    auto value = Pack{\n        first,  // first\n        second,\n    };\n";
    Check(EmitFormatLayoutProgram(list, 4, 40) == expected,
        "independent lowering preserves trailing comments and captures structural indentation");
    Check(EmitFormatLayoutProgram(list, 4, 40) == expected,
        "lowered program owns its text and replays after syntax and region destruction");
    const auto macro = lower("#define VALUE Make(first_argument, second)\n", 0, true);
    Check(EmitFormatLayoutProgram(macro, 4, 40) ==
        "#define VALUE \\\n    Make(first_argument, second)\n",
        "independent lowering captures macro continuation mode");
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
        FormatOutput output(2, 80);
        output.Write("#define X", 0);
        output.BlankLine(true);
        output.BlankLine(true);
        Check(output.State().macroContinuation && output.CurrentColumn(0) == 2, "blank macro lines preserve continuation indentation");
        output.Write("x", 0);
        output.BlankLine();
        output.Write("next", 0);
        Check(output.Finish() == "#define X \\\n          \\\n  x\n\nnext\n", "blank macro lines collapse and align without extending the final line");
    }
    {
        FormatOutput output(2, 80);
        output.Write("#define X", 0);
        output.NewLine(true);
        output.Write("struct X {", 0);
        output.BlankLine(true);
        output.Write("int x;", 1);
        output.NewLine(true);
        output.SetPendingIndent(3);
        output.BlankLine(true);
        output.SetPendingIndent(3);
        output.Write("int y;", 1);
        output.NewLine(true);
        output.Write("};", 0);
        Check(output.Finish() == "#define X    \\\n  struct X { \\\n             \\\n    int x;   \\\n             \\\n      int y; \\\n  };\n", "macro continuation alignment includes blank lines and preserves content indentation");
    }
    {
        FormatOutput output(2, 12);
        output.Write("#define X", 0);
        output.NewLine(true);
        output.WriteAtIndent("1234567890", 0);
        output.NewLine(true);
        output.WriteAtIndent("12345678901", 0);
        output.NewLine(true);
        output.WriteAtIndent("123456789012", 0);
        output.NewLine(true);
        output.WriteAtIndent("1234567890123", 0);
        output.NewLine(true);
        output.WriteAtIndent("x", 0);
        output.NewLine(true);
        output.WriteAtIndent("last_line_is_longer", 0);
        output.NewLine();
        output.Write("#define Y", 0);
        output.NewLine(true);
        output.Write("y", 0);
        output.NewLine(true);
        output.Write("end", 0);
        Check(output.Finish() ==
            "#define X  \\\n1234567890 \\\n12345678901 \\\n123456789012 \\\n1234567890123 \\\nx          \\\nlast_line_is_longer\n"
            "#define Y \\\n  y       \\\n  end\n",
            "alignment includes a suffix at the limit, excludes overflowing and final lines, and resets per macro");
    }
    {
        FormatOutput output(2, 4);
        output.WriteAtIndent("abcd", 0);
        output.NewLine(true);
        output.WriteAtIndent("abcdef", 0);
        output.NewLine(true);
        output.WriteAtIndent("end", 0);
        Check(output.Finish() == "abcd \\\nabcdef \\\nend\n", "macros with no fitting continuation lines keep minimal suffixes");
    }
    {
        FormatOutput output(2, 8);
        output.WriteAtIndent("\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9", 0);
        output.NewLine(true);
        output.WriteAtIndent("x", 0);
        output.NewLine(true);
        output.WriteAtIndent("end", 0);
        Check(output.Finish() == "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9 \\\nx      \\\nend\n", "continuation alignment measures Unicode columns instead of bytes");
    }
    {
        for (int limit : {19, 24}) {
            SyntaxNode group;
            FormatOutput output(2, limit);
            output.Write("#define X", 0);
            output.NewLine(true);
            output.Write("a;", 0);
            output.WriteComment("// longer", 0, &group, FormatOutputComment::Trailing, true);
            output.NewLine(true);
            output.Write("long;", 0);
            output.WriteComment("// x", 0, &group, FormatOutputComment::Trailing, true);
            output.NewLine(true);
            output.Write("end", 0);
            const std::string_view expected = limit == 24 ?
                "#define X          \\\n  a;     // longer \\\n  long;  // x      \\\n  end\n" :
                "#define X     \\\n  a;     // longer \\\n  long;  // x \\\n  end\n";
            Check(output.Finish() == expected, "backslash alignment and overflow exclusion use final comment padding");
        }
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


void TestParseMacroConfiguration() {
    FormatterConfig config;
    const auto check = [&](std::string_view name, bool expected) {
        const auto model = ParseFormatModel(std::string(name) + " value;", config);
        Check(model.parse.ok, "configured and unconfigured identifiers parse");
        bool hasMacroItem = false;
        for (const auto& node : model.nodes) {
            hasMacroItem |= node.kind == SyntaxNodeKind::BareMacroItem || node.kind == SyntaxNodeKind::MacroCallItem;
        }
        Check(hasMacroItem == expected, "macro classification follows the current parse configuration");
    };
    config.itemMacros = {"LEFT"};
    check("LEFT", true);
    check("LONG", false);
    check("RIGHT", false);
    config.itemMacros = {"R*"};
    check("RIGHT", true);
    check("LEFT", false);
    // Direct API configurations retain the matcher's empty-prefix behavior.
    config.itemMacros = {"*"};
    check("LEFT", true);
    check("RIGHT", true);
    config.itemMacros.clear();
    check("LEFT", false);
    check("RIGHT", false);
}

void TestSyntaxMap() {
    FormatSyntaxMap<size_t> values;
    std::vector<SyntaxNode> nodes(4096);
    SyntaxNode missing;
    Check(values.Find(nullptr) == nullptr && values.Find(&missing) == nullptr, "empty syntax map lookup");
    Check(values.Insert(nullptr, 17).second, "syntax map supports a null identity");
    for (size_t i = 0; i < nodes.size(); ++i) {
        const size_t index = i * 37 % nodes.size();
        Check(values.Insert(&nodes[index], index * 3).second, "distinct syntax identities insert once");
    }
    values.Reserve(nodes.size() * 3);
    for (size_t index = 0; index < nodes.size(); ++index) {
        const auto* value = values.Find(&nodes[index]);
        Check(value != nullptr && *value == index * 3, "syntax values survive collisions and growth");
    }
    const auto duplicate = values.Insert(&nodes[123], 99);
    Check(!duplicate.second && *duplicate.first == 369, "duplicate insertion preserves the stored value");
    values.InsertOrAssign(&nodes[123], 99);
    values.InsertOrAssign(nullptr, 42);
    Check(*values.Find(&nodes[123]) == 99 && *values.Find(nullptr) == 42 && values.Find(&missing) == nullptr,
        "assignment preserves distinct and missing identities");
    FormatSyntaxMap<bool> membership;
    membership.Insert(&nodes[0], false);
    Check(membership.Contains(&nodes[0]) && !membership.Contains(&nodes[1]), "membership is independent of value");
}

void TestPersistentLayoutOwners() {
    FormatterConfig config;
    auto syntax = ParseFormatModel("int value = left + [] { work(); return middle; }() + right;", config);
    Check(syntax.parse.ok, "persistent layout fixture parses");
    const auto tokens = BuildPrintTokens(syntax, config.tabWidth);
    FormatLayoutTree tree(tokens);
    const auto ownerId = tree.SourceItem(tokens.front().node);
    Check(ownerId != 0, "source item has a stable layout owner");
    const auto& owner = tree.Owner(ownerId);
    // Check complete source extents independently of the tree's bottom-up construction.
    for (const auto& token : tokens) {
        for (const SyntaxNode* syntax = token.node; syntax != nullptr; syntax = syntax->parent) {
            size_t begin = tokens.size(), end = 0;
            for (size_t index = 0; index < tokens.size(); ++index) {
                if (PrintTokenSyntaxPathContains(tokens[index], syntax)) {
                    begin = std::min(begin, index);
                    end = index + 1;
                }
            }
            const auto& extent = tree.Owner(tree.FindOwner(syntax));
            Check(extent.begin == begin && extent.end == end, "owner range covers exactly its descendant tokens");
        }
    }
    const auto& complete = tree.CompleteModel(ownerId);
    const auto* originalRoot = complete.root;
    const auto* firstToken = &tokens.front();
    const auto& first = tree.AddRegion(std::span(tokens).first(3), {});
    const auto* firstRoot = first.model.root;
    for (size_t index = 3; index < tokens.size(); ++index) {
        tree.AddRegion(std::span(tokens).subspan(index, 1), {});
    }
    Check(&owner == &tree.Owner(ownerId) && complete.root == originalRoot &&
        &complete == &tree.CompleteModel(ownerId), "cost regions preserve enclosing owners and complete models");
    Check(first.model.root == firstRoot && first.tokens.front().node == firstToken->node &&
        &first.tokens.front() != firstToken, "regions retain stable token projections independently");
}

void TestPersistentHeaderIndent() {
    FormatterConfig config;
    auto syntax = ParseFormatModel("Record::Record() : first_(0), last_(1) { Work(); Done(); }", config);
    Check(syntax.parse.ok, "persistent header fixture parses");
    const auto tokens = BuildPrintTokens(syntax, config.tabWidth);
    FormatLayoutTree tree(tokens);
    tree.BeginToken(tokens.front(), 2);
    for (const auto& token : tokens) tree.BeginToken(token, 7);
    size_t begin = 0;
    size_t end = 0;
    for (size_t index = 0; index < tokens.size(); ++index) {
        if (FormatTokenText(tokens[index]) == "last_") begin = index;
        if (tokens[index].syntaxKind == SyntaxNodeKind::LeftBrace) { end = index + 1; break; }
    }
    FormatLayoutRegionContext context;
    tree.ConstrainBodyHeader(context, std::span(tokens).subspan(begin, end - begin));
    Check(context.continuedBodyHeader != nullptr && context.continuedBodyHeaderOwnerIndent == 2,
        "later continuation indentation cannot replace the declaration owner's structural indentation");
}

void TestCompleteConditionalLayout() {
    FormatterConfig config;
    auto syntax = ParseFormatModel("int values[] = {\n#if ENABLED\n1,\n#else\n2,\n#endif\n3};", config);
    Check(syntax.parse.ok, "complete conditional fixture parses");
    const auto tokens = BuildPrintTokens(syntax, config.tabWidth);
    FormatLayoutTree tree(tokens);
    const auto& model = tree.CompleteModel(tree.SourceItem(tokens.front().node));
    std::unordered_set<const SyntaxNode*> retained;
    const auto visit = [&](auto&& self, const FormatBreakNode& node) -> void {
        const auto token = [&](const FormatBreakToken& value) { if (value.token != nullptr) retained.insert(value.token->node); };
        token(node.token);
        token(node.leadingTrailingComment);
        token(node.sourceTrailingComma);
        for (const auto& op : node.operators) token(op);
        for (const auto& comments : node.commentsBeforeOperators) for (const auto& comment : comments) token(comment);
        for (const auto* child : node.children) self(self, *child);
        for (const auto* operand : node.operands) self(self, *operand);
        for (const auto& item : node.items) {
            self(self, *item.node);
            token(item.separator);
            token(item.trailingComment);
        }
    };
    visit(visit, *model.root);
    for (const auto& token : tokens) {
        Check(token.kind == PrintTokenKind::BlankLine || retained.contains(token.node),
            "complete layouts retain directive headers, branch children, and separators");
    }
}

void TestChainContinuation() {
    FormatterConfig config;
    FormatModel model = ParseFormatModel(
        "void f(int x, int y) { use(a + [] { work(); return b + c + e; }() + d); }", config
    );
    Check(model.parse.ok, "chain continuation fixture parses");
    const auto tokens = BuildPrintTokens(model, config.tabWidth);
    FormatLayoutTree tree(tokens);
    auto& continuation = tree.Chains();
    size_t blocks = 0;
    for (size_t index = 0; index < tokens.size(); ++index) {
        const PrintToken& token = tokens[index];
        if (token.syntaxKind != SyntaxNodeKind::LeftBrace) {
            continue;
        }
        ++blocks;
        continuation.AnalyzeBlock(index);
        FormatLayoutRegionContext context;
        continuation.Constrain(context);
        if (blocks == 1) {
            Check(context.chainPlacements == nullptr,
                "operators inside the function body do not cross its opening brace");
        } else {
            Check(context.chainPlacements != nullptr,
                "only the two enclosing plus operators cross the lambda body");
            const SyntaxNode* body = token.node->parent;
            for (const PrintToken& candidate : tokens) {
                if (candidate.syntaxKind == SyntaxNodeKind::Plus) {
                    Check(context.chainPlacements->Lookup(candidate.node).has_value() !=
                        PrintTokenSyntaxPathContains(candidate, body), "inner and enclosing chain operators stay distinct");
                }
            }
            continuation.FinishBoundary(3);
            continuation.Constrain(context);
            size_t constrained = 0;
            for (const auto& candidate : tokens) {
                if (const auto layout = context.chainPlacements->Lookup(candidate.node)) {
                    Check(layout->baseIndent == 3 && layout->requiredBreak,
                        "operators share the selected placement of their complete chain owner");
                    ++constrained;
                }
            }
            Check(constrained == 2, "only enclosing operators reference the persistent chain placement");
        }
    }
    Check(blocks == 2, "both function and lambda block boundaries were exercised");
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
    FormatLayoutTree tree(tokens);
    auto& continuation = tree.Lists();
    const auto* plan = continuation.PlanBlock(1);
    Check(plan != nullptr && plan->listBoundaries.size() == 1, "block plan retains its enclosing list delimiter");
    Check(plan->listBoundaries.front().forceSplit, "following list item requires the complete list to split");
    continuation.RecordSelection(&open, 3, 1);
    Check(continuation.ResolveBlock() == 3, "selected list indentation is retained across the block");
    Check(!continuation.BoundaryFor(tokens[1], FormatListContinuationKind::Block), "body header precedes its list boundary");
    continuation.AfterBlock(tokens[2], &tokens[3]);

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
    Check(!continuation.BoundaryFor(token(nestedComma), FormatListContinuationKind::Block), "nested list separator cannot consume its enclosing continuation");
    const auto separator = continuation.BoundaryFor(tokens[3], FormatListContinuationKind::Block);
    Check(separator && !separator->beforeToken && separator->indent == 3, "separator breaks after itself at the selected item indent");
    const auto closer = continuation.BoundaryFor(tokens[4], FormatListContinuationKind::Block);
    Check(closer && closer->beforeToken && closer->indent == 1, "closer breaks before itself at the selected close indent");
    Check(continuation.BoundaryFor(tokens[4], FormatListContinuationKind::Block)->indent == 1, "boundary lookup never consumes the selected owner placement");
}


void TestChoiceHistory() {
    FormatChoiceHistory history;
    const auto first = history.AddChoice(nullptr, 1, FormatBreakChoice::Split, 2);
    const auto latest = history.AddChoice(first, 1, FormatBreakChoice::Compact, 9);
    Check(history.Concat(nullptr, latest) == latest && history.Concat(latest, nullptr) == latest,
        "empty history is a concatenation identity");
    Check(FormatChoiceHistory::Find(latest, 1) == FormatBreakChoice::Compact, "lookup gives the latest matching record");
    auto records = history.AddAttachedOperator(latest, 9);
    records = history.AddAttachedOperator(records, 4);
    records = history.AddAttachedOperator(records, 9);
    const auto solution = FormatChoiceHistory::Materialize(records, 4);
    Check(solution.choices[1] == FormatBreakChoice::Split && solution.indentLevels[1] == 2,
        "materialization retains the first choice and render base");
    Check(solution.attachedChainOperators == std::vector<std::uint32_t>({4, 9}), "attached operator indexes are sorted and unique");
    Check(solution.choices[3] == FormatBreakChoice::Compact && solution.indentLevels[3] == -1,
        "unassigned nodes retain materialization defaults");
    auto branch = history.AddChoice(first, 2, FormatBreakChoice::Split, 5);
    const auto branchSolution = FormatChoiceHistory::Materialize(branch, 4);
    Check(branchSolution.choices[2] == FormatBreakChoice::Split && branchSolution.indentLevels[2] == 5,
        "branches retain their own choices and render bases");
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
        TestLayoutProgram();
        TestResolvedLayoutIndentation();
        TestIndependentLayoutLowering();
        TestParseMacroConfiguration();
        TestIncrementalMacroParsing();
        TestPersistentLayoutOwners();
        TestSyntaxMap();
        TestPersistentHeaderIndent();
        TestCompleteConditionalLayout();
        TestChainContinuation();
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

#include <iostream>
#include <stdexcept>
#include <string_view>

#include "format/impl/format_model.h"
#include "format/impl/format_output.h"

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

}  // namespace

int main() {
    try {
        TestOutput();
    } catch (const std::exception& error) {
        std::cerr << "Layout contract test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Passed layout contract tests.\n";
}

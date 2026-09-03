#include "format/impl/format_break_model_dump.h"

#include <string>
#include <string_view>

#include "format/impl/format_break_model_inline_helpers.h"

namespace {

constexpr int kIndentSpaces = 2;

void WriteIndent(FILE* output, int indent) {
    for (int index = 0; index < indent * kIndentSpaces; ++index) {
        std::fputc(' ', output);
    }
}

void WriteString(FILE* output, std::string_view text) {
    if (!text.empty()) {
        std::fwrite(text.data(), 1, text.size(), output);
    }
}

void WriteQuotedText(FILE* output, std::string_view text) {
    std::fputc('"', output);
    for (const unsigned char ch : text) {
        switch (ch) {
            case '\\':
                std::fputs("\\\\", output);
                break;
            case '"':
                std::fputs("\\\"", output);
                break;
            case '\n':
                std::fputs("\\n", output);
                break;
            case '\r':
                std::fputs("\\r", output);
                break;
            case '\t':
                std::fputs("\\t", output);
                break;
            default:
                if (ch < 0x20) {
                    std::fprintf(output, "\\x%02X", static_cast<unsigned int>(ch));
                } else {
                    std::fputc(static_cast<char>(ch), output);
                }
                break;
        }
    }
    std::fputc('"', output);
}

std::string_view NodeKindName(FormatBreakNodeKind kind) {
    switch (kind) {
        case FormatBreakNodeKind::Token:
            return "Token";
        case FormatBreakNodeKind::Sequence:
            return "Sequence";
        case FormatBreakNodeKind::Delimited:
            return "Delimited";
        case FormatBreakNodeKind::PrefixList:
            return "PrefixList";
        case FormatBreakNodeKind::StatementSequence:
            return "StatementSequence";
        case FormatBreakNodeKind::FunctionSignature:
            return "FunctionSignature";
        case FormatBreakNodeKind::BodyHeader:
            return "BodyHeader";
        case FormatBreakNodeKind::Chain:
            return "Chain";
        case FormatBreakNodeKind::AdjacentStrings:
            return "AdjacentStrings";
    }
    return "Unknown";
}

std::string_view DelimiterKindName(FormatBreakDelimiterKind kind) {
    switch (kind) {
        case FormatBreakDelimiterKind::None:
            return "none";
        case FormatBreakDelimiterKind::Paren:
            return "paren";
        case FormatBreakDelimiterKind::Bracket:
            return "bracket";
        case FormatBreakDelimiterKind::Brace:
            return "brace";
        case FormatBreakDelimiterKind::Angle:
            return "angle";
    }
    return "unknown";
}

std::string_view ChainKindName(FormatBreakChainKind kind) {
    switch (kind) {
        case FormatBreakChainKind::AfterOperator:
            return "after-operator";
        case FormatBreakChainKind::CallApplication:
            return "call-application";
        case FormatBreakChainKind::MemberBeforeOperator:
            return "member-before-operator";
        case FormatBreakChainKind::StreamBeforeOperator:
            return "stream-before-operator";
        case FormatBreakChainKind::Ternary:
            return "ternary";
    }
    return "unknown";
}

std::string_view ChoiceName(FormatBreakChoice choice) {
    switch (choice) {
        case FormatBreakChoice::Compact:
            return "compact";
        case FormatBreakChoice::Split:
            return "split";
        case FormatBreakChoice::SplitPacked:
            return "split-packed";
        case FormatBreakChoice::BodyHeaderSplitAtParentIndent:
            return "body-header-split-at-parent-indent";
        case FormatBreakChoice::BodyHeaderDetachedBody:
            return "body-header-detached-body";
        case FormatBreakChoice::SplitAttachedOpen:
            return "split-attached-open";
        case FormatBreakChoice::SplitDelimiterStack:
            return "split-delimiter-stack";
        case FormatBreakChoice::SplitDelimiterStackDetachedLeaf:
            return "split-delimiter-stack-detached-leaf";
        case FormatBreakChoice::SplitDelimiterStackRun:
            return "split-delimiter-stack-run";
        case FormatBreakChoice::CallCompactTail:
            return "call-compact-tail";
        case FormatBreakChoice::MemberCompactTail:
            return "member-compact-tail";
        case FormatBreakChoice::StreamCompactTail:
            return "stream-compact-tail";
        case FormatBreakChoice::TernaryBreakAfterQuestion:
            return "ternary-break-after-question";
        case FormatBreakChoice::TernaryBreakAfterColon:
            return "ternary-break-after-colon";
    }
    return "unknown";
}

bool OwnsBreakChoice(const FormatBreakNode& node) {
    return node.kind != FormatBreakNodeKind::Token && node.kind != FormatBreakNodeKind::Sequence;
}

void WriteBooleanField(FILE* output, int indent, std::string_view name, bool value) {
    if (!value) {
        return;
    }
    WriteIndent(output, indent);
    WriteString(output, name);
    std::fputs(": true\n", output);
}

void WriteTokenField(FILE* output, int indent, std::string_view name, const FormatBreakToken& token) {
    if (token.token == nullptr) {
        return;
    }
    WriteIndent(output, indent);
    WriteString(output, name);
    std::fputs(": ", output);
    WriteQuotedText(output, FormatTokenText(FormatBreakTokenValue(token)));
    std::fputc('\n', output);
}

void
    WriteNode(FILE* output, const FormatBreakNode& node, const FormatBreakSolution& solution, int indent, bool listItem)
{
    const int fieldIndent = listItem ? indent + 1 : indent;
    WriteIndent(output, indent);
    std::fputs(listItem ? "- kind: " : "kind: ", output);
    WriteString(output, NodeKindName(node.kind));
    std::fputc('\n', output);

    WriteIndent(output, fieldIndent);
    std::fprintf(output, "id: %d\n", node.id);
    if (OwnsBreakChoice(node)) {
        WriteIndent(output, fieldIndent);
        std::fprintf(output, "raw-depth: %d\n", node.rawDepth);
        WriteIndent(output, fieldIndent);
        std::fprintf(output, "surcharge: %d\n", node.structuralDepth - node.rawDepth);
        WriteIndent(output, fieldIndent);
        std::fprintf(output, "discount: %d\n", node.structuralDepth - node.breakCost);
        WriteIndent(output, fieldIndent);
        std::fprintf(output, "effective-cost: %d\n", node.breakCost);
        if (node.id >= 0 && static_cast<size_t>(node.id) < solution.choices.size()) {
            WriteIndent(output, fieldIndent);
            std::fputs("selected: ", output);
            WriteString(output, ChoiceName(solution.choices[static_cast<size_t>(node.id)]));
            std::fputc('\n', output);
        }
        if (node.id >= 0 && static_cast<size_t>(node.id) < solution.indentLevels.size()) {
            const int selectedIndent = solution.indentLevels[static_cast<size_t>(node.id)];
            if (selectedIndent >= 0) {
                WriteIndent(output, fieldIndent);
                std::fprintf(output, "selected-indent: %d\n", selectedIndent);
            }
        }
    }
    if (node.kind == FormatBreakNodeKind::Token) {
        WriteTokenField(output, fieldIndent, "text", node.token);
        WriteBooleanField(output, fieldIndent, "space-before", node.token.spaceBefore);
        WriteBooleanField(output, fieldIndent, "context-only", node.token.contextOnly);
    }
    if (node.kind == FormatBreakNodeKind::Delimited) {
        WriteIndent(output, fieldIndent);
        std::fputs("delimiter: ", output);
        WriteString(output, DelimiterKindName(node.delimiterKind));
        std::fputc('\n', output);
    }
    if (node.kind == FormatBreakNodeKind::Chain) {
        WriteIndent(output, fieldIndent);
        std::fputs("chain: ", output);
        WriteString(output, ChainKindName(node.chainKind));
        std::fputc('\n', output);
    }
    WriteBooleanField(output, fieldIndent, "force-split", node.forceSplit);
    WriteBooleanField(output, fieldIndent, "blank-line-before-close", node.blankLineBeforeClose);
    WriteBooleanField(output, fieldIndent, "compact-requires-unbroken-items", node.compactRequiresUnbrokenItems);
    WriteBooleanField(output, fieldIndent, "flat-split-indent", node.flatSplitIndent);
    WriteBooleanField(output, fieldIndent, "function-signature-has-body", node.functionSignatureHasBody);
    WriteBooleanField(output, fieldIndent, "body-header-is-lambda", node.bodyHeaderIsLambda);
    WriteBooleanField(output, fieldIndent, "body-header-single-statement-body", node.bodyHeaderSingleStatementBody);
    WriteBooleanField(
        output,
        fieldIndent,
        "body-header-detach-body-after-expanded-header",
        node.bodyHeaderDetachBodyAfterExpandedHeader
    );
    WriteTokenField(output, fieldIndent, "leading-trailing-comment", node.leadingTrailingComment);

    if (!node.children.empty()) {
        WriteIndent(output, fieldIndent);
        std::fputs("children:\n", output);
        for (const FormatBreakNode* child : node.children) {
            if (child != nullptr) {
                WriteNode(output, *child, solution, fieldIndent + 1, true);
            }
        }
    }
    if (!node.items.empty()) {
        WriteIndent(output, fieldIndent);
        std::fputs("items:\n", output);
        for (const FormatBreakListItem& item : node.items) {
            WriteIndent(output, fieldIndent + 1);
            std::fputs("- node:\n", output);
            if (item.node != nullptr) {
                WriteNode(output, *item.node, solution, fieldIndent + 3, false);
            }
            WriteTokenField(output, fieldIndent + 2, "separator", item.separator);
            WriteTokenField(output, fieldIndent + 2, "trailing-comment", item.trailingComment);
            WriteBooleanField(output, fieldIndent + 2, "blank-line-before", item.blankLineBefore);
        }
    }
    if (!node.operands.empty()) {
        WriteIndent(output, fieldIndent);
        std::fputs("operands:\n", output);
        for (const FormatBreakNode* operand : node.operands) {
            if (operand != nullptr) {
                WriteNode(output, *operand, solution, fieldIndent + 1, true);
            }
        }
    }
    if (!node.operators.empty()) {
        WriteIndent(output, fieldIndent);
        std::fputs("operators:\n", output);
        for (const FormatBreakToken& token : node.operators) {
            WriteIndent(output, fieldIndent + 1);
            if (token.token == nullptr) {
                std::fputs("- null\n", output);
            } else {
                std::fputs("- ", output);
                WriteQuotedText(output, FormatTokenText(FormatBreakTokenValue(token)));
                std::fputc('\n', output);
            }
        }
    }
}

std::string SegmentText(std::span<const PrintToken> tokens) {
    std::string result;
    const PrintToken* previous = nullptr;
    for (const PrintToken& token : tokens) {
        if (FormatTokenNeedsSpace(previous, token) && !result.empty()) {
            result.push_back(' ');
        }
        result.append(FormatTokenText(token));
        previous = &token;
    }
    return result;
}

}  // namespace

void FormatBreakModelDumpWriter::WriteSegment(
    std::span<const PrintToken> tokens,
    const FormatBreakModel& model,
    const FormatBreakSolution& solution,
    int startColumn,
    int baseIndent,
    int breakLineSuffixWidth
) {
    if (output_ == nullptr || model.root == nullptr) {
        return;
    }
    if (segmentCount_ != 0) {
        std::fputs("---\n", output_);
    }
    std::fprintf(output_, "segment: %zu\n", ++segmentCount_);
    std::fputs("tokens: ", output_);
    WriteQuotedText(output_, SegmentText(tokens));
    std::fputc('\n', output_);
    std::fprintf(output_, "start-column: %d\n", startColumn);
    std::fprintf(output_, "base-indent: %d\n", baseIndent);
    std::fprintf(output_, "break-line-suffix-width: %d\n", breakLineSuffixWidth);
    std::fputs("tree:\n", output_);
    WriteNode(output_, *model.root, solution, 1, false);
}

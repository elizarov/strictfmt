#include "format/impl/format_break_emitter.h"

#include <algorithm>
#include <utility>

#include "format/impl/format_break_model_inline_helpers.h"
#include "format/impl/format_break_solution.h"
#include "format/impl/format_config.h"
#include "format/impl/format_delimiter_stack.h"

namespace {

class BreakEmitter {
public:
    BreakEmitter(const FormatterConfig& config, FormatBreakOutput& output, const SyntaxNode* terminalToken) :
        config_(config), output_(output), terminalToken_(terminalToken) {}

    FormatBreakEmissionSummary Emit(const FormatBreakNode& root, const FormatBreakSolution& solution, int baseIndent) {
        CollectSplitContexts(root, solution, summary_.splitLists);
        EmitBreakNode(root, solution, baseIndent);
        return std::move(summary_);
    }

private:
    const FormatterConfig& config_;
    FormatBreakOutput& output_;
    const SyntaxNode* terminalToken_;
    FormatBreakEmissionSummary summary_;
    bool suppressNextBreakTokenSpace_ = false;

    void NewLineWithIndent(int indent) { output_.BreakLine(indent, false); }
    void BreakListLine(int indent, bool blankLine) { output_.BreakLine(indent, blankLine); }
    void Write(std::string_view text) { output_.Write(text); }
    void Space() { output_.Space(); }

    void WriteBreakTokenText(
        const FormatBreakToken& token, std::string_view text, std::optional<int> continuationBaseIndent = std::nullopt
    ) {
        const bool suppressSpace = suppressNextBreakTokenSpace_;
        suppressNextBreakTokenSpace_ = false;
        if (token.contextOnly) {
            return;
        }
        const PrintToken& printToken = FormatBreakTokenValue(token);
        if (
            continuationBaseIndent &&
            printToken.kind == PrintTokenKind::Known &&
            printToken.syntaxKind == SyntaxNodeKind::LeftBrace &&
            printToken.node == terminalToken_
        ) {
            summary_.blockOpenIndent = std::max(0, *continuationBaseIndent - (printToken.inMacroValue ? 1 : 0));
        }
        output_.WriteToken(token, text, continuationBaseIndent, suppressSpace);
    }

    FormatBreakChoice ChoiceFor(const FormatBreakSolution& solution, int nodeId) const {
        if (nodeId < 0 || static_cast<size_t>(nodeId) >= solution.choices.size()) {
            return FormatBreakChoice::Compact;
        }
        return solution.choices[static_cast<size_t>(nodeId)];
    }

    static bool IsAttachedChainOperator(const FormatBreakSolution& solution, const FormatBreakToken& op) {
        const std::uint32_t sourceIndex = FormatBreakTokenValue(op).sourceIndex;
        return std::binary_search(
            solution.attachedChainOperators.begin(), solution.attachedChainOperators.end(), sourceIndex
        );
    }

    static bool IsSplitChoice(FormatBreakChoice choice) {
        return choice == FormatBreakChoice::Split ||
            choice == FormatBreakChoice::SplitPacked ||
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

    void WriteBreakToken(const FormatBreakToken& token, std::optional<int> continuationBaseIndent = std::nullopt) {
        WriteBreakTokenText(token, FormatTokenText(FormatBreakTokenValue(token)), continuationBaseIndent);
    }

    void EmitBreakNode(const FormatBreakNode& node, const FormatBreakSolution& solution, int baseIndent) {
        if (
            node.id >= 0 &&
            static_cast<size_t>(node.id) < solution.indentLevels.size() &&
            solution.indentLevels[static_cast<size_t>(node.id)] >= 0
        ) {
            baseIndent = solution.indentLevels[static_cast<size_t>(node.id)];
        }
        const FormatBreakOutputState state = output_.State();
        if (state.atLineStart && state.pendingIndentLevel) {
            baseIndent = std::max(baseIndent, *state.pendingIndentLevel);
        }
        switch (node.kind) {
            case FormatBreakNodeKind::Token:
                WriteBreakToken(node.token, baseIndent);
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
        const FormatBreakNode& node, const FormatBreakSolution& solution, size_t index
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
            !FormatBreakHasTrailingComment(node, index) &&
            !HasBlankLineBeforeItem(node, index + 1);
    }

    struct DelimiterStackRun {
        size_t begin = 0;
        size_t end = 0;
        int indentLevel = 0;
    };

    void EmitDelimiterStackNode(const FormatBreakNode& node, const FormatBreakSolution& solution, int baseIndent) {
        const std::optional<FormatDelimiterStack> stack =
            CollectFormatDelimiterStack(node, FormatDelimiterStackPolicy::Emission);
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
            if (ChoiceFor(solution, delimiter->children.front()->id) == FormatBreakChoice::SplitDelimiterStackRun) {
                currentLineIndent = nextOpenIndent;
                NewLineWithIndent(currentLineIndent);
                ++nextOpenIndent;
            }
            if (delimiterRuns.empty() || delimiterRuns.back().indentLevel != currentLineIndent) {
                delimiterRuns
                    .push_back(DelimiterStackRun{.begin = index, .end = index, .indentLevel = currentLineIndent});
            }
            delimiterRuns.back().end = index + 1;
            WriteBreakToken(open);
        }
        const bool detachLeaf = ChoiceFor(solution, node.id) == FormatBreakChoice::SplitDelimiterStackDetachedLeaf;
        if (detachLeaf && output_.State().lineHasText) {
            NewLineWithIndent(nextOpenIndent);
        }
        EmitBreakNode(*stack->leaf, solution, nextOpenIndent);
        for (size_t runIndex = delimiterRuns.size(); runIndex-- > 0;) {
            const DelimiterStackRun& run = delimiterRuns[runIndex];
            const bool firstClosingRun = runIndex + 1 == delimiterRuns.size();
            if (output_.State().lineHasText && (detachLeaf || !firstClosingRun)) {
                NewLineWithIndent(run.indentLevel);
            }
            for (size_t index = run.end; index-- > run.begin;) {
                WriteBreakToken(stack->delimiters[index]->children.back()->token);
            }
        }
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
            if (FormatBreakHasLeadingTrailingComment(node)) {
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
                if (FormatBreakHasTrailingComment(node, index)) {
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
        if (FormatBreakHasLeadingTrailingComment(node)) {
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
            if (choice == FormatBreakChoice::Split && node.splitTrailingCommaItem == index) {
                Write(",");
            }
            if (FormatBreakHasTrailingComment(node, index)) {
                WriteBreakToken(item.trailingComment);
            }
            if (ShouldCombineSplitDelimitedItemBoundary(node, solution, index)) {
                Space();
            } else if (choice == FormatBreakChoice::SplitPacked && index + 1 < node.items.size()) {
                continue;
            } else if (closesInContext && index + 1 == node.items.size()) {
                continue;
            } else {
                const bool hasNextItem = index + 1 < node.items.size();
                if (hasNextItem) {
                    BreakListLine(baseIndent + 1, HasBlankLineBeforeItem(node, index + 1));
                } else {
                    BreakListLine(baseIndent, node.blankLineBeforeClose);
                }
            }
        }
        EmitBreakNode(*node.children[1], solution, baseIndent);
    }

    void EmitPrefixListNode(const FormatBreakNode& node, const FormatBreakSolution& solution, int baseIndent) {
        const FormatBreakChoice choice = ChoiceFor(solution, node.id);
        if (!IsSplitChoice(choice)) {
            EmitBreakNode(*node.children[0], solution, baseIndent);
            if (FormatBreakHasLeadingTrailingComment(node)) {
                WriteBreakToken(node.leadingTrailingComment);
            }
            for (size_t index = 0; index < node.items.size(); ++index) {
                const FormatBreakListItem& item = node.items[index];
                EmitBreakNode(*item.node, solution, baseIndent);
                if (FormatBreakTokenKind(item.separator) == PrintTokenKind::Known) {
                    WriteBreakToken(item.separator);
                }
                if (FormatBreakHasTrailingComment(node, index)) {
                    WriteBreakToken(item.trailingComment);
                }
            }
            return;
        }

        EmitBreakNode(*node.children[0], solution, baseIndent);
        if (FormatBreakHasLeadingTrailingComment(node)) {
            WriteBreakToken(node.leadingTrailingComment);
        }
        BreakListLine(baseIndent + 1, HasBlankLineBeforeItem(node, 0));
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& item = node.items[index];
            EmitBreakNode(*item.node, solution, baseIndent + 1);
            if (FormatBreakTokenKind(item.separator) == PrintTokenKind::Known) {
                WriteBreakToken(item.separator);
            }
            if (FormatBreakHasTrailingComment(node, index)) {
                WriteBreakToken(item.trailingComment);
            }
            if (choice != FormatBreakChoice::SplitPacked && index + 1 < node.items.size()) {
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
            if (FormatBreakHasTrailingComment(node, index)) {
                WriteBreakToken(item.trailingComment);
            }
        }
    }

    void EmitDelimitedNodeAfterAttachedOpen(
        const FormatBreakNode& node, const FormatBreakSolution& solution, int baseIndent
    ) {
        EmitBreakNode(*node.children[0], solution, baseIndent);
        if (FormatBreakHasLeadingTrailingComment(node)) {
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
            if (node.splitTrailingCommaItem == index) {
                Write(",");
            }
            if (FormatBreakHasTrailingComment(node, index)) {
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
                    hasNextItem ? HasBlankLineBeforeItem(node, index + 1) : node.blankLineBeforeClose
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

    void EmitCommentsBeforeChainOperator(const FormatBreakNode& node, size_t index) {
        if (index >= node.commentsBeforeOperators.size()) {
            return;
        }
        for (const FormatBreakToken& comment : node.commentsBeforeOperators[index]) {
            WriteBreakToken(comment);
        }
    }

    void EmitChainNode(const FormatBreakNode& node, const FormatBreakSolution& solution, int baseIndent) {
        const FormatBreakChoice choice = ChoiceFor(solution, node.id);
        if (choice == FormatBreakChoice::Compact) {
            for (size_t index = 0; index < node.operands.size(); ++index) {
                EmitBreakNode(*node.operands[index], solution, baseIndent);
                if (index < node.operators.size()) {
                    EmitCommentsBeforeChainOperator(node, index);
                    WriteBreakToken(node.operators[index]);
                }
            }
            return;
        }
        const int splitBaseIndent = node.requiredChainBreakBaseIndent.value_or(baseIndent);
        summary_.chainIndents.push_back({.chain = &node, .baseIndent = splitBaseIndent});

        if (node.chainKind == FormatBreakChainKind::StreamBeforeOperator) {
            if (!node.chainStartsWithOperator) {
                EmitBreakNode(*node.operands.front(), solution, baseIndent);
            }
            if (node.chainStartsWithOperator && output_.State().atLineStart) {
                output_.SetPendingIndent(splitBaseIndent + 1);
            } else {
                NewLineWithIndent(splitBaseIndent + 1);
            }
            for (size_t index = 0; index < node.operators.size(); ++index) {
                EmitCommentsBeforeChainOperator(node, index);
                WriteBreakToken(node.operators[index]);
                EmitBreakNode(*node.operands[index + 1], solution, splitBaseIndent + 1);
                if (
                    choice == FormatBreakChoice::Split &&
                    index + 1 < node.operators.size() &&
                    !IsFormatBreakStreamConfigurationOperand(
                        *node.operands[index + 1], config_.streamShiftConfigurationMethods
                    ) &&
                    !IsAttachedChainOperator(solution, node.operators[index + 1])
                ) {
                    NewLineWithIndent(splitBaseIndent + 1);
                }
            }
            return;
        }

        if (node.chainKind == FormatBreakChainKind::CallApplication) {
            EmitBreakNode(*node.operands.front(), solution, baseIndent);
            if (choice == FormatBreakChoice::CallCompactTail) {
                NewLineWithIndent(splitBaseIndent + 1);
                for (size_t index = 1; index < node.operands.size(); ++index) {
                    EmitBreakNode(*node.operands[index], solution, splitBaseIndent + 1);
                }
                return;
            }
            for (size_t index = 1; index < node.operands.size(); ++index) {
                NewLineWithIndent(splitBaseIndent + 1);
                EmitBreakNode(*node.operands[index], solution, splitBaseIndent + 1);
            }
            return;
        }

        if (node.chainKind == FormatBreakChainKind::MemberBeforeOperator) {
            EmitBreakNode(*node.operands.front(), solution, baseIndent);
            if (choice == FormatBreakChoice::MemberCompactTail) {
                NewLineWithIndent(splitBaseIndent + 1);
                for (size_t index = 0; index < node.operators.size(); ++index) {
                    EmitCommentsBeforeChainOperator(node, index);
                    WriteBreakToken(node.operators[index]);
                    EmitBreakNode(*node.operands[index + 1], solution, splitBaseIndent + 1);
                }
                return;
            }
            for (size_t index = 0; index < node.operators.size(); ++index) {
                NewLineWithIndent(splitBaseIndent + 1);
                EmitCommentsBeforeChainOperator(node, index);
                WriteBreakToken(node.operators[index]);
                EmitBreakNode(*node.operands[index + 1], solution, splitBaseIndent + 1);
            }
            return;
        }

        if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() > 2) {
            for (size_t index = 0; index < node.operands.size(); ++index) {
                EmitBreakNode(*node.operands[index], solution, index == 0 ? baseIndent : splitBaseIndent + 1);
                if (index < node.operators.size()) {
                    WriteBreakToken(node.operators[index]);
                    if (
                        FormatBreakTokenKind(node.operators[index]) == PrintTokenKind::Known &&
                        FormatBreakTokenSyntaxKind(node.operators[index]) == SyntaxNodeKind::Colon
                    ) {
                        NewLineWithIndent(splitBaseIndent + 1);
                    }
                }
            }
            return;
        }

        if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() == 2) {
            const int continuationIndent = node.flatSplitIndent ? splitBaseIndent : splitBaseIndent + 1;
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

        const int continuationIndent = node.flatSplitIndent ? splitBaseIndent : splitBaseIndent + 1;
        for (size_t index = 0; index < node.operands.size(); ++index) {
            EmitBreakNode(*node.operands[index], solution, index == 0 ? baseIndent : continuationIndent);
            if (index < node.operators.size()) {
                WriteBreakToken(node.operators[index]);
                if (IsAttachedChainOperator(solution, node.operators[index])) {
                    continue;
                }
                if (
                    index + 1 < node.operands.size() &&
                    node.operands[index + 1]->kind == FormatBreakNodeKind::Delimited &&
                    ChoiceFor(solution, node.operands[index + 1]->id) == FormatBreakChoice::SplitAttachedOpen
                ) {
                    const size_t attachedOperandIndex = index + 1;
                    EmitDelimitedNodeAfterAttachedOpen(
                        *node.operands[attachedOperandIndex], solution, continuationIndent
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
        const bool hasCompactTexts = node.compactStringTexts.size() == node.operands.size() &&
            std::all_of(node.operands.begin(), node.operands.end(), [](const FormatBreakNode* operand) {
                return operand != nullptr && operand->kind == FormatBreakNodeKind::Token;
            });
        if (choice == FormatBreakChoice::Compact && hasCompactTexts) {
            for (size_t index = 0; index < node.operands.size(); ++index) {
                if (node.compactStringTexts[index].empty()) {
                    continue;
                }
                WriteBreakTokenText(node.operands[index]->token, node.compactStringTexts[index]);
            }
            return;
        }
        for (size_t index = 0; index < node.operands.size(); ++index) {
            if (choice == FormatBreakChoice::Split && index > 0) {
                NewLineWithIndent(continuationIndent);
            }
            EmitBreakNode(*node.operands[index], solution, index == 0 ? baseIndent : continuationIndent);
        }
    }

    void CollectSplitContexts(
        const FormatBreakNode& node, const FormatBreakSolution& solution, std::vector<FormatBreakSplitList>& result
    ) const {
        if (
            node.kind == FormatBreakNodeKind::Delimited &&
            node.children.size() > 1 &&
            node.children[1]->kind == FormatBreakNodeKind::Token &&
            node.children[1]->token.contextOnly &&
            IsSplitChoice(ChoiceFor(solution, node.id))
        ) {
            const FormatBreakToken& open = node.children.front()->token;
            if (open.token != nullptr && open.token->node != nullptr) {
                const int baseIndent = solution.indentLevels[static_cast<size_t>(node.id)];
                result.push_back(
                    {.openToken = open.token->node, .itemIndent = baseIndent + 1, .closeIndent = baseIndent}
                );
            }
        }
        if (
            node.kind == FormatBreakNodeKind::PrefixList &&
            !node.children.empty() &&
            node.children.front()->kind == FormatBreakNodeKind::Token &&
            ChoiceFor(solution, node.id) == FormatBreakChoice::Split
        ) {
            const FormatBreakToken& prefix = node.children.front()->token;
            if (prefix.token != nullptr && prefix.token->node != nullptr) {
                const int baseIndent = solution.indentLevels[static_cast<size_t>(node.id)];
                result.push_back({.openToken = prefix.token->node, .itemIndent = baseIndent + 1});
            }
        }
        for (const FormatBreakNode* child : node.children) {
            if (child) {
                CollectSplitContexts(*child, solution, result);
            }
        }
        for (const FormatBreakListItem& item : node.items) {
            if (item.node) {
                CollectSplitContexts(*item.node, solution, result);
            }
        }
        for (const FormatBreakNode* operand : node.operands) {
            if (operand) {
                CollectSplitContexts(*operand, solution, result);
            }
        }
    }

};

}  // namespace

FormatBreakEmissionSummary EmitFormatBreakModel(
    const FormatterConfig& config,
    const FormatBreakModel& model,
    const FormatBreakSolution& solution,
    int baseIndent,
    const SyntaxNode* terminalToken,
    FormatBreakOutput& output
) {
    if (model.root == nullptr) {
        return {};
    }
    return BreakEmitter(config, output, terminalToken).Emit(*model.root, solution, baseIndent);
}

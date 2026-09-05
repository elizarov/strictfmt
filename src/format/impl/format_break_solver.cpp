#include "format/impl/format_break_solver.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "format/impl/format_break_model_inline_helpers.h"
#include "format/impl/format_value_profile.h"
#include "format/impl/format_compact_layout.h"
#include "format/impl/format_choice_history.h"
#include "util/utf8.h"

namespace {

struct NodeResult {
    bool valid = false;
    int endColumn = 0;
    int endIndentLevel = 0;
    bool endLineHasText = false;
    // Exact delimiter-stack search can rewind a line after pricing it. Normal results keep this false.
    bool currentLineOverflowRecorded = false;
    int extraLines = 0;
    FormatValueProfile overflowSizeProfile;
    FormatValueProfile expansionDepthProfile;
    bool ownExpansionCharged = false;
    bool compactNextStreamOperand = false;
    FormatChoiceHistory::Handle choices = nullptr;
};

class NodeResults {
public:
    using iterator = NodeResult*;
    using const_iterator = const NodeResult*;

    NodeResults() = default;

    NodeResults(std::initializer_list<NodeResult> values) {
        for (const NodeResult& value : values) {
            push_back(value);
        }
    }

    NodeResults(const NodeResults& other) {
        for (const NodeResult& value : other) {
            push_back(value);
        }
    }

    NodeResults(NodeResults&& other) noexcept { MoveFrom(std::move(other)); }

    ~NodeResults() { clear(); }

    NodeResults& operator=(const NodeResults& other) {
        if (this != &other) {
            clear();
            usingHeap_ = false;
            for (const NodeResult& value : other) {
                push_back(value);
            }
        }
        return *this;
    }

    NodeResults& operator=(NodeResults&& other) noexcept {
        if (this != &other) {
            clear();
            usingHeap_ = false;
            MoveFrom(std::move(other));
        }
        return *this;
    }

    iterator begin() { return usingHeap_ ? heap_.data() : InlineData(); }

    iterator end() { return begin() + size(); }

    const_iterator begin() const { return usingHeap_ ? heap_.data() : InlineData(); }

    const_iterator end() const { return begin() + size(); }

    bool empty() const { return size() == 0; }

    size_t size() const { return usingHeap_ ? heap_.size() : inlineSize_; }

    NodeResult& operator[](size_t index) { return begin()[index]; }

    const NodeResult& operator[](size_t index) const { return begin()[index]; }

    void push_back(const NodeResult& value) { PushBack(value); }

    void push_back(NodeResult&& value) { PushBack(std::move(value)); }

private:
    template <typename Value>
    void PushBack(Value&& value) {
        if (usingHeap_) {
            heap_.push_back(std::forward<Value>(value));
            return;
        }
        if (inlineSize_ < kInlineCapacity) {
            std::construct_at(InlineData() + inlineSize_, std::forward<Value>(value));
            ++inlineSize_;
            return;
        }
        MoveInlineToHeap();
        heap_.push_back(std::forward<Value>(value));
    }

public:
    iterator erase(iterator it) {
        const size_t index = static_cast<size_t>(it - begin());
        if (usingHeap_) {
            heap_.erase(heap_.begin() + static_cast<std::ptrdiff_t>(index));
            return heap_.data() + index;
        }
        for (size_t cursor = index + 1; cursor < inlineSize_; ++cursor) {
            InlineData()[cursor - 1] = std::move(InlineData()[cursor]);
        }
        --inlineSize_;
        std::destroy_at(InlineData() + inlineSize_);
        return InlineData() + index;
    }

    void clear() {
        if (usingHeap_) {
            heap_.clear();
            return;
        }
        for (size_t index = 0; index < inlineSize_; ++index) {
            std::destroy_at(InlineData() + index);
        }
        inlineSize_ = 0;
    }

private:
    static constexpr size_t kInlineCapacity = 8;

    NodeResult* InlineData() { return std::launder(reinterpret_cast<NodeResult*>(inlineStorage_)); }

    const NodeResult* InlineData() const { return std::launder(reinterpret_cast<const NodeResult*>(inlineStorage_)); }

    void MoveFrom(NodeResults&& other) {
        if (other.usingHeap_) {
            heap_ = std::move(other.heap_);
            usingHeap_ = true;
            other.usingHeap_ = false;
            return;
        }
        for (NodeResult& value : other) {
            push_back(std::move(value));
        }
        other.clear();
    }

    void MoveInlineToHeap() {
        heap_.reserve(kInlineCapacity * 2);
        for (size_t index = 0; index < inlineSize_; ++index) {
            heap_.push_back(std::move(InlineData()[index]));
            std::destroy_at(InlineData() + index);
        }
        inlineSize_ = 0;
        usingHeap_ = true;
    }

    alignas(NodeResult) std::byte inlineStorage_[sizeof(NodeResult) * kInlineCapacity];
    size_t inlineSize_ = 0;
    bool usingHeap_ = false;
    std::vector<NodeResult> heap_;
};

struct ResultMemoEntry {
    int column = 0;
    int indentLevel = 0;
    bool lineHasText = false;
    NodeResult result;
    ResultMemoEntry* next = nullptr;
};

struct AlternativesMemoEntry {
    int column = 0;
    int indentLevel = 0;
    bool lineHasText = false;
    NodeResults results;
    AlternativesMemoEntry* next = nullptr;
};

struct DelimiterStackView {
    std::vector<const FormatBreakNode*> delimiters;
    const FormatBreakNode* leaf = nullptr;
};

struct DelimiterStackRun {
    size_t begin = 0;
    size_t end = 0;
    int indentLevel = 0;
};

struct DelimiterStackPartitionPath {
    const DelimiterStackPartitionPath* previous = nullptr;
    size_t runStart = 0;
};

struct DelimiterStackPartitionCandidate {
    NodeResult result;
    const DelimiterStackPartitionPath* path = nullptr;
};

class Solver {
public:
    Solver(const FormatterConfig& config, const FormatBreakModel& model, int indentWidth, int breakLineSuffixWidth) :
        config_(config),
        indentWidth_(indentWidth),
        breakLineSuffixWidth_(breakLineSuffixWidth),
        memoHeads_(model.nodes == nullptr ? 1 : model.nodes->size() + 1, nullptr),
        alternativesMemoHeads_(model.nodes == nullptr ? 1 : model.nodes->size() + 1, nullptr),
        compactLayout_(model),
        containsForceSplitAdjacentStrings_(model.nodes == nullptr ? 1 : model.nodes->size() + 1, -1),
        containsNonSingleStatementBodyHeader_(model.nodes == nullptr ? 1 : model.nodes->size() + 1, -1) {}

    NodeResult Solve(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        if (const NodeResult* found = FindMemoizedResult(node.id, column, indentLevel, lineHasText)) {
            return *found;
        }

        if (std::optional<NodeResult> compact = SolveCompactOneLine(node, column, indentLevel, lineHasText)) {
            StoreMemoizedResult(node.id, column, indentLevel, lineHasText, *compact);
            return *compact;
        }

        NodeResult result;
        switch (node.kind) {
            case FormatBreakNodeKind::Token:
                result = SolveToken(node.token, column, indentLevel, lineHasText);
                break;
            case FormatBreakNodeKind::Sequence:
                result = SolveChildren(node.children, column, indentLevel, lineHasText);
                break;
            case FormatBreakNodeKind::Delimited:
                result = SolveDelimited(node, column, indentLevel, lineHasText);
                break;
            case FormatBreakNodeKind::PrefixList:
                result = SolvePrefixList(node, column, indentLevel, lineHasText);
                break;
            case FormatBreakNodeKind::StatementSequence:
                result = SolveStatementSequence(node, column, indentLevel, lineHasText);
                break;
            case FormatBreakNodeKind::FunctionSignature:
                result = SolveFunctionSignature(node, column, indentLevel, lineHasText);
                break;
            case FormatBreakNodeKind::BodyHeader:
                result = SolveBodyHeader(node, column, indentLevel, lineHasText);
                break;
            case FormatBreakNodeKind::Chain:
                result = SolveChain(node, column, indentLevel, lineHasText);
                break;
            case FormatBreakNodeKind::AdjacentStrings:
                result = SolveAdjacentStrings(node, column, indentLevel, lineHasText);
                break;
        }
        StoreMemoizedResult(node.id, column, indentLevel, lineHasText, result);
        return result;
    }

private:
    enum class CompactTailExpansionKind {
        None,
        IntrinsicMultilineLiteral,
        Structural,
    };

    const FormatterConfig& config_;
    int indentWidth_ = 4;
    int breakLineSuffixWidth_ = 0;
    std::vector<ResultMemoEntry*> memoHeads_;
    std::deque<ResultMemoEntry> memoEntries_;
    std::vector<AlternativesMemoEntry*> alternativesMemoHeads_;
    std::deque<AlternativesMemoEntry> alternativesMemoEntries_;
    FormatChoiceHistory choiceHistory_;
    std::deque<DelimiterStackPartitionPath> delimiterStackPartitionPathArena_;
    FormatCompactLayout compactLayout_;
    std::vector<signed char> containsForceSplitAdjacentStrings_;
    std::vector<signed char> containsNonSingleStatementBodyHeader_;

    int IndentColumn(int indentLevel) const { return std::max(0, indentLevel) * indentWidth_; }

    static bool HasBlankLineBeforeItem(const FormatBreakNode& node, size_t index) {
        return index < node.items.size() && node.items[index].blankLineBefore;
    }

    static int SpaceBeforeToken(const FormatBreakToken& token, bool lineHasText) {
        if (!lineHasText) {
            return 0;
        }
        if (IsLineCommentToken(FormatBreakTokenValue(token))) {
            return 2;
        }
        return token.spaceBefore ? 1 : 0;
    }

    int CurrentLineOverflow(const NodeResult& result) const {
        return result.endLineHasText && !result.currentLineOverflowRecorded ?
            std::max(0, result.endColumn - config_.columnLimit) : 0;
    }

    int MaximumOverflow(const NodeResult& result) const {
        return std::max(result.overflowSizeProfile.GreatestValue(), CurrentLineOverflow(result));
    }

    bool HasOverflow(const NodeResult& result) const {
        return !result.overflowSizeProfile.Empty() || CurrentLineOverflow(result) > 0;
    }

    void FinishCurrentLine(NodeResult& result, int suffixWidth = 0) const {
        if (result.endLineHasText && !result.currentLineOverflowRecorded) {
            result.overflowSizeProfile.AddValue(std::max(0, result.endColumn + suffixWidth - config_.columnLimit));
            result.currentLineOverflowRecorded = true;
        }
    }

    NodeResult SolveTokenText(
        const FormatBreakToken& token, std::string_view text, int column, int indentLevel, bool lineHasText
    ) const {
        if (token.contextOnly) {
            return {.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        }
        const int space = SpaceBeforeToken(token, lineHasText);
        NodeResult result{
            .valid = true, .endColumn = column + space, .endIndentLevel = indentLevel, .endLineHasText = lineHasText
        };
        while (!text.empty()) {
            const size_t newline = text.find('\n');
            std::string_view line = text.substr(0, newline);
            if (newline != std::string_view::npos && line.ends_with('\r')) {
                line.remove_suffix(1);
            }
            result.endColumn += Utf8CharacterCount(line);
            result.endLineHasText = result.endLineHasText || !line.empty();
            if (newline == std::string_view::npos) {
                break;
            }
            FinishCurrentLine(result);
            ++result.extraLines;
            result.endColumn = 0;
            result.endLineHasText = false;
            result.currentLineOverflowRecorded = false;
            text.remove_prefix(newline + 1);
        }
        if (IsCommentToken(FormatBreakTokenKind(token))) {
            FinishCurrentLine(result);
            ++result.extraLines;
            result.endColumn = IndentColumn(indentLevel);
            result.endLineHasText = false;
            result.currentLineOverflowRecorded = false;
        }
        return result;
    }

    NodeResult SolveToken(const FormatBreakToken& token, int column, int indentLevel, bool lineHasText) const {
        return SolveTokenText(token, FormatTokenText(FormatBreakTokenValue(token)), column, indentLevel, lineHasText);
    }

    void AddChoice(NodeResult& result, int nodeId, FormatBreakChoice choice, int indentLevel = -1) {
        result.choices = choiceHistory_.AddChoice(result.choices, nodeId, choice, indentLevel);
    }
    void AddDeclarationValueContinuationLines(NodeResult& result, int nodeId, int continuationLines) {
        result.choices = choiceHistory_.AddContinuationLines(result.choices, nodeId, continuationLines);
    }
    void AddAttachedChainOperator(NodeResult& result, const FormatBreakToken& op) {
        result.choices = choiceHistory_.AddAttachedOperator(result.choices, FormatBreakTokenValue(op).sourceIndex);
    }

    void Merge(NodeResult& left, const NodeResult& right) {
        left.valid = left.valid && right.valid;
        left.endColumn = right.endColumn;
        left.endIndentLevel = right.endIndentLevel;
        left.endLineHasText = right.endLineHasText;
        left.currentLineOverflowRecorded = right.currentLineOverflowRecorded;
        left.extraLines += right.extraLines;
        left.overflowSizeProfile.Add(right.overflowSizeProfile);
        left.expansionDepthProfile.Add(right.expansionDepthProfile);
        // A child's expansion charge does not pay for its parent's own breaks.
        left.choices = choiceHistory_.Concat(left.choices, right.choices);
    }

    const NodeResult* FindMemoizedResult(int nodeId, int column, int indentLevel, bool lineHasText) const {
        for (
            const ResultMemoEntry* entry = memoHeads_[static_cast<size_t>(nodeId)];
            entry != nullptr;
            entry = entry->next
        ) {
            if (entry->column == column && entry->indentLevel == indentLevel && entry->lineHasText == lineHasText) {
                return &entry->result;
            }
        }
        return nullptr;
    }

    void StoreMemoizedResult(int nodeId, int column, int indentLevel, bool lineHasText, NodeResult result) {
        ResultMemoEntry*& head = memoHeads_[static_cast<size_t>(nodeId)];
        memoEntries_.push_back({
            .column = column,
            .indentLevel = indentLevel,
            .lineHasText = lineHasText,
            .result = std::move(result),
            .next = head,
        });
        head = &memoEntries_.back();
    }

    const NodeResults* FindMemoizedAlternatives(int nodeId, int column, int indentLevel, bool lineHasText) const {
        for (
            const AlternativesMemoEntry* entry = alternativesMemoHeads_[static_cast<size_t>(nodeId)];
            entry != nullptr;
            entry = entry->next
        ) {
            if (entry->column == column && entry->indentLevel == indentLevel && entry->lineHasText == lineHasText) {
                return &entry->results;
            }
        }
        return nullptr;
    }

    const NodeResults&
        StoreMemoizedAlternatives(int nodeId, int column, int indentLevel, bool lineHasText, NodeResults results)
    {
        AlternativesMemoEntry*& head = alternativesMemoHeads_[static_cast<size_t>(nodeId)];
        alternativesMemoEntries_.push_back({
            .column = column,
            .indentLevel = indentLevel,
            .lineHasText = lineHasText,
            .results = std::move(results),
            .next = head,
        });
        head = &alternativesMemoEntries_.back();
        return head->results;
    }

    const NodeResults& SolveAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        if (const NodeResults* found = FindMemoizedAlternatives(node.id, column, indentLevel, lineHasText)) {
            return *found;
        }
        return StoreMemoizedAlternatives(
            node.id, column, indentLevel, lineHasText, EnumerateAlternatives(node, column, indentLevel, lineHasText)
        );
    }

    NodeResults EnumerateAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        switch (node.kind) {
            case FormatBreakNodeKind::Token:
                return {SolveToken(node.token, column, indentLevel, lineHasText)};
            case FormatBreakNodeKind::Sequence:
                return SolveChildrenAlternatives(node.children, column, indentLevel, lineHasText);
            case FormatBreakNodeKind::Delimited: {
                if (std::optional<DelimiterStackView> stack = CollectDelimiterStack(node)) {
                    return SolveTransparentDelimiterStackAlternatives(node, *stack, column, indentLevel, lineHasText);
                }
                NodeResult split = SolveDelimitedSplit(node, column, indentLevel, lineHasText);
                NodeResults packed = SolveDelimitedPackedAlternatives(node, column, indentLevel, lineHasText);
                if (CanSkipDelimitedCompact(node, split, column, indentLevel, lineHasText)) {
                    packed.push_back(std::move(split));
                    return packed;
                }
                NodeResults alternatives;
                for (NodeResult compact : SolveDelimitedCompactAlternatives(node, column, indentLevel, lineHasText)) {
                    const CompactTailExpansionKind tailExpansion = DelimitedCompactTailExpansion(node, compact);
                    if (!node.forceSplit && !(compact.valid && compact.extraLines > 0 && (
                        (!lineHasText && tailExpansion != CompactTailExpansionKind::IntrinsicMultilineLiteral) ||
                        ContainsForceSplitAdjacentStrings(node) ||
                        tailExpansion == CompactTailExpansionKind::None
                    ))) {
                        alternatives.push_back(std::move(compact));
                    }
                }
                for (NodeResult& candidate : packed) {
                    alternatives.push_back(std::move(candidate));
                }
                if (split.valid) {
                    alternatives.push_back(std::move(split));
                }
                return alternatives;
            }
            case FormatBreakNodeKind::PrefixList:
                return SolvePrefixListAlternatives(node, column, indentLevel, lineHasText);
            case FormatBreakNodeKind::StatementSequence: {
                NodeResults alternatives;
                NodeResult compact = SolveStatementSequenceCompact(node, column, indentLevel, lineHasText);
                if (!node.forceSplit && !(compact.valid && compact.extraLines > 0)) {
                    alternatives.push_back(std::move(compact));
                }
                NodeResult split = SolveStatementSequenceSplit(node, column, indentLevel, lineHasText);
                if (split.valid) {
                    alternatives.push_back(std::move(split));
                }
                return alternatives;
            }
            case FormatBreakNodeKind::FunctionSignature:
                return SolveFunctionSignatureAlternatives(node, column, indentLevel, lineHasText);
            case FormatBreakNodeKind::BodyHeader:
                return SolveBodyHeaderAlternatives(node, column, indentLevel, lineHasText);
            case FormatBreakNodeKind::Chain: {
                if (node.forceSplit) {
                    if (node.chainKind == FormatBreakChainKind::StreamBeforeOperator) {
                        return SolveStreamSplitAlternatives(
                            node, column, indentLevel, lineHasText, FormatBreakChoice::Split
                        );
                    }
                    if (node.chainKind == FormatBreakChainKind::CallApplication) {
                        return {SolveCallApplicationSplit(node, column, indentLevel, lineHasText)};
                    }
                    if (node.chainKind == FormatBreakChainKind::MemberBeforeOperator) {
                        return {SolveChainSplitBeforeOperator(node, column, indentLevel, lineHasText)};
                    }
                    if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() > 2) {
                        return SolveTernaryChainSplitAlternatives(node, column, indentLevel, lineHasText);
                    }
                    if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() == 2) {
                        return SolveSingleTernaryAlternatives(
                            node, column, indentLevel, lineHasText, FormatBreakChoice::Split
                        );
                    }
                    return SolveChainSplitAfterOperatorAlternatives(node, column, indentLevel, lineHasText);
                }
                if (node.ternaryRequiresQuestionBreak || node.ternaryRequiresColonBreaks) {
                    if (node.operators.size() > 2) {
                        return SolveTernaryChainSplitAlternatives(node, column, indentLevel, lineHasText);
                    }
                    if (node.operators.size() == 2) {
                        const FormatBreakChoice choice = node.ternaryRequiresQuestionBreak &&
                            node.ternaryRequiresColonBreaks ? FormatBreakChoice::Split :
                            node.ternaryRequiresQuestionBreak ? FormatBreakChoice::TernaryBreakAfterQuestion :
                            FormatBreakChoice::TernaryBreakAfterColon;
                        return SolveSingleTernaryAlternatives(node, column, indentLevel, lineHasText, choice);
                    }
                }
                NodeResults alternatives;
                for (NodeResult compact : SolveChainCompactAlternatives(node, column, indentLevel, lineHasText)) {
                    if (node.chainCompactRequiresFitOnOneLine && (compact.extraLines > 0 || HasOverflow(compact))) {
                        continue;
                    }
                    if (node.chainPrefersSplitWhenCompactBreaks && compact.valid && compact.extraLines > 0) {
                        continue;
                    }
                    if (
                        !compact.valid ||
                        compact.extraLines == 0 ||
                        !RequiresChainCompactExtraLinesGuard(node) ||
                        CanKeepChainCompactWithExtraLines(node, compact)
                    ) {
                        alternatives.push_back(std::move(compact));
                    }
                }
                if (node.chainKind == FormatBreakChainKind::StreamBeforeOperator) {
                    for (NodeResult candidate : SolveStreamSplitAlternatives(
                        node, column, indentLevel, lineHasText, FormatBreakChoice::StreamCompactTail
                    )) {
                        alternatives.push_back(std::move(candidate));
                    }
                    for (NodeResult candidate : SolveStreamSplitAlternatives(
                        node, column, indentLevel, lineHasText, FormatBreakChoice::Split
                    )) {
                        alternatives.push_back(std::move(candidate));
                    }
                } else if (node.chainKind == FormatBreakChainKind::CallApplication) {
                    alternatives.push_back(SolveCallApplicationCompactTail(node, column, indentLevel, lineHasText));
                    alternatives.push_back(SolveCallApplicationSplit(node, column, indentLevel, lineHasText));
                } else if (node.chainKind == FormatBreakChainKind::MemberBeforeOperator) {
                    alternatives.push_back(SolveMemberCompactTail(node, column, indentLevel, lineHasText));
                    alternatives.push_back(SolveChainSplitBeforeOperator(node, column, indentLevel, lineHasText));
                } else if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() > 2) {
                    for (NodeResult candidate : SolveTernaryChainSplitAlternatives(
                        node, column, indentLevel, lineHasText
                    )) {
                        alternatives.push_back(std::move(candidate));
                    }
                } else if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() == 2) {
                    for (const FormatBreakChoice choice : {
                        FormatBreakChoice::TernaryBreakAfterQuestion,
                        FormatBreakChoice::TernaryBreakAfterColon,
                        FormatBreakChoice::Split,
                    }) {
                        for (NodeResult candidate : SolveSingleTernaryAlternatives(
                            node, column, indentLevel, lineHasText, choice
                        )) {
                            alternatives.push_back(std::move(candidate));
                        }
                    }
                } else {
                    for (NodeResult candidate : SolveChainSplitAfterOperatorAlternatives(
                        node, column, indentLevel, lineHasText
                    )) {
                        alternatives.push_back(std::move(candidate));
                    }
                }
                return alternatives;
            }
            case FormatBreakNodeKind::AdjacentStrings: {
                NodeResults alternatives;
                if (!node.forceSplit) {
                    alternatives.push_back(SolveAdjacentStringsCompact(node, column, indentLevel, lineHasText));
                }
                NodeResult split = SolveAdjacentStringsSplit(node, column, indentLevel, lineHasText);
                if (split.valid) {
                    alternatives.push_back(std::move(split));
                }
                return alternatives;
            }
        }
        return {};
    }

    NodeResult SolveChildren(std::span<FormatBreakNode* const> children, int column, int indentLevel, bool lineHasText)
    {
        NodeResult best;
        for (const NodeResult& candidate : SolveChildrenAlternatives(children, column, indentLevel, lineHasText)) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    NodeResults SolveChildrenAlternatives(
        std::span<FormatBreakNode* const> children, int column, int indentLevel, bool lineHasText
    ) {
        std::vector<const FormatBreakNode*> sequenceChildren;
        AppendSequenceChildren(children, sequenceChildren);
        NodeResults
            current{{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText}};
        for (const FormatBreakNode* child : sequenceChildren) {
            NodeResults next;
            for (const NodeResult& prefix : current) {
                if (child->kind == FormatBreakNodeKind::Token) {
                    // A token has exactly one layout, so memoized alternative enumeration cannot add a candidate.
                    NodeResult candidate = prefix;
                    Merge(
                        candidate,
                        SolveToken(child->token, prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText)
                    );
                    AddPrunedResult(next, std::move(candidate));
                    continue;
                }
                for (const NodeResult& childResult : SolveAlternatives(
                    *child, prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText
                )) {
                    if (!childResult.valid) {
                        continue;
                    }
                    NodeResult candidate = prefix;
                    Merge(candidate, childResult);
                    AddPrunedResult(next, std::move(candidate));
                }
            }
            SortPrunedResults(next);
            current = std::move(next);
        }
        return current;
    }

    static void
        AppendSequenceChildren(std::span<FormatBreakNode* const> children, std::vector<const FormatBreakNode*>& output)
    {
        for (const FormatBreakNode* child : children) {
            if (!child) {
                continue;
            }
            if (child->kind == FormatBreakNodeKind::Sequence) {
                AppendSequenceChildren(child->children, output);
                continue;
            }
            output.push_back(child);
        }
    }

    void AppendBreak(NodeResult& result, int indentLevel, int breakCost) const {
        // Some formats add text only when a line actually breaks. Account for that physical suffix here so it
        // participates in the same DP cost as ordinary tokens without inventing a printer-side break decision.
        FinishCurrentLine(result, breakLineSuffixWidth_);
        ++result.extraLines;
        result.endIndentLevel = indentLevel;
        result.endColumn = IndentColumn(indentLevel);
        result.endLineHasText = false;
        result.currentLineOverflowRecorded = false;
        if (!result.ownExpansionCharged) {
            result.expansionDepthProfile.AddValue(breakCost);
            result.ownExpansionCharged = true;
        }
    }

    NodeResult AddBreak(NodeResult result, int indentLevel, int breakCost) const {
        AppendBreak(result, indentLevel, breakCost);
        return result;
    }

    void AppendListBreak(NodeResult& result, int indentLevel, int breakCost, bool blankLine) const {
        AppendBreak(result, indentLevel, breakCost);
        if (blankLine) {
            AppendBreak(result, indentLevel, breakCost);
        }
    }

    void AppendListBreakAfterOptionalComment(
        NodeResult& result, int indentLevel, int breakCost, bool blankLine, bool commentTerminatedLine
    ) const {
        if (!commentTerminatedLine) {
            AppendListBreak(result, indentLevel, breakCost, blankLine);
            return;
        }
        result.endIndentLevel = indentLevel;
        result.endColumn = IndentColumn(indentLevel);
        result.endLineHasText = false;
        result.currentLineOverflowRecorded = false;
        if (blankLine) {
            AppendBreak(result, indentLevel, breakCost);
        }
    }

    void AppendToken(NodeResult& result, const FormatBreakToken& token) {
        NodeResult tokenResult = SolveToken(token, result.endColumn, result.endIndentLevel, result.endLineHasText);
        Merge(result, tokenResult);
    }

    static void AppendTrailingComma(NodeResult& result) {
        ++result.endColumn;
        result.endLineHasText = true;
    }

    NodeResult AddToken(NodeResult result, const FormatBreakToken& token) {
        AppendToken(result, token);
        return result;
    }

    NodeResults SolveListItemWithSuffixAlternatives(
        const FormatBreakListItem& listItem, int column, int indentLevel, bool lineHasText, bool trailingComma = false
    ) {
        if (listItem.node == nullptr) {
            return {};
        }
        NodeResults alternatives;
        for (NodeResult item : SolveAlternatives(*listItem.node, column, indentLevel, lineHasText)) {
            if (!item.valid) {
                continue;
            }
            if (FormatBreakTokenKind(listItem.separator) == PrintTokenKind::Known) {
                AppendToken(item, listItem.separator);
            }
            if (trailingComma) {
                AppendTrailingComma(item);
            }
            if (IsCommentToken(FormatBreakTokenKind(listItem.trailingComment))) {
                AppendToken(item, listItem.trailingComment);
            }
            AddPrunedResult(alternatives, std::move(item));
        }
        SortPrunedResults(alternatives);
        return alternatives;
    }

    NodeResult SolveListItemWithSuffix(
        const FormatBreakListItem& listItem, int column, int indentLevel, bool lineHasText, bool trailingComma = false
    ) {
        if (listItem.node != nullptr) {
            std::optional<NodeResult> compact =
                SolveCompactPhysicalLine(*listItem.node, column, indentLevel, lineHasText, true);
            if (compact) {
                if (FormatBreakTokenKind(listItem.separator) == PrintTokenKind::Known) {
                    AppendToken(*compact, listItem.separator);
                }
                if (trailingComma) {
                    AppendTrailingComma(*compact);
                }
                if (IsCommentToken(FormatBreakTokenKind(listItem.trailingComment))) {
                    AppendToken(*compact, listItem.trailingComment);
                }
                if (!HasOverflow(*compact)) {
                    // Including the suffix, this candidate has the minimum overflow profile, no expansion cost,
                    // and the fewest possible lines. Further alternatives can only add breaks, so none can win.
                    return *compact;
                }
            }
        }
        NodeResult best;
        for (const NodeResult& item : SolveListItemWithSuffixAlternatives(
            listItem, column, indentLevel, lineHasText, trailingComma
        )) {
            if (Better(item, best)) {
                best = item;
            }
        }
        return best;
    }

    NodeResult SolveNodeWithSuffix(
        const FormatBreakNode& node, const FormatBreakToken* suffix, int column, int indentLevel, bool lineHasText
    ) {
        NodeResult best;
        for (NodeResult candidate : SolveAlternatives(node, column, indentLevel, lineHasText)) {
            if (!candidate.valid) {
                continue;
            }
            if (suffix != nullptr && FormatBreakTokenKind(*suffix) == PrintTokenKind::Known) {
                AppendToken(candidate, *suffix);
            }
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    NodeResult SolveNodeWithoutBreaks(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        const std::optional<NodeResult> compact =
            SolveCompactPhysicalLine(node, column, indentLevel, lineHasText, false);
        if (compact) {
            // An all-compact rendering has no expansion cost and is therefore best among zero-line candidates.
            return *compact;
        }
        NodeResult best;
        for (NodeResult candidate : SolveAlternatives(node, column, indentLevel, lineHasText)) {
            if (candidate.valid && candidate.extraLines == 0 && Better(candidate, best)) {
                best = std::move(candidate);
            }
        }
        return best;
    }

    bool Better(const NodeResult& candidate, const NodeResult& incumbent) const {
        if (!candidate.valid) {
            return false;
        }
        if (!incumbent.valid) {
            return true;
        }
        const int overflowProfileComparison = CompareFormatValueProfilesWithAdditionalValues(
            candidate.overflowSizeProfile,
            CurrentLineOverflow(candidate),
            incumbent.overflowSizeProfile,
            CurrentLineOverflow(incumbent)
        );
        if (overflowProfileComparison != 0) {
            return overflowProfileComparison < 0;
        }
        const int depthProfileComparison =
            CompareFormatValueProfiles(candidate.expansionDepthProfile, incumbent.expansionDepthProfile);
        if (depthProfileComparison != 0) {
            return depthProfileComparison < 0;
        }
        if (candidate.extraLines != incumbent.extraLines) {
            return candidate.extraLines < incumbent.extraLines;
        }
        return false;
    }

    static bool SameResultState(const NodeResult& left, const NodeResult& right) {
        return left.endColumn == right.endColumn &&
            left.endIndentLevel == right.endIndentLevel &&
            left.endLineHasText == right.endLineHasText &&
            left.currentLineOverflowRecorded == right.currentLineOverflowRecorded &&
            left.ownExpansionCharged == right.ownExpansionCharged &&
            left.compactNextStreamOperand == right.compactNextStreamOperand;
    }

    bool DominatesResult(const NodeResult& left, const NodeResult& right) const {
        if (
            left.endIndentLevel != right.endIndentLevel ||
            left.endLineHasText != right.endLineHasText ||
            left.currentLineOverflowRecorded != right.currentLineOverflowRecorded ||
            left.ownExpansionCharged != right.ownExpansionCharged ||
            left.compactNextStreamOperand != right.compactNextStreamOperand
        ) {
            return false;
        }
        if (left.endColumn > right.endColumn) {
            return false;
        }
        const int overflowProfileComparison = CompareFormatValueProfilesWithAdditionalValues(
            left.overflowSizeProfile, CurrentLineOverflow(left), right.overflowSizeProfile, CurrentLineOverflow(right)
        );
        const int depthProfileComparison =
            CompareFormatValueProfiles(left.expansionDepthProfile, right.expansionDepthProfile);
        if (overflowProfileComparison > 0 || depthProfileComparison > 0 || left.extraLines > right.extraLines) {
            return false;
        }
        return overflowProfileComparison < 0 || depthProfileComparison < 0 || left.extraLines < right.extraLines;
    }

    static bool ResultStateLess(const NodeResult& left, const NodeResult& right) {
        if (left.endColumn != right.endColumn) {
            return left.endColumn < right.endColumn;
        }
        if (left.endIndentLevel != right.endIndentLevel) {
            return left.endIndentLevel < right.endIndentLevel;
        }
        if (left.endLineHasText != right.endLineHasText) {
            return left.endLineHasText < right.endLineHasText;
        }
        if (left.currentLineOverflowRecorded != right.currentLineOverflowRecorded) {
            return left.currentLineOverflowRecorded < right.currentLineOverflowRecorded;
        }
        if (left.ownExpansionCharged != right.ownExpansionCharged) {
            return left.ownExpansionCharged < right.ownExpansionCharged;
        }
        return left.compactNextStreamOperand < right.compactNextStreamOperand;
    }

    void AddPrunedResult(NodeResults& results, NodeResult candidate) const {
        if (!candidate.valid) {
            return;
        }
        for (auto it = results.begin(); it != results.end();) {
            if (SameResultState(*it, candidate)) {
                if (Better(candidate, *it)) {
                    *it = std::move(candidate);
                }
                return;
            }
            if (DominatesResult(*it, candidate)) {
                return;
            }
            if (DominatesResult(candidate, *it)) {
                it = results.erase(it);
                continue;
            }
            ++it;
        }
        results.push_back(std::move(candidate));
    }

    const DelimiterStackPartitionPath*
        ExtendDelimiterStackPartitionPath(const DelimiterStackPartitionPath* previous, size_t runStart)
    {
        delimiterStackPartitionPathArena_.push_back({.previous = previous, .runStart = runStart});
        return &delimiterStackPartitionPathArena_.back();
    }

    static std::vector<size_t> DelimiterStackRunStarts(const DelimiterStackPartitionPath* path) {
        std::vector<size_t> starts;
        for (const DelimiterStackPartitionPath* cursor = path; cursor != nullptr; cursor = cursor->previous) {
            starts.push_back(cursor->runStart);
        }
        std::reverse(starts.begin(), starts.end());
        return starts;
    }

    static bool PreferLaterDelimiterStackBreaks(
        const DelimiterStackPartitionPath* candidate, const DelimiterStackPartitionPath* incumbent
    ) {
        const std::vector<size_t> candidateStarts = DelimiterStackRunStarts(candidate);
        const std::vector<size_t> incumbentStarts = DelimiterStackRunStarts(incumbent);
        return std::lexicographical_compare(
            incumbentStarts.begin(), incumbentStarts.end(), candidateStarts.begin(), candidateStarts.end()
        );
    }

    void AddPrunedDelimiterStackPartitionCandidate(
        std::vector<DelimiterStackPartitionCandidate>& candidates, DelimiterStackPartitionCandidate candidate
    ) const {
        if (!candidate.result.valid) {
            return;
        }
        for (auto it = candidates.begin(); it != candidates.end();) {
            if (SameResultState(it->result, candidate.result)) {
                if (
                    Better(candidate.result, it->result) ||
                    (!Better(it->result, candidate.result) && PreferLaterDelimiterStackBreaks(candidate.path, it->path))
                ) {
                    *it = std::move(candidate);
                }
                return;
            }
            if (DominatesResult(it->result, candidate.result)) {
                return;
            }
            if (DominatesResult(candidate.result, it->result)) {
                it = candidates.erase(it);
                continue;
            }
            ++it;
        }
        candidates.push_back(std::move(candidate));
    }

    static void SortPrunedResults(NodeResults& results) { std::sort(results.begin(), results.end(), ResultStateLess); }

    bool CompactLineEndsOverLimit(const NodeResult& compact) const {
        return compact.endLineHasText && compact.endColumn > config_.columnLimit;
    }

    std::optional<NodeResult>
        SolveCompactOneLine(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) const
    {
        return SolveCompactPhysicalLine(node, column, indentLevel, lineHasText, true);
    }

    std::optional<NodeResult> SolveCompactPhysicalLine(
        const FormatBreakNode& node, int column, int indentLevel, bool lineHasText, bool requireFit
    ) const {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        if (!AppendCompactOneLine(node, result, requireFit)) {
            return std::nullopt;
        }
        if (requireFit && result.endLineHasText && result.endColumn > config_.columnLimit) {
            return std::nullopt;
        }
        return result;
    }

    bool AddCompactToken(NodeResult& result, const FormatBreakToken& token, bool requireFit) const {
        return AddCompactTokenText(result, token, FormatTokenText(FormatBreakTokenValue(token)), requireFit);
    }

    bool AddCompactTokenText(
        NodeResult& result, const FormatBreakToken& token, std::string_view text, bool requireFit
    ) const {
        if (token.contextOnly) {
            return true;
        }
        const FormatCompactLine shape = FormatCompactLayout::MeasureToken(token, text);
        if (!shape.valid) {
            return false;
        }
        result.endColumn += result.endLineHasText ? shape.widthWithLeadingText : shape.widthWithoutLeadingText;
        result.endLineHasText = result.endLineHasText || shape.producesText;
        return !requireFit || !result.endLineHasText || result.endColumn <= config_.columnLimit;
    }

    bool AppendCompactListItems(const FormatBreakNode& node, NodeResult& result, bool requireFit) const {
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& item = node.items[index];
            if (item.node != nullptr && !AppendCompactOneLine(*item.node, result, requireFit)) {
                return false;
            }
            if (
                FormatBreakTokenKind(item.separator) == PrintTokenKind::Known &&
                !AddCompactToken(result, item.separator, requireFit)
            ) {
                return false;
            }
            if (FormatBreakHasTrailingComment(node, index)) {
                return false;
            }
        }
        return true;
    }

    bool AppendCompactOneLine(const FormatBreakNode& node, NodeResult& result, bool requireFit) const {
        const FormatCompactLine& shape = compactLayout_.Measure(node);
        if (!shape.valid) {
            return false;
        }
        result.endColumn += result.endLineHasText ? shape.widthWithLeadingText : shape.widthWithoutLeadingText;
        result.endLineHasText = result.endLineHasText || shape.producesText;
        return !requireFit || !result.endLineHasText || result.endColumn <= config_.columnLimit;
    }

    bool DelimitedInlinePrefixRequiresOverflowOrBreak(const FormatBreakNode& node, NodeResult prefix) const {
        for (size_t index = 0; index + 1 < node.items.size(); ++index) {
            const FormatBreakListItem& item = node.items[index];
            if (item.node == nullptr || !AppendCompactOneLine(*item.node, prefix, true)) {
                return true;
            }
            if (
                FormatBreakTokenKind(item.separator) == PrintTokenKind::Known &&
                !AddCompactToken(prefix, item.separator, true)
            ) {
                return true;
            }
            if (FormatBreakHasTrailingComment(node, index)) {
                return true;
            }
        }
        return false;
    }

    bool CanSkipDelimitedCompact(
        const FormatBreakNode& node, const NodeResult& split, int column, int indentLevel, bool lineHasText
    ) const {
        if (!split.valid) {
            return false;
        }
        if (node.forceSplit) {
            return true;
        }
        if (HasOverflow(split) || node.children.empty() || node.items.size() < 2) {
            return false;
        }
        NodeResult
            prefix{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        return !AppendCompactOneLine(*node.children.front(), prefix, true) ||
            DelimitedInlinePrefixRequiresOverflowOrBreak(node, prefix);
    }

    static bool HasRealSeparators(const FormatBreakNode& node) {
        return std::any_of(node.items.begin(), node.items.end(), [](const FormatBreakListItem& item) {
            return FormatBreakTokenKind(item.separator) == PrintTokenKind::Known;
        });
    }

    static bool IsTransparentSingleItemDelimiter(const FormatBreakNode& node) {
        if (
            node.forceSplit ||
            node.delimiterKind != FormatBreakDelimiterKind::Paren ||
            node.children.size() < 2 ||
            node.items.size() != 1 ||
            HasRealSeparators(node) ||
            FormatBreakHasTrailingComment(node, 0) ||
            HasBlankLineBeforeItem(node, 0) ||
            node.items.front().node == nullptr
        ) {
            return false;
        }
        const FormatBreakNode* open = node.children[0];
        if (open == nullptr || open->kind != FormatBreakNodeKind::Token) {
            return false;
        }
        const PrintToken& token = FormatBreakTokenValue(open->token);
        return !SyntaxNodeKindHasClass(token.parentKind, SyntaxNodeClass::SemanticDelimitedParent) &&
            !SyntaxNodeKindHasClass(token.grandParentKind, SyntaxNodeClass::SemanticDelimitedParent);
    }

    static const FormatBreakNode* SingleChildSequenceNode(const FormatBreakNode& node) {
        if (node.kind != FormatBreakNodeKind::Sequence || node.children.size() != 1) {
            return nullptr;
        }
        return node.children.front();
    }

    static const FormatBreakNode* TransparentStackChild(const FormatBreakNode& node) {
        if (!IsTransparentSingleItemDelimiter(node)) {
            return nullptr;
        }
        const FormatBreakNode* item = node.items.front().node;
        while (item != nullptr) {
            if (IsTransparentSingleItemDelimiter(*item)) {
                return item;
            }
            item = SingleChildSequenceNode(*item);
        }
        return nullptr;
    }

    static bool IsDelimiterStackItem(const FormatBreakNode& node) {
        return IsTransparentSingleItemDelimiter(node) && TransparentStackChild(node) != nullptr;
    }

    static std::optional<DelimiterStackView> CollectDelimiterStack(const FormatBreakNode& node) {
        if (!IsDelimiterStackItem(node)) {
            return std::nullopt;
        }
        DelimiterStackView stack;
        const FormatBreakNode* current = &node;
        while (current != nullptr) {
            stack.delimiters.push_back(current);
            const FormatBreakNode* child = TransparentStackChild(*current);
            if (child == nullptr) {
                stack.leaf = current->items.front().node;
                break;
            }
            current = child;
        }
        return stack.leaf != nullptr && stack.delimiters.size() > 1 ? std::optional(stack) : std::nullopt;
    }

    NodeResults SolveDelimitedInlineItems(const FormatBreakNode& node, const NodeResult& result) {
        NodeResults current{result};
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& listItem = node.items[index];
            NodeResults nextByState;
            const bool canKeepMultilineItem = node.items.size() == 1 ||
                (index + 1 == node.items.size() && node.delimiterKind != FormatBreakDelimiterKind::Angle);
            for (const NodeResult& prefix : current) {
                if (listItem.node->kind == FormatBreakNodeKind::Token) {
                    // A token has exactly one layout, so memoized alternative enumeration cannot add a candidate.
                    NodeResult next = prefix;
                    Merge(
                        next,
                        SolveToken(listItem.node->token, prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText)
                    );
                    if (FormatBreakTokenKind(listItem.separator) == PrintTokenKind::Known) {
                        AppendToken(next, listItem.separator);
                    }
                    if (FormatBreakHasTrailingComment(node, index)) {
                        AppendToken(next, listItem.trailingComment);
                    }
                    AddPrunedResult(nextByState, std::move(next));
                    continue;
                }
                if (!canKeepMultilineItem) {
                    const std::optional<NodeResult> item = SolveCompactPhysicalLine(
                        *listItem.node, prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText, false
                    );
                    if (item) {
                        // Every noncompact layout of an otherwise compactable item adds a physical line, so this
                        // is the only candidate that can satisfy the caller's single-line requirement.
                        NodeResult next = prefix;
                        Merge(next, *item);
                        if (FormatBreakTokenKind(listItem.separator) == PrintTokenKind::Known) {
                            AppendToken(next, listItem.separator);
                        }
                        if (FormatBreakHasTrailingComment(node, index)) {
                            AppendToken(next, listItem.trailingComment);
                        }
                        AddPrunedResult(nextByState, std::move(next));
                        continue;
                    }
                }
                for (const NodeResult& item : SolveAlternatives(
                    *listItem.node, prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText
                )) {
                    if (!item.valid) {
                        continue;
                    }
                    if (node.compactRequiresUnbrokenItems && HasSelectedBreak(*listItem.node, item)) {
                        continue;
                    }
                    if (!canKeepMultilineItem && item.extraLines > 0) {
                        continue;
                    }
                    NodeResult next = prefix;
                    Merge(next, item);
                    if (FormatBreakTokenKind(listItem.separator) == PrintTokenKind::Known) {
                        AppendToken(next, listItem.separator);
                    }
                    if (FormatBreakHasTrailingComment(node, index)) {
                        AppendToken(next, listItem.trailingComment);
                    }
                    AddPrunedResult(nextByState, std::move(next));
                }
            }
            SortPrunedResults(nextByState);
            current = std::move(nextByState);
        }
        return current;
    }

    NodeResults
        SolveDelimitedCompactAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Compact, indentLevel);
        AppendToken(result, node.children[0]->token);
        if (FormatBreakHasLeadingTrailingComment(node)) {
            AppendToken(result, node.leadingTrailingComment);
        }

        NodeResults alternatives;
        for (const NodeResult& candidate : SolveDelimitedInlineItems(node, result)) {
            AddPrunedResult(alternatives, AddToken(candidate, node.children[1]->token));
        }
        SortPrunedResults(alternatives);
        return alternatives;
    }

    NodeResults
        SolveDelimitedPackedAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        if (
            node.forceSplit ||
            node.items.size() < 2 ||
            !std::all_of(node.items.begin(), node.items.end() - 1, [](const FormatBreakListItem& item) {
                return FormatBreakTokenSyntaxKind(item.separator) == SyntaxNodeKind::Comma;
            })
        ) {
            return {};
        }
        NodeResult
            prefix{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(prefix, node.id, FormatBreakChoice::SplitPacked, indentLevel);
        AppendToken(prefix, node.children.front()->token);
        AppendBreak(prefix, indentLevel + 1, node.breakCost);
        const NodeResult start{
            .valid = true,
            .endColumn = prefix.endColumn,
            .endIndentLevel = prefix.endIndentLevel,
            .endLineHasText = false,
            .ownExpansionCharged = prefix.ownExpansionCharged,
        };
        if (DelimitedInlinePrefixRequiresOverflowOrBreak(node, start)) {
            return {};
        }
        NodeResults alternatives;
        for (const NodeResult& body : SolveDelimitedInlineItems(node, start)) {
            if (!body.valid || (
                body.extraLines > 0 &&
                (ContainsForceSplitAdjacentStrings(node) || !CanKeepDelimitedCompactWithExtraLines(node, body))
            )) {
                continue;
            }
            const NodeResult closedBody = AddBreak(body, indentLevel, node.breakCost);
            if (HasOverflow(closedBody)) {
                continue;
            }
            NodeResult candidate = prefix;
            Merge(candidate, closedBody);
            AppendToken(candidate, node.children.back()->token);
            AddPrunedResult(alternatives, std::move(candidate));
        }
        SortPrunedResults(alternatives);
        return alternatives;
    }

    NodeResult SolveDelimitedExpanded(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult best = SolveDelimitedSplit(node, column, indentLevel, lineHasText);
        for (const NodeResult& packed : SolveDelimitedPackedAlternatives(node, column, indentLevel, lineHasText)) {
            if (Better(packed, best)) {
                best = packed;
            }
        }
        return best;
    }

    NodeResult SolveDelimitedSplit(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        if (node.items.empty()) {
            return {};
        }
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, indentLevel);
        AppendToken(result, node.children[0]->token);
        if (FormatBreakHasLeadingTrailingComment(node)) {
            AppendToken(result, node.leadingTrailingComment);
        }
        AppendListBreakAfterOptionalComment(
            result,
            indentLevel + 1,
            node.breakCost,
            HasBlankLineBeforeItem(node, 0),
            FormatBreakHasLeadingTrailingComment(node)
        );
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& listItem = node.items[index];
            NodeResult item = SolveListItemWithSuffix(
                listItem,
                result.endColumn,
                result.endIndentLevel,
                result.endLineHasText,
                node.splitTrailingCommaItem == index
            );
            Merge(result, item);
            const bool hasNextItem = index + 1 < node.items.size();
            AppendListBreakAfterOptionalComment(
                result,
                hasNextItem ? indentLevel + 1 : indentLevel,
                node.breakCost,
                hasNextItem && HasBlankLineBeforeItem(node, index + 1),
                FormatBreakHasTrailingComment(node, index)
            );
        }
        AppendToken(result, node.children[1]->token);
        return result;
    }

    NodeResult SolveTransparentDelimiterStackSuffix(
        const DelimiterStackView& stack, size_t firstDelimiter, int column, int indentLevel, bool lineHasText
    ) {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        for (size_t index = firstDelimiter; index < stack.delimiters.size(); ++index) {
            const FormatBreakNode* delimiter = stack.delimiters[index];
            AddChoice(result, delimiter->id, FormatBreakChoice::Compact, indentLevel);
            AppendToken(result, delimiter->children.front()->token);
        }
        NodeResult leaf = Solve(*stack.leaf, result.endColumn, result.endIndentLevel, result.endLineHasText);
        if (!leaf.valid) {
            return {};
        }
        Merge(result, leaf);
        for (size_t index = stack.delimiters.size(); index-- > firstDelimiter;) {
            AppendToken(result, stack.delimiters[index]->children.back()->token);
        }
        return result;
    }

    NodeResult SolveTransparentDelimiterStackSplit(
        const FormatBreakNode& node, const DelimiterStackView& stack, int column, int indentLevel, bool lineHasText
    ) {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, indentLevel);
        AppendToken(result, node.children.front()->token);
        AppendListBreak(result, indentLevel + 1, node.breakCost, HasBlankLineBeforeItem(node, 0));
        NodeResult suffix = SolveTransparentDelimiterStackSuffix(
            stack, 1, result.endColumn, result.endIndentLevel, result.endLineHasText
        );
        if (!suffix.valid) {
            return {};
        }
        if (suffix.extraLines > 0) {
            return {};
        }
        Merge(result, suffix);
        AppendListBreak(result, indentLevel, node.breakCost, false);
        AppendToken(result, node.children.back()->token);
        return result;
    }

    NodeResults SolveTransparentDelimiterStackAlternatives(
        const FormatBreakNode& node, const DelimiterStackView& stack, int column, int indentLevel, bool lineHasText
    ) {
        NodeResults alternatives;
        NodeResult compact = SolveTransparentDelimiterStackSuffix(stack, 0, column, indentLevel, lineHasText);
        if (!node.forceSplit && compact.valid && (
            compact.extraLines == 0 ||
            CompactTailExpansion(node, compact) == CompactTailExpansionKind::IntrinsicMultilineLiteral
        )) {
            alternatives.push_back(std::move(compact));
        }
        NodeResult attachedLeaf = SolveDelimiterStack(node, stack, column, indentLevel, lineHasText, false);
        if (attachedLeaf.valid) {
            alternatives.push_back(std::move(attachedLeaf));
        }
        NodeResult detachedLeaf = SolveDelimiterStack(node, stack, column, indentLevel, lineHasText, true);
        if (detachedLeaf.valid) {
            alternatives.push_back(std::move(detachedLeaf));
        }
        return alternatives;
    }

    NodeResult SolveDelimited(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult best;
        for (const NodeResult& candidate : SolveAlternatives(node, column, indentLevel, lineHasText)) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    int TokenColumnAdvance(const FormatBreakToken& token, bool lineHasText) const {
        return SpaceBeforeToken(token, lineHasText) + FormatTokenWidth(FormatBreakTokenValue(token));
    }

    bool TokenWouldOverflow(const NodeResult& result, const FormatBreakToken& token) const {
        return result.endLineHasText &&
            result.endColumn <= config_.columnLimit &&
            result.endColumn + TokenColumnAdvance(token, result.endLineHasText) > config_.columnLimit;
    }

    void AddDelimiterStackPartitionChoices(
        NodeResult& result, const DelimiterStackView& stack, const DelimiterStackPartitionPath* path
    ) {
        for (const size_t runStart : DelimiterStackRunStarts(path)) {
            if (runStart >= stack.delimiters.size()) {
                continue;
            }
            const FormatBreakNode* open = stack.delimiters[runStart]->children.front();
            AddChoice(result, open->id, FormatBreakChoice::SplitDelimiterStackRun);
        }
    }

    NodeResult SolveGreedyDelimiterStack(
        const FormatBreakNode& node,
        const DelimiterStackView& stack,
        int column,
        int indentLevel,
        bool lineHasText,
        bool detachLeaf
    ) {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(
            result,
            node.id,
            detachLeaf ? FormatBreakChoice::SplitDelimiterStackDetachedLeaf : FormatBreakChoice::SplitDelimiterStack,
            indentLevel
        );

        int currentLineIndent = indentLevel;
        int nextOpenIndent = indentLevel + 1;
        std::vector<DelimiterStackRun> delimiterRuns;
        delimiterRuns.reserve(stack.delimiters.size());
        for (size_t index = 0; index < stack.delimiters.size(); ++index) {
            const FormatBreakNode* delimiter = stack.delimiters[index];
            const FormatBreakToken& open = delimiter->children.front()->token;
            if (TokenWouldOverflow(result, open)) {
                currentLineIndent = nextOpenIndent;
                AppendBreak(result, currentLineIndent, node.breakCost);
                ++nextOpenIndent;
                AddChoice(result, delimiter->children.front()->id, FormatBreakChoice::SplitDelimiterStackRun);
            }
            if (delimiterRuns.empty() || delimiterRuns.back().indentLevel != currentLineIndent) {
                delimiterRuns
                    .push_back(DelimiterStackRun{.begin = index, .end = index, .indentLevel = currentLineIndent});
            }
            delimiterRuns.back().end = index + 1;
            AppendToken(result, open);
        }

        if (detachLeaf && result.endLineHasText) {
            AppendBreak(result, nextOpenIndent, node.breakCost);
        }
        NodeResult leaf = detachLeaf ?
            Solve(*stack.leaf, result.endColumn, result.endIndentLevel, result.endLineHasText) :
            SolveNodeWithoutBreaks(*stack.leaf, result.endColumn, result.endIndentLevel, result.endLineHasText);
        if (!leaf.valid) {
            return {};
        }
        Merge(result, leaf);

        for (size_t runIndex = delimiterRuns.size(); runIndex-- > 0;) {
            const DelimiterStackRun& run = delimiterRuns[runIndex];
            const bool firstClosingRun = runIndex + 1 == delimiterRuns.size();
            if (result.endLineHasText && (detachLeaf || !firstClosingRun)) {
                AppendBreak(result, run.indentLevel, node.breakCost);
            }
            for (size_t index = run.end; index-- > run.begin;) {
                AppendToken(result, stack.delimiters[index]->children.back()->token);
            }
        }
        return result;
    }

    NodeResult SolveDelimiterStackPartition(
        const FormatBreakNode& node,
        const DelimiterStackView& stack,
        int column,
        int indentLevel,
        bool lineHasText,
        bool detachLeaf,
        std::span<const size_t> runStarts
    ) {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(
            result,
            node.id,
            detachLeaf ? FormatBreakChoice::SplitDelimiterStackDetachedLeaf : FormatBreakChoice::SplitDelimiterStack,
            indentLevel
        );

        int currentLineIndent = indentLevel;
        int nextOpenIndent = indentLevel + 1;
        size_t nextRunStart = 0;
        std::vector<DelimiterStackRun> delimiterRuns;
        delimiterRuns.reserve(runStarts.size() + 1);
        for (size_t index = 0; index < stack.delimiters.size(); ++index) {
            const FormatBreakNode* delimiter = stack.delimiters[index];
            if (nextRunStart < runStarts.size() && runStarts[nextRunStart] == index) {
                currentLineIndent = nextOpenIndent;
                AppendBreak(result, currentLineIndent, node.breakCost);
                ++nextOpenIndent;
                ++nextRunStart;
                AddChoice(result, delimiter->children.front()->id, FormatBreakChoice::SplitDelimiterStackRun);
            }
            if (delimiterRuns.empty() || delimiterRuns.back().indentLevel != currentLineIndent) {
                delimiterRuns
                    .push_back(DelimiterStackRun{.begin = index, .end = index, .indentLevel = currentLineIndent});
            }
            delimiterRuns.back().end = index + 1;
            AppendToken(result, delimiter->children.front()->token);
        }
        if (nextRunStart != runStarts.size()) {
            return {};
        }

        if (detachLeaf && result.endLineHasText) {
            AppendBreak(result, nextOpenIndent, node.breakCost);
        }
        NodeResult leaf = detachLeaf ?
            Solve(*stack.leaf, result.endColumn, result.endIndentLevel, result.endLineHasText) :
            SolveNodeWithoutBreaks(*stack.leaf, result.endColumn, result.endIndentLevel, result.endLineHasText);
        if (!leaf.valid) {
            return {};
        }
        Merge(result, leaf);

        for (size_t runIndex = delimiterRuns.size(); runIndex-- > 0;) {
            const DelimiterStackRun& run = delimiterRuns[runIndex];
            const bool firstClosingRun = runIndex + 1 == delimiterRuns.size();
            if (result.endLineHasText && (detachLeaf || !firstClosingRun)) {
                AppendBreak(result, run.indentLevel, node.breakCost);
            }
            for (size_t index = run.end; index-- > run.begin;) {
                AppendToken(result, stack.delimiters[index]->children.back()->token);
            }
        }
        return result;
    }

    NodeResult SolveZeroOverflowAttachedDelimiterStack(
        const FormatBreakNode& node, const DelimiterStackView& stack, int column, int indentLevel, bool lineHasText
    ) {
        NodeResult best;
        std::vector<size_t> bestRunStarts;
        const int lastInitialBreak = lineHasText ? 1 : 0;
        for (int breakBeforeFirst = 0; breakBeforeFirst <= lastInitialBreak; ++breakBeforeFirst) {
            for (size_t terminalBegin = 0; terminalBegin < stack.delimiters.size(); ++terminalBegin) {
                NodeResult prefix{
                    .valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText
                };
                int nextOpenIndent = indentLevel + 1;
                std::vector<size_t> runStarts;
                if (breakBeforeFirst != 0) {
                    AppendBreak(prefix, nextOpenIndent, node.breakCost);
                    ++nextOpenIndent;
                    runStarts.push_back(0);
                }
                bool prefixFits = true;
                for (size_t index = 0; index < terminalBegin; ++index) {
                    const FormatBreakToken& open = stack.delimiters[index]->children.front()->token;
                    if (TokenWouldOverflow(prefix, open)) {
                        AppendBreak(prefix, nextOpenIndent, node.breakCost);
                        ++nextOpenIndent;
                        runStarts.push_back(index);
                    }
                    AppendToken(prefix, open);
                    if (HasOverflow(prefix)) {
                        prefixFits = false;
                        break;
                    }
                }
                if (!prefixFits) {
                    continue;
                }
                if (terminalBegin > 0) {
                    runStarts.push_back(terminalBegin);
                }
                NodeResult candidate =
                    SolveDelimiterStackPartition(node, stack, column, indentLevel, lineHasText, false, runStarts);
                if (!candidate.valid || HasOverflow(candidate)) {
                    continue;
                }
                const bool equalCost = !Better(candidate, best) && !Better(best, candidate);
                if (Better(candidate, best) || (equalCost && std::lexicographical_compare(
                    bestRunStarts.begin(), bestRunStarts.end(), runStarts.begin(), runStarts.end()
                ))) {
                    best = candidate;
                    bestRunStarts = std::move(runStarts);
                }
            }
        }
        return best;
    }

    NodeResult SolveExactDelimiterStackWithInitialBreak(
        const FormatBreakNode& node,
        const DelimiterStackView& stack,
        int column,
        int indentLevel,
        bool lineHasText,
        bool detachLeaf,
        bool breakBeforeFirst,
        int maximumOverflow
    ) {
        const size_t delimiterCount = stack.delimiters.size();
        const int firstRunIndent = indentLevel + (breakBeforeFirst ? 1 : 0);
        const int maximumLineColumn = config_.columnLimit + maximumOverflow;
        const int maximumRunIndent = (maximumLineColumn - 1) / std::max(1, indentWidth_);
        if (firstRunIndent > maximumRunIndent) {
            return {};
        }
        const size_t maximumRunCount =
            std::min(delimiterCount, static_cast<size_t>(maximumRunIndent - firstRunIndent + 1));
        using PartitionCandidates = std::vector<DelimiterStackPartitionCandidate>;
        std::vector<PartitionCandidates> states((maximumRunCount + 1) * (delimiterCount + 1));
        const auto state = [&](size_t runCount, size_t delimiterIndex) -> PartitionCandidates& {
            return states[runCount * (delimiterCount + 1) + delimiterIndex];
        };

        NodeResult
            initial{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        const DelimiterStackPartitionPath* initialPath = nullptr;
        if (breakBeforeFirst) {
            AppendBreak(initial, firstRunIndent, node.breakCost);
            initialPath = ExtendDelimiterStackPartitionPath(nullptr, 0);
        }
        state(0, 0).push_back({.result = initial, .path = initialPath});

        for (size_t completedRuns = 0; completedRuns < maximumRunCount; ++completedRuns) {
            for (size_t begin = 0; begin < delimiterCount; ++begin) {
                const PartitionCandidates& prefixes = state(completedRuns, begin);
                if (prefixes.empty()) {
                    continue;
                }
                const int runIndent = firstRunIndent + static_cast<int>(completedRuns);
                for (const DelimiterStackPartitionCandidate& prefix : prefixes) {
                    const DelimiterStackPartitionPath* path = prefix.path;
                    if (completedRuns > 0) {
                        path = ExtendDelimiterStackPartitionPath(path, begin);
                    }
                    for (size_t end = begin + 1; end <= delimiterCount; ++end) {
                        NodeResult candidate = prefix.result;
                        const int outerEndColumn = candidate.endColumn;
                        const int outerEndIndentLevel = candidate.endIndentLevel;
                        const bool outerEndLineHasText = candidate.endLineHasText;
                        if (completedRuns > 0) {
                            AppendBreak(candidate, runIndent, node.breakCost);
                        }
                        for (size_t index = begin; index < end; ++index) {
                            AppendToken(candidate, stack.delimiters[index]->children.front()->token);
                        }
                        if (MaximumOverflow(candidate) > maximumOverflow) {
                            break;
                        }
                        AppendBreak(candidate, runIndent, node.breakCost);
                        for (size_t index = end; index-- > begin;) {
                            AppendToken(candidate, stack.delimiters[index]->children.back()->token);
                        }
                        if (MaximumOverflow(candidate) > maximumOverflow) {
                            break;
                        }
                        if (completedRuns > 0) {
                            FinishCurrentLine(candidate);
                            candidate.endColumn = outerEndColumn;
                            candidate.endIndentLevel = outerEndIndentLevel;
                            candidate.endLineHasText = outerEndLineHasText;
                            candidate.currentLineOverflowRecorded = outerEndLineHasText;
                        }
                        AddPrunedDelimiterStackPartitionCandidate(
                            state(completedRuns + 1, end), {.result = candidate, .path = path}
                        );
                    }
                }
            }
        }

        DelimiterStackPartitionCandidate best;
        const auto consider = [&](NodeResult candidate, const DelimiterStackPartitionPath* path) {
            if (!candidate.valid) {
                return;
            }
            if (
                Better(candidate, best.result) ||
                (!Better(best.result, candidate) && PreferLaterDelimiterStackBreaks(path, best.path))
            ) {
                best = {.result = candidate, .path = path};
            }
        };

        if (detachLeaf) {
            for (size_t runCount = 1; runCount <= maximumRunCount; ++runCount) {
                const int leafIndent = firstRunIndent + static_cast<int>(runCount);
                for (const DelimiterStackPartitionCandidate& partition : state(runCount, delimiterCount)) {
                    NodeResult candidate = partition.result;
                    const int outerEndColumn = candidate.endColumn;
                    const int outerEndIndentLevel = candidate.endIndentLevel;
                    const bool outerEndLineHasText = candidate.endLineHasText;
                    AppendBreak(candidate, leafIndent, node.breakCost);
                    NodeResult leaf =
                        Solve(*stack.leaf, candidate.endColumn, candidate.endIndentLevel, candidate.endLineHasText);
                    if (!leaf.valid) {
                        continue;
                    }
                    Merge(candidate, leaf);
                    if (MaximumOverflow(candidate) > maximumOverflow) {
                        continue;
                    }
                    FinishCurrentLine(candidate);
                    candidate.endColumn = outerEndColumn;
                    candidate.endIndentLevel = outerEndIndentLevel;
                    candidate.endLineHasText = outerEndLineHasText;
                    candidate.currentLineOverflowRecorded = outerEndLineHasText;
                    consider(candidate, partition.path);
                }
            }
        } else {
            for (size_t completedRuns = 0; completedRuns < maximumRunCount; ++completedRuns) {
                for (size_t begin = 0; begin < delimiterCount; ++begin) {
                    const int runIndent = firstRunIndent + static_cast<int>(completedRuns);
                    for (const DelimiterStackPartitionCandidate& prefix : state(completedRuns, begin)) {
                        NodeResult candidate = prefix.result;
                        const int outerEndColumn = candidate.endColumn;
                        const int outerEndIndentLevel = candidate.endIndentLevel;
                        const bool outerEndLineHasText = candidate.endLineHasText;
                        const DelimiterStackPartitionPath* path = prefix.path;
                        if (completedRuns > 0) {
                            AppendBreak(candidate, runIndent, node.breakCost);
                            path = ExtendDelimiterStackPartitionPath(path, begin);
                        }
                        for (size_t index = begin; index < delimiterCount; ++index) {
                            AppendToken(candidate, stack.delimiters[index]->children.front()->token);
                        }
                        if (MaximumOverflow(candidate) > maximumOverflow) {
                            continue;
                        }
                        NodeResult leaf = SolveNodeWithoutBreaks(
                            *stack.leaf, candidate.endColumn, candidate.endIndentLevel, candidate.endLineHasText
                        );
                        if (!leaf.valid) {
                            continue;
                        }
                        Merge(candidate, leaf);
                        for (size_t index = delimiterCount; index-- > begin;) {
                            AppendToken(candidate, stack.delimiters[index]->children.back()->token);
                        }
                        if (MaximumOverflow(candidate) > maximumOverflow) {
                            continue;
                        }
                        if (completedRuns > 0) {
                            FinishCurrentLine(candidate);
                            candidate.endColumn = outerEndColumn;
                            candidate.endIndentLevel = outerEndIndentLevel;
                            candidate.endLineHasText = outerEndLineHasText;
                            candidate.currentLineOverflowRecorded = outerEndLineHasText;
                        }
                        consider(candidate, path);
                    }
                }
            }
        }

        if (!best.result.valid) {
            return {};
        }
        AddChoice(
            best.result,
            node.id,
            detachLeaf ? FormatBreakChoice::SplitDelimiterStackDetachedLeaf : FormatBreakChoice::SplitDelimiterStack,
            indentLevel
        );
        AddDelimiterStackPartitionChoices(best.result, stack, best.path);
        return best.result;
    }

    NodeResult SolveExactDelimiterStack(
        const FormatBreakNode& node,
        const DelimiterStackView& stack,
        int column,
        int indentLevel,
        bool lineHasText,
        bool detachLeaf,
        int maximumOverflow
    ) {
        if (!detachLeaf && maximumOverflow == 0) {
            return SolveZeroOverflowAttachedDelimiterStack(node, stack, column, indentLevel, lineHasText);
        }
        NodeResult best = SolveExactDelimiterStackWithInitialBreak(
            node, stack, column, indentLevel, lineHasText, detachLeaf, false, maximumOverflow
        );
        if (lineHasText) {
            NodeResult breakBeforeFirst = SolveExactDelimiterStackWithInitialBreak(
                node, stack, column, indentLevel, lineHasText, detachLeaf, true, maximumOverflow
            );
            if (Better(breakBeforeFirst, best)) {
                best = breakBeforeFirst;
            }
        }
        return best;
    }

    NodeResult SolveDelimiterStack(
        const FormatBreakNode& node,
        const DelimiterStackView& stack,
        int column,
        int indentLevel,
        bool lineHasText,
        bool detachLeaf
    ) {
        NodeResult greedy = SolveGreedyDelimiterStack(node, stack, column, indentLevel, lineHasText, detachLeaf);
        // docs/break_solver.md owns the zero-overflow greedy proof.
        if (!greedy.valid || !HasOverflow(greedy)) {
            return greedy;
        }
        int exactMaximumOverflow = MaximumOverflow(greedy);
        if (!detachLeaf) {
            const NodeResult detached = SolveGreedyDelimiterStack(node, stack, column, indentLevel, lineHasText, true);
            if (detached.valid) {
                exactMaximumOverflow = std::min(exactMaximumOverflow, MaximumOverflow(detached));
            }
        }
        NodeResult exact =
            SolveExactDelimiterStack(node, stack, column, indentLevel, lineHasText, detachLeaf, exactMaximumOverflow);
        return Better(exact, greedy) ? exact : greedy;
    }

    static FormatBreakChoice ChoiceFor(const NodeResult& result, const FormatBreakNode& node) {
        const std::optional<FormatBreakChoice> choice = FormatChoiceHistory::Find(result.choices, node.id);
        return choice.value_or(FormatBreakChoice::Compact);
    }

    static bool IsBreakingChoice(FormatBreakChoice choice) { return choice != FormatBreakChoice::Compact; }

    static bool HasSelectedBreak(const FormatBreakNode& node, const NodeResult& result) {
        if (node.kind != FormatBreakNodeKind::Token && IsBreakingChoice(ChoiceFor(result, node))) {
            return true;
        }
        for (const FormatBreakNode* child : node.children) {
            if (child != nullptr && HasSelectedBreak(*child, result)) {
                return true;
            }
        }
        for (const FormatBreakListItem& item : node.items) {
            if (item.node != nullptr && HasSelectedBreak(*item.node, result)) {
                return true;
            }
        }
        for (const FormatBreakNode* operand : node.operands) {
            if (operand != nullptr && HasSelectedBreak(*operand, result)) {
                return true;
            }
        }
        return false;
    }

    static bool TokenHasPhysicalLineBreak(const FormatBreakToken& token) {
        if (token.contextOnly) {
            return false;
        }
        return FormatBreakTokenKind(token) == PrintTokenKind::BlankLine ||
            IsCommentToken(FormatBreakTokenKind(token)) ||
            FormatTokenText(FormatBreakTokenValue(token)).find('\n') != std::string_view::npos;
    }

    static bool HasPhysicalLineBreak(const FormatBreakNode& node, const NodeResult& result) {
        if (node.kind == FormatBreakNodeKind::Token) {
            return TokenHasPhysicalLineBreak(node.token);
        }
        if (IsBreakingChoice(ChoiceFor(result, node)) || FormatBreakHasLeadingTrailingComment(node)) {
            return true;
        }
        for (const FormatBreakNode* child : node.children) {
            if (child != nullptr && HasPhysicalLineBreak(*child, result)) {
                return true;
            }
        }
        for (size_t index = 0; index < node.items.size(); ++index) {
            if (
                FormatBreakHasTrailingComment(node, index) ||
                (node.items[index].node != nullptr && HasPhysicalLineBreak(*node.items[index].node, result))
            ) {
                return true;
            }
        }
        for (const FormatBreakNode* operand : node.operands) {
            if (operand != nullptr && HasPhysicalLineBreak(*operand, result)) {
                return true;
            }
        }
        return std::any_of(
            node.commentsBeforeOperators.begin(),
            node.commentsBeforeOperators.end(),
            [](const std::vector<FormatBreakToken>& comments) { return !comments.empty(); }
        );
    }

    static bool IsIntrinsicMultilineLiteral(const FormatBreakNode& node) {
        if (node.kind != FormatBreakNodeKind::Token || node.token.contextOnly) {
            return false;
        }
        const PrintToken& token = FormatBreakTokenValue(node.token);
        return IsStringLike(token) && FormatTokenText(token).find('\n') != std::string_view::npos;
    }

    static bool IsTailExpansionBreak(const FormatBreakNode& node, const NodeResult& compact) {
        return (node.kind == FormatBreakNodeKind::Delimited || node.kind == FormatBreakNodeKind::BodyHeader) &&
            IsBreakingChoice(ChoiceFor(compact, node));
    }

    static CompactTailExpansionKind CompactTailExpansion(const FormatBreakNode& node, const NodeResult& compact) {
        if (IsIntrinsicMultilineLiteral(node)) {
            return CompactTailExpansionKind::IntrinsicMultilineLiteral;
        }
        if (IsTailExpansionBreak(node, compact)) {
            return CompactTailExpansionKind::Structural;
        }
        if (node.kind == FormatBreakNodeKind::Delimited) {
            return DelimitedCompactTailExpansion(node, compact);
        }
        if (node.kind == FormatBreakNodeKind::Chain) {
            if (node.operands.empty() || ChoiceFor(compact, node) != FormatBreakChoice::Compact) {
                return CompactTailExpansionKind::None;
            }
            for (size_t index = 0; index + 1 < node.operands.size(); ++index) {
                if (node.operands[index] != nullptr && HasPhysicalLineBreak(*node.operands[index], compact)) {
                    return CompactTailExpansionKind::None;
                }
            }
            return CompactTailExpansion(*node.operands.back(), compact);
        }
        if (node.kind == FormatBreakNodeKind::BodyHeader) {
            if (node.children.empty() || ChoiceFor(compact, node) != FormatBreakChoice::Compact) {
                return CompactTailExpansionKind::None;
            }
            for (size_t index = 0; index + 1 < node.children.size(); ++index) {
                if (node.children[index] != nullptr && HasPhysicalLineBreak(*node.children[index], compact)) {
                    return CompactTailExpansionKind::None;
                }
            }
            return CompactTailExpansion(*node.children.back(), compact);
        }
        if (node.kind != FormatBreakNodeKind::Sequence || node.children.empty()) {
            return CompactTailExpansionKind::None;
        }
        for (size_t index = 0; index + 1 < node.children.size(); ++index) {
            if (node.children[index] != nullptr && HasPhysicalLineBreak(*node.children[index], compact)) {
                return CompactTailExpansionKind::None;
            }
        }
        return CompactTailExpansion(*node.children.back(), compact);
    }

    static bool CanKeepCompactPrefixEndingInTailExpansion(const FormatBreakNode& node, const NodeResult& compact) {
        return CompactTailExpansion(node, compact) != CompactTailExpansionKind::None;
    }

    static CompactTailExpansionKind
        DelimitedCompactTailExpansion(const FormatBreakNode& node, const NodeResult& compact)
    {
        if (node.items.empty()) {
            return CompactTailExpansionKind::None;
        }
        const FormatBreakNode* tail = node.items.back().node;
        if (tail == nullptr) {
            return CompactTailExpansionKind::None;
        }
        if (node.items.size() == 1 && !HasRealSeparators(node)) {
            return CompactTailExpansion(*tail, compact);
        }
        if (!HasRealSeparators(node)) {
            return CompactTailExpansionKind::None;
        }
        for (size_t index = 0; index + 1 < node.items.size(); ++index) {
            if (node.items[index].node != nullptr && HasPhysicalLineBreak(*node.items[index].node, compact)) {
                return CompactTailExpansionKind::None;
            }
        }
        return CompactTailExpansion(*tail, compact);
    }

    static bool CanKeepDelimitedCompactWithExtraLines(const FormatBreakNode& node, const NodeResult& compact) {
        return DelimitedCompactTailExpansion(node, compact) != CompactTailExpansionKind::None;
    }

    static bool CanKeepChainCompactWithExtraLines(const FormatBreakNode& node, const NodeResult& compact) {
        if (node.operands.empty() || ChoiceFor(compact, node) != FormatBreakChoice::Compact) {
            return false;
        }
        if (
            node.chainKind == FormatBreakChainKind::CallApplication ||
            node.chainKind == FormatBreakChainKind::MemberBeforeOperator
        ) {
            for (size_t index = 1; index + 1 < node.operands.size(); ++index) {
                if (node.operands[index] != nullptr && HasSelectedBreak(*node.operands[index], compact)) {
                    return false;
                }
            }
            return true;
        }
        for (size_t index = 0; index + 1 < node.operands.size(); ++index) {
            if (node.operands[index] != nullptr && HasSelectedBreak(*node.operands[index], compact)) {
                return false;
            }
        }
        const FormatBreakNode* tail = node.operands.back();
        return tail != nullptr && CanKeepCompactPrefixEndingInTailExpansion(*tail, compact);
    }

    bool RequiresChainCompactExtraLinesGuard(const FormatBreakNode& node) {
        if (!node.operands.empty() && ContainsForceSplitAdjacentStrings(*node.operands.back())) {
            return true;
        }
        if (node.chainKind == FormatBreakChainKind::Ternary) {
            return node.operators.size() > 2;
        }
        if (node.chainKind == FormatBreakChainKind::StreamBeforeOperator) {
            return true;
        }
        if (node.chainKind == FormatBreakChainKind::CallApplication) {
            return node.operands.size() > 2;
        }
        return IsFormatBreakUniformChain(node) && node.operators.size() > 1;
    }

    bool ContainsForceSplitAdjacentStrings(const FormatBreakNode& node) {
        // These subtree predicates depend only on the immutable break model and are shared by many candidates.
        signed char& cached = containsForceSplitAdjacentStrings_[static_cast<size_t>(node.id)];
        if (cached >= 0) {
            return cached != 0;
        }
        bool result = node.kind == FormatBreakNodeKind::AdjacentStrings && node.forceSplit;
        for (const FormatBreakNode* child : node.children) {
            if (!result && child && ContainsForceSplitAdjacentStrings(*child)) {
                result = true;
            }
        }
        for (const FormatBreakListItem& item : node.items) {
            if (!result && item.node && ContainsForceSplitAdjacentStrings(*item.node)) {
                result = true;
            }
        }
        for (const FormatBreakNode* operand : node.operands) {
            if (!result && operand && ContainsForceSplitAdjacentStrings(*operand)) {
                result = true;
            }
        }
        cached = result ? 1 : 0;
        return result;
    }

    bool ContainsNonSingleStatementBodyHeader(const FormatBreakNode& node) {
        signed char& cached = containsNonSingleStatementBodyHeader_[static_cast<size_t>(node.id)];
        if (cached >= 0) {
            return cached != 0;
        }
        bool result = node.kind == FormatBreakNodeKind::BodyHeader && !node.bodyHeaderSingleStatementBody;
        for (const FormatBreakNode* child : node.children) {
            if (!result && child && ContainsNonSingleStatementBodyHeader(*child)) {
                result = true;
            }
        }
        for (const FormatBreakListItem& item : node.items) {
            if (!result && item.node && ContainsNonSingleStatementBodyHeader(*item.node)) {
                result = true;
            }
        }
        for (const FormatBreakNode* operand : node.operands) {
            if (!result && operand && ContainsNonSingleStatementBodyHeader(*operand)) {
                result = true;
            }
        }
        cached = result ? 1 : 0;
        return result;
    }

    static bool TrailingBodyHeaderHeaderHasSelectedBreak(const FormatBreakNode& node, const NodeResult& result) {
        if (node.kind == FormatBreakNodeKind::BodyHeader) {
            return !node.children.empty() &&
                node.children.front() != nullptr &&
                HasSelectedBreak(*node.children.front(), result);
        }
        if (node.kind != FormatBreakNodeKind::Sequence) {
            return false;
        }

        std::vector<const FormatBreakNode*> sequenceChildren;
        AppendSequenceChildren(node.children, sequenceChildren);
        return !sequenceChildren.empty() && TrailingBodyHeaderHeaderHasSelectedBreak(*sequenceChildren.back(), result);
    }

    NodeResults
        SolvePrefixListCompactAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Compact, indentLevel);
        AppendToken(result, node.children[0]->token);
        if (FormatBreakHasLeadingTrailingComment(node)) {
            AppendToken(result, node.leadingTrailingComment);
        }
        NodeResults current{result};
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& listItem = node.items[index];
            NodeResults next;
            for (const NodeResult& prefix : current) {
                for (const NodeResult& item : SolveListItemWithSuffixAlternatives(
                    listItem, prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText
                )) {
                    NodeResult candidate = prefix;
                    Merge(candidate, item);
                    AddPrunedResult(next, std::move(candidate));
                }
            }
            SortPrunedResults(next);
            current = std::move(next);
        }
        return current;
    }

    NodeResult SolvePrefixListPacked(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        if (node.forceSplit || node.items.size() < 2 || FormatBreakHasLeadingTrailingComment(node)) {
            return {};
        }
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::SplitPacked, indentLevel);
        AppendToken(result, node.children.front()->token);
        AppendBreak(result, indentLevel + 1, node.breakCost);
        NodeResult body{.valid = true, .endColumn = result.endColumn, .endIndentLevel = result.endIndentLevel};
        if (!AppendCompactListItems(node, body, true)) {
            return {};
        }
        Merge(result, body);
        return result;
    }

    NodeResults
        SolvePrefixListSplitAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, indentLevel);
        AppendToken(result, node.children[0]->token);
        if (FormatBreakHasLeadingTrailingComment(node)) {
            AppendToken(result, node.leadingTrailingComment);
        }
        AppendListBreakAfterOptionalComment(
            result,
            indentLevel + 1,
            node.breakCost,
            HasBlankLineBeforeItem(node, 0),
            FormatBreakHasLeadingTrailingComment(node)
        );
        NodeResults current{result};
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& listItem = node.items[index];
            NodeResults next;
            for (const NodeResult& prefix : current) {
                for (const NodeResult& item : SolveListItemWithSuffixAlternatives(
                    listItem, prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText
                )) {
                    NodeResult candidate = prefix;
                    Merge(candidate, item);
                    if (index + 1 < node.items.size()) {
                        AppendListBreakAfterOptionalComment(
                            candidate,
                            indentLevel + 1,
                            node.breakCost,
                            HasBlankLineBeforeItem(node, index + 1),
                            FormatBreakHasTrailingComment(node, index)
                        );
                    }
                    AddPrunedResult(next, std::move(candidate));
                }
            }
            SortPrunedResults(next);
            current = std::move(next);
        }
        return current;
    }

    NodeResults SolvePrefixListAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResults alternatives;
        for (NodeResult compact : SolvePrefixListCompactAlternatives(node, column, indentLevel, lineHasText)) {
            if (!node.forceSplit && !(compact.valid && compact.extraLines > 0)) {
                alternatives.push_back(std::move(compact));
            }
        }
        NodeResult packed = SolvePrefixListPacked(node, column, indentLevel, lineHasText);
        if (packed.valid) {
            alternatives.push_back(std::move(packed));
        }
        for (NodeResult split : SolvePrefixListSplitAlternatives(node, column, indentLevel, lineHasText)) {
            if (split.valid) {
                alternatives.push_back(std::move(split));
            }
        }
        return alternatives;
    }

    NodeResult SolvePrefixList(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult best;
        for (const NodeResult& candidate : SolvePrefixListAlternatives(node, column, indentLevel, lineHasText)) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    NodeResult SolveStatementSequenceCompact(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Compact, indentLevel);
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& listItem = node.items[index];
            NodeResult item =
                SolveListItemWithSuffix(listItem, result.endColumn, result.endIndentLevel, result.endLineHasText);
            Merge(result, item);
        }
        return result;
    }

    NodeResult SolveStatementSequenceSplit(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, indentLevel);
        for (size_t index = 0; index < node.items.size(); ++index) {
            if (index > 0) {
                AppendListBreakAfterOptionalComment(
                    result,
                    indentLevel,
                    node.breakCost,
                    HasBlankLineBeforeItem(node, index),
                    FormatBreakHasTrailingComment(node, index - 1)
                );
            }
            const FormatBreakListItem& listItem = node.items[index];
            NodeResult item =
                SolveListItemWithSuffix(listItem, result.endColumn, result.endIndentLevel, result.endLineHasText);
            Merge(result, item);
        }
        return result;
    }

    NodeResult SolveStatementSequence(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult compact = SolveStatementSequenceCompact(node, column, indentLevel, lineHasText);
        NodeResult split = SolveStatementSequenceSplit(node, column, indentLevel, lineHasText);
        if (node.forceSplit && split.valid) {
            return split;
        }
        return Better(split, compact) ? split : compact;
    }

    NodeResults SolveFunctionSignatureCompactAlternatives(
        const FormatBreakNode& node, int column, int indentLevel, bool lineHasText
    ) {
        if (node.children.size() < 2) {
            return {};
        }
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Compact, indentLevel);
        NodeResults current{result};
        std::optional<bool> compactPrefixFits;
        for (size_t index = 0; index < node.children.size(); ++index) {
            const FormatBreakNode* child = node.children[index];
            NodeResults next;
            for (const NodeResult& prefix : current) {
                for (const NodeResult& item : SolveAlternatives(
                    *child, prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText
                )) {
                    if (index == 1 && item.extraLines > 0) {
                        if (!compactPrefixFits.has_value()) {
                            compactPrefixFits =
                                FunctionSignatureCompactPrefixFits(node, column, indentLevel, lineHasText);
                        }
                        if (!*compactPrefixFits) {
                            continue;
                        }
                    }
                    NodeResult candidate = prefix;
                    Merge(candidate, item);
                    AddPrunedResult(next, std::move(candidate));
                }
            }
            SortPrunedResults(next);
            current = std::move(next);
        }
        return current;
    }

    NodeResult SolveFunctionSignatureCompact(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResult best;
        for (const NodeResult& candidate : SolveFunctionSignatureCompactAlternatives(
            node, column, indentLevel, lineHasText
        )) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    static bool IsParameterListDelimited(const FormatBreakNode& node) {
        if (
            node.kind != FormatBreakNodeKind::Delimited ||
            node.delimiterKind != FormatBreakDelimiterKind::Paren ||
            node.children.empty() ||
            node.children.front() == nullptr ||
            node.children.front()->kind != FormatBreakNodeKind::Token
        ) {
            return false;
        }
        const PrintToken& open = FormatBreakTokenValue(node.children.front()->token);
        return open.parentKind == SyntaxNodeKind::ParameterList;
    }

    bool FunctionSignatureCompactPrefixFits(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        if (node.children.size() < 2) {
            return false;
        }
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        NodeResult returnType = Solve(*node.children[0], column, indentLevel, lineHasText);
        if (!returnType.valid) {
            return false;
        }
        Merge(result, returnType);
        bool splitParameterList = false;
        bool compactHeaderFits = true;
        // Only the prefix through the opener must fit; an empty parameter list need not have a split layout.
        SolveWithFirstParameterListSplit(
            *node.children[1],
            result.endColumn,
            result.endIndentLevel,
            result.endLineHasText,
            splitParameterList,
            compactHeaderFits
        );
        return splitParameterList && compactHeaderFits;
    }

    NodeResult SolveWithFirstParameterListSplit(
        const FormatBreakNode& node,
        int column,
        int indentLevel,
        bool lineHasText,
        bool& splitParameterList,
        bool& compactHeaderFits
    ) {
        if (IsParameterListDelimited(node) && !splitParameterList) {
            splitParameterList = true;
            const NodeResult open = SolveToken(node.children.front()->token, column, indentLevel, lineHasText);
            compactHeaderFits = open.valid && open.endColumn <= config_.columnLimit;
            return SolveDelimitedExpanded(node, column, indentLevel, lineHasText);
        }
        if (node.kind != FormatBreakNodeKind::Sequence && node.kind != FormatBreakNodeKind::FunctionSignature) {
            return Solve(node, column, indentLevel, lineHasText);
        }

        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        std::vector<const FormatBreakNode*> sequenceChildren;
        AppendSequenceChildren(node.children, sequenceChildren);
        for (const FormatBreakNode* child : sequenceChildren) {
            if (child == nullptr) {
                continue;
            }
            NodeResult item = SolveWithFirstParameterListSplit(
                *child,
                result.endColumn,
                result.endIndentLevel,
                result.endLineHasText,
                splitParameterList,
                compactHeaderFits
            );
            if (!item.valid) {
                return {};
            }
            Merge(result, item);
        }
        return result;
    }

    NodeResult SolveFunctionSignatureCompactWithSplitParameters(
        const FormatBreakNode& node, int column, int indentLevel, bool lineHasText
    ) {
        if (node.children.size() < 2) {
            return {};
        }
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Compact, indentLevel);
        NodeResult returnType =
            Solve(*node.children[0], result.endColumn, result.endIndentLevel, result.endLineHasText);
        if (!returnType.valid) {
            return {};
        }
        Merge(result, returnType);
        bool splitParameterList = false;
        bool compactHeaderFits = true;
        NodeResult declarator = SolveWithFirstParameterListSplit(
            *node.children[1],
            result.endColumn,
            result.endIndentLevel,
            result.endLineHasText,
            splitParameterList,
            compactHeaderFits
        );
        if (!splitParameterList || !compactHeaderFits || !declarator.valid) {
            return {};
        }
        Merge(result, declarator);
        if (node.children.size() > 2) {
            NodeResult tail = Solve(*node.children[2], result.endColumn, result.endIndentLevel, result.endLineHasText);
            if (!tail.valid) {
                return {};
            }
            Merge(result, tail);
        }
        return result;
    }

    NodeResults SolveFunctionSignatureSplitAlternatives(
        const FormatBreakNode& node, int column, int indentLevel, bool lineHasText
    ) {
        if (node.children.size() < 2) {
            return {};
        }
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, indentLevel);
        NodeResults current;
        for (const NodeResult& returnType : SolveAlternatives(
            *node.children[0], result.endColumn, result.endIndentLevel, result.endLineHasText
        )) {
            NodeResult candidate = result;
            Merge(candidate, returnType);
            AppendBreak(candidate, indentLevel + 1, node.breakCost);
            AddPrunedResult(current, std::move(candidate));
        }
        SortPrunedResults(current);

        NodeResults declarators;
        for (const NodeResult& prefix : current) {
            for (const NodeResult& declarator : SolveAlternatives(
                *node.children[1], prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText
            )) {
                NodeResult candidate = prefix;
                Merge(candidate, declarator);
                AddPrunedResult(declarators, std::move(candidate));
            }
        }
        SortPrunedResults(declarators);
        if (node.children.size() == 2) {
            return declarators;
        }

        NodeResults alternatives;
        for (NodeResult prefix : declarators) {
            if (node.functionSignatureHasBody) {
                AppendBreak(prefix, indentLevel, node.breakCost);
            }
            for (const NodeResult& tail : SolveAlternatives(
                *node.children[2], prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText
            )) {
                NodeResult candidate = prefix;
                Merge(candidate, tail);
                AddPrunedResult(alternatives, std::move(candidate));
            }
        }
        SortPrunedResults(alternatives);
        return alternatives;
    }

    NodeResult SolveFunctionSignatureSplit(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult best;
        for (const NodeResult& candidate : SolveFunctionSignatureSplitAlternatives(
            node, column, indentLevel, lineHasText
        )) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    NodeResults
        SolveFunctionSignatureAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResults alternatives;
        for (NodeResult compact : SolveFunctionSignatureCompactAlternatives(node, column, indentLevel, lineHasText)) {
            alternatives.push_back(std::move(compact));
        }
        NodeResult splitParameters =
            SolveFunctionSignatureCompactWithSplitParameters(node, column, indentLevel, lineHasText);
        NodeResults split = SolveFunctionSignatureSplitAlternatives(node, column, indentLevel, lineHasText);
        if (splitParameters.valid) {
            alternatives.push_back(std::move(splitParameters));
        }
        for (NodeResult& candidate : split) {
            alternatives.push_back(std::move(candidate));
        }
        return alternatives;
    }

    NodeResult SolveFunctionSignature(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult compact = SolveFunctionSignatureCompact(node, column, indentLevel, lineHasText);
        NodeResult splitParameters =
            SolveFunctionSignatureCompactWithSplitParameters(node, column, indentLevel, lineHasText);
        NodeResult split = SolveFunctionSignatureSplit(node, column, indentLevel, lineHasText);
        NodeResult best = compact;
        if (Better(splitParameters, best)) {
            best = splitParameters;
        }
        return Better(split, best) ? split : best;
    }

    static bool
        ExpandedBodyHeaderNeedsDetachedBody(const FormatBreakNode& node, const NodeResult& header, int ownerIndentLevel)
    {
        return node.bodyHeaderDetachBodyAfterExpandedHeader &&
            header.valid &&
            header.extraLines > 0 &&
            header.endIndentLevel == ownerIndentLevel + 1;
    }

    NodeResults
        SolveBodyHeaderCompactAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        if (node.children.size() < 2 || node.bodyHeaderRequiresDetachedBody) {
            return {};
        }
        NodeResults alternatives;
        for (const NodeResult& header : SolveAlternatives(*node.children[0], column, indentLevel, lineHasText)) {
            if (
                ExpandedBodyHeaderNeedsDetachedBody(node, header, indentLevel) ||
                (node.bodyHeaderSingleStatementBody && header.extraLines > 0) || (
                    !lineHasText &&
                    node.bodyHeaderSplitAtParentIndentWhenLineStarts &&
                    !node.bodyHeaderSingleStatementBody
                )
            ) {
                continue;
            }
            for (const NodeResult& body : SolveAlternatives(
                *node.children[1], header.endColumn, header.endIndentLevel, header.endLineHasText
            )) {
                NodeResult candidate{
                    .valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText
                };
                Merge(candidate, header);
                Merge(candidate, body);
                if (node.bodyHeaderSingleStatementBody && (candidate.extraLines > 0 || HasOverflow(candidate))) {
                    continue;
                }
                AddChoice(candidate, node.id, FormatBreakChoice::Compact, indentLevel);
                AddPrunedResult(alternatives, std::move(candidate));
            }
        }
        SortPrunedResults(alternatives);
        return alternatives;
    }

    NodeResults SolveBodyHeaderAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResults alternatives = SolveBodyHeaderCompactAlternatives(node, column, indentLevel, lineHasText);
        for (NodeResult candidate : SolveBodyHeaderSplitWithChoiceAlternatives(
            node, column, indentLevel, lineHasText, FormatBreakChoice::Split, indentLevel
        )) {
            alternatives.push_back(std::move(candidate));
        }
        if (node.bodyHeaderDetachBodyAfterExpandedHeader || node.bodyHeaderRequiresDetachedBody) {
            for (NodeResult candidate : SolveBodyHeaderSplitWithChoiceAlternatives(
                node, column, indentLevel, lineHasText, FormatBreakChoice::BodyHeaderDetachedBody, indentLevel
            )) {
                alternatives.push_back(std::move(candidate));
            }
        }
        if (!lineHasText && node.bodyHeaderSplitAtParentIndentWhenLineStarts) {
            for (NodeResult candidate : SolveBodyHeaderSplitWithChoiceAlternatives(
                node,
                column,
                indentLevel,
                lineHasText,
                FormatBreakChoice::BodyHeaderSplitAtParentIndent,
                std::max(0, indentLevel - 1),
                node.bodyHeaderSingleStatementBody
            )) {
                alternatives.push_back(std::move(candidate));
            }
        }
        return alternatives;
    }

    NodeResults SolveBodyHeaderSplitWithChoiceAlternatives(
        const FormatBreakNode& node,
        int column,
        int indentLevel,
        bool lineHasText,
        FormatBreakChoice choice,
        int bodyIndentLevel,
        bool requireHeaderBreak = false
    ) {
        if (node.children.size() < 2) {
            return {};
        }
        NodeResults alternatives;
        for (const NodeResult& header : SolveAlternatives(*node.children[0], column, indentLevel, lineHasText)) {
            if (
                choice == FormatBreakChoice::Split &&
                (node.bodyHeaderRequiresDetachedBody || ExpandedBodyHeaderNeedsDetachedBody(node, header, indentLevel))
            ) {
                continue;
            }
            if (requireHeaderBreak && header.extraLines == 0) {
                continue;
            }
            NodeResult result{
                .valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText
            };
            AddChoice(result, node.id, choice, indentLevel);
            Merge(result, header);
            if (
                choice == FormatBreakChoice::BodyHeaderSplitAtParentIndent ||
                choice == FormatBreakChoice::BodyHeaderDetachedBody
            ) {
                AppendBreak(result, bodyIndentLevel, node.breakCost);
            }
            NodeResult body = SolveBodyHeaderSplitBody(
                *node.children[1], result.endColumn, result.endIndentLevel, result.endLineHasText
            );
            Merge(result, body);
            AddPrunedResult(alternatives, std::move(result));
        }
        SortPrunedResults(alternatives);
        return alternatives;
    }

    NodeResult SolveBodyHeaderSplitWithChoice(
        const FormatBreakNode& node,
        int column,
        int indentLevel,
        bool lineHasText,
        FormatBreakChoice choice,
        int bodyIndentLevel
    ) {
        NodeResult best;
        for (const NodeResult& candidate : SolveBodyHeaderSplitWithChoiceAlternatives(
            node, column, indentLevel, lineHasText, choice, bodyIndentLevel
        )) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    NodeResult SolveBodyHeaderSplitBody(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        if (node.kind == FormatBreakNodeKind::Delimited) {
            return SolveDelimitedSplit(node, column, indentLevel, lineHasText);
        }
        return Solve(node, column, indentLevel, lineHasText);
    }

    NodeResult
        SolveBodyHeaderSplitAtParentIndent(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        return SolveBodyHeaderSplitWithChoice(
            node,
            column,
            indentLevel,
            lineHasText,
            FormatBreakChoice::BodyHeaderSplitAtParentIndent,
            std::max(0, indentLevel - 1)
        );
    }

    NodeResult SolveTrailingBodyHeaderSplitAtParentIndent(
        const FormatBreakNode& node, int column, int indentLevel, bool lineHasText
    ) {
        if (node.kind == FormatBreakNodeKind::BodyHeader) {
            return SolveBodyHeaderSplitAtParentIndent(node, column, indentLevel, lineHasText);
        }
        if (node.kind != FormatBreakNodeKind::Sequence) {
            return {};
        }

        std::vector<const FormatBreakNode*> sequenceChildren;
        AppendSequenceChildren(node.children, sequenceChildren);
        if (sequenceChildren.empty()) {
            return {};
        }

        NodeResults
            current{{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText}};
        for (size_t index = 0; index < sequenceChildren.size(); ++index) {
            NodeResults next;
            const bool last = index + 1 == sequenceChildren.size();
            for (const NodeResult& prefix : current) {
                NodeResults trailingResults;
                const NodeResults* childResults = nullptr;
                if (last) {
                    trailingResults.push_back(SolveTrailingBodyHeaderSplitAtParentIndent(
                        *sequenceChildren[index], prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText
                    ));
                    childResults = &trailingResults;
                } else {
                    childResults = &SolveAlternatives(
                        *sequenceChildren[index], prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText
                    );
                }
                for (const NodeResult& child : *childResults) {
                    if (!child.valid) {
                        continue;
                    }
                    NodeResult candidate = prefix;
                    Merge(candidate, child);
                    AddPrunedResult(next, std::move(candidate));
                }
            }
            SortPrunedResults(next);
            current = std::move(next);
        }

        NodeResult best;
        for (const NodeResult& candidate : current) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    NodeResult SolveBodyHeader(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult best;
        for (const NodeResult& candidate : SolveBodyHeaderAlternatives(node, column, indentLevel, lineHasText)) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    NodeResult SolveChainCompact(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult best;
        for (const NodeResult& candidate : SolveChainCompactAlternatives(node, column, indentLevel, lineHasText)) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    void AppendCommentsBeforeChainOperator(const FormatBreakNode& node, size_t index, NodeResult& result) {
        if (index >= node.commentsBeforeOperators.size()) {
            return;
        }
        for (const FormatBreakToken& comment : node.commentsBeforeOperators[index]) {
            AppendToken(result, comment);
        }
    }

    bool CompactLiteralFollowerFits(const FormatBreakNode& node, size_t operatorIndex, NodeResult prefix) const {
        if (
            !prefix.endLineHasText ||
            !IsFormatBreakLiteralOperand(*node.operands[operatorIndex], SyntaxNodeClass::StringLike) ||
            !compactLayout_.Measure(*node.operands[operatorIndex]).valid
        ) {
            return false;
        }
        const bool streamChain = node.chainKind == FormatBreakChainKind::StreamBeforeOperator;
        for (size_t index = operatorIndex; index < node.operators.size(); ++index) {
            const FormatBreakToken& op = node.operators[index];
            if (
                FormatBreakTokenKind(op) != PrintTokenKind::Known ||
                FormatBreakTokenSyntaxKind(op) != (streamChain ? SyntaxNodeKind::LessLess : SyntaxNodeKind::Plus) ||
                op.contextOnly ||
                (index < node.commentsBeforeOperators.size() && !node.commentsBeforeOperators[index].empty()) ||
                (streamChain && !AddCompactToken(prefix, op, true)) ||
                compactLayout_.Measure(*node.operands[index + 1]).hasContextOnlyTokens ||
                !AppendCompactOneLine(*node.operands[index + 1], prefix, true)
            ) {
                return false;
            }
            if (streamChain && IsFormatBreakStreamConfigurationOperand(
                *node.operands[index + 1], config_.streamShiftConfigurationMethods
            )) {
                continue;
            }
            if (IsFormatBreakLiteralOperand(*node.operands[index + 1])) {
                return false;
            }
            if (
                !streamChain &&
                index + 1 < node.operators.size() &&
                !AddCompactToken(prefix, node.operators[index + 1], true)
            ) {
                return false;
            }
            return (streamChain && index + 1 < node.operators.size()) ||
                prefix.endColumn + breakLineSuffixWidth_ <= config_.columnLimit;
        }
        return false;
    }

    NodeResults
        SolveChainCompactAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        if (node.forceSplit || node.chainStartsWithOperator) {
            return {};
        }
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Compact, indentLevel);
        NodeResults current{result};
        const bool restrictIntermediateBreaks = RequiresChainCompactExtraLinesGuard(node);
        for (size_t index = 0; index < node.operands.size(); ++index) {
            NodeResults nextByState;
            for (const NodeResult& prefix : current) {
                const bool mustRemainStructurallyCompact =
                    restrictIntermediateBreaks && index + 1 < node.operands.size() && !(
                        (
                            node.chainKind == FormatBreakChainKind::CallApplication ||
                            node.chainKind == FormatBreakChainKind::MemberBeforeOperator
                        ) && index == 0
                    );
                NodeResults compactOperands;
                const NodeResults* operands = nullptr;
                if (mustRemainStructurallyCompact) {
                    const std::optional<NodeResult> compact = SolveCompactPhysicalLine(
                        *node.operands[index], prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText, false
                    );
                    if (compact) {
                        // The compact-chain legality guard below rejects every selected break in this operand.
                        // When the all-compact walker succeeds, it is the unique candidate with no selected break,
                        // including when it overflows. Multiline tokens and comments fall back to exact filtering.
                        compactOperands.push_back(std::move(*compact));
                    } else {
                        for (const NodeResult& operand : SolveAlternatives(
                            *node.operands[index], prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText
                        )) {
                            if (!HasSelectedBreak(*node.operands[index], operand)) {
                                compactOperands.push_back(operand);
                            }
                        }
                    }
                    operands = &compactOperands;
                } else {
                    operands = &SolveAlternatives(
                        *node.operands[index], prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText
                    );
                }
                for (const NodeResult& operand : *operands) {
                    if (!operand.valid) {
                        continue;
                    }
                    NodeResult next = prefix;
                    Merge(next, operand);
                    if (node.declarationValueOwner != nullptr && index + 1 == node.operands.size()) {
                        AddDeclarationValueContinuationLines(next, node.id, operand.extraLines);
                    }
                    if (index < node.operators.size()) {
                        AppendCommentsBeforeChainOperator(node, index, next);
                        AppendToken(next, node.operators[index]);
                    }
                    AddPrunedResult(nextByState, std::move(next));
                }
            }
            SortPrunedResults(nextByState);
            current = std::move(nextByState);
        }
        return current;
    }

    NodeResult SolveDelimitedSplitAttachedOpen(const FormatBreakNode& node, NodeResult prefix, int baseIndent) {
        if (node.kind != FormatBreakNodeKind::Delimited || node.items.empty()) {
            return {};
        }
        NodeResult result{
            .valid = true,
            .endColumn = prefix.endColumn,
            .endIndentLevel = prefix.endIndentLevel,
            .endLineHasText = prefix.endLineHasText,
        };
        AddChoice(result, node.id, FormatBreakChoice::SplitAttachedOpen, baseIndent);
        AppendToken(result, node.children[0]->token);
        if (FormatBreakHasLeadingTrailingComment(node)) {
            AppendToken(result, node.leadingTrailingComment);
        }
        AppendListBreakAfterOptionalComment(
            result,
            baseIndent + 1,
            node.breakCost,
            HasBlankLineBeforeItem(node, 0),
            FormatBreakHasLeadingTrailingComment(node)
        );
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& listItem = node.items[index];
            NodeResult item = Solve(*listItem.node, result.endColumn, result.endIndentLevel, result.endLineHasText);
            Merge(result, item);
            if (FormatBreakTokenKind(listItem.separator) == PrintTokenKind::Known) {
                AppendToken(result, listItem.separator);
            }
            if (node.splitTrailingCommaItem == index) {
                AppendTrailingComma(result);
            }
            if (FormatBreakHasTrailingComment(node, index)) {
                AppendToken(result, listItem.trailingComment);
            }
            const bool hasNextItem = index + 1 < node.items.size();
            AppendListBreakAfterOptionalComment(
                result,
                hasNextItem ? baseIndent + 1 : baseIndent,
                node.breakCost,
                hasNextItem && HasBlankLineBeforeItem(node, index + 1),
                FormatBreakHasTrailingComment(node, index)
            );
        }
        AppendToken(result, node.children[1]->token);
        Merge(prefix, result);
        return prefix;
    }

    static bool CanAttachSplitOpenAfterOperator(const FormatBreakToken& op, const FormatBreakNode& operand) {
        const PrintToken& opToken = FormatBreakTokenValue(op);
        return opToken.kind == PrintTokenKind::Known &&
            opToken.parentKind == SyntaxNodeKind::BinaryExpression &&
            SyntaxNodeKindHasClass(opToken.syntaxKind, SyntaxNodeClass::BinaryOperator) &&
            operand.kind == FormatBreakNodeKind::Delimited &&
            operand.delimiterKind == FormatBreakDelimiterKind::Paren;
    }

    NodeResults SolveChainSplitAfterOperatorAlternatives(
        const FormatBreakNode& node, int column, int indentLevel, bool lineHasText
    ) {
        const int splitBaseIndent = node.requiredChainBreakBaseIndent.value_or(indentLevel);
        const int continuationIndent = node.flatSplitIndent ? splitBaseIndent : splitBaseIndent + 1;
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, splitBaseIndent);
        if (node.operands.empty()) {
            return {result};
        }
        const FormatBreakToken* firstSuffix = node.operators.empty() ? nullptr : &node.operators.front();
        NodeResult first = SolveNodeWithSuffix(
            *node.operands.front(), firstSuffix, result.endColumn, result.endIndentLevel, result.endLineHasText
        );
        if (!first.valid) {
            return {};
        }
        Merge(result, first);
        NodeResults current{result};
        for (size_t index = 0; index < node.operators.size(); ++index) {
            NodeResults next;
            for (const NodeResult& prefix : current) {
                NodeResult normal = AddBreak(prefix, continuationIndent, node.breakCost);
                const bool splitTrailingBodyHeaderAtParentIndent =
                    node.splitTrailingBodyHeaderAtParentIndent && index + 1 == node.operands.size() - 1;
                const FormatBreakToken* nextSuffix =
                    index + 1 < node.operators.size() ? &node.operators[index + 1] : nullptr;
                NodeResult operand = SolveNodeWithSuffix(
                    *node.operands[index + 1],
                    nextSuffix,
                    normal.endColumn,
                    normal.endIndentLevel,
                    normal.endLineHasText
                );
                if (splitTrailingBodyHeaderAtParentIndent) {
                    NodeResult parentIndentOperand = SolveTrailingBodyHeaderSplitAtParentIndent(
                        *node.operands[index + 1], normal.endColumn, normal.endIndentLevel, normal.endLineHasText
                    );
                    if (nextSuffix != nullptr && parentIndentOperand.valid) {
                        AppendToken(parentIndentOperand, *nextSuffix);
                    }
                    if (
                        !TrailingBodyHeaderHeaderHasSelectedBreak(*node.operands[index + 1], operand) &&
                        (operand.extraLines > 0 || ContainsNonSingleStatementBodyHeader(*node.operands[index + 1])) &&
                        parentIndentOperand.valid
                    ) {
                        operand = parentIndentOperand;
                    } else {
                        operand = Better(parentIndentOperand, operand) ? parentIndentOperand : operand;
                    }
                }
                Merge(normal, operand);
                if (node.declarationValueOwner != nullptr && index + 2 == node.operands.size()) {
                    AddDeclarationValueContinuationLines(normal, node.id, operand.extraLines + 1);
                }

                NodeResult attached;
                if (CanAttachSplitOpenAfterOperator(node.operators[index], *node.operands[index + 1])) {
                    attached = SolveDelimitedSplitAttachedOpen(*node.operands[index + 1], prefix, continuationIndent);
                    if (nextSuffix != nullptr && attached.valid) {
                        AppendToken(attached, *nextSuffix);
                    }
                }
                AddPrunedResult(next, Better(attached, normal) ? std::move(attached) : std::move(normal));
                if (CompactLiteralFollowerFits(node, index, prefix)) {
                    NodeResult paired = prefix;
                    NodeResult follower = SolveNodeWithoutBreaks(
                        *node.operands[index + 1], paired.endColumn, paired.endIndentLevel, paired.endLineHasText
                    );
                    Merge(paired, follower);
                    if (nextSuffix != nullptr) {
                        AppendToken(paired, *nextSuffix);
                    }
                    AddAttachedChainOperator(paired, node.operators[index]);
                    AddPrunedResult(next, std::move(paired));
                }
            }
            SortPrunedResults(next);
            current = std::move(next);
        }
        return current;
    }

    NodeResult SolveChainSplitAfterOperator(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResult best;
        for (const NodeResult& candidate : SolveChainSplitAfterOperatorAlternatives(
            node, column, indentLevel, lineHasText
        )) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    NodeResult SolveChainSplitBeforeOperator(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        const int splitBaseIndent = node.requiredChainBreakBaseIndent.value_or(indentLevel);
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, splitBaseIndent);
        if (node.operands.empty()) {
            return result;
        }

        NodeResult receiver =
            Solve(*node.operands.front(), result.endColumn, result.endIndentLevel, result.endLineHasText);
        Merge(result, receiver);
        for (size_t index = 0; index < node.operators.size(); ++index) {
            AppendBreak(result, splitBaseIndent + 1, node.breakCost);
            AppendCommentsBeforeChainOperator(node, index, result);
            AppendToken(result, node.operators[index]);
            NodeResult operand =
                Solve(*node.operands[index + 1], result.endColumn, result.endIndentLevel, result.endLineHasText);
            Merge(result, operand);
        }
        return result;
    }

    NodeResult SolveMemberCompactTail(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::MemberCompactTail, indentLevel);
        if (node.operands.empty()) {
            return result;
        }

        NodeResult receiver =
            Solve(*node.operands.front(), result.endColumn, result.endIndentLevel, result.endLineHasText);
        if (!receiver.valid) {
            return {};
        }
        Merge(result, receiver);
        AppendBreak(result, indentLevel + 1, node.breakCost);
        for (size_t index = 0; index < node.operators.size(); ++index) {
            AppendCommentsBeforeChainOperator(node, index, result);
            AppendToken(result, node.operators[index]);
            const bool finalOperand = index + 1 == node.operands.size() - 1;
            NodeResult operand = finalOperand ?
                Solve(*node.operands[index + 1], result.endColumn, result.endIndentLevel, result.endLineHasText) :
                SolveNodeWithoutBreaks(
                    *node.operands[index + 1], result.endColumn, result.endIndentLevel, result.endLineHasText
                );
            if (!operand.valid) {
                return {};
            }
            Merge(result, operand);
        }
        return result;
    }

    NodeResult
        SolveCallApplicationCompactTail(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::CallCompactTail, indentLevel);
        if (node.operands.empty()) {
            return result;
        }

        NodeResult receiver =
            Solve(*node.operands.front(), result.endColumn, result.endIndentLevel, result.endLineHasText);
        if (!receiver.valid) {
            return {};
        }
        Merge(result, receiver);
        AppendBreak(result, indentLevel + 1, node.breakCost);
        for (size_t index = 1; index < node.operands.size(); ++index) {
            const bool finalOperand = index + 1 == node.operands.size();
            NodeResult operand = finalOperand ?
                Solve(*node.operands[index], result.endColumn, result.endIndentLevel, result.endLineHasText) :
                SolveNodeWithoutBreaks(
                    *node.operands[index], result.endColumn, result.endIndentLevel, result.endLineHasText
                );
            if (!operand.valid) {
                return {};
            }
            Merge(result, operand);
        }
        return result;
    }

    NodeResult SolveCallApplicationSplit(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, indentLevel);
        if (node.operands.empty()) {
            return result;
        }

        NodeResult receiver =
            Solve(*node.operands.front(), result.endColumn, result.endIndentLevel, result.endLineHasText);
        if (!receiver.valid) {
            return {};
        }
        Merge(result, receiver);
        for (size_t index = 1; index < node.operands.size(); ++index) {
            AppendBreak(result, indentLevel + 1, node.breakCost);
            NodeResult operand =
                Solve(*node.operands[index], result.endColumn, result.endIndentLevel, result.endLineHasText);
            if (!operand.valid) {
                return {};
            }
            Merge(result, operand);
        }
        return result;
    }

    NodeResults SolveStreamSplitAlternatives(
        const FormatBreakNode& node, int column, int indentLevel, bool lineHasText, FormatBreakChoice choice
    ) {
        const int splitBaseIndent = node.requiredChainBreakBaseIndent.value_or(indentLevel);
        NodeResult
            initial{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(initial, node.id, choice, splitBaseIndent);
        NodeResults current{initial};
        if (!node.chainStartsWithOperator) {
            NodeResults next;
            for (const NodeResult& prefix : current) {
                for (const NodeResult& receiver : SolveAlternatives(
                    *node.operands.front(), prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText
                )) {
                    if (!receiver.valid) {
                        continue;
                    }
                    NodeResult candidate = prefix;
                    Merge(candidate, receiver);
                    AddPrunedResult(next, std::move(candidate));
                }
            }
            SortPrunedResults(next);
            current = std::move(next);
        }
        for (NodeResult& prefix : current) {
            if (node.chainStartsWithOperator && !prefix.endLineHasText) {
                prefix.endIndentLevel = splitBaseIndent + 1;
                prefix.endColumn = IndentColumn(prefix.endIndentLevel);
            } else {
                AppendBreak(prefix, splitBaseIndent + 1, node.breakCost);
            }
        }
        for (size_t index = 0; index < node.operators.size(); ++index) {
            NodeResults next;
            for (const NodeResult& prefix : current) {
                NodeResult withOperator = prefix;
                const bool attachedLiteralFollower = withOperator.compactNextStreamOperand;
                withOperator.compactNextStreamOperand = false;
                AppendCommentsBeforeChainOperator(node, index, withOperator);
                AppendToken(withOperator, node.operators[index]);
                NodeResults compactTailOperands;
                const NodeResults* operands = nullptr;
                if (choice == FormatBreakChoice::StreamCompactTail || attachedLiteralFollower) {
                    compactTailOperands.push_back(SolveNodeWithoutBreaks(
                        *node.operands[index + 1],
                        withOperator.endColumn,
                        withOperator.endIndentLevel,
                        withOperator.endLineHasText
                    ));
                    operands = &compactTailOperands;
                } else {
                    operands = &SolveAlternatives(
                        *node.operands[index + 1],
                        withOperator.endColumn,
                        withOperator.endIndentLevel,
                        withOperator.endLineHasText
                    );
                }
                for (const NodeResult& operand : *operands) {
                    if (!operand.valid) {
                        continue;
                    }
                    NodeResult candidate = withOperator;
                    Merge(candidate, operand);
                    if (
                        choice == FormatBreakChoice::Split &&
                        index + 1 < node.operators.size() &&
                        !IsFormatBreakStreamConfigurationOperand(
                            *node.operands[index + 1], config_.streamShiftConfigurationMethods
                        )
                    ) {
                        if (CompactLiteralFollowerFits(node, index + 1, candidate)) {
                            NodeResult attached = candidate;
                            AddAttachedChainOperator(attached, node.operators[index + 1]);
                            attached.compactNextStreamOperand = true;
                            AddPrunedResult(next, std::move(attached));
                        }
                        AppendBreak(candidate, splitBaseIndent + 1, node.breakCost);
                    }
                    AddPrunedResult(next, std::move(candidate));
                }
            }
            SortPrunedResults(next);
            current = std::move(next);
        }
        return current;
    }

    NodeResult SolveStreamSplit(
        const FormatBreakNode& node, int column, int indentLevel, bool lineHasText, FormatBreakChoice choice
    ) {
        NodeResult best;
        for (
            const NodeResult& candidate : SolveStreamSplitAlternatives(node, column, indentLevel, lineHasText, choice)
        ) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    NodeResults
        SolveTernaryChainSplitAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        const int splitBaseIndent = node.requiredChainBreakBaseIndent.value_or(indentLevel);
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, splitBaseIndent);
        NodeResults current{result};
        for (size_t index = 0; index < node.operands.size(); ++index) {
            NodeResults next;
            for (const NodeResult& prefix : current) {
                for (const NodeResult& operand : SolveAlternatives(
                    *node.operands[index], prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText
                )) {
                    NodeResult candidate = prefix;
                    Merge(candidate, operand);
                    if (index < node.operators.size()) {
                        AppendToken(candidate, node.operators[index]);
                        if (
                            FormatBreakTokenKind(node.operators[index]) == PrintTokenKind::Known &&
                            FormatBreakTokenSyntaxKind(node.operators[index]) == SyntaxNodeKind::Colon
                        ) {
                            AppendBreak(candidate, splitBaseIndent + 1, node.breakCost);
                        }
                    }
                    AddPrunedResult(next, std::move(candidate));
                }
            }
            SortPrunedResults(next);
            current = std::move(next);
        }
        return current;
    }

    NodeResult SolveTernaryChainSplit(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult best;
        for (const NodeResult& candidate : SolveTernaryChainSplitAlternatives(node, column, indentLevel, lineHasText)) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    NodeResults SolveSingleTernaryAlternatives(
        const FormatBreakNode& node, int column, int indentLevel, bool lineHasText, FormatBreakChoice choice
    ) {
        const int splitBaseIndent = node.requiredChainBreakBaseIndent.value_or(indentLevel);
        const bool breakAfterQuestion =
            choice == FormatBreakChoice::TernaryBreakAfterQuestion || choice == FormatBreakChoice::Split;
        const bool breakAfterColon =
            choice == FormatBreakChoice::TernaryBreakAfterColon || choice == FormatBreakChoice::Split;
        const int continuationIndent = node.flatSplitIndent ? splitBaseIndent : splitBaseIndent + 1;
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, choice, splitBaseIndent);
        NodeResults current{result};
        for (size_t index = 0; index < node.operands.size(); ++index) {
            NodeResults next;
            for (const NodeResult& prefix : current) {
                for (const NodeResult& operand : SolveAlternatives(
                    *node.operands[index], prefix.endColumn, prefix.endIndentLevel, prefix.endLineHasText
                )) {
                    NodeResult candidate = prefix;
                    Merge(candidate, operand);
                    if (index < node.operators.size()) {
                        AppendToken(candidate, node.operators[index]);
                        if ((index == 0 && breakAfterQuestion) || (index == 1 && breakAfterColon)) {
                            AppendBreak(candidate, continuationIndent, node.breakCost);
                        }
                    }
                    AddPrunedResult(next, std::move(candidate));
                }
            }
            SortPrunedResults(next);
            current = std::move(next);
        }
        return current;
    }

    NodeResult SolveSingleTernary(
        const FormatBreakNode& node, int column, int indentLevel, bool lineHasText, FormatBreakChoice choice
    ) {
        NodeResult best;
        for (
            const NodeResult& candidate : SolveSingleTernaryAlternatives(node, column, indentLevel, lineHasText, choice)
        ) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    NodeResult SolveChain(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        if (node.forceSplit) {
            if (node.chainKind == FormatBreakChainKind::StreamBeforeOperator) {
                return SolveStreamSplit(node, column, indentLevel, lineHasText, FormatBreakChoice::Split);
            }
            if (node.chainKind == FormatBreakChainKind::CallApplication) {
                return SolveCallApplicationSplit(node, column, indentLevel, lineHasText);
            }
            if (node.chainKind == FormatBreakChainKind::MemberBeforeOperator) {
                return SolveChainSplitBeforeOperator(node, column, indentLevel, lineHasText);
            }
            if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() > 2) {
                return SolveTernaryChainSplit(node, column, indentLevel, lineHasText);
            }
            if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() == 2) {
                return SolveSingleTernary(node, column, indentLevel, lineHasText, FormatBreakChoice::Split);
            }
            return SolveChainSplitAfterOperator(node, column, indentLevel, lineHasText);
        }
        if (node.ternaryRequiresQuestionBreak || node.ternaryRequiresColonBreaks) {
            if (node.operators.size() > 2) {
                return SolveTernaryChainSplit(node, column, indentLevel, lineHasText);
            }
            if (node.operators.size() == 2) {
                const FormatBreakChoice choice =
                    node.ternaryRequiresQuestionBreak && node.ternaryRequiresColonBreaks ? FormatBreakChoice::Split :
                        node.ternaryRequiresQuestionBreak ? FormatBreakChoice::TernaryBreakAfterQuestion :
                        FormatBreakChoice::TernaryBreakAfterColon;
                return SolveSingleTernary(node, column, indentLevel, lineHasText, choice);
            }
        }
        NodeResult compact = SolveChainCompact(node, column, indentLevel, lineHasText);
        if (
            compact.valid &&
            compact.extraLines > 0 &&
            RequiresChainCompactExtraLinesGuard(node) &&
            !CanKeepChainCompactWithExtraLines(node, compact)
        ) {
            compact = {};
        }
        if (node.chainKind == FormatBreakChainKind::StreamBeforeOperator) {
            NodeResult compactTail =
                SolveStreamSplit(node, column, indentLevel, lineHasText, FormatBreakChoice::StreamCompactTail);
            NodeResult split = SolveStreamSplit(node, column, indentLevel, lineHasText, FormatBreakChoice::Split);
            NodeResult best = Better(compactTail, compact) ? compactTail : compact;
            if (compact.valid && best.valid && CompactLineEndsOverLimit(compact) && !HasOverflow(best)) {
                if (compactTail.valid && !HasOverflow(compactTail)) {
                    return compactTail;
                }
                return best;
            }
            return Better(split, best) ? split : best;
        }
        if (node.chainKind == FormatBreakChainKind::CallApplication) {
            NodeResult compactTail = SolveCallApplicationCompactTail(node, column, indentLevel, lineHasText);
            NodeResult split = SolveCallApplicationSplit(node, column, indentLevel, lineHasText);
            NodeResult best = Better(compactTail, compact) ? compactTail : compact;
            if (compact.valid && best.valid && CompactLineEndsOverLimit(compact) && !HasOverflow(best)) {
                return best;
            }
            return Better(split, best) ? split : best;
        }
        if (node.chainKind == FormatBreakChainKind::MemberBeforeOperator) {
            NodeResult compactTail = SolveMemberCompactTail(node, column, indentLevel, lineHasText);
            NodeResult split = SolveChainSplitBeforeOperator(node, column, indentLevel, lineHasText);
            NodeResult best = Better(compactTail, compact) ? compactTail : compact;
            if (node.chainPrefersSplitWhenCompactBreaks && compact.valid && compact.extraLines > 0 && split.valid) {
                return split;
            }
            if (compact.valid && best.valid && CompactLineEndsOverLimit(compact) && !HasOverflow(best)) {
                return best;
            }
            return Better(split, best) ? split : best;
        }
        if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() > 2) {
            NodeResult split = SolveTernaryChainSplit(node, column, indentLevel, lineHasText);
            return Better(split, compact) ? split : compact;
        }
        if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() == 2) {
            NodeResult best = compact;
            NodeResults alternatives{
                SolveSingleTernary(
                    node, column, indentLevel, lineHasText, FormatBreakChoice::TernaryBreakAfterQuestion
                ),
                SolveSingleTernary(node, column, indentLevel, lineHasText, FormatBreakChoice::TernaryBreakAfterColon),
                SolveSingleTernary(node, column, indentLevel, lineHasText, FormatBreakChoice::Split),
            };
            for (const NodeResult& alternative : alternatives) {
                if (Better(alternative, best)) {
                    best = alternative;
                }
            }
            if (compact.valid && best.valid && CompactLineEndsOverLimit(compact) && !HasOverflow(best)) {
                return best;
            }
            return best;
        }
        NodeResult split = SolveChainSplitAfterOperator(node, column, indentLevel, lineHasText);
        if (
            node.chainCompactRequiresFitOnOneLine &&
            compact.valid &&
            (compact.extraLines > 0 || HasOverflow(compact)) &&
            split.valid
        ) {
            return split;
        }
        if (node.chainPrefersSplitWhenCompactBreaks && compact.valid && compact.extraLines > 0 && split.valid) {
            return split;
        }
        if (compact.valid && split.valid && CompactLineEndsOverLimit(compact) && !HasOverflow(split)) {
            return split;
        }
        return Better(split, compact) ? split : compact;
    }

    NodeResult SolveAdjacentStringsCompact(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Compact, indentLevel);
        if (node.compactStringTexts.size() != node.operands.size()) {
            return {};
        }
        for (size_t index = 0; index < node.operands.size(); ++index) {
            if (node.compactStringTexts[index].empty()) {
                continue;
            }
            const FormatBreakNode* operand = node.operands[index];
            if (operand == nullptr || operand->kind != FormatBreakNodeKind::Token) {
                return {};
            }
            NodeResult item = SolveTokenText(
                operand->token,
                node.compactStringTexts[index],
                result.endColumn,
                result.endIndentLevel,
                result.endLineHasText
            );
            Merge(result, item);
        }
        return result;
    }

    NodeResult SolveAdjacentStringsSplit(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, indentLevel);
        const int continuationIndent = node.flatSplitIndent ? indentLevel : indentLevel + 1;
        for (size_t index = 0; index < node.operands.size(); ++index) {
            if (index > 0) {
                AppendBreak(result, continuationIndent, node.breakCost);
            }
            NodeResult item =
                Solve(*node.operands[index], result.endColumn, result.endIndentLevel, result.endLineHasText);
            Merge(result, item);
        }
        return result;
    }

    NodeResult SolveAdjacentStrings(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult compact = SolveAdjacentStringsCompact(node, column, indentLevel, lineHasText);
        NodeResult split = SolveAdjacentStringsSplit(node, column, indentLevel, lineHasText);
        if (node.forceSplit && split.valid) {
            return split;
        }
        return Better(split, compact) ? split : compact;
    }
};

}  // namespace

FormatBreakSolution SolveFormatBreaks(
    const FormatterConfig& config,
    const FormatBreakModel& model,
    int startColumn,
    int indentLevel,
    int indentWidth,
    int breakLineSuffixWidth
) {
    FormatBreakSolution solution;
    if (!model.root) {
        return solution;
    }
    Solver solver(config, model, indentWidth, breakLineSuffixWidth);
    NodeResult result = solver.Solve(*model.root, startColumn, indentLevel, startColumn > indentLevel * indentWidth);
    if (!result.valid) {
        return solution;
    }
    const size_t choiceCount = model.nodes == nullptr ? 0 : model.nodes->size() + 1;
    return FormatChoiceHistory::Materialize(result.choices, choiceCount);
}

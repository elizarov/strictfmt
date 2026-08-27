#include "format/impl/format_break_solver.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "format/impl/format_break_model_inline_helpers.h"

namespace {

struct SolveKey {
    int nodeId = 0;
    int column = 0;
    int indentLevel = 0;
    bool lineHasText = false;

    friend bool operator==(const SolveKey& left, const SolveKey& right) {
        return left.nodeId == right.nodeId &&
            left.column == right.column &&
            left.indentLevel == right.indentLevel &&
            left.lineHasText == right.lineHasText;
    }
};

struct SolveKeyHash {
    size_t operator()(const SolveKey& key) const {
        size_t hash = static_cast<size_t>(key.nodeId);
        hash = hash * 131u + static_cast<size_t>(key.column);
        hash = hash * 131u + static_cast<size_t>(key.indentLevel);
        hash = hash * 2u + static_cast<size_t>(key.lineHasText);
        return hash;
    }
};

struct NodeResult {
    bool valid = false;
    int endColumn = 0;
    int endIndentLevel = 0;
    bool endLineHasText = false;
    int extraLines = 0;
    int maxOverflow = 0;
    int overflowLines = 0;
    int deepestBreakDepth = -1;
    int totalBreakDepth = 0;
    const struct ChoiceTree* choices = nullptr;
};

class NodeResults {
public:
    using iterator = NodeResult*;
    using const_iterator = const NodeResult*;

    NodeResults() = default;

    NodeResults(std::initializer_list<NodeResult> values) {
        for (NodeResult value : values) {
            push_back(std::move(value));
        }
    }

    NodeResults(const NodeResults& other) {
        for (const NodeResult& value : other) {
            push_back(value);
        }
    }

    NodeResults(NodeResults&& other) noexcept {
        MoveFrom(std::move(other));
    }

    ~NodeResults() {
        clear();
    }

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

    iterator begin() {
        return usingHeap_ ? heap_.data() : InlineData();
    }

    iterator end() {
        return begin() + size();
    }

    const_iterator begin() const {
        return usingHeap_ ? heap_.data() : InlineData();
    }

    const_iterator end() const {
        return begin() + size();
    }

    bool empty() const {
        return size() == 0;
    }

    size_t size() const {
        return usingHeap_ ? heap_.size() : inlineSize_;
    }

    NodeResult& operator[](size_t index) {
        return begin()[index];
    }

    const NodeResult& operator[](size_t index) const {
        return begin()[index];
    }

    void push_back(NodeResult value) {
        if (usingHeap_) {
            heap_.push_back(std::move(value));
            return;
        }
        if (inlineSize_ < kInlineCapacity) {
            std::construct_at(InlineData() + inlineSize_, std::move(value));
            ++inlineSize_;
            return;
        }
        MoveInlineToHeap();
        heap_.push_back(std::move(value));
    }

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

    NodeResult* InlineData() {
        return std::launder(reinterpret_cast<NodeResult*>(inlineStorage_));
    }

    const NodeResult* InlineData() const {
        return std::launder(reinterpret_cast<const NodeResult*>(inlineStorage_));
    }

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

struct ChoiceTree {
    const ChoiceTree* left = nullptr;
    const ChoiceTree* right = nullptr;
    int nodeId = 0;
    int indentLevel = -1;
    int declarationValueContinuationLines = -1;
    FormatBreakChoice choice = FormatBreakChoice::Compact;
    bool leaf = false;
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
    Solver(const FormatterConfig& config, int indentWidth, int breakLineSuffixWidth) :
        config_(config),
        indentWidth_(indentWidth),
        breakLineSuffixWidth_(breakLineSuffixWidth) {}

    NodeResult Solve(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        const SolveKey key{node.id, column, indentLevel, lineHasText};
        const auto found = memo_.find(key);
        if (found != memo_.end()) {
            return found->second;
        }

        if (std::optional<NodeResult> compact = SolveCompactOneLine(node, column, indentLevel, lineHasText)) {
            memo_.emplace(key, *compact);
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
        memo_.emplace(key, result);
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
    std::unordered_map<SolveKey, NodeResult, SolveKeyHash> memo_;
    std::deque<ChoiceTree> choiceArena_;
    std::deque<DelimiterStackPartitionPath> delimiterStackPartitionPathArena_;

    int IndentColumn(int indentLevel) const {
        return std::max(0, indentLevel) * indentWidth_;
    }

    static bool HasTrailingComment(const FormatBreakNode& node, size_t index) {
        return index < node.items.size() && IsCommentToken(FormatBreakTokenKind(node.items[index].trailingComment));
    }

    static bool HasLeadingTrailingComment(const FormatBreakNode& node) {
        return IsCommentToken(FormatBreakTokenKind(node.leadingTrailingComment));
    }

    static bool HasBlankLineBeforeItem(const FormatBreakNode& node, size_t index) {
        return index < node.items.size() && node.items[index].blankLineBefore;
    }

    static int SpaceBeforeToken(const FormatBreakToken& token, bool lineHasText) {
        if (!lineHasText) {
            return 0;
        }
        if (IsCommentToken(FormatBreakTokenKind(token))) {
            return 2;
        }
        return token.spaceBefore ? 1 : 0;
    }

    NodeResult SolveTokenText(
        const FormatBreakToken& token,
        std::string_view text,
        int column,
        int indentLevel,
        bool lineHasText
    ) const {
        if (token.contextOnly) {
            return {.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        }
        const int space = SpaceBeforeToken(token, lineHasText);
        NodeResult result{
            .valid = true,
            .endColumn = column + space,
            .endIndentLevel = indentLevel,
            .endLineHasText = lineHasText
        };
        bool lineWasOverLimit = lineHasText && column > config_.columnLimit;
        for (size_t index = 0; index < text.size(); ++index) {
            if (text[index] == '\r' && index + 1 < text.size() && text[index + 1] == '\n') {
                continue;
            }
            if (text[index] == '\n') {
                const bool lineIsOverLimit = result.endLineHasText && result.endColumn > config_.columnLimit;
                result.maxOverflow = std::max(result.maxOverflow, result.endColumn - config_.columnLimit);
                if (!lineWasOverLimit && lineIsOverLimit) {
                    ++result.overflowLines;
                }
                ++result.extraLines;
                result.endColumn = 0;
                result.endLineHasText = false;
                lineWasOverLimit = false;
                continue;
            }
            ++result.endColumn;
            result.endLineHasText = true;
        }
        const bool lineIsOverLimit = result.endLineHasText && result.endColumn > config_.columnLimit;
        result.maxOverflow = std::max(result.maxOverflow, result.endColumn - config_.columnLimit);
        if (!lineWasOverLimit && lineIsOverLimit) {
            ++result.overflowLines;
        }
        if (IsCommentToken(FormatBreakTokenKind(token))) {
            ++result.extraLines;
            result.endColumn = IndentColumn(indentLevel);
            result.endLineHasText = false;
        }
        return result;
    }

    NodeResult SolveToken(const FormatBreakToken& token, int column, int indentLevel, bool lineHasText) const {
        return SolveTokenText(token, FormatTokenText(FormatBreakTokenValue(token)), column, indentLevel, lineHasText);
    }

    const ChoiceTree* MakeChoice(int nodeId, FormatBreakChoice choice, int indentLevel) {
        choiceArena_
            .push_back(ChoiceTree{.nodeId = nodeId, .indentLevel = indentLevel, .choice = choice, .leaf = true});
        return &choiceArena_.back();
    }

    const ChoiceTree* ConcatChoices(const ChoiceTree* left, const ChoiceTree* right) {
        if (left == nullptr) {
            return right;
        }
        if (right == nullptr) {
            return left;
        }
        choiceArena_.push_back(ChoiceTree{.left = left, .right = right});
        return &choiceArena_.back();
    }

    void AddChoice(NodeResult& result, int nodeId, FormatBreakChoice choice, int indentLevel = -1) {
        result.choices = ConcatChoices(result.choices, MakeChoice(nodeId, choice, indentLevel));
    }

    void AddDeclarationValueContinuationLines(NodeResult& result, int nodeId, int continuationLines) {
        choiceArena_.push_back(
            ChoiceTree{.nodeId = nodeId, .declarationValueContinuationLines = continuationLines, .leaf = true}
        );
        result.choices = ConcatChoices(result.choices, &choiceArena_.back());
    }

    void Merge(NodeResult& left, const NodeResult& right) {
        left.valid = left.valid && right.valid;
        left.endColumn = right.endColumn;
        left.endIndentLevel = right.endIndentLevel;
        left.endLineHasText = right.endLineHasText;
        left.extraLines += right.extraLines;
        left.maxOverflow = std::max(left.maxOverflow, right.maxOverflow);
        left.overflowLines += right.overflowLines;
        left.deepestBreakDepth = std::max(left.deepestBreakDepth, right.deepestBreakDepth);
        left.totalBreakDepth += right.totalBreakDepth;
        left.choices = ConcatChoices(left.choices, right.choices);
    }

    NodeResults SolveAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
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
                if (CanSkipDelimitedCompact(node, split, column, indentLevel, lineHasText)) {
                    return {split};
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
                if (split.valid) {
                    alternatives.push_back(split);
                }
                return alternatives;
            }
            case FormatBreakNodeKind::PrefixList: {
                NodeResults alternatives;
                for (NodeResult compact : SolvePrefixListCompactAlternatives(node, column, indentLevel, lineHasText)) {
                    if (!node.forceSplit && !(compact.valid && compact.extraLines > 0)) {
                        alternatives.push_back(std::move(compact));
                    }
                }
                for (NodeResult split : SolvePrefixListSplitAlternatives(node, column, indentLevel, lineHasText)) {
                    if (split.valid) {
                        alternatives.push_back(std::move(split));
                    }
                }
                return alternatives;
            }
            case FormatBreakNodeKind::StatementSequence: {
                NodeResults alternatives;
                NodeResult compact = SolveStatementSequenceCompact(node, column, indentLevel, lineHasText);
                if (!node.forceSplit && !(compact.valid && compact.extraLines > 0)) {
                    alternatives.push_back(compact);
                }
                NodeResult split = SolveStatementSequenceSplit(node, column, indentLevel, lineHasText);
                if (split.valid) {
                    alternatives.push_back(split);
                }
                return alternatives;
            }
            case FormatBreakNodeKind::FunctionSignature:
                return SolveFunctionSignatureAlternatives(node, column, indentLevel, lineHasText);
            case FormatBreakNodeKind::BodyHeader:
                return SolveBodyHeaderAlternatives(node, column, indentLevel, lineHasText);
            case FormatBreakNodeKind::Chain: {
                NodeResults alternatives;
                for (NodeResult compact : SolveChainCompactAlternatives(node, column, indentLevel, lineHasText)) {
                    if (node.chainCompactRequiresFitOnOneLine && (compact.extraLines > 0 || compact.maxOverflow > 0)) {
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
                        node,
                        column,
                        indentLevel,
                        lineHasText,
                        FormatBreakChoice::StreamCompactTail
                    )) {
                        alternatives.push_back(std::move(candidate));
                    }
                    for (NodeResult candidate : SolveStreamSplitAlternatives(
                        node,
                        column,
                        indentLevel,
                        lineHasText,
                        FormatBreakChoice::Split
                    )) {
                        alternatives.push_back(std::move(candidate));
                    }
                } else if (node.chainKind == FormatBreakChainKind::MemberBeforeOperator) {
                    alternatives.push_back(SolveMemberCompactTail(node, column, indentLevel, lineHasText));
                    alternatives.push_back(SolveChainSplitBeforeOperator(node, column, indentLevel, lineHasText));
                } else if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() > 2) {
                    alternatives.push_back(SolveTernaryChainSplit(node, column, indentLevel, lineHasText));
                } else if (node.chainKind == FormatBreakChainKind::Ternary && node.operators.size() == 2) {
                    alternatives.push_back(SolveSingleTernary(
                        node,
                        column,
                        indentLevel,
                        lineHasText,
                        FormatBreakChoice::TernaryBreakAfterQuestion
                    ));
                    alternatives.push_back(SolveSingleTernary(
                        node,
                        column,
                        indentLevel,
                        lineHasText,
                        FormatBreakChoice::TernaryBreakAfterColon
                    ));
                    alternatives.push_back(
                        SolveSingleTernary(node, column, indentLevel, lineHasText, FormatBreakChoice::Split)
                    );
                } else {
                    alternatives.push_back(SolveChainSplitAfterOperator(node, column, indentLevel, lineHasText));
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
                    alternatives.push_back(split);
                }
                return alternatives;
            }
        }
        return {};
    }

    NodeResult
        SolveChildren(std::span<FormatBreakNode* const> children, int column, int indentLevel, bool lineHasText)
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
        std::span<FormatBreakNode* const> children,
        int column,
        int indentLevel,
        bool lineHasText
    ) {
        std::vector<const FormatBreakNode*> sequenceChildren;
        AppendSequenceChildren(children, sequenceChildren);
        NodeResults
            current{{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText}};
        for (const FormatBreakNode* child : sequenceChildren) {
            NodeResults next;
            for (const NodeResult& prefix : current) {
                for (const NodeResult& childResult : SolveAlternatives(
                    *child,
                    prefix.endColumn,
                    prefix.endIndentLevel,
                    prefix.endLineHasText
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

    NodeResult AddBreak(NodeResult result, int indentLevel, int structuralDepth) const {
        // Some formats add text only when a line actually breaks. Account for that physical suffix here so it
        // participates in the same DP cost as ordinary tokens without inventing a printer-side break decision.
        if (result.endLineHasText && breakLineSuffixWidth_ > 0) {
            const bool lineWasOverLimit = result.endColumn > config_.columnLimit;
            const int suffixedColumn = result.endColumn + breakLineSuffixWidth_;
            const bool lineIsOverLimit = suffixedColumn > config_.columnLimit;
            result.maxOverflow = std::max(result.maxOverflow, suffixedColumn - config_.columnLimit);
            if (!lineWasOverLimit && lineIsOverLimit) {
                ++result.overflowLines;
            }
        }
        ++result.extraLines;
        result.endIndentLevel = indentLevel;
        result.endColumn = IndentColumn(indentLevel);
        result.endLineHasText = false;
        result.deepestBreakDepth = std::max(result.deepestBreakDepth, structuralDepth);
        result.totalBreakDepth += structuralDepth;
        return result;
    }

    NodeResult AddListBreak(NodeResult result, int indentLevel, int structuralDepth, bool blankLine) const {
        result = AddBreak(result, indentLevel, structuralDepth);
        if (blankLine) {
            result = AddBreak(result, indentLevel, structuralDepth);
        }
        return result;
    }

    NodeResult AddListBreakAfterOptionalComment(
        NodeResult result,
        int indentLevel,
        int structuralDepth,
        bool blankLine,
        bool commentTerminatedLine
    ) const {
        if (!commentTerminatedLine) {
            return AddListBreak(result, indentLevel, structuralDepth, blankLine);
        }
        result.endIndentLevel = indentLevel;
        result.endColumn = IndentColumn(indentLevel);
        result.endLineHasText = false;
        return blankLine ? AddBreak(result, indentLevel, structuralDepth) : result;
    }

    NodeResult AddToken(NodeResult result, const FormatBreakToken& token) {
        NodeResult tokenResult = SolveToken(token, result.endColumn, result.endIndentLevel, result.endLineHasText);
        Merge(result, tokenResult);
        return result;
    }

    NodeResults SolveListItemWithSuffixAlternatives(
        const FormatBreakListItem& listItem,
        int column,
        int indentLevel,
        bool lineHasText
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
                item = AddToken(item, listItem.separator);
            }
            if (IsCommentToken(FormatBreakTokenKind(listItem.trailingComment))) {
                item = AddToken(item, listItem.trailingComment);
            }
            AddPrunedResult(alternatives, std::move(item));
        }
        SortPrunedResults(alternatives);
        return alternatives;
    }

    NodeResult
        SolveListItemWithSuffix(const FormatBreakListItem& listItem, int column, int indentLevel, bool lineHasText)
    {
        NodeResult best;
        for (const NodeResult& item : SolveListItemWithSuffixAlternatives(listItem, column, indentLevel, lineHasText)) {
            if (Better(item, best)) {
                best = item;
            }
        }
        return best;
    }

    NodeResult SolveNodeWithSuffix(
        const FormatBreakNode& node,
        const FormatBreakToken* suffix,
        int column,
        int indentLevel,
        bool lineHasText
    ) {
        NodeResult best;
        for (NodeResult candidate : SolveAlternatives(node, column, indentLevel, lineHasText)) {
            if (!candidate.valid) {
                continue;
            }
            if (suffix != nullptr && FormatBreakTokenKind(*suffix) == PrintTokenKind::Known) {
                candidate = AddToken(candidate, *suffix);
            }
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    NodeResult SolveNodeWithoutBreaks(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
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
        if (candidate.maxOverflow != incumbent.maxOverflow) {
            return candidate.maxOverflow < incumbent.maxOverflow;
        }
        if (candidate.overflowLines != incumbent.overflowLines) {
            return candidate.overflowLines < incumbent.overflowLines;
        }
        if (candidate.extraLines != incumbent.extraLines) {
            return candidate.extraLines < incumbent.extraLines;
        }
        if (candidate.deepestBreakDepth != incumbent.deepestBreakDepth) {
            return candidate.deepestBreakDepth < incumbent.deepestBreakDepth;
        }
        if (candidate.totalBreakDepth != incumbent.totalBreakDepth) {
            return candidate.totalBreakDepth < incumbent.totalBreakDepth;
        }
        return false;
    }

    static bool SameResultState(const NodeResult& left, const NodeResult& right) {
        return left.endColumn == right.endColumn &&
            left.endIndentLevel == right.endIndentLevel &&
            left.endLineHasText == right.endLineHasText;
    }

    static bool DominatesResult(const NodeResult& left, const NodeResult& right) {
        if (left.endIndentLevel != right.endIndentLevel || left.endLineHasText != right.endLineHasText) {
            return false;
        }
        if (left.endColumn > right.endColumn) {
            return false;
        }
        if (
            left.maxOverflow > right.maxOverflow ||
            left.overflowLines > right.overflowLines ||
            left.extraLines > right.extraLines ||
            left.deepestBreakDepth > right.deepestBreakDepth ||
            left.totalBreakDepth > right.totalBreakDepth
        ) {
            return false;
        }
        return left.maxOverflow < right.maxOverflow ||
            left.overflowLines < right.overflowLines ||
            left.extraLines < right.extraLines ||
            left.deepestBreakDepth < right.deepestBreakDepth ||
            left.totalBreakDepth < right.totalBreakDepth;
    }

    static bool ResultStateLess(const NodeResult& left, const NodeResult& right) {
        if (left.endColumn != right.endColumn) {
            return left.endColumn < right.endColumn;
        }
        if (left.endIndentLevel != right.endIndentLevel) {
            return left.endIndentLevel < right.endIndentLevel;
        }
        return left.endLineHasText < right.endLineHasText;
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
        const DelimiterStackPartitionPath* candidate,
        const DelimiterStackPartitionPath* incumbent
    ) {
        const std::vector<size_t> candidateStarts = DelimiterStackRunStarts(candidate);
        const std::vector<size_t> incumbentStarts = DelimiterStackRunStarts(incumbent);
        return std::lexicographical_compare(
            incumbentStarts.begin(),
            incumbentStarts.end(),
            candidateStarts.begin(),
            candidateStarts.end()
        );
    }

    void AddPrunedDelimiterStackPartitionCandidate(
        std::vector<DelimiterStackPartitionCandidate>& candidates,
        DelimiterStackPartitionCandidate candidate
    ) const {
        if (!candidate.result.valid) {
            return;
        }
        for (auto it = candidates.begin(); it != candidates.end();) {
            if (SameResultState(it->result, candidate.result)) {
                if (Better(candidate.result, it->result) || (
                    !Better(it->result, candidate.result) && PreferLaterDelimiterStackBreaks(candidate.path, it->path)
                )) {
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

    static void SortPrunedResults(NodeResults& results) {
        std::sort(results.begin(), results.end(), ResultStateLess);
    }

    bool CompactLineEndsOverLimit(const NodeResult& compact) const {
        return compact.endLineHasText && compact.endColumn > config_.columnLimit;
    }

    std::optional<NodeResult>
        SolveCompactOneLine(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) const
    {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        if (!AppendCompactOneLine(node, result)) {
            return std::nullopt;
        }
        if (result.endLineHasText && result.endColumn > config_.columnLimit) {
            return std::nullopt;
        }
        return result;
    }

    bool AddCompactToken(NodeResult& result, const FormatBreakToken& token) const {
        return AddCompactTokenText(result, token, FormatTokenText(FormatBreakTokenValue(token)));
    }

    bool AddCompactTokenText(NodeResult& result, const FormatBreakToken& token, std::string_view text) const {
        if (token.contextOnly) {
            return true;
        }
        const PrintToken& printToken = FormatBreakTokenValue(token);
        if (
            printToken.kind == PrintTokenKind::BlankLine ||
            IsCommentToken(printToken.kind) ||
            text.find_first_of("\r\n") != std::string_view::npos
        ) {
            return false;
        }
        const int space = SpaceBeforeToken(token, result.endLineHasText);
        const int width = static_cast<int>(text.size());
        result.endColumn += space + width;
        result.endLineHasText = result.endLineHasText || width > 0;
        return !result.endLineHasText || result.endColumn <= config_.columnLimit;
    }

    bool AppendCompactOneLine(const FormatBreakNode& node, NodeResult& result) const {
        if (node.forceSplit) {
            return false;
        }
        switch (node.kind) {
            case FormatBreakNodeKind::Token:
                return AddCompactToken(result, node.token);
            case FormatBreakNodeKind::Sequence:
            case FormatBreakNodeKind::FunctionSignature:
                for (const FormatBreakNode* child : node.children) {
                    if (child != nullptr && !AppendCompactOneLine(*child, result)) {
                        return false;
                    }
                }
                return true;
            case FormatBreakNodeKind::BodyHeader:
                if (node.bodyHeaderRequiresDetachedBody) {
                    return false;
                }
                for (const FormatBreakNode* child : node.children) {
                    if (child != nullptr && !AppendCompactOneLine(*child, result)) {
                        return false;
                    }
                }
                return true;
            case FormatBreakNodeKind::Delimited:
                if (node.children.size() < 2 || !AppendCompactOneLine(*node.children[0], result)) {
                    return false;
                }
                if (HasLeadingTrailingComment(node)) {
                    return false;
                }
                for (size_t index = 0; index < node.items.size(); ++index) {
                    const FormatBreakListItem& item = node.items[index];
                    if (item.node != nullptr && !AppendCompactOneLine(*item.node, result)) {
                        return false;
                    }
                    if (
                        FormatBreakTokenKind(item.separator) == PrintTokenKind::Known &&
                        !AddCompactToken(result, item.separator)
                    ) {
                        return false;
                    }
                    if (HasTrailingComment(node, index)) {
                        return false;
                    }
                }
                return AppendCompactOneLine(*node.children[1], result);
            case FormatBreakNodeKind::PrefixList:
                if (node.children.empty() || !AppendCompactOneLine(*node.children[0], result)) {
                    return false;
                }
                if (HasLeadingTrailingComment(node)) {
                    return false;
                }
                for (size_t index = 0; index < node.items.size(); ++index) {
                    const FormatBreakListItem& item = node.items[index];
                    if (item.node != nullptr && !AppendCompactOneLine(*item.node, result)) {
                        return false;
                    }
                    if (
                        FormatBreakTokenKind(item.separator) == PrintTokenKind::Known &&
                        !AddCompactToken(result, item.separator)
                    ) {
                        return false;
                    }
                    if (HasTrailingComment(node, index)) {
                        return false;
                    }
                }
                return true;
            case FormatBreakNodeKind::StatementSequence:
                for (size_t index = 0; index < node.items.size(); ++index) {
                    const FormatBreakListItem& item = node.items[index];
                    if (item.node != nullptr && !AppendCompactOneLine(*item.node, result)) {
                        return false;
                    }
                    if (
                        FormatBreakTokenKind(item.separator) == PrintTokenKind::Known &&
                        !AddCompactToken(result, item.separator)
                    ) {
                        return false;
                    }
                    if (HasTrailingComment(node, index)) {
                        return false;
                    }
                }
                return true;
            case FormatBreakNodeKind::Chain:
                if (node.chainStartsWithOperator) {
                    return false;
                }
                for (size_t index = 0; index < node.operands.size(); ++index) {
                    if (node.operands[index] != nullptr && !AppendCompactOneLine(*node.operands[index], result)) {
                        return false;
                    }
                    if (index < node.commentsBeforeOperators.size() && !node.commentsBeforeOperators[index].empty()) {
                        return false;
                    }
                    if (index < node.operators.size() && !AddCompactToken(result, node.operators[index])) {
                        return false;
                    }
                }
                return true;
            case FormatBreakNodeKind::AdjacentStrings:
                if (node.compactStringTexts.size() != node.operands.size()) {
                    return false;
                }
                for (size_t index = 0; index < node.operands.size(); ++index) {
                    if (node.compactStringTexts[index].empty()) {
                        continue;
                    }
                    const FormatBreakNode* operand = node.operands[index];
                    if (
                        operand == nullptr ||
                        operand->kind != FormatBreakNodeKind::Token ||
                        !AddCompactTokenText(result, operand->token, node.compactStringTexts[index])
                    ) {
                        return false;
                    }
                }
                return true;
        }
        return false;
    }

    bool DelimitedCompactPrefixRequiresOverflowOrBreak(
        const FormatBreakNode& node,
        int column,
        int indentLevel,
        bool lineHasText
    ) const {
        if (node.children.empty() || node.items.size() < 2) {
            return false;
        }
        NodeResult
            prefix{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        if (!AppendCompactOneLine(*node.children.front(), prefix)) {
            return true;
        }
        for (size_t index = 0; index + 1 < node.items.size(); ++index) {
            const FormatBreakListItem& item = node.items[index];
            if (item.node == nullptr || !AppendCompactOneLine(*item.node, prefix)) {
                return true;
            }
            if (
                FormatBreakTokenKind(item.separator) == PrintTokenKind::Known &&
                !AddCompactToken(prefix, item.separator)
            ) {
                return true;
            }
            if (HasTrailingComment(node, index)) {
                return true;
            }
        }
        return false;
    }

    bool CanSkipDelimitedCompact(
        const FormatBreakNode& node,
        const NodeResult& split,
        int column,
        int indentLevel,
        bool lineHasText
    ) const {
        return split.valid && (node.forceSplit || (
            split.maxOverflow == 0 &&
            DelimitedCompactPrefixRequiresOverflowOrBreak(node, column, indentLevel, lineHasText)
        ));
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
            HasTrailingComment(node, 0) ||
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

    NodeResults SolveDelimitedCompactItemAlternatives(
        const FormatBreakNode& item,
        int column,
        int indentLevel,
        bool lineHasText
    ) {
        if (IsDelimiterStackItem(item)) {
            return {Solve(item, column, indentLevel, lineHasText)};
        }
        return SolveAlternatives(item, column, indentLevel, lineHasText);
    }

    NodeResults
        SolveDelimitedCompactAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Compact, indentLevel);
        result = AddToken(result, node.children[0]->token);
        if (HasLeadingTrailingComment(node)) {
            result = AddToken(result, node.leadingTrailingComment);
        }
        NodeResults current{result};
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& listItem = node.items[index];
            NodeResults nextByState;
            const bool canKeepMultilineItem = node.items.size() == 1 ||
                (index + 1 == node.items.size() && node.delimiterKind != FormatBreakDelimiterKind::Angle);
            for (const NodeResult& prefix : current) {
                for (const NodeResult& item : SolveDelimitedCompactItemAlternatives(
                    *listItem.node,
                    prefix.endColumn,
                    prefix.endIndentLevel,
                    prefix.endLineHasText
                )) {
                    if (!item.valid) {
                        continue;
                    }
                    if (!canKeepMultilineItem && item.extraLines > 0) {
                        continue;
                    }
                    NodeResult next = prefix;
                    Merge(next, item);
                    if (FormatBreakTokenKind(listItem.separator) == PrintTokenKind::Known) {
                        next = AddToken(next, listItem.separator);
                    }
                    if (HasTrailingComment(node, index)) {
                        next = AddToken(next, listItem.trailingComment);
                    }
                    AddPrunedResult(nextByState, std::move(next));
                }
            }
            SortPrunedResults(nextByState);
            current = std::move(nextByState);
        }

        NodeResults alternatives;
        for (const NodeResult& candidate : current) {
            AddPrunedResult(alternatives, AddToken(candidate, node.children[1]->token));
        }
        SortPrunedResults(alternatives);
        return alternatives;
    }

    NodeResult SolveDelimitedCompact(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult best;
        for (const NodeResult& candidate : SolveDelimitedCompactAlternatives(node, column, indentLevel, lineHasText)) {
            if (Better(candidate, best)) {
                best = candidate;
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
        result = AddToken(result, node.children[0]->token);
        if (HasLeadingTrailingComment(node)) {
            result = AddToken(result, node.leadingTrailingComment);
        }
        result = AddListBreakAfterOptionalComment(
            result,
            indentLevel + 1,
            node.structuralDepth,
            HasBlankLineBeforeItem(node, 0),
            HasLeadingTrailingComment(node)
        );
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& listItem = node.items[index];
            NodeResult item =
                SolveListItemWithSuffix(listItem, result.endColumn, result.endIndentLevel, result.endLineHasText);
            Merge(result, item);
            const bool hasNextItem = index + 1 < node.items.size();
            result = AddListBreakAfterOptionalComment(
                result,
                hasNextItem ? indentLevel + 1 : indentLevel,
                node.structuralDepth,
                hasNextItem && HasBlankLineBeforeItem(node, index + 1),
                HasTrailingComment(node, index)
            );
        }
        result = AddToken(result, node.children[1]->token);
        return result;
    }

    NodeResult SolveTransparentDelimiterStackSuffix(
        const DelimiterStackView& stack,
        size_t firstDelimiter,
        int column,
        int indentLevel,
        bool lineHasText
    ) {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        for (size_t index = firstDelimiter; index < stack.delimiters.size(); ++index) {
            const FormatBreakNode* delimiter = stack.delimiters[index];
            AddChoice(result, delimiter->id, FormatBreakChoice::Compact, indentLevel);
            result = AddToken(result, delimiter->children.front()->token);
        }
        NodeResult leaf = Solve(*stack.leaf, result.endColumn, result.endIndentLevel, result.endLineHasText);
        if (!leaf.valid) {
            return {};
        }
        Merge(result, leaf);
        for (size_t index = stack.delimiters.size(); index-- > firstDelimiter;) {
            result = AddToken(result, stack.delimiters[index]->children.back()->token);
        }
        return result;
    }

    NodeResult SolveTransparentDelimiterStackSplit(
        const FormatBreakNode& node,
        const DelimiterStackView& stack,
        int column,
        int indentLevel,
        bool lineHasText
    ) {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, indentLevel);
        result = AddToken(result, node.children.front()->token);
        result = AddListBreak(result, indentLevel + 1, node.structuralDepth, HasBlankLineBeforeItem(node, 0));
        NodeResult suffix = SolveTransparentDelimiterStackSuffix(
            stack,
            1,
            result.endColumn,
            result.endIndentLevel,
            result.endLineHasText
        );
        if (!suffix.valid) {
            return {};
        }
        if (suffix.extraLines > 0) {
            return {};
        }
        Merge(result, suffix);
        result = AddListBreak(result, indentLevel, node.structuralDepth, false);
        result = AddToken(result, node.children.back()->token);
        return result;
    }

    NodeResults SolveTransparentDelimiterStackAlternatives(
        const FormatBreakNode& node,
        const DelimiterStackView& stack,
        int column,
        int indentLevel,
        bool lineHasText
    ) {
        NodeResults alternatives;
        NodeResult compact = SolveTransparentDelimiterStackSuffix(stack, 0, column, indentLevel, lineHasText);
        if (!node.forceSplit && compact.valid && (
            compact.extraLines == 0 ||
            CompactTailExpansion(node, compact) == CompactTailExpansionKind::IntrinsicMultilineLiteral
        )) {
            alternatives.push_back(compact);
        }
        NodeResult attachedLeaf = SolveDelimiterStack(node, stack, column, indentLevel, lineHasText, false);
        if (attachedLeaf.valid) {
            alternatives.push_back(attachedLeaf);
        }
        NodeResult detachedLeaf = SolveDelimiterStack(node, stack, column, indentLevel, lineHasText, true);
        if (detachedLeaf.valid) {
            alternatives.push_back(detachedLeaf);
        }
        return alternatives;
    }

    NodeResult SolveTransparentDelimiterStack(
        const FormatBreakNode& node,
        const DelimiterStackView& stack,
        int column,
        int indentLevel,
        bool lineHasText
    ) {
        NodeResult compact = SolveTransparentDelimiterStackSuffix(stack, 0, column, indentLevel, lineHasText);
        NodeResult attachedLeaf = SolveDelimiterStack(node, stack, column, indentLevel, lineHasText, false);
        NodeResult detachedLeaf = SolveDelimiterStack(node, stack, column, indentLevel, lineHasText, true);
        if (
            compact.extraLines > 0 &&
            DelimitedCompactTailExpansion(node, compact) != CompactTailExpansionKind::IntrinsicMultilineLiteral
        ) {
            compact = {};
        }
        NodeResult best = compact;
        best = Better(attachedLeaf, best) ? attachedLeaf : best;
        return Better(detachedLeaf, best) ? detachedLeaf : best;
    }

    NodeResult SolveDelimited(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        if (std::optional<DelimiterStackView> stack = CollectDelimiterStack(node)) {
            return SolveTransparentDelimiterStack(node, *stack, column, indentLevel, lineHasText);
        }
        NodeResult split = SolveDelimitedSplit(node, column, indentLevel, lineHasText);
        if (CanSkipDelimitedCompact(node, split, column, indentLevel, lineHasText)) {
            return split;
        }
        NodeResult compact = SolveDelimitedCompact(node, column, indentLevel, lineHasText);
        if (
            !lineHasText &&
            compact.valid &&
            split.valid &&
            compact.extraLines > 0 &&
            DelimitedCompactTailExpansion(node, compact) != CompactTailExpansionKind::IntrinsicMultilineLiteral
        ) {
            return split;
        }
        if (compact.valid && split.valid && CompactLineEndsOverLimit(compact) && split.maxOverflow == 0) {
            return split;
        }
        if (compact.valid && split.valid && compact.extraLines > 0 && ContainsForceSplitAdjacentStrings(node)) {
            return split;
        }
        if (compact.valid && split.valid && compact.extraLines > 0 && !CanKeepDelimitedCompactWithExtraLines(
            node,
            compact
        )) {
            return split;
        }
        return Better(split, compact) ? split : compact;
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
        NodeResult& result,
        const DelimiterStackView& stack,
        const DelimiterStackPartitionPath* path
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
        bool detachLeaf,
        bool& delimiterOverflow
    ) {
        delimiterOverflow = false;
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
                result = AddBreak(result, currentLineIndent, node.structuralDepth);
                ++nextOpenIndent;
                AddChoice(result, delimiter->children.front()->id, FormatBreakChoice::SplitDelimiterStackRun);
            }
            if (delimiterRuns.empty() || delimiterRuns.back().indentLevel != currentLineIndent) {
                delimiterRuns
                    .push_back(DelimiterStackRun{.begin = index, .end = index, .indentLevel = currentLineIndent});
            }
            delimiterRuns.back().end = index + 1;
            result = AddToken(result, open);
            delimiterOverflow = delimiterOverflow || result.endColumn + breakLineSuffixWidth_ > config_.columnLimit;
        }

        if (detachLeaf && result.endLineHasText) {
            result = AddBreak(result, nextOpenIndent, node.structuralDepth);
        }
        NodeResult leaf = Solve(*stack.leaf, result.endColumn, result.endIndentLevel, result.endLineHasText);
        if (!leaf.valid || (!detachLeaf && leaf.extraLines > 0)) {
            return {};
        }
        Merge(result, leaf);

        for (size_t runIndex = delimiterRuns.size(); runIndex-- > 0;) {
            const DelimiterStackRun& run = delimiterRuns[runIndex];
            const bool firstClosingRun = runIndex + 1 == delimiterRuns.size();
            if (result.endLineHasText && (detachLeaf || !firstClosingRun)) {
                result = AddBreak(result, run.indentLevel, node.structuralDepth);
            }
            for (size_t index = run.end; index-- > run.begin;) {
                result = AddToken(result, stack.delimiters[index]->children.back()->token);
                delimiterOverflow = delimiterOverflow || result.endColumn + breakLineSuffixWidth_ > config_.columnLimit;
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
                result = AddBreak(result, currentLineIndent, node.structuralDepth);
                ++nextOpenIndent;
                ++nextRunStart;
                AddChoice(result, delimiter->children.front()->id, FormatBreakChoice::SplitDelimiterStackRun);
            }
            if (delimiterRuns.empty() || delimiterRuns.back().indentLevel != currentLineIndent) {
                delimiterRuns
                    .push_back(DelimiterStackRun{.begin = index, .end = index, .indentLevel = currentLineIndent});
            }
            delimiterRuns.back().end = index + 1;
            result = AddToken(result, delimiter->children.front()->token);
        }
        if (nextRunStart != runStarts.size()) {
            return {};
        }

        if (detachLeaf && result.endLineHasText) {
            result = AddBreak(result, nextOpenIndent, node.structuralDepth);
        }
        NodeResult leaf = Solve(*stack.leaf, result.endColumn, result.endIndentLevel, result.endLineHasText);
        if (!leaf.valid || (!detachLeaf && leaf.extraLines > 0)) {
            return {};
        }
        Merge(result, leaf);

        for (size_t runIndex = delimiterRuns.size(); runIndex-- > 0;) {
            const DelimiterStackRun& run = delimiterRuns[runIndex];
            const bool firstClosingRun = runIndex + 1 == delimiterRuns.size();
            if (result.endLineHasText && (detachLeaf || !firstClosingRun)) {
                result = AddBreak(result, run.indentLevel, node.structuralDepth);
            }
            for (size_t index = run.end; index-- > run.begin;) {
                result = AddToken(result, stack.delimiters[index]->children.back()->token);
            }
        }
        return result;
    }

    NodeResult SolveZeroOverflowAttachedDelimiterStack(
        const FormatBreakNode& node,
        const DelimiterStackView& stack,
        int column,
        int indentLevel,
        bool lineHasText
    ) {
        NodeResult best;
        std::vector<size_t> bestRunStarts;
        const int lastInitialBreak = lineHasText ? 1 : 0;
        for (int breakBeforeFirst = 0; breakBeforeFirst <= lastInitialBreak; ++breakBeforeFirst) {
            for (size_t terminalBegin = 0; terminalBegin < stack.delimiters.size(); ++terminalBegin) {
                NodeResult prefix{
                    .valid = true,
                    .endColumn = column,
                    .endIndentLevel = indentLevel,
                    .endLineHasText = lineHasText
                };
                int nextOpenIndent = indentLevel + 1;
                std::vector<size_t> runStarts;
                if (breakBeforeFirst != 0) {
                    prefix = AddBreak(prefix, nextOpenIndent, node.structuralDepth);
                    ++nextOpenIndent;
                    runStarts.push_back(0);
                }
                bool prefixFits = true;
                for (size_t index = 0; index < terminalBegin; ++index) {
                    const FormatBreakToken& open = stack.delimiters[index]->children.front()->token;
                    if (TokenWouldOverflow(prefix, open)) {
                        prefix = AddBreak(prefix, nextOpenIndent, node.structuralDepth);
                        ++nextOpenIndent;
                        runStarts.push_back(index);
                    }
                    prefix = AddToken(prefix, open);
                    if (prefix.maxOverflow > 0) {
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
                if (!candidate.valid || candidate.maxOverflow > 0) {
                    continue;
                }
                const bool equalCost = !Better(candidate, best) && !Better(best, candidate);
                if (Better(candidate, best) || (equalCost && std::lexicographical_compare(
                    bestRunStarts.begin(),
                    bestRunStarts.end(),
                    runStarts.begin(),
                    runStarts.end()
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
            initial = AddBreak(initial, firstRunIndent, node.structuralDepth);
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
                            candidate = AddBreak(candidate, runIndent, node.structuralDepth);
                        }
                        for (size_t index = begin; index < end; ++index) {
                            candidate = AddToken(candidate, stack.delimiters[index]->children.front()->token);
                        }
                        if (candidate.maxOverflow > maximumOverflow) {
                            break;
                        }
                        candidate = AddBreak(candidate, runIndent, node.structuralDepth);
                        for (size_t index = end; index-- > begin;) {
                            candidate = AddToken(candidate, stack.delimiters[index]->children.back()->token);
                        }
                        if (candidate.maxOverflow > maximumOverflow) {
                            break;
                        }
                        if (completedRuns > 0) {
                            candidate.endColumn = outerEndColumn;
                            candidate.endIndentLevel = outerEndIndentLevel;
                            candidate.endLineHasText = outerEndLineHasText;
                        }
                        AddPrunedDelimiterStackPartitionCandidate(
                            state(completedRuns + 1, end),
                            {.result = candidate, .path = path}
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
            if (Better(candidate, best.result) || (
                !Better(best.result, candidate) && PreferLaterDelimiterStackBreaks(path, best.path)
            )) {
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
                    candidate = AddBreak(candidate, leafIndent, node.structuralDepth);
                    NodeResult leaf =
                        Solve(*stack.leaf, candidate.endColumn, candidate.endIndentLevel, candidate.endLineHasText);
                    if (!leaf.valid) {
                        continue;
                    }
                    Merge(candidate, leaf);
                    if (candidate.maxOverflow > maximumOverflow) {
                        continue;
                    }
                    candidate.endColumn = outerEndColumn;
                    candidate.endIndentLevel = outerEndIndentLevel;
                    candidate.endLineHasText = outerEndLineHasText;
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
                            candidate = AddBreak(candidate, runIndent, node.structuralDepth);
                            path = ExtendDelimiterStackPartitionPath(path, begin);
                        }
                        for (size_t index = begin; index < delimiterCount; ++index) {
                            candidate = AddToken(candidate, stack.delimiters[index]->children.front()->token);
                        }
                        if (candidate.maxOverflow > maximumOverflow) {
                            continue;
                        }
                        NodeResult leaf =
                            Solve(*stack.leaf, candidate.endColumn, candidate.endIndentLevel, candidate.endLineHasText);
                        if (!leaf.valid || leaf.extraLines > 0) {
                            continue;
                        }
                        Merge(candidate, leaf);
                        for (size_t index = delimiterCount; index-- > begin;) {
                            candidate = AddToken(candidate, stack.delimiters[index]->children.back()->token);
                        }
                        if (candidate.maxOverflow > maximumOverflow) {
                            continue;
                        }
                        if (completedRuns > 0) {
                            candidate.endColumn = outerEndColumn;
                            candidate.endIndentLevel = outerEndIndentLevel;
                            candidate.endLineHasText = outerEndLineHasText;
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
            node,
            stack,
            column,
            indentLevel,
            lineHasText,
            detachLeaf,
            false,
            maximumOverflow
        );
        if (lineHasText) {
            NodeResult breakBeforeFirst = SolveExactDelimiterStackWithInitialBreak(
                node,
                stack,
                column,
                indentLevel,
                lineHasText,
                detachLeaf,
                true,
                maximumOverflow
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
        bool delimiterOverflow = false;
        NodeResult greedy =
            SolveGreedyDelimiterStack(node, stack, column, indentLevel, lineHasText, detachLeaf, delimiterOverflow);
        // Zero overflow is the primary optimum. At that point, delaying each opener-run break until the
        // next opener would overflow is also line-optimal: by induction, an earlier break leaves at least
        // as many openers for a line with the same or deeper indentation, so it cannot use fewer runs.
        // Fewer runs mean fewer opener and closer lines, and the latest equal-run partition is the
        // source-order-stable compact choice. This is therefore an equivalence-preserving fast path, not
        // a local layout decision. Once overflow is unavoidable, that proof no longer applies because an
        // additional run can reduce maximum overflow; the exact partition DP below must make the choice.
        if (!greedy.valid || greedy.maxOverflow == 0) {
            return greedy;
        }
        // A detached leaf starts one level after the final opener run. If all greedy delimiter lines fit,
        // the minimum-run proof also gives the shallowest possible leaf indentation; repartitioning cannot
        // improve the leaf, and adding a run can only move it deeper. Overflow confined to that leaf is
        // therefore another proven fast path.
        if (detachLeaf && !delimiterOverflow) {
            return greedy;
        }
        int exactMaximumOverflow = greedy.maxOverflow;
        if (!detachLeaf) {
            bool detachedDelimiterOverflow = false;
            const NodeResult detached = SolveGreedyDelimiterStack(
                node,
                stack,
                column,
                indentLevel,
                lineHasText,
                true,
                detachedDelimiterOverflow
            );
            if (detached.valid) {
                exactMaximumOverflow = std::min(exactMaximumOverflow, detached.maxOverflow);
            }
        }
        NodeResult exact =
            SolveExactDelimiterStack(node, stack, column, indentLevel, lineHasText, detachLeaf, exactMaximumOverflow);
        return Better(exact, greedy) ? exact : greedy;
    }

    static FormatBreakChoice ChoiceFor(const NodeResult& result, const FormatBreakNode& node) {
        const std::optional<FormatBreakChoice> choice = FindChoice(result.choices, node.id);
        return choice.value_or(FormatBreakChoice::Compact);
    }

    static bool IsBreakingChoice(FormatBreakChoice choice) {
        return choice != FormatBreakChoice::Compact;
    }

    static std::optional<FormatBreakChoice> FindChoice(const ChoiceTree* tree, int nodeId) {
        if (tree == nullptr) {
            return std::nullopt;
        }
        if (tree->leaf) {
            return tree->nodeId == nodeId ? std::optional(tree->choice) : std::nullopt;
        }
        if (std::optional<FormatBreakChoice> choice = FindChoice(tree->right, nodeId)) {
            return choice;
        }
        return FindChoice(tree->left, nodeId);
    }

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
        if (IsBreakingChoice(ChoiceFor(result, node)) || HasLeadingTrailingComment(node)) {
            return true;
        }
        for (const FormatBreakNode* child : node.children) {
            if (child != nullptr && HasPhysicalLineBreak(*child, result)) {
                return true;
            }
        }
        for (size_t index = 0; index < node.items.size(); ++index) {
            if (HasTrailingComment(node, index) || (
                node.items[index].node != nullptr && HasPhysicalLineBreak(*node.items[index].node, result)
            )) {
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
            if (IsBreakingChoice(ChoiceFor(compact, node))) {
                return CompactTailExpansionKind::Structural;
            }
            if (node.items.size() != 1 || HasRealSeparators(node)) {
                return CompactTailExpansionKind::None;
            }
            const FormatBreakNode* item = node.items.front().node;
            return item == nullptr ? CompactTailExpansionKind::None : CompactTailExpansion(*item, compact);
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
        if (tail == nullptr || TrailingBodyHeaderHeaderHasSelectedBreak(*tail, compact)) {
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
        if (node.chainKind == FormatBreakChainKind::MemberBeforeOperator) {
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

    static bool IsFormatterOwnedChainOperator(const FormatBreakToken& token) {
        return FormatBreakTokenKind(token) == PrintTokenKind::Known &&
            SyntaxNodeKindHasClass(FormatBreakTokenSyntaxKind(token), SyntaxNodeClass::ChainOperator);
    }

    static bool IsFormatterOwnedChain(const FormatBreakNode& node) {
        if (
            node.chainKind == FormatBreakChainKind::MemberBeforeOperator ||
            node.chainKind == FormatBreakChainKind::StreamBeforeOperator ||
            node.chainKind == FormatBreakChainKind::Ternary
        ) {
            return true;
        }
        return !node.operators.empty() &&
            std::all_of(node.operators.begin(), node.operators.end(), IsFormatterOwnedChainOperator);
    }

    static bool RequiresChainCompactExtraLinesGuard(const FormatBreakNode& node) {
        if (!node.operands.empty() && ContainsForceSplitAdjacentStrings(*node.operands.back())) {
            return true;
        }
        if (node.chainKind == FormatBreakChainKind::Ternary) {
            return node.operators.size() > 2;
        }
        if (node.chainKind == FormatBreakChainKind::StreamBeforeOperator) {
            return true;
        }
        return IsFormatterOwnedChain(node) && node.operators.size() > 1;
    }

    static bool ContainsForceSplitAdjacentStrings(const FormatBreakNode& node) {
        if (node.kind == FormatBreakNodeKind::AdjacentStrings && node.forceSplit) {
            return true;
        }
        for (const FormatBreakNode* child : node.children) {
            if (child && ContainsForceSplitAdjacentStrings(*child)) {
                return true;
            }
        }
        for (const FormatBreakListItem& item : node.items) {
            if (item.node && ContainsForceSplitAdjacentStrings(*item.node)) {
                return true;
            }
        }
        for (const FormatBreakNode* operand : node.operands) {
            if (operand && ContainsForceSplitAdjacentStrings(*operand)) {
                return true;
            }
        }
        return false;
    }

    static bool ContainsNonSingleStatementBodyHeader(const FormatBreakNode& node) {
        if (node.kind == FormatBreakNodeKind::BodyHeader && !node.bodyHeaderSingleStatementBody) {
            return true;
        }
        for (const FormatBreakNode* child : node.children) {
            if (child && ContainsNonSingleStatementBodyHeader(*child)) {
                return true;
            }
        }
        for (const FormatBreakListItem& item : node.items) {
            if (item.node && ContainsNonSingleStatementBodyHeader(*item.node)) {
                return true;
            }
        }
        for (const FormatBreakNode* operand : node.operands) {
            if (operand && ContainsNonSingleStatementBodyHeader(*operand)) {
                return true;
            }
        }
        return false;
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
        result = AddToken(result, node.children[0]->token);
        if (HasLeadingTrailingComment(node)) {
            result = AddToken(result, node.leadingTrailingComment);
        }
        NodeResults current{result};
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& listItem = node.items[index];
            NodeResults next;
            for (const NodeResult& prefix : current) {
                for (const NodeResult& item : SolveListItemWithSuffixAlternatives(
                    listItem,
                    prefix.endColumn,
                    prefix.endIndentLevel,
                    prefix.endLineHasText
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

    NodeResult SolvePrefixListCompact(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult best;
        for (const NodeResult& candidate : SolvePrefixListCompactAlternatives(node, column, indentLevel, lineHasText)) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    NodeResults
        SolvePrefixListSplitAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, indentLevel);
        result = AddToken(result, node.children[0]->token);
        if (HasLeadingTrailingComment(node)) {
            result = AddToken(result, node.leadingTrailingComment);
        }
        result = AddListBreakAfterOptionalComment(
            result,
            indentLevel + 1,
            node.structuralDepth,
            HasBlankLineBeforeItem(node, 0),
            HasLeadingTrailingComment(node)
        );
        NodeResults current{result};
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& listItem = node.items[index];
            NodeResults next;
            for (const NodeResult& prefix : current) {
                for (const NodeResult& item : SolveListItemWithSuffixAlternatives(
                    listItem,
                    prefix.endColumn,
                    prefix.endIndentLevel,
                    prefix.endLineHasText
                )) {
                    NodeResult candidate = prefix;
                    Merge(candidate, item);
                    if (index + 1 < node.items.size()) {
                        candidate = AddListBreakAfterOptionalComment(
                            candidate,
                            indentLevel + 1,
                            node.structuralDepth,
                            HasBlankLineBeforeItem(node, index + 1),
                            HasTrailingComment(node, index)
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

    NodeResult SolvePrefixListSplit(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult best;
        for (const NodeResult& candidate : SolvePrefixListSplitAlternatives(node, column, indentLevel, lineHasText)) {
            if (Better(candidate, best)) {
                best = candidate;
            }
        }
        return best;
    }

    NodeResult SolvePrefixList(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult compact = SolvePrefixListCompact(node, column, indentLevel, lineHasText);
        NodeResult split = SolvePrefixListSplit(node, column, indentLevel, lineHasText);
        if (node.forceSplit && split.valid) {
            return split;
        }
        if (compact.valid && split.valid && compact.extraLines > 0) {
            return split;
        }
        if (compact.valid && split.valid && CompactLineEndsOverLimit(compact) && split.maxOverflow == 0) {
            return split;
        }
        return Better(split, compact) ? split : compact;
    }

    NodeResult
        SolveStatementSequenceCompact(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
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
                result = AddListBreakAfterOptionalComment(
                    result,
                    indentLevel,
                    node.structuralDepth,
                    HasBlankLineBeforeItem(node, index),
                    HasTrailingComment(node, index - 1)
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

    NodeResult
        SolveFunctionSignatureCompact(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        if (node.children.size() < 2) {
            return {};
        }
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Compact, indentLevel);
        for (size_t index = 0; index < node.children.size(); ++index) {
            const FormatBreakNode* child = node.children[index];
            NodeResult item = Solve(*child, result.endColumn, result.endIndentLevel, result.endLineHasText);
            if (
                index == 1 &&
                item.extraLines > 0 &&
                !FunctionSignatureCompactPrefixFits(node, column, indentLevel, lineHasText)
            ) {
                return {};
            }
            Merge(result, item);
        }
        return result;
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

    bool
        FunctionSignatureCompactPrefixFits(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
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
        NodeResult declarator = SolveWithFirstParameterListSplit(
            *node.children[1],
            result.endColumn,
            result.endIndentLevel,
            result.endLineHasText,
            splitParameterList,
            compactHeaderFits
        );
        return splitParameterList && compactHeaderFits && declarator.valid;
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
            return SolveDelimitedSplit(node, column, indentLevel, lineHasText);
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
        const FormatBreakNode& node,
        int column,
        int indentLevel,
        bool lineHasText
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

    NodeResult SolveFunctionSignatureSplit(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        if (node.children.size() < 2) {
            return {};
        }
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, indentLevel);
        NodeResult returnType =
            Solve(*node.children[0], result.endColumn, result.endIndentLevel, result.endLineHasText);
        Merge(result, returnType);
        result = AddBreak(result, indentLevel + 1, node.structuralDepth);
        NodeResult declarator =
            Solve(*node.children[1], result.endColumn, result.endIndentLevel, result.endLineHasText);
        Merge(result, declarator);
        if (node.children.size() > 2) {
            if (node.functionSignatureHasBody) {
                result = AddBreak(result, indentLevel, node.structuralDepth);
            }
            NodeResult tail = Solve(*node.children[2], result.endColumn, result.endIndentLevel, result.endLineHasText);
            Merge(result, tail);
        }
        return result;
    }

    NodeResults
        SolveFunctionSignatureAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResults alternatives;
        NodeResult compact = SolveFunctionSignatureCompact(node, column, indentLevel, lineHasText);
        NodeResult splitParameters =
            SolveFunctionSignatureCompactWithSplitParameters(node, column, indentLevel, lineHasText);
        NodeResult split = SolveFunctionSignatureSplit(node, column, indentLevel, lineHasText);
        if (compact.valid) {
            alternatives.push_back(compact);
        }
        if (splitParameters.valid) {
            alternatives.push_back(splitParameters);
        }
        if (split.valid) {
            alternatives.push_back(split);
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

    NodeResult SolveBodyHeaderCompact(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        if (node.children.size() < 2) {
            return {};
        }
        NodeResult best;
        for (NodeResult candidate : SolveChildrenAlternatives(node.children, column, indentLevel, lineHasText)) {
            AddChoice(candidate, node.id, FormatBreakChoice::Compact, indentLevel);
            if (Better(candidate, best)) {
                best = std::move(candidate);
            }
        }
        return best;
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
        SolveBodyHeaderAlternatives(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResults alternatives;
        NodeResult compact = SolveBodyHeaderCompact(node, column, indentLevel, lineHasText);
        NodeResult header =
            node.children.empty() ? NodeResult{} : Solve(*node.children[0], column, indentLevel, lineHasText);
        const bool expandedHeaderRequiresDetachedBody = ExpandedBodyHeaderNeedsDetachedBody(node, header, indentLevel);
        const bool lineStartParentIndentBody = !lineHasText &&
            node.bodyHeaderSplitAtParentIndentWhenLineStarts &&
            (!node.bodyHeaderSingleStatementBody || (header.valid && header.extraLines > 0));
        if (
            !node.bodyHeaderRequiresDetachedBody &&
            !expandedHeaderRequiresDetachedBody &&
            compact.valid &&
            !(node.bodyHeaderSingleStatementBody && header.valid && header.extraLines > 0) &&
            !lineStartParentIndentBody
        ) {
            alternatives.push_back(compact);
        }
        NodeResult split = SolveBodyHeaderSplit(node, column, indentLevel, lineHasText);
        if (!node.bodyHeaderRequiresDetachedBody && !expandedHeaderRequiresDetachedBody && split.valid) {
            alternatives.push_back(split);
        }
        if (node.bodyHeaderDetachBodyAfterExpandedHeader || node.bodyHeaderRequiresDetachedBody) {
            NodeResult detached = SolveBodyHeaderDetachedBody(node, column, indentLevel, lineHasText);
            if (detached.valid) {
                alternatives.push_back(detached);
            }
        }
        if (lineStartParentIndentBody) {
            NodeResult parentIndent = SolveBodyHeaderSplitAtParentIndent(node, column, indentLevel, lineHasText);
            if (parentIndent.valid) {
                alternatives.push_back(parentIndent);
            }
        }
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
        if (node.children.size() < 2) {
            return {};
        }
        NodeResult best;
        for (const NodeResult& header : SolveAlternatives(*node.children[0], column, indentLevel, lineHasText)) {
            NodeResult result{
                .valid = true,
                .endColumn = column,
                .endIndentLevel = indentLevel,
                .endLineHasText = lineHasText
            };
            AddChoice(result, node.id, choice, indentLevel);
            Merge(result, header);
            if (
                choice == FormatBreakChoice::BodyHeaderSplitAtParentIndent ||
                choice == FormatBreakChoice::BodyHeaderDetachedBody
            ) {
                result = AddBreak(result, bodyIndentLevel, node.structuralDepth);
            }
            NodeResult body = SolveBodyHeaderSplitBody(
                *node.children[1],
                result.endColumn,
                result.endIndentLevel,
                result.endLineHasText
            );
            Merge(result, body);
            if (Better(result, best)) {
                best = std::move(result);
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

    NodeResult SolveBodyHeaderSplit(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        return SolveBodyHeaderSplitWithChoice(
            node,
            column,
            indentLevel,
            lineHasText,
            FormatBreakChoice::Split,
            indentLevel
        );
    }

    NodeResult SolveBodyHeaderDetachedBody(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        return SolveBodyHeaderSplitWithChoice(
            node,
            column,
            indentLevel,
            lineHasText,
            FormatBreakChoice::BodyHeaderDetachedBody,
            indentLevel
        );
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
        const FormatBreakNode& node,
        int column,
        int indentLevel,
        bool lineHasText
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
                NodeResults childResults = last ? NodeResults{SolveTrailingBodyHeaderSplitAtParentIndent(
                    *sequenceChildren[index],
                    prefix.endColumn,
                    prefix.endIndentLevel,
                    prefix.endLineHasText
                )} : SolveAlternatives(
                    *sequenceChildren[index],
                    prefix.endColumn,
                    prefix.endIndentLevel,
                    prefix.endLineHasText
                );
                for (const NodeResult& child : childResults) {
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
        NodeResult compact = SolveBodyHeaderCompact(node, column, indentLevel, lineHasText);
        NodeResult split = SolveBodyHeaderSplit(node, column, indentLevel, lineHasText);
        NodeResult detached = node.bodyHeaderDetachBodyAfterExpandedHeader ?
            SolveBodyHeaderDetachedBody(node, column, indentLevel, lineHasText) : NodeResult{};
        if (node.bodyHeaderRequiresDetachedBody) {
            return SolveBodyHeaderDetachedBody(node, column, indentLevel, lineHasText);
        }
        NodeResult parentIndent = !lineHasText && node.bodyHeaderSplitAtParentIndentWhenLineStarts ?
            SolveBodyHeaderSplitAtParentIndent(node, column, indentLevel, lineHasText) : NodeResult{};
        if (compact.valid && compact.extraLines > 0 && parentIndent.valid) {
            return parentIndent;
        }
        NodeResult header =
            node.children.empty() ? NodeResult{} : Solve(*node.children[0], column, indentLevel, lineHasText);
        if (ExpandedBodyHeaderNeedsDetachedBody(node, header, indentLevel) && detached.valid) {
            return detached;
        }
        const bool lineStartParentIndentBody = !lineHasText &&
            node.bodyHeaderSplitAtParentIndentWhenLineStarts &&
            (!node.bodyHeaderSingleStatementBody || (header.valid && header.extraLines > 0));
        if (lineStartParentIndentBody && parentIndent.valid) {
            return parentIndent;
        }
        if (node.bodyHeaderSingleStatementBody && header.valid && header.extraLines > 0 && split.valid) {
            return split;
        }
        NodeResult best = Better(split, compact) ? split : compact;
        return Better(detached, best) ? detached : best;
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

    NodeResult AddCommentsBeforeChainOperator(const FormatBreakNode& node, size_t index, NodeResult result) {
        if (index >= node.commentsBeforeOperators.size()) {
            return result;
        }
        for (const FormatBreakToken& comment : node.commentsBeforeOperators[index]) {
            result = AddToken(result, comment);
        }
        return result;
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
        for (size_t index = 0; index < node.operands.size(); ++index) {
            NodeResults nextByState;
            for (const NodeResult& prefix : current) {
                for (const NodeResult& operand : SolveAlternatives(
                    *node.operands[index],
                    prefix.endColumn,
                    prefix.endIndentLevel,
                    prefix.endLineHasText
                )) {
                    if (!operand.valid) {
                        continue;
                    }
                    NodeResult next = prefix;
                    Merge(next, operand);
                    if (node.declarationValueOwner != nullptr && index + 1 == node.operands.size()) {
                        AddDeclarationValueContinuationLines(next, node.id, operand.extraLines);
                    }
                    if (index < node.operators.size()) {
                        next = AddCommentsBeforeChainOperator(node, index, next);
                        next = AddToken(next, node.operators[index]);
                    }
                    AddPrunedResult(nextByState, std::move(next));
                }
            }
            SortPrunedResults(nextByState);
            current = std::move(nextByState);
        }
        return current;
    }

    NodeResult SolveDelimitedSplitAttachedOpen(const FormatBreakNode& node, NodeResult result, int baseIndent) {
        if (node.kind != FormatBreakNodeKind::Delimited || node.items.empty()) {
            return {};
        }
        AddChoice(result, node.id, FormatBreakChoice::SplitAttachedOpen, baseIndent);
        result = AddToken(result, node.children[0]->token);
        if (HasLeadingTrailingComment(node)) {
            result = AddToken(result, node.leadingTrailingComment);
        }
        result = AddListBreakAfterOptionalComment(
            result,
            baseIndent + 1,
            node.structuralDepth,
            HasBlankLineBeforeItem(node, 0),
            HasLeadingTrailingComment(node)
        );
        for (size_t index = 0; index < node.items.size(); ++index) {
            const FormatBreakListItem& listItem = node.items[index];
            NodeResult item = Solve(*listItem.node, result.endColumn, result.endIndentLevel, result.endLineHasText);
            Merge(result, item);
            if (FormatBreakTokenKind(listItem.separator) == PrintTokenKind::Known) {
                result = AddToken(result, listItem.separator);
            }
            if (HasTrailingComment(node, index)) {
                result = AddToken(result, listItem.trailingComment);
            }
            const bool hasNextItem = index + 1 < node.items.size();
            result = AddListBreakAfterOptionalComment(
                result,
                hasNextItem ? baseIndent + 1 : baseIndent,
                node.structuralDepth,
                hasNextItem && HasBlankLineBeforeItem(node, index + 1),
                HasTrailingComment(node, index)
            );
        }
        result = AddToken(result, node.children[1]->token);
        return result;
    }

    static bool CanAttachSplitOpenAfterOperator(const FormatBreakToken& op, const FormatBreakNode& operand) {
        const PrintToken& opToken = FormatBreakTokenValue(op);
        return opToken.kind == PrintTokenKind::Known &&
            opToken.parentKind == SyntaxNodeKind::BinaryExpression &&
            SyntaxNodeKindHasClass(opToken.syntaxKind, SyntaxNodeClass::BinaryOperator) &&
            operand.kind == FormatBreakNodeKind::Delimited &&
            operand.delimiterKind == FormatBreakDelimiterKind::Paren;
    }

    NodeResult
        SolveChainSplitAfterOperator(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        const int continuationIndent = node.flatSplitIndent ? indentLevel : indentLevel + 1;
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, indentLevel);
        if (node.operands.empty()) {
            return result;
        }
        const FormatBreakToken* firstSuffix = node.operators.empty() ? nullptr : &node.operators.front();
        NodeResult first = SolveNodeWithSuffix(
            *node.operands.front(),
            firstSuffix,
            result.endColumn,
            result.endIndentLevel,
            result.endLineHasText
        );
        if (!first.valid) {
            return {};
        }
        Merge(result, first);
        for (size_t index = 0; index < node.operators.size(); ++index) {
            NodeResult normal = AddBreak(result, continuationIndent, node.structuralDepth);
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
                    *node.operands[index + 1],
                    normal.endColumn,
                    normal.endIndentLevel,
                    normal.endLineHasText
                );
                if (nextSuffix != nullptr && parentIndentOperand.valid) {
                    parentIndentOperand = AddToken(parentIndentOperand, *nextSuffix);
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
                attached = SolveDelimitedSplitAttachedOpen(*node.operands[index + 1], result, continuationIndent);
                if (nextSuffix != nullptr && attached.valid) {
                    attached = AddToken(attached, *nextSuffix);
                }
            }
            result = Better(attached, normal) ? attached : normal;
        }
        return result;
    }

    NodeResult
        SolveChainSplitBeforeOperator(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText)
    {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, indentLevel);
        if (node.operands.empty()) {
            return result;
        }

        NodeResult receiver =
            Solve(*node.operands.front(), result.endColumn, result.endIndentLevel, result.endLineHasText);
        Merge(result, receiver);
        for (size_t index = 0; index < node.operators.size(); ++index) {
            result = AddBreak(result, indentLevel + 1, node.structuralDepth);
            result = AddCommentsBeforeChainOperator(node, index, result);
            result = AddToken(result, node.operators[index]);
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
        result = AddBreak(result, indentLevel + 1, node.structuralDepth);
        for (size_t index = 0; index < node.operators.size(); ++index) {
            result = AddCommentsBeforeChainOperator(node, index, result);
            result = AddToken(result, node.operators[index]);
            const bool finalOperand = index + 1 == node.operands.size() - 1;
            NodeResult operand = finalOperand ?
                Solve(*node.operands[index + 1], result.endColumn, result.endIndentLevel, result.endLineHasText) :
                SolveNodeWithoutBreaks(
                    *node.operands[index + 1],
                    result.endColumn,
                    result.endIndentLevel,
                    result.endLineHasText
                );
            if (!operand.valid) {
                return {};
            }
            Merge(result, operand);
        }
        return result;
    }

    NodeResults SolveStreamSplitAlternatives(
        const FormatBreakNode& node,
        int column,
        int indentLevel,
        bool lineHasText,
        FormatBreakChoice choice
    ) {
        NodeResult
            initial{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(initial, node.id, choice, indentLevel);
        NodeResults current{initial};
        if (!node.chainStartsWithOperator) {
            NodeResults next;
            for (const NodeResult& prefix : current) {
                for (const NodeResult& receiver : SolveAlternatives(
                    *node.operands.front(),
                    prefix.endColumn,
                    prefix.endIndentLevel,
                    prefix.endLineHasText
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
                prefix.endIndentLevel = indentLevel + 1;
                prefix.endColumn = IndentColumn(prefix.endIndentLevel);
            } else {
                prefix = AddBreak(prefix, indentLevel + 1, node.structuralDepth);
            }
        }
        for (size_t index = 0; index < node.operators.size(); ++index) {
            NodeResults next;
            for (const NodeResult& prefix : current) {
                NodeResult withOperator = AddCommentsBeforeChainOperator(node, index, prefix);
                withOperator = AddToken(withOperator, node.operators[index]);
                NodeResults operands;
                if (choice == FormatBreakChoice::StreamCompactTail) {
                    operands.push_back(SolveNodeWithoutBreaks(
                        *node.operands[index + 1],
                        withOperator.endColumn,
                        withOperator.endIndentLevel,
                        withOperator.endLineHasText
                    ));
                } else {
                    operands = SolveAlternatives(
                        *node.operands[index + 1],
                        withOperator.endColumn,
                        withOperator.endIndentLevel,
                        withOperator.endLineHasText
                    );
                }
                for (const NodeResult& operand : operands) {
                    if (!operand.valid) {
                        continue;
                    }
                    NodeResult candidate = withOperator;
                    Merge(candidate, operand);
                    if (
                        choice == FormatBreakChoice::Split &&
                        index + 1 < node.operators.size() &&
                        !IsFormatBreakStreamConfigurationOperand(
                            *node.operands[index + 1],
                            config_.streamShiftConfigurationMethods
                        )
                    ) {
                        candidate = AddBreak(candidate, indentLevel + 1, node.structuralDepth);
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
        const FormatBreakNode& node,
        int column,
        int indentLevel,
        bool lineHasText,
        FormatBreakChoice choice
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

    NodeResult SolveTernaryChainSplit(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, FormatBreakChoice::Split, indentLevel);
        for (size_t index = 0; index < node.operands.size(); ++index) {
            NodeResult operand =
                Solve(*node.operands[index], result.endColumn, result.endIndentLevel, result.endLineHasText);
            Merge(result, operand);
            if (index < node.operators.size()) {
                result = AddToken(result, node.operators[index]);
                if (
                    FormatBreakTokenKind(node.operators[index]) == PrintTokenKind::Known &&
                    FormatBreakTokenSyntaxKind(node.operators[index]) == SyntaxNodeKind::Colon
                ) {
                    result = AddBreak(result, indentLevel + 1, node.structuralDepth);
                }
            }
        }
        return result;
    }

    NodeResult SolveSingleTernary(
        const FormatBreakNode& node,
        int column,
        int indentLevel,
        bool lineHasText,
        FormatBreakChoice choice
    ) {
        const bool breakAfterQuestion =
            choice == FormatBreakChoice::TernaryBreakAfterQuestion || choice == FormatBreakChoice::Split;
        const bool breakAfterColon =
            choice == FormatBreakChoice::TernaryBreakAfterColon || choice == FormatBreakChoice::Split;
        const int continuationIndent = node.flatSplitIndent ? indentLevel : indentLevel + 1;
        NodeResult
            result{.valid = true, .endColumn = column, .endIndentLevel = indentLevel, .endLineHasText = lineHasText};
        AddChoice(result, node.id, choice, indentLevel);
        for (size_t index = 0; index < node.operands.size(); ++index) {
            NodeResult operand =
                Solve(*node.operands[index], result.endColumn, result.endIndentLevel, result.endLineHasText);
            Merge(result, operand);
            if (index < node.operators.size()) {
                result = AddToken(result, node.operators[index]);
                if ((index == 0 && breakAfterQuestion) || (index == 1 && breakAfterColon)) {
                    result = AddBreak(result, continuationIndent, node.structuralDepth);
                }
            }
        }
        return result;
    }

    NodeResult SolveChain(const FormatBreakNode& node, int column, int indentLevel, bool lineHasText) {
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
            if (compact.valid && best.valid && CompactLineEndsOverLimit(compact) && best.maxOverflow == 0) {
                if (compactTail.valid && compactTail.maxOverflow == 0) {
                    return compactTail;
                }
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
            if (compact.valid && best.valid && CompactLineEndsOverLimit(compact) && best.maxOverflow == 0) {
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
                    node,
                    column,
                    indentLevel,
                    lineHasText,
                    FormatBreakChoice::TernaryBreakAfterQuestion
                ),
                SolveSingleTernary(node, column, indentLevel, lineHasText, FormatBreakChoice::TernaryBreakAfterColon),
                SolveSingleTernary(node, column, indentLevel, lineHasText, FormatBreakChoice::Split)
            };
            for (const NodeResult& alternative : alternatives) {
                if (Better(alternative, best)) {
                    best = alternative;
                }
            }
            if (compact.valid && best.valid && CompactLineEndsOverLimit(compact) && best.maxOverflow == 0) {
                return best;
            }
            return best;
        }
        NodeResult split = SolveChainSplitAfterOperator(node, column, indentLevel, lineHasText);
        if (
            node.chainCompactRequiresFitOnOneLine &&
            compact.valid &&
            (compact.extraLines > 0 || compact.maxOverflow > 0) &&
            split.valid
        ) {
            return split;
        }
        if (node.chainPrefersSplitWhenCompactBreaks && compact.valid && compact.extraLines > 0 && split.valid) {
            return split;
        }
        if (compact.valid && split.valid && CompactLineEndsOverLimit(compact) && split.maxOverflow == 0) {
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
                result = AddBreak(result, continuationIndent, node.structuralDepth);
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

void AppendChoices(
    const ChoiceTree* tree,
    std::vector<FormatBreakChoice>& choices,
    std::vector<int>& indentLevels,
    std::vector<bool>& assigned
) {
    if (tree == nullptr) {
        return;
    }
    if (tree->leaf) {
        const size_t index = static_cast<size_t>(tree->nodeId);
        if (index < choices.size() && !assigned[index]) {
            choices[index] = tree->choice;
            indentLevels[index] = tree->indentLevel;
            assigned[index] = true;
        }
        return;
    }
    AppendChoices(tree->left, choices, indentLevels, assigned);
    AppendChoices(tree->right, choices, indentLevels, assigned);
}

void AppendDeclarationValueContinuationLines(const ChoiceTree* tree, std::vector<int>& continuationLines) {
    if (tree == nullptr) {
        return;
    }
    if (tree->leaf) {
        const size_t index = static_cast<size_t>(tree->nodeId);
        if (index < continuationLines.size() && tree->declarationValueContinuationLines >= 0) {
            continuationLines[index] = tree->declarationValueContinuationLines;
        }
        return;
    }
    AppendDeclarationValueContinuationLines(tree->left, continuationLines);
    AppendDeclarationValueContinuationLines(tree->right, continuationLines);
}

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
    Solver solver(config, indentWidth, breakLineSuffixWidth);
    NodeResult result = solver.Solve(*model.root, startColumn, indentLevel, startColumn > indentLevel * indentWidth);
    if (!result.valid) {
        return solution;
    }
    const size_t choiceCount = model.nodes == nullptr ? 0 : model.nodes->size() + 1;
    solution.choices.assign(choiceCount, FormatBreakChoice::Compact);
    solution.indentLevels.assign(choiceCount, -1);
    solution.declarationValueContinuationLines.assign(choiceCount, -1);
    std::vector<bool> assigned(choiceCount, false);
    AppendChoices(result.choices, solution.choices, solution.indentLevels, assigned);
    AppendDeclarationValueContinuationLines(result.choices, solution.declarationValueContinuationLines);
    return solution;
}

#pragma once

#include <algorithm>
#include <deque>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "format/impl/format_spacing.h"

enum class FormatBreakNodeKind {
    Token,
    Sequence,
    Delimited,
    PrefixList,
    StatementSequence,
    FunctionSignature,
    BodyHeader,
    Chain,
    AdjacentStrings,
};

enum class FormatBreakDelimiterKind {
    None,
    Paren,
    Bracket,
    Brace,
    Angle,
};

enum class FormatBreakChainKind {
    AfterOperator,
    MemberBeforeOperator,
    StreamBeforeOperator,
    Ternary,
};

enum class FormatBreakChoice {
    Compact,
    Split,
    BodyHeaderSplitAtParentIndent,
    BodyHeaderDetachedBody,
    SplitAttachedOpen,
    SplitDelimiterStack,
    SplitDelimiterStackDetachedLeaf,
    SplitDelimiterStackRun,
    MemberCompactTail,
    StreamCompactTail,
    TernaryBreakAfterQuestion,
    TernaryBreakAfterColon,
};

struct FormatBreakNode;

struct FormatBreakToken {
    const PrintToken* token = nullptr;
    bool spaceBefore = false;
    bool contextOnly = false;
};

struct FormatBreakListItem {
    FormatBreakNode* node = nullptr;
    FormatBreakToken separator;
    FormatBreakToken trailingComment;
    bool blankLineBefore = false;
};

struct FormatBreakNode {
    int id = 0;
    int structuralDepth = 0;
    FormatBreakNodeKind kind = FormatBreakNodeKind::Sequence;
    FormatBreakToken token;
    FormatBreakDelimiterKind delimiterKind = FormatBreakDelimiterKind::None;
    FormatBreakChainKind chainKind = FormatBreakChainKind::AfterOperator;
    bool forceSplit = false;
    bool flatSplitIndent = false;
    bool suppressCompactDelimiterPadding = false;
    bool functionSignatureHasBody = false;
    bool bodyHeaderSingleStatementBody = false;
    bool bodyHeaderDetachBodyAfterExpandedHeader = false;
    bool bodyHeaderRequiresDetachedBody = false;
    bool bodyHeaderSplitAtParentIndentWhenLineStarts = false;
    bool chainPrefersSplitWhenCompactBreaks = false;
    bool chainCompactRequiresFitOnOneLine = false;
    bool chainStartsWithOperator = false;
    bool splitTrailingBodyHeaderAtParentIndent = false;
    const SyntaxNode* declarationValueOwner = nullptr;
    FormatBreakToken leadingTrailingComment;
    std::span<FormatBreakNode*> children;
    std::vector<FormatBreakListItem> items;
    std::span<FormatBreakNode*> operands;
    std::span<FormatBreakToken> operators;
    std::vector<std::vector<FormatBreakToken>> commentsBeforeOperators;
};

template <typename T>
class FormatBreakArena {
public:
    std::span<T> Append(std::span<const T> values);

private:
    static constexpr size_t kBlockSize = 256;

    std::span<T> Allocate(size_t count);
    void AllocateBlock(size_t capacity);

    std::vector<std::unique_ptr<T[]>> blocks_;
    T* cursor_ = nullptr;
    size_t remaining_ = 0;
};

template <typename T>
std::span<T> FormatBreakArena<T>::Append(std::span<const T> values) {
    std::span<T> result = Allocate(values.size());
    std::copy(values.begin(), values.end(), result.begin());
    return result;
}

template <typename T>
std::span<T> FormatBreakArena<T>::Allocate(size_t count) {
    if (count == 0) {
        return {};
    }
    if (remaining_ < count) {
        AllocateBlock(std::max(count, kBlockSize));
    }
    T* result = cursor_;
    cursor_ += count;
    remaining_ -= count;
    return {result, count};
}

template <typename T>
void FormatBreakArena<T>::AllocateBlock(size_t capacity) {
    blocks_.push_back(std::make_unique<T[]>(capacity));
    cursor_ = blocks_.back().get();
    remaining_ = capacity;
}

struct FormatBreakModel {
    std::unique_ptr<std::deque<FormatBreakNode>> nodes;
    FormatBreakArena<FormatBreakNode*> nodePointers;
    FormatBreakArena<FormatBreakToken> tokens;
    FormatBreakNode* root = nullptr;
};

struct FormatBreakVirtualDelimiter {
    const SyntaxNode* open = nullptr;
    FormatBreakToken close;
    bool forceSplit = false;
};

struct FormatBreakModelContext {
    std::vector<FormatBreakVirtualDelimiter> virtualDelimiters;
    bool forceSplitStreamChain = false;
};

bool FormatBreakLeadingNameMatches(const FormatBreakNode& node, std::string_view candidate);
bool IsFormatBreakStreamConfigurationOperand(
    const FormatBreakNode& node,
    const std::vector<std::string>& configurationMethods
);

#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>
#include <memory>
#include <memory_resource>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "format/impl/format_spacing.h"
#include "format/impl/format_syntax_map.h"

enum class FormatBreakNodeKind : std::uint8_t {
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

enum class FormatBreakDelimiterKind : std::uint8_t {
    None,
    Paren,
    Bracket,
    Brace,
    Angle,
};

enum class FormatBreakChainKind : std::uint8_t {
    AfterOperator,
    CallApplication,
    MemberBeforeOperator,
    StreamBeforeOperator,
    Ternary,
};

enum class FormatBreakChoice {
    Compact,
    Split,
    SplitPacked,
    BodyHeaderSplitAtParentIndent,
    BodyHeaderDetachedBody,
    SplitAttachedOpen,
    SplitDelimiterStack,
    SplitDelimiterStackDetachedLeaf,
    SplitDelimiterStackRun,
    CallCompactTail,
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

using FormatBreakWorkspace = FormatSyntaxMap<FormatBreakToken>::Workspace;

struct FormatBreakListItem {
    FormatBreakNode* node = nullptr;
    FormatBreakToken separator;
    FormatBreakToken trailingComment;
    bool blankLineBefore = false;
    bool bracedInitializerRecord = false;
    bool preserveSeparator = false;
};

// Projection copies this metadata directly while rebuilding child collections.
struct FormatBreakNodeData {
    int id = 0;
    int rawDepth = 0;
    int structuralDepth = 0;
    int breakCost = 0;
    const SyntaxNode* syntaxOwner = nullptr;
    const FormatBreakNode* origin = nullptr;
    FormatBreakToken token;
    FormatBreakNodeKind kind = FormatBreakNodeKind::Sequence;
    FormatBreakDelimiterKind delimiterKind = FormatBreakDelimiterKind::None;
    FormatBreakChainKind chainKind = FormatBreakChainKind::AfterOperator;
    bool forceSplit = false;
    bool hasIndependentBodyItems : 1 = false;
    bool blankLineBeforeClose : 1 = false;
    bool compactRequiresUnbrokenItems : 1 = false;
    bool flatSplitIndent : 1 = false;
    bool suppressCompactDelimiterPadding : 1 = false;
    bool functionSignatureHasBody : 1 = false;
    bool bodyHeaderIsLambda : 1 = false;
    bool bodyHeaderSingleStatementBody : 1 = false;
    bool bodyHeaderDetachBodyAfterExpandedHeader : 1 = false;
    bool bodyHeaderRequiresDetachedBody : 1 = false;
    bool bodyHeaderSplitAtParentIndentWhenLineStarts : 1 = false;
    bool chainPrefersSplitWhenCompactBreaks : 1 = false;
    bool chainCompactRequiresFitOnOneLine : 1 = false;
    bool chainStartsWithOperator : 1 = false;
    bool ternaryRequiresQuestionBreak : 1 = false;
    bool ternaryRequiresColonBreaks : 1 = false;
    bool splitTrailingBodyHeaderAtParentIndent : 1 = false;
    const SyntaxNode* bodySyntax = nullptr;
    std::optional<size_t> splitTrailingCommaItem;
    std::optional<int> continuedBodyHeaderOwnerIndent;
    std::optional<int> requiredChainBreakBaseIndent;
    const SyntaxNode* declarationValueOwner = nullptr;
    FormatBreakToken leadingTrailingComment;
    FormatBreakToken sourceTrailingComma;
};

struct FormatBreakNode : FormatBreakNodeData {
    FormatBreakNode() = default;
    explicit FormatBreakNode(const FormatBreakNodeData& data) : FormatBreakNodeData(data) {}
    FormatBreakNode(FormatBreakNodeKind kind, int depth, const SyntaxNode* owner, int id, FormatBreakToken token = {}) :
        FormatBreakNodeData{
            .id = id,
            .rawDepth = depth,
            .structuralDepth = depth,
            .breakCost = depth,
            .syntaxOwner = owner,
            .token = token,
            .kind = kind,
        } {}

    std::span<FormatBreakNode*> children;
    std::span<FormatBreakListItem> items;
    std::span<FormatBreakNode*> operands;
    std::span<FormatBreakToken> operators;
    std::span<const std::span<const FormatBreakToken>> commentsBeforeOperators;
    // AdjacentStrings compact spelling by operand. Empty entries are absorbed into the preceding non-empty run.
    std::span<const std::string> compactStringTexts;
};

static_assert(std::is_trivially_destructible_v<FormatBreakNode>);

template <typename T, size_t BlockSize = 256>
class FormatBreakArena {
    static_assert(std::is_trivially_copyable_v<T>);

public:
    explicit FormatBreakArena(std::pmr::memory_resource* resource = nullptr) : resource_(resource) {}
    std::span<T> Append(std::span<const T> values);
    std::span<T> Allocate(size_t count);

private:
    std::span<T> AllocateStorage(size_t count);
    void AllocateBlock(size_t capacity);

    struct Deallocate {
        std::pmr::memory_resource* resource;
        size_t capacity;

        void operator()(T* memory) const { resource->deallocate(memory, capacity * sizeof(T), alignof(T)); }
    };

    std::pmr::memory_resource* resource_;
    std::vector<std::unique_ptr<T, Deallocate>> blocks_;
    T* cursor_ = nullptr;
    size_t remaining_ = 0;
};

template <typename T, size_t BlockSize>
std::span<T> FormatBreakArena<T, BlockSize>::Append(std::span<const T> values) {
    std::span<T> result = AllocateStorage(values.size());
    std::uninitialized_copy(values.begin(), values.end(), result.begin());
    return result;
}

template <typename T, size_t BlockSize>
std::span<T> FormatBreakArena<T, BlockSize>::Allocate(size_t count) {
    std::span<T> result = AllocateStorage(count);
    std::uninitialized_default_construct(result.begin(), result.end());
    return result;
}

template <typename T, size_t BlockSize>
std::span<T> FormatBreakArena<T, BlockSize>::AllocateStorage(size_t count) {
    if (count == 0) {
        return {};
    }
    if (remaining_ < count) {
        AllocateBlock(std::max(count, BlockSize));
    }
    T* result = cursor_;
    cursor_ += count;
    remaining_ -= count;
    return {result, count};
}

template <typename T, size_t BlockSize>
void FormatBreakArena<T, BlockSize>::AllocateBlock(size_t capacity) {
    if (resource_ == nullptr) {
        resource_ = std::pmr::get_default_resource();
    }
    auto block = std::unique_ptr<T, Deallocate>(
        static_cast<T*>(resource_->allocate(capacity * sizeof(T), alignof(T))), {resource_, capacity}
    );
    blocks_.push_back(std::move(block));
    cursor_ = blocks_.back().get();
    remaining_ = capacity;
}

struct FormatBreakModel {
    FormatBreakModel() = default;
    explicit FormatBreakModel(std::pmr::memory_resource* resource) :
        nodes(std::make_unique<std::pmr::deque<FormatBreakNode>>(
            resource == nullptr ? std::pmr::get_default_resource() : resource
        )),
        nodePointers(resource),
        tokens(resource),
        listItems(resource),
        commentLists(resource) {}

    std::unique_ptr<std::pmr::deque<FormatBreakNode>> nodes;
    FormatBreakArena<FormatBreakNode*> nodePointers;
    FormatBreakArena<FormatBreakToken> tokens;
    FormatBreakArena<FormatBreakListItem, 16> listItems;
    FormatBreakArena<std::span<const FormatBreakToken>, 16> commentLists;
    std::vector<std::vector<std::string>> stringRuns;
    FormatBreakNode* root = nullptr;
    bool hasLayoutChoice = false;
    // Projections retain source ids and append fresh ids for synthesized or changed token nodes.
    size_t nodeIdCount = 0;

    size_t NodeIdCount() const { return nodeIdCount != 0 ? nodeIdCount : (nodes == nullptr ? 0 : nodes->size()); }
};

bool FormatBreakLeadingNameMatches(const FormatBreakNode& node, std::string_view candidate);
bool IsFormatBreakUniformChain(const FormatBreakNode& node);
bool IsFormatBreakQualifiedName(const FormatBreakNode& node);
bool IsFormatBreakLiteralOperand(const FormatBreakNode& node, SyntaxNodeClass literalClass = SyntaxNodeClass::Literal);
bool IsFormatBreakStreamConfigurationOperand(
    const FormatBreakNode& node, const std::vector<std::string>& configurationMethods
);

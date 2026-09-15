#pragma once

#include <algorithm>
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

// Projection preserves these values while rebuilding child collections. Keeping
// them separate avoids copying owned vectors that would immediately be discarded.
struct FormatBreakNodeData {
    int id = 0;
    const SyntaxNode* syntaxOwner = nullptr;
    const FormatBreakNode* origin = nullptr;
    int rawDepth = 0;
    int structuralDepth = 0;
    int breakCost = 0;
    FormatBreakNodeKind kind = FormatBreakNodeKind::Sequence;
    FormatBreakToken token;
    FormatBreakDelimiterKind delimiterKind = FormatBreakDelimiterKind::None;
    FormatBreakChainKind chainKind = FormatBreakChainKind::AfterOperator;
    bool forceSplit = false;
    bool hasIndependentBodyItems = false;
    bool blankLineBeforeClose = false;
    bool compactRequiresUnbrokenItems = false;
    bool flatSplitIndent = false;
    bool suppressCompactDelimiterPadding = false;
    bool functionSignatureHasBody = false;
    const SyntaxNode* bodySyntax = nullptr;
    bool bodyHeaderIsLambda = false;
    bool bodyHeaderSingleStatementBody = false;
    bool bodyHeaderDetachBodyAfterExpandedHeader = false;
    bool bodyHeaderRequiresDetachedBody = false;
    bool bodyHeaderSplitAtParentIndentWhenLineStarts = false;
    bool chainPrefersSplitWhenCompactBreaks = false;
    bool chainCompactRequiresFitOnOneLine = false;
    bool chainStartsWithOperator = false;
    bool ternaryRequiresQuestionBreak = false;
    bool ternaryRequiresColonBreaks = false;
    bool splitTrailingBodyHeaderAtParentIndent = false;
    std::optional<size_t> splitTrailingCommaItem;
    std::optional<int> continuedBodyHeaderOwnerIndent;
    std::optional<int> requiredChainBreakBaseIndent;
    const SyntaxNode* declarationValueOwner = nullptr;
    FormatBreakToken leadingTrailingComment;
    FormatBreakToken sourceTrailingComma;
};

struct FormatBreakNode : FormatBreakNodeData {
    explicit FormatBreakNode(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) :
        items(resource) {}

    std::span<FormatBreakNode*> children;
    std::pmr::vector<FormatBreakListItem> items;
    std::span<FormatBreakNode*> operands;
    std::span<FormatBreakToken> operators;
    std::vector<std::vector<FormatBreakToken>> commentsBeforeOperators;
    // AdjacentStrings compact spelling by operand. Empty entries are absorbed into the preceding non-empty run.
    std::vector<std::string> compactStringTexts;
};

template <typename T>
class FormatBreakArena {
    static_assert(std::is_trivially_copyable_v<T>);

public:
    explicit FormatBreakArena(std::pmr::memory_resource* resource = nullptr) : resource_(resource) {}
    std::span<T> Append(std::span<const T> values);
    std::span<T> Allocate(size_t count);

private:
    static constexpr size_t kBlockSize = 256;

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

template <typename T>
std::span<T> FormatBreakArena<T>::Append(std::span<const T> values) {
    std::span<T> result = AllocateStorage(values.size());
    std::uninitialized_copy(values.begin(), values.end(), result.begin());
    return result;
}

template <typename T>
std::span<T> FormatBreakArena<T>::Allocate(size_t count) {
    std::span<T> result = AllocateStorage(count);
    std::uninitialized_default_construct(result.begin(), result.end());
    return result;
}

template <typename T>
std::span<T> FormatBreakArena<T>::AllocateStorage(size_t count) {
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
        tokens(resource) {}

    std::unique_ptr<std::pmr::deque<FormatBreakNode>> nodes;
    FormatBreakArena<FormatBreakNode*> nodePointers;
    FormatBreakArena<FormatBreakToken> tokens;
    FormatBreakNode* root = nullptr;
    bool hasLayoutChoice = false;
};

bool FormatBreakLeadingNameMatches(const FormatBreakNode& node, std::string_view candidate);
bool IsFormatBreakUniformChain(const FormatBreakNode& node);
bool IsFormatBreakQualifiedName(const FormatBreakNode& node);
bool IsFormatBreakLiteralOperand(const FormatBreakNode& node, SyntaxNodeClass literalClass = SyntaxNodeClass::Literal);
bool IsFormatBreakStreamConfigurationOperand(
    const FormatBreakNode& node, const std::vector<std::string>& configurationMethods
);

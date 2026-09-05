#pragma once

#include <cstdint>
#include <memory>
#include <memory_resource>
#include <string>
#include <string_view>
#include <vector>

#include "format/impl/format_syntax_info.h"

struct ParseResult {
    bool ok = false;
    std::string error;
};

struct PrintToken;

struct SyntaxNode;

using SyntaxChildList = std::pmr::vector<SyntaxNode*>;

bool CallableBodyAllowsCompactSingleStatementForm(const SyntaxNode& node, SyntaxNodeKind parentKind);

struct SyntaxNode {
    explicit SyntaxNode(std::pmr::memory_resource* childResource = std::pmr::get_default_resource());

    // Keep nodes maximally generic and space-efficient; avoid fields that only apply to one node kind.
    SyntaxNodeKind kind = SyntaxNodeKind::Unknown;
    // Grammar field roles survive flattened wrappers.
    bool isDeclarator = false;
    bool isCondition = false;
    bool isName = false;
    std::uint64_t classes = 0;
    std::string_view text;
    const SyntaxNode* parent = nullptr;
    size_t depth = 0;
    SyntaxChildList children;

    // Break model scratch storage. These fields are valid only for the active formatting pass mark.
    mutable const PrintToken* formatPrintToken = nullptr;
    mutable std::uint32_t formatSelectionMark = 0;
    mutable std::uint32_t formatTokenMark = 0;
    mutable bool formatSpaceBefore = false;
    mutable std::uint8_t compactCallableBodyCache = 0;
};

// Owns normalized nodes, child storage, and source text. Reserve node capacity
// before construction so borrowed node pointers remain stable; consumers may
// cache facts but must not restructure the model during printing.
struct FormatModel {
    FormatModel();
    FormatModel(const FormatModel&) = delete;
    FormatModel& operator=(const FormatModel&) = delete;
    FormatModel(FormatModel&&) noexcept = default;
    FormatModel& operator=(FormatModel&&) noexcept = default;

    ParseResult parse;
    std::unique_ptr<std::string> sourceText;
    std::unique_ptr<std::pmr::monotonic_buffer_resource> childStorage;
    std::vector<SyntaxNode> nodes;
    SyntaxNode* root = nullptr;
};

inline bool SyntaxNodeHasClass(const SyntaxNode& node, SyntaxNodeClass syntaxNodeClass) {
    return (node.classes & static_cast<std::uint64_t>(syntaxNodeClass)) != 0 ||
        SyntaxNodeKindHasClass(node.kind, syntaxNodeClass);
}

// Header membership uses grammar roles, including conditions projected through flattened wrappers.
bool IsConditionalPreprocessorHeaderChild(const SyntaxNode& node, size_t index);

// Construction operations share the model arena and maintain parent/depth metadata.
SyntaxNode* MakeSyntaxNode(FormatModel& model, SyntaxNodeKind kind = SyntaxNodeKind::Unknown);
void ReparentSyntaxNode(SyntaxNode& node, const SyntaxNode* parent);
void AppendSyntaxChild(SyntaxNode& parent, SyntaxNode* child);

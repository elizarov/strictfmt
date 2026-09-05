#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>

#include "format/impl/format_print_token.h"

struct FormatterConfig;
struct FormatModelTextStats;
struct FormatBreakModel;
struct FormatBreakModelContext;
struct FormatBreakSolution;

struct FormatDeclarationLayoutView {
    const FormatBreakModel* model;
    const FormatBreakSolution* solution;
};

// Pre-analyzes declaration values, owns grouping state and cached solved layouts.
// Tokens/model/config must outlive this object; boundary flags parallel tokens and
// are consumed only during construction. Query boundaries in emission order.
// Reuse requires identical token adjacency, incoming state, and model context;
// diagnostic dumping must bypass reuse. Returned views live as long as this object.
class FormatDeclarationLayout {
public:
    FormatDeclarationLayout(
        const FormatterConfig& config,
        std::span<const PrintToken> tokens,
        std::span<const std::uint8_t> mandatoryBlockOpens,
        FormatModelTextStats* stats = nullptr
    );
    ~FormatDeclarationLayout();

    bool NeedsBlankLineBefore(size_t tokenIndex);
    std::optional<FormatDeclarationLayoutView> FindReusableLayout(
        std::span<const PrintToken> tokens,
        const FormatBreakModelContext& context,
        int startColumn,
        int baseIndentLevel,
        int breakLineSuffixWidth
    ) const;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

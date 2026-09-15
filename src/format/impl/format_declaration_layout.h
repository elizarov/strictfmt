#pragma once

#include <memory>
#include <optional>
#include <span>

#include "format/impl/format_print_token.h"

class FormatLayoutTree;
struct FormatLayoutProgram;

struct FormatDeclarationBoundary {
    const SyntaxNode* left = nullptr;
    const SyntaxNode* right = nullptr;
    bool required = false;
};

// Schedules declaration-group boundaries in source order. Once planning finishes,
// grouping observes the selected program; it never builds or solves a layout.
class FormatDeclarationLayout {
public:
    explicit FormatDeclarationLayout(std::span<const PrintToken> tokens);
    ~FormatDeclarationLayout();
    std::optional<FormatDeclarationBoundary> BoundaryBefore(size_t tokenIndex);
    void Resolve(const FormatLayoutTree& tree, FormatLayoutProgram& program) const;

private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

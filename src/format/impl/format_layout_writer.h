#pragma once

#include <optional>
#include <span>
#include <string_view>

#include "format/impl/format_output.h"

class FormatLayoutProgramBuilder;
class FormatLayoutTree;
struct FormatBreakNode;
struct FormatBreakToken;
struct PrintToken;

struct FormatLayoutWriteContext {
    std::span<const PrintToken> sourceTokens;
    int structuralIndent = 0;
    int indentWidth = 4;
    bool macroContinuation = false;
};

// Converts selected token writes and boundaries into measured program commands.
// Context is a value snapshot; the writer cannot call back into the planner.
class FormatLayoutWriter final {
public:
    FormatLayoutWriter(
        FormatLayoutProgramBuilder& program, const FormatLayoutTree& tree, FormatLayoutWriteContext context
    );

    void EnterNode(const FormatBreakNode& node);
    void LeaveNode();
    const FormatOutputState& State() const;
    void WriteToken(
        const FormatBreakToken& token,
        std::string_view text,
        std::optional<int> continuationBaseIndent,
        bool suppressSpace
    );
    void WriteComment(
        const PrintToken& token, std::string_view text, FormatOutputComment placement, bool spaceBefore = false
    );
    void Write(std::string_view text);
    void Space();
    void BreakLine(int indentLevel, bool blankLine);
    void SetPendingIndent(int indentLevel);

private:
    FormatLayoutProgramBuilder& program_;
    const FormatLayoutTree& tree_;
    const FormatLayoutWriteContext context_;
};

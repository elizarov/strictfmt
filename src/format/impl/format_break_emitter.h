#pragma once

#include <optional>
#include <string_view>
#include <vector>

struct FormatterConfig;
struct FormatBreakModel;
struct FormatBreakNode;
struct FormatBreakToken;
struct FormatBreakSolution;
struct SyntaxNode;

struct FormatBreakOutputState {
    bool atLineStart;
    bool lineHasText;
    std::optional<int> pendingIndentLevel;
};

// Physical output adapter: token writes own spacing, comments, and macro line
// state; line breaks honor the requested indentation and optional blank line.
// It does not choose layouts or expose the surrounding printer's structural state.
class FormatBreakOutput {
public:
    virtual ~FormatBreakOutput() = default;
    virtual FormatBreakOutputState State() const = 0;
    virtual void WriteToken(
        const FormatBreakToken& token,
        std::string_view text,
        std::optional<int> continuationBaseIndent,
        bool suppressSpace
    ) = 0;
    virtual void Write(std::string_view text) = 0;
    virtual void Space() = 0;
    virtual void BreakLine(int indentLevel, bool blankLine) = 0;
    virtual void SetPendingIndent(int indentLevel) = 0;
};

struct FormatBreakSplitList {
    const SyntaxNode* openToken = nullptr;
    int itemIndent = 0;
    int closeIndent = 0;
};

struct FormatBreakChainIndent {
    const FormatBreakNode* chain;
    int baseIndent;
};

struct FormatBreakEmissionSummary {
    std::optional<int> blockOpenIndent;
    std::vector<FormatBreakSplitList> splitLists;
    std::vector<FormatBreakChainIndent> chainIndents;
};

// Emits exactly the solved choices/render bases for one segment. terminalToken
// identifies its final source node, whose block-open indentation may be needed by
// the caller. The summary preserves deferred list choices and chain bases in
// traversal order; its references borrow the model and must be consumed before it dies.
FormatBreakEmissionSummary EmitFormatBreakModel(
    const FormatterConfig& config,
    const FormatBreakModel& model,
    const FormatBreakSolution& solution,
    int baseIndent,
    const SyntaxNode* terminalToken,
    FormatBreakOutput& output
);

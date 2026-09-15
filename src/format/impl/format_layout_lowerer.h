#pragma once

#include <optional>
#include <string_view>
#include <vector>

class FormatLayoutTree;
struct FormatterConfig;
struct FormatBreakModel;
struct FormatBreakNode;
struct FormatBreakToken;
struct FormatBreakSolution;
struct SyntaxNode;

struct FormatLayoutSinkState {
    bool atLineStart;
    bool lineHasText;
    std::optional<int> pendingIndentLevel;
};

// Planning adapter: records token writes and boundaries, measuring their physical
// state exactly. Node scopes attach indentation anchors to persistent owners.
class FormatLayoutSink {
public:
    virtual ~FormatLayoutSink() = default;
    virtual void EnterNode(const FormatBreakNode& node) = 0;
    virtual void LeaveNode() = 0;
    virtual FormatLayoutSinkState State() const = 0;
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

// Lowers one selected cost region into the persistent output program and records
// placements on its owners. Physical emission consumes only the completed program.
void LowerFormatLayout(
    const FormatterConfig& config,
    const FormatBreakModel& model,
    const FormatBreakSolution& solution,
    int baseIndent,
    FormatLayoutTree& tree,
    FormatLayoutSink& output
);

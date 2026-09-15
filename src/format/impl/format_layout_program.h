#pragma once

#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "format/impl/format_break_model.h"
#include "format/impl/format_output.h"
#include "format/impl/format_declaration_layout.h"

using FormatLayoutOwnerId = size_t;

enum class FormatLayoutCommandKind {
    Write,
    WriteMacroText,
    WriteVerbatim,
    CompleteLines,
    Space,
    HardLine,
    BlankLine,
    Reopen,
    ResetComments,
    Comment,
    GroupBoundary,
};

struct FormatLayoutIndentAnchor {
    FormatLayoutOwnerId owner = 0;
    int indent = 0;
};

struct FormatLayoutCommand {
    FormatLayoutCommandKind kind;
    std::string_view text;
    size_t anchor = 0;
    std::span<const size_t> continuations;
    const SyntaxNode* alignmentGroup = nullptr;
    FormatOutputComment commentPlacement = FormatOutputComment::Trailing;
    bool flag = false;
    bool secondaryFlag = false;
    size_t boundary = 0;
};

struct FormatLayoutTokenLines {
    size_t first = std::numeric_limits<size_t>::max();
    size_t last = 0;
};

// The selected physical layout. Text, offsets, and immutable indentation anchors
// are owned here. Writes carry resolved indentation, independent of prior line
// state. Syntax pointers are borrowed grouping or comment-alignment identities.
struct FormatLayoutProgram {
    std::vector<FormatLayoutCommand> commands;
    std::vector<FormatLayoutIndentAnchor> anchors{{}};
    FormatBreakArena<char> textStorage;
    FormatBreakArena<size_t> offsetStorage;
    std::vector<FormatLayoutTokenLines> tokenLines;
    std::vector<FormatDeclarationBoundary> groupBoundaries;
    size_t reserveSize = 0;
};

// Planning uses the same physical state machine as emission, without running its
// final alignment pass. Each write records its resolved indentation for replay.
class FormatLayoutProgramBuilder {
public:
    FormatLayoutProgramBuilder(int indentWidth, int columnLimit);

    class Scope {
    public:
        explicit Scope(FormatLayoutProgramBuilder& builder) : builder_(builder) {}
        ~Scope() { builder_.PopOwner(); }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

    private:
        FormatLayoutProgramBuilder& builder_;
    };

    Scope TokenScope(const PrintToken& token, FormatLayoutOwnerId owner);
    void PushOwner(FormatLayoutOwnerId owner);
    void PopOwner();
    void SetTokenCount(size_t count);
    void Reserve(size_t size);
    FormatLayoutProgram Finish();
    const FormatOutputState& State() const;
    int CurrentColumn(int structuralIndent) const;
    int CurrentLineIndentLevel() const;
    void SetPendingIndent(std::optional<int> indent);
    void ForceColumnZero();
    void NewLine(bool macroContinuation = false);
    void BlankLine(bool macroContinuation = false);
    void GroupBoundary(FormatDeclarationBoundary boundary, bool macroContinuation);
    void ReopenLastLine(bool discardBlankLines = false);
    void Write(std::string_view text, int structuralIndent);
    void WriteMacroText(std::string_view text, std::span<const size_t> continuations, int structuralIndent);
    void WriteAtIndent(std::string_view text, int indent);
    void WriteVerbatim(std::string_view text);
    void AppendCompleteLines(std::string_view text);
    void Space();
    void ResetCommentContinuation();
    void WriteComment(
        std::string_view text,
        int structuralIndent,
        const SyntaxNode* alignmentGroup,
        FormatOutputComment placement,
        bool lineComment,
        bool spaceBefore = false
    );

private:
    struct OwnerState {
        FormatLayoutOwnerId owner = 0;
        std::optional<size_t> token;
    };

    int indentWidth_;
    FormatOutput measure_;
    FormatLayoutProgram program_;
    OwnerState owner_;
    std::vector<OwnerState> ownerStack_;
    bool finished_ = false;

    void CheckPlanning() const;
    int ResolvedIndent(int structuralIndent) const;
    size_t Anchor(int indent);
    void Record(FormatLayoutCommand command);
    void TraceText(std::string_view text, size_t firstLine);
};

std::string EmitFormatLayoutProgram(const FormatLayoutProgram& program, int indentWidth, int columnLimit);

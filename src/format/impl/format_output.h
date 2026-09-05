#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

struct SyntaxNode;

struct FormatOutputState {
    bool atLineStart = true;
    bool lineHasText = false;
    bool macroContinuation = false;
    std::optional<int> pendingIndentLevel;
};

enum class FormatOutputComment {
    Trailing,
    Standalone,
    Continuation,
};

// Owns physical text, columns, pending line indentation, macro line suffixes, and
// deferred comment alignment. The caller supplies structural indentation and
// chooses breaks; this module never traverses syntax or chooses a layout.
// Comment groups are borrowed identity keys, valid until Finish. Verbatim text
// already has its indentation; complete-line text also includes its final newline.
// Finish trims and aligns the accumulated output and consumes this one-shot buffer.
class FormatOutput {
public:
    FormatOutput(int indentWidth, int columnLimit);
    ~FormatOutput();

    void Reserve(size_t size);
    std::string Finish();
    const FormatOutputState& State() const;
    int CurrentColumn(int structuralIndent) const;
    int CurrentLineIndentLevel() const;
    void SetPendingIndent(std::optional<int> indent);
    void ForceColumnZero();

    void NewLine(bool macroContinuation = false);
    void BlankLine();
    void ReopenLastLine(bool discardBlankLines = false);
    void Write(std::string_view text, int structuralIndent);
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
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

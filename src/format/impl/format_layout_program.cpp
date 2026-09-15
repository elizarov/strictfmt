#include "format/impl/format_layout_program.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

FormatLayoutProgramBuilder::FormatLayoutProgramBuilder(int indentWidth, int columnLimit) :
    indentWidth_(std::max(1, indentWidth)), measure_(indentWidth_, columnLimit) {}

FormatLayoutProgramBuilder::Scope
    FormatLayoutProgramBuilder::TokenScope(const PrintToken& token, FormatLayoutOwnerId owner)
{
    PushOwner(owner);
    owner_.token = token.sourceIndex;
    return Scope(*this);
}
void FormatLayoutProgramBuilder::PushOwner(FormatLayoutOwnerId owner) {
    CheckPlanning();
    ownerStack_.push_back(owner_);
    owner_ = {.owner = owner == 0 ? owner_.owner : owner};
}
void FormatLayoutProgramBuilder::PopOwner() {
    owner_ = ownerStack_.back();
    ownerStack_.pop_back();
}
void FormatLayoutProgramBuilder::SetTokenCount(size_t count) {
    CheckPlanning();
    program_.tokenLines.resize(count);
    // Most tokens add one write and some add spacing or line commands. Reserve
    // from the known input size instead of repeatedly copying resolved commands.
    program_.commands.reserve(count + count / 2);
    program_.anchors.reserve(count + 1);
}
void FormatLayoutProgramBuilder::Reserve(size_t size) {
    CheckPlanning();
    program_.reserveSize = size;
    measure_.Reserve(size);
}
FormatLayoutProgram FormatLayoutProgramBuilder::Finish() {
    if (finished_ || !ownerStack_.empty()) {
        throw std::logic_error("layout program is not ready to finish");
    }
    finished_ = true;
    return std::move(program_);
}
const FormatOutputState& FormatLayoutProgramBuilder::State() const { return measure_.State(); }
int FormatLayoutProgramBuilder::CurrentColumn(int indent) const { return measure_.CurrentColumn(indent); }
int FormatLayoutProgramBuilder::CurrentLineIndentLevel() const { return measure_.CurrentLineIndentLevel(); }

void FormatLayoutProgramBuilder::CheckPlanning() const {
    if (finished_) {
        throw std::logic_error("layout program is already complete");
    }
}
int FormatLayoutProgramBuilder::ResolvedIndent(int indent) const {
    return measure_.State().atLineStart ? measure_.CurrentColumn(indent) / indentWidth_ : indent;
}
size_t FormatLayoutProgramBuilder::Anchor(int indent) {
    CheckPlanning();
    if (
        program_.anchors.size() > 1 &&
        program_.anchors.back().owner == owner_.owner &&
        program_.anchors.back().indent == indent
    ) {
        return program_.anchors.size() - 1;
    }
    program_.anchors.push_back({owner_.owner, indent});
    return program_.anchors.size() - 1;
}
void FormatLayoutProgramBuilder::Record(FormatLayoutCommand command) {
    CheckPlanning();
    const auto text = program_.textStorage.Append(std::span(command.text.data(), command.text.size()));
    command.text = text.empty() ? std::string_view{} : std::string_view(text.data(), text.size());
    command.continuations = program_.offsetStorage.Append(command.continuations);
    program_.commands.push_back(command);
}
void FormatLayoutProgramBuilder::TraceText(std::string_view text, size_t firstLine) {
    if (text.empty() || !owner_.token || *owner_.token >= program_.tokenLines.size()) {
        return;
    }
    auto& lines = program_.tokenLines[*owner_.token];
    lines.first = std::min(lines.first, firstLine);
    lines.last = std::max(lines.last, measure_.CurrentLineIndex());
}
void FormatLayoutProgramBuilder::SetPendingIndent(std::optional<int> indent) {
    CheckPlanning();
    measure_.SetPendingIndent(indent);
}
void FormatLayoutProgramBuilder::ForceColumnZero() {
    CheckPlanning();
    measure_.ForceColumnZero();
}
void FormatLayoutProgramBuilder::NewLine(bool macroContinuation) {
    Record({.kind = FormatLayoutCommandKind::HardLine, .flag = macroContinuation});
    measure_.NewLine(macroContinuation);
}
void FormatLayoutProgramBuilder::BlankLine(bool macroContinuation) {
    Record({.kind = FormatLayoutCommandKind::BlankLine, .flag = macroContinuation});
    measure_.BlankLine(macroContinuation);
}
void FormatLayoutProgramBuilder::GroupBoundary(FormatDeclarationBoundary boundary, bool macroContinuation) {
    // A complete declaration already ends its physical line. Conditional empty
    // lines do not change the geometry passed to subsequent cost regions.
    if (!measure_.State().atLineStart) {
        throw std::logic_error("optional declaration boundary is not at a line boundary");
    }
    Record({
        .kind = FormatLayoutCommandKind::GroupBoundary,
        .flag = macroContinuation,
        .boundary = program_.groupBoundaries.size(),
    });
    program_.groupBoundaries.push_back(boundary);
}
void FormatLayoutProgramBuilder::ReopenLastLine(bool discardBlankLines) {
    Record({.kind = FormatLayoutCommandKind::Reopen, .flag = discardBlankLines});
    measure_.ReopenLastLine(discardBlankLines);
}
void FormatLayoutProgramBuilder::Write(std::string_view text, int indent) {
    Record({.kind = FormatLayoutCommandKind::Write, .text = text, .anchor = Anchor(ResolvedIndent(indent))});
    const auto first = measure_.CurrentLineIndex();
    measure_.Write(text, indent);
    TraceText(text, first);
}
void FormatLayoutProgramBuilder::WriteAtIndent(std::string_view text, int indent) {
    Record({.kind = FormatLayoutCommandKind::Write, .text = text, .anchor = Anchor(indent)});
    const auto first = measure_.CurrentLineIndex();
    measure_.WriteAtIndent(text, indent);
    TraceText(text, first);
}
void
    FormatLayoutProgramBuilder::WriteMacroText(std::string_view text, std::span<const size_t> continuations, int indent)
{
    Record({
        .kind = FormatLayoutCommandKind::WriteMacroText,
        .text = text,
        .anchor = Anchor(ResolvedIndent(indent)),
        .continuations = continuations,
    });
    const auto first = measure_.CurrentLineIndex();
    measure_.WriteMacroText(text, continuations, indent);
    TraceText(text, first);
}
void FormatLayoutProgramBuilder::WriteVerbatim(std::string_view text) {
    Record({.kind = FormatLayoutCommandKind::WriteVerbatim, .text = text});
    const auto first = measure_.CurrentLineIndex();
    measure_.WriteVerbatim(text);
    TraceText(text, first);
}
void FormatLayoutProgramBuilder::AppendCompleteLines(std::string_view text) {
    Record({.kind = FormatLayoutCommandKind::CompleteLines, .text = text});
    const auto first = measure_.CurrentLineIndex();
    measure_.AppendCompleteLines(text);
    TraceText(text, first);
}
void FormatLayoutProgramBuilder::Space() {
    Record({.kind = FormatLayoutCommandKind::Space});
    measure_.Space();
}
void FormatLayoutProgramBuilder::ResetCommentContinuation() {
    CheckPlanning();
    // Only a comment can establish a continuation anchor. Repeated resets have
    // no effect in either measurement or replay and need no output command.
    if (measure_.ResetCommentContinuation()) {
        Record({.kind = FormatLayoutCommandKind::ResetComments});
    }
}
void FormatLayoutProgramBuilder::WriteComment(
    std::string_view text,
    int indent,
    const SyntaxNode* group,
    FormatOutputComment placement,
    bool lineComment,
    bool spaceBefore
) {
    Record({
        .kind = FormatLayoutCommandKind::Comment,
        .text = text,
        .anchor = Anchor(ResolvedIndent(indent)),
        .alignmentGroup = group,
        .commentPlacement = placement,
        .flag = lineComment,
        .secondaryFlag = spaceBefore,
    });
    const auto first = measure_.CurrentLineIndex();
    measure_.WriteComment(text, indent, group, placement, lineComment, spaceBefore);
    TraceText(text, first);
}

std::string EmitFormatLayoutProgram(const FormatLayoutProgram& program, int indentWidth, int columnLimit) {
    FormatOutput output(indentWidth, columnLimit);
    output.Reserve(program.reserveSize);
    for (const auto& command : program.commands) {
        const int indent = program.anchors.at(command.anchor).indent;
        switch (command.kind) {
            case FormatLayoutCommandKind::Write:
                output.WriteAtIndent(command.text, indent);
                break;
            case FormatLayoutCommandKind::WriteMacroText:
                output.SetPendingIndent(indent);
                output.WriteMacroText(command.text, command.continuations, indent);
                break;
            case FormatLayoutCommandKind::WriteVerbatim:
                output.WriteVerbatim(command.text);
                break;
            case FormatLayoutCommandKind::CompleteLines:
                output.AppendCompleteLines(command.text);
                break;
            case FormatLayoutCommandKind::Space:
                output.Space();
                break;
            case FormatLayoutCommandKind::HardLine:
                output.NewLine(command.flag);
                break;
            case FormatLayoutCommandKind::BlankLine:
                output.BlankLine(command.flag);
                break;
            case FormatLayoutCommandKind::Reopen:
                output.ReopenLastLine(command.flag);
                break;
            case FormatLayoutCommandKind::ResetComments:
                output.ResetCommentContinuation();
                break;
            case FormatLayoutCommandKind::GroupBoundary:
                if (program.groupBoundaries.at(command.boundary).required) {
                    output.BlankLine(command.flag);
                }
                break;
            case FormatLayoutCommandKind::Comment:
                output.SetPendingIndent(indent);
                output.WriteComment(
                    command.text,
                    indent,
                    command.alignmentGroup,
                    command.commentPlacement,
                    command.flag,
                    command.secondaryFlag
                );
                break;
        }
    }
    return output.Finish();
}

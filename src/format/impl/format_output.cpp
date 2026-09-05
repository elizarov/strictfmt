#include "format/impl/format_output.h"

#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>

#include "util/utf8.h"

namespace {

constexpr size_t kNoCommentPosition = static_cast<size_t>(-1);

struct LineCommentPosition {
    const SyntaxNode* alignmentGroup = nullptr;
    size_t commentOffset = 0;
    int commentColumn = 0;
    int commentWidth = 0;
    int continuationWidth = 0;
    size_t continuationAnchor = kNoCommentPosition;
    bool alignTrailingRun = false;
};

}  // namespace

struct FormatOutput::Impl {
    Impl(int indentWidth, int columnLimit) : indentWidth_(indentWidth), columnLimit_(columnLimit) {}

    int indentWidth_;
    int columnLimit_;
    FormatOutputState state_;
    std::string output_;
    std::vector<LineCommentPosition> lineComments_;
    std::optional<size_t> activeCommentContinuationAnchor_;
    int currentColumn_ = 0;
    bool forceColumnZeroLine_ = false;

    void TrimTrailingSpaces() {
        while (!output_.empty() && output_.back() == ' ') {
            output_.pop_back();
            currentColumn_ = std::max(0, currentColumn_ - 1);
        }
    }

    bool HasOutputContent() const {
        for (char ch : output_) {
            if (ch != '\n') {
                return true;
            }
        }
        return false;
    }

    void FinishLine() { TrimTrailingSpaces(); }

    void TrimTrailingBlankLines() {
        while (output_.size() >= 2 && output_.back() == '\n' && output_[output_.size() - 2] == '\n') {
            output_.pop_back();
        }
    }

    void NewLine(bool macroContinuation = false) {
        FinishLine();
        if (macroContinuation && state_.lineHasText) {
            output_.append(" \\");
        }
        if (output_.empty() || output_.back() != '\n') {
            output_.push_back('\n');
        }
        state_.atLineStart = true;
        state_.lineHasText = false;
        currentColumn_ = 0;
        state_.macroContinuation = macroContinuation;
        forceColumnZeroLine_ = false;
        state_.pendingIndentLevel.reset();
    }

    void BlankLine() {
        if (!HasOutputContent() && !state_.lineHasText) {
            state_.atLineStart = true;
            currentColumn_ = 0;
            state_.macroContinuation = false;
            forceColumnZeroLine_ = false;
            state_.pendingIndentLevel.reset();
            return;
        }
        NewLine(false);
        if (output_.size() < 2 || output_[output_.size() - 2] != '\n') {
            output_.push_back('\n');
        }
        state_.atLineStart = true;
        state_.lineHasText = false;
        currentColumn_ = 0;
        state_.macroContinuation = false;
        forceColumnZeroLine_ = false;
        state_.pendingIndentLevel.reset();
    }

    void ReopenLastOutputLine() {
        if (!output_.empty() && output_.back() == '\n') {
            output_.pop_back();
        }
        currentColumn_ = 0;
        AdvanceCurrentColumn(output_);
        state_.atLineStart = false;
        state_.lineHasText = currentColumn_ > 0;
        state_.macroContinuation = false;
        forceColumnZeroLine_ = false;
        state_.pendingIndentLevel.reset();
    }

    void WriteIndentIfNeeded(int structuralIndent) {
        if (!state_.atLineStart) {
            return;
        }
        const int macroOffset = state_.macroContinuation ? 1 : 0;
        const int indentLevel =
            state_.pendingIndentLevel.value_or(forceColumnZeroLine_ ? 0 : structuralIndent + macroOffset);
        currentColumn_ = std::max(0, indentLevel) * indentWidth_;
        output_.append(static_cast<size_t>(currentColumn_), ' ');
        state_.atLineStart = false;
        state_.macroContinuation = false;
        forceColumnZeroLine_ = false;
        state_.pendingIndentLevel.reset();
    }

    void WriteAtIndent(std::string_view text, int structuralIndent) {
        if (!state_.atLineStart) {
            output_.append(text);
            AdvanceCurrentColumn(text);
            state_.lineHasText = state_.lineHasText || !text.empty();
            return;
        }
        const int adjustedIndent = std::max(0, structuralIndent);
        currentColumn_ = adjustedIndent * indentWidth_;
        output_.append(static_cast<size_t>(currentColumn_), ' ');
        state_.atLineStart = false;
        state_.macroContinuation = false;
        forceColumnZeroLine_ = false;
        state_.pendingIndentLevel.reset();
        output_.append(text);
        AdvanceCurrentColumn(text);
        state_.lineHasText = state_.lineHasText || !text.empty();
    }

    void Write(std::string_view text, int structuralIndent) {
        WriteIndentIfNeeded(structuralIndent);
        output_.append(text);
        AdvanceCurrentColumn(text);
        state_.lineHasText = state_.lineHasText || !text.empty();
    }

    size_t RecordLineCommentPosition(
        const SyntaxNode* alignmentGroup,
        std::string_view text,
        bool alignTrailingRun,
        size_t continuationAnchor = kNoCommentPosition
    ) {
        const size_t index = lineComments_.size();
        lineComments_.push_back({
            .alignmentGroup = alignmentGroup,
            .commentOffset = output_.size(),
            .commentColumn = currentColumn_,
            .commentWidth = Utf8CharacterCount(text),
            .continuationAnchor = continuationAnchor,
            .alignTrailingRun = alignTrailingRun,
        });
        if (continuationAnchor != kNoCommentPosition) {
            LineCommentPosition& anchor = lineComments_[continuationAnchor];
            anchor.continuationWidth = std::max(anchor.continuationWidth, Utf8CharacterCount(text));
        }
        return index;
    }

    void WriteTrailingComment(
        const SyntaxNode* alignmentGroup,
        std::string_view text,
        bool spaceBefore,
        bool lineComment,
        int structuralIndent
    ) {
        if (spaceBefore || lineComment) {
            Space();
        }
        if (lineComment) {
            output_.push_back(' ');
            ++currentColumn_;
            activeCommentContinuationAnchor_ = RecordLineCommentPosition(alignmentGroup, text, true);
        } else {
            activeCommentContinuationAnchor_.reset();
        }
        Write(text, structuralIndent);
    }

    void WriteStandaloneTrailingComment(
        const SyntaxNode* alignmentGroup, std::string_view text, bool lineComment, int structuralIndent
    ) {
        WriteIndentIfNeeded(structuralIndent);
        if (lineComment) {
            activeCommentContinuationAnchor_ = RecordLineCommentPosition(alignmentGroup, text, false);
        } else {
            activeCommentContinuationAnchor_.reset();
        }
        Write(text, structuralIndent);
    }

    void WriteCommentContinuation(
        const SyntaxNode* alignmentGroup, std::string_view text, bool lineComment, int structuralIndent
    ) {
        if (lineComment && activeCommentContinuationAnchor_) {
            if (state_.atLineStart) {
                forceColumnZeroLine_ = true;
                state_.pendingIndentLevel.reset();
            }
            WriteIndentIfNeeded(structuralIndent);
            RecordLineCommentPosition(alignmentGroup, text, false, *activeCommentContinuationAnchor_);
        }
        Write(text, structuralIndent);
    }

    bool AreOnAdjacentLines(const LineCommentPosition& left, const LineCommentPosition& right) const {
        return std::count(
            output_.begin() + static_cast<std::ptrdiff_t>(left.commentOffset),
            output_.begin() + static_cast<std::ptrdiff_t>(right.commentOffset),
            '\n'
        ) == 1;
    }

    void AlignLineComments() {
        std::vector<int> padding(lineComments_.size());
        for (size_t begin = 0; begin < lineComments_.size();) {
            if (!lineComments_[begin].alignTrailingRun) {
                ++begin;
                continue;
            }
            size_t end = begin + 1;
            while (
                end < lineComments_.size() &&
                lineComments_[end].alignTrailingRun &&
                lineComments_[begin].alignmentGroup != nullptr &&
                lineComments_[end].alignmentGroup == lineComments_[begin].alignmentGroup &&
                AreOnAdjacentLines(lineComments_[end - 1], lineComments_[end])
            ) {
                ++end;
            }
            if (end - begin >= 2) {
                int alignedColumn = 0;
                for (size_t index = begin; index < end; ++index) {
                    alignedColumn = std::max(alignedColumn, lineComments_[index].commentColumn);
                }
                bool fits = true;
                for (size_t index = begin; index < end; ++index) {
                    if (
                        alignedColumn +
                            std::max(lineComments_[index].commentWidth, lineComments_[index].continuationWidth) >
                            columnLimit_
                    ) {
                        fits = false;
                        break;
                    }
                }
                if (fits) {
                    for (size_t index = begin; index < end; ++index) {
                        padding[index] = alignedColumn - lineComments_[index].commentColumn;
                    }
                }
            }
            begin = end;
        }
        for (size_t index = 0; index < lineComments_.size(); ++index) {
            const size_t anchor = lineComments_[index].continuationAnchor;
            if (anchor == kNoCommentPosition) {
                continue;
            }
            const int anchorColumn = lineComments_[anchor].commentColumn + padding[anchor];
            padding[index] = std::max(0, anchorColumn - lineComments_[index].commentColumn);
        }
        const size_t totalPadding = std::accumulate(padding.begin(), padding.end(), size_t{0});
        if (totalPadding == 0) {
            return;
        }
        std::string aligned;
        aligned.reserve(output_.size() + totalPadding);
        size_t copied = 0;
        for (size_t index = 0; index < lineComments_.size(); ++index) {
            const size_t commentOffset = lineComments_[index].commentOffset;
            aligned.append(output_, copied, commentOffset - copied);
            aligned.append(static_cast<size_t>(padding[index]), ' ');
            copied = commentOffset;
        }
        aligned.append(output_, copied, output_.size() - copied);
        output_ = std::move(aligned);
    }

    void Space() {
        if (!state_.atLineStart && !output_.empty() && output_.back() != ' ' && output_.back() != '\n') {
            output_.push_back(' ');
            ++currentColumn_;
        }
    }

    int CurrentColumn(int structuralIndent) const {
        if (state_.atLineStart) {
            const int macroOffset = state_.macroContinuation ? 1 : 0;
            const int indentLevel =
                state_.pendingIndentLevel.value_or(forceColumnZeroLine_ ? 0 : structuralIndent + macroOffset);
            return std::max(0, indentLevel) * indentWidth_;
        }
        return currentColumn_;
    }

    void AdvanceCurrentColumn(std::string_view text) {
        const size_t newline = text.find_last_of('\n');
        if (newline == std::string_view::npos) {
            currentColumn_ += Utf8CharacterCount(text);
            return;
        }
        currentColumn_ = Utf8CharacterCount(text.substr(newline + 1));
    }

    int CurrentLineIndentLevel() const {
        const size_t lineStart = output_.find_last_of('\n');
        size_t cursor = lineStart == std::string::npos ? 0 : lineStart + 1;
        int spaces = 0;
        while (cursor < output_.size() && output_[cursor] == ' ') {
            ++spaces;
            ++cursor;
        }
        return spaces / indentWidth_;
    }

    void Reserve(size_t size) { output_.reserve(size); }
    std::string Finish() {
        FinishLine();
        TrimTrailingBlankLines();
        if (!output_.empty() && output_.back() != '\n') {
            output_.push_back('\n');
        }
        AlignLineComments();
        return std::move(output_);
    }
    void ForceColumnZero() {
        forceColumnZeroLine_ = true;
        state_.pendingIndentLevel.reset();
    }
    void WriteVerbatim(std::string_view text) {
        output_.append(text);
        AdvanceCurrentColumn(text);
        state_.lineHasText = true;
        state_.atLineStart = false;
    }
    void AppendCompleteLines(std::string_view text) {
        output_.append(text);
        currentColumn_ = 0;
        state_.atLineStart = true;
        state_.lineHasText = false;
    }
};

FormatOutput::FormatOutput(int indentWidth, int columnLimit) :
    impl_(std::make_unique<Impl>(std::max(1, indentWidth), columnLimit)) {}
FormatOutput::~FormatOutput() = default;

void FormatOutput::Reserve(size_t size) { impl_->Reserve(size); }
std::string FormatOutput::Finish() { return impl_->Finish(); }
const FormatOutputState& FormatOutput::State() const { return impl_->state_; }
int FormatOutput::CurrentColumn(int structuralIndent) const { return impl_->CurrentColumn(structuralIndent); }
int FormatOutput::CurrentLineIndentLevel() const { return impl_->CurrentLineIndentLevel(); }
void FormatOutput::SetPendingIndent(std::optional<int> indent) { impl_->state_.pendingIndentLevel = indent; }
void FormatOutput::ForceColumnZero() { impl_->ForceColumnZero(); }
void FormatOutput::NewLine(bool macroContinuation) { impl_->NewLine(macroContinuation); }
void FormatOutput::BlankLine() { impl_->BlankLine(); }
void FormatOutput::ReopenLastLine(bool discardBlankLines) {
    if (discardBlankLines) {
        impl_->TrimTrailingBlankLines();
    }
    impl_->ReopenLastOutputLine();
}
void FormatOutput::Write(std::string_view text, int structuralIndent) { impl_->Write(text, structuralIndent); }
void FormatOutput::WriteAtIndent(std::string_view text, int indent) { impl_->WriteAtIndent(text, indent); }
void FormatOutput::WriteVerbatim(std::string_view text) { impl_->WriteVerbatim(text); }
void FormatOutput::AppendCompleteLines(std::string_view text) { impl_->AppendCompleteLines(text); }
void FormatOutput::Space() { impl_->Space(); }
void FormatOutput::ResetCommentContinuation() { impl_->activeCommentContinuationAnchor_.reset(); }
void FormatOutput::WriteComment(
    std::string_view text,
    int structuralIndent,
    const SyntaxNode* alignmentGroup,
    FormatOutputComment placement,
    bool lineComment,
    bool spaceBefore
) {
    switch (placement) {
        case FormatOutputComment::Trailing:
            impl_->WriteTrailingComment(alignmentGroup, text, spaceBefore, lineComment, structuralIndent);
            break;
        case FormatOutputComment::Standalone:
            impl_->WriteStandaloneTrailingComment(alignmentGroup, text, lineComment, structuralIndent);
            break;
        case FormatOutputComment::Continuation:
            impl_->WriteCommentContinuation(alignmentGroup, text, lineComment, structuralIndent);
            break;
    }
}

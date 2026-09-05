#include "format/impl/format_candidates.h"

#include <algorithm>
#include <memory>
#include <utility>

FormatLayoutCandidates::FormatLayoutCandidates(const FormatLayoutCandidates& other) {
    for (const FormatLayoutCandidate& value : other) {
        push_back(value);
    }
}

FormatLayoutCandidates::FormatLayoutCandidates(FormatLayoutCandidates&& other) noexcept { MoveFrom(std::move(other)); }

FormatLayoutCandidates::~FormatLayoutCandidates() { clear(); }

FormatLayoutCandidates& FormatLayoutCandidates::operator=(const FormatLayoutCandidates& other) {
    if (this != &other) {
        clear();
        usingHeap_ = false;
        for (const FormatLayoutCandidate& value : other) {
            push_back(value);
        }
    }
    return *this;
}

FormatLayoutCandidates& FormatLayoutCandidates::operator=(FormatLayoutCandidates&& other) noexcept {
    if (this != &other) {
        clear();
        usingHeap_ = false;
        MoveFrom(std::move(other));
    }
    return *this;
}

FormatLayoutCandidates::iterator FormatLayoutCandidates::erase(iterator it) {
    const size_t index = static_cast<size_t>(it - begin());
    if (usingHeap_) {
        heap_.erase(heap_.begin() + static_cast<std::ptrdiff_t>(index));
        return heap_.data() + index;
    }
    for (size_t cursor = index + 1; cursor < inlineSize_; ++cursor) {
        InlineData()[cursor - 1] = std::move(InlineData()[cursor]);
    }
    --inlineSize_;
    std::destroy_at(InlineData() + inlineSize_);
    return InlineData() + index;
}

void FormatLayoutCandidates::clear() {
    if (usingHeap_) {
        heap_.clear();
        return;
    }
    for (size_t index = 0; index < inlineSize_; ++index) {
        std::destroy_at(InlineData() + index);
    }
    inlineSize_ = 0;
}

void FormatLayoutCandidates::MoveFrom(FormatLayoutCandidates&& other) {
    if (other.usingHeap_) {
        heap_ = std::move(other.heap_);
        usingHeap_ = true;
        other.usingHeap_ = false;
        return;
    }
    for (FormatLayoutCandidate& value : other) {
        push_back(std::move(value));
    }
    other.clear();
}

void FormatLayoutCandidates::MoveInlineToHeap() {
    heap_.reserve(kInlineCapacity * 2);
    for (size_t index = 0; index < inlineSize_; ++index) {
        heap_.push_back(std::move(InlineData()[index]));
        std::destroy_at(InlineData() + index);
    }
    inlineSize_ = 0;
    usingHeap_ = true;
}

int FormatCandidateOrder::MaximumOverflow(const FormatLayoutCandidate& result) const {
    return std::max(result.overflowSizeProfile.GreatestValue(), CurrentLineOverflow(result));
}

bool FormatCandidateOrder::HasOverflow(const FormatLayoutCandidate& result) const {
    return !result.overflowSizeProfile.Empty() || CurrentLineOverflow(result) > 0;
}

void FormatCandidateOrder::FinishCurrentLine(FormatLayoutCandidate& result, int suffixWidth) const {
    if (result.endLineHasText && !result.currentLineOverflowRecorded) {
        result.overflowSizeProfile.AddValue(std::max(0, result.endColumn + suffixWidth - columnLimit_));
        result.currentLineOverflowRecorded = true;
    }
}

bool FormatCandidateOrder::Better(const FormatLayoutCandidate& candidate, const FormatLayoutCandidate& incumbent) const
{
    if (!candidate.valid) {
        return false;
    }
    if (!incumbent.valid) {
        return true;
    }
    const int overflowProfileComparison = CompareFormatValueProfilesWithAdditionalValues(
        candidate.overflowSizeProfile,
        CurrentLineOverflow(candidate),
        incumbent.overflowSizeProfile,
        CurrentLineOverflow(incumbent)
    );
    if (overflowProfileComparison != 0) {
        return overflowProfileComparison < 0;
    }
    const int depthProfileComparison =
        CompareFormatValueProfiles(candidate.expansionDepthProfile, incumbent.expansionDepthProfile);
    if (depthProfileComparison != 0) {
        return depthProfileComparison < 0;
    }
    if (candidate.extraLines != incumbent.extraLines) {
        return candidate.extraLines < incumbent.extraLines;
    }
    return false;
}

bool FormatCandidateOrder::Dominates(const FormatLayoutCandidate& left, const FormatLayoutCandidate& right) const {
    if (
        left.endIndentLevel != right.endIndentLevel ||
        left.endLineHasText != right.endLineHasText ||
        left.currentLineOverflowRecorded != right.currentLineOverflowRecorded ||
        left.ownExpansionCharged != right.ownExpansionCharged ||
        left.compactNextStreamOperand != right.compactNextStreamOperand
    ) {
        return false;
    }
    if (left.endColumn > right.endColumn) {
        return false;
    }
    const int overflowProfileComparison = CompareFormatValueProfilesWithAdditionalValues(
        left.overflowSizeProfile, CurrentLineOverflow(left), right.overflowSizeProfile, CurrentLineOverflow(right)
    );
    const int depthProfileComparison =
        CompareFormatValueProfiles(left.expansionDepthProfile, right.expansionDepthProfile);
    if (overflowProfileComparison > 0 || depthProfileComparison > 0 || left.extraLines > right.extraLines) {
        return false;
    }
    return overflowProfileComparison < 0 || depthProfileComparison < 0 || left.extraLines < right.extraLines;
}

bool FormatCandidateOrder::StateLess(const FormatLayoutCandidate& left, const FormatLayoutCandidate& right) {
    if (left.endColumn != right.endColumn) {
        return left.endColumn < right.endColumn;
    }
    if (left.endIndentLevel != right.endIndentLevel) {
        return left.endIndentLevel < right.endIndentLevel;
    }
    if (left.endLineHasText != right.endLineHasText) {
        return left.endLineHasText < right.endLineHasText;
    }
    if (left.currentLineOverflowRecorded != right.currentLineOverflowRecorded) {
        return left.currentLineOverflowRecorded < right.currentLineOverflowRecorded;
    }
    if (left.ownExpansionCharged != right.ownExpansionCharged) {
        return left.ownExpansionCharged < right.ownExpansionCharged;
    }
    return left.compactNextStreamOperand < right.compactNextStreamOperand;
}

void FormatCandidateOrder::AddPruned(FormatLayoutCandidates& results, FormatLayoutCandidate candidate) const {
    if (!candidate.valid) {
        return;
    }
    for (auto it = results.begin(); it != results.end();) {
        if (SameState(*it, candidate)) {
            if (Better(candidate, *it)) {
                *it = std::move(candidate);
            }
            return;
        }
        if (Dominates(*it, candidate)) {
            return;
        }
        if (Dominates(candidate, *it)) {
            it = results.erase(it);
            continue;
        }
        ++it;
    }
    results.push_back(std::move(candidate));
}

void FormatCandidateOrder::Sort(FormatLayoutCandidates& results) {
    std::sort(results.begin(), results.end(), StateLess);
}

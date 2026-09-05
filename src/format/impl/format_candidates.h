#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <initializer_list>
#include <new>
#include <vector>

#include "format/impl/format_choice_history.h"
#include "format/impl/format_value_profile.h"

// Candidate costs and the complete continuation state needed by exact pruning.
// Choices borrow immutable history entries; copying candidates owns their cost
// profiles but does not extend the history lifetime.
struct FormatLayoutCandidate {
    bool valid = false;
    int endColumn = 0;
    int endIndentLevel = 0;
    bool endLineHasText = false;
    // Exact delimiter-stack search can rewind a line after pricing it. Normal results keep this false.
    bool currentLineOverflowRecorded = false;
    int extraLines = 0;
    FormatValueProfile overflowSizeProfile;
    FormatValueProfile expansionDepthProfile;
    bool ownExpansionCharged = false;
    bool compactNextStreamOperand = false;
    FormatChoiceHistory::Handle choices = nullptr;
};

// Ordered candidate storage with eight inline entries. It preserves value
// semantics and contiguous iteration; allocation/spill mechanics stay private.
// Tiny append paths stay inline so one-candidate temporaries avoid call/copy overhead.
class FormatLayoutCandidates {
public:
    using iterator = FormatLayoutCandidate*;
    using const_iterator = const FormatLayoutCandidate*;

    FormatLayoutCandidates() = default;
    FormatLayoutCandidates(std::initializer_list<FormatLayoutCandidate> values) {
        for (const FormatLayoutCandidate& value : values) {
            push_back(value);
        }
    }
    FormatLayoutCandidates(const FormatLayoutCandidates& other);
    FormatLayoutCandidates(FormatLayoutCandidates&& other) noexcept;
    ~FormatLayoutCandidates();
    FormatLayoutCandidates& operator=(const FormatLayoutCandidates& other);
    FormatLayoutCandidates& operator=(FormatLayoutCandidates&& other) noexcept;
    iterator begin() { return usingHeap_ ? heap_.data() : InlineData(); }
    iterator end() { return begin() + size(); }
    const_iterator begin() const { return usingHeap_ ? heap_.data() : InlineData(); }
    const_iterator end() const { return begin() + size(); }
    bool empty() const { return size() == 0; }
    size_t size() const { return usingHeap_ ? heap_.size() : inlineSize_; }
    FormatLayoutCandidate& operator[](size_t index) { return begin()[index]; }
    const FormatLayoutCandidate& operator[](size_t index) const { return begin()[index]; }
    void push_back(const FormatLayoutCandidate& value) { PushBack(value); }
    void push_back(FormatLayoutCandidate&& value) { PushBack(std::move(value)); }
    iterator erase(iterator it);
    void clear();

private:
    static constexpr size_t kInlineCapacity = 8;

    FormatLayoutCandidate* InlineData() {
        return std::launder(reinterpret_cast<FormatLayoutCandidate*>(inlineStorage_));
    }
    const FormatLayoutCandidate* InlineData() const {
        return std::launder(reinterpret_cast<const FormatLayoutCandidate*>(inlineStorage_));
    }

    template <typename Value>
    void PushBack(Value&& value) {
        if (usingHeap_) {
            heap_.push_back(std::forward<Value>(value));
            return;
        }
        if (inlineSize_ < kInlineCapacity) {
            std::construct_at(InlineData() + inlineSize_, std::forward<Value>(value));
            ++inlineSize_;
            return;
        }
        MoveInlineToHeap();
        heap_.push_back(std::forward<Value>(value));
    }

    void MoveFrom(FormatLayoutCandidates&& other);
    void MoveInlineToHeap();

    alignas(FormatLayoutCandidate) std::byte inlineStorage_[sizeof(FormatLayoutCandidate) * kInlineCapacity];
    size_t inlineSize_ = 0;
    bool usingHeap_ = false;
    std::vector<FormatLayoutCandidate> heap_;
};

// Owns physical overflow accounting, cost order, and continuation-safe dominance.
// AddPruned keeps the earlier candidate on equal state/cost; Sort provides the
// existing deterministic state order. Candidate generation and specialized
// delimiter-partition tie-breaking remain with the solver.
class FormatCandidateOrder {
public:
    explicit FormatCandidateOrder(int columnLimit) : columnLimit_(columnLimit) {}
    int CurrentLineOverflow(const FormatLayoutCandidate& result) const {
        return result.endLineHasText && !result.currentLineOverflowRecorded ?
            std::max(0, result.endColumn - columnLimit_) : 0;
    }
    int MaximumOverflow(const FormatLayoutCandidate& result) const;
    bool HasOverflow(const FormatLayoutCandidate& result) const;
    void FinishCurrentLine(FormatLayoutCandidate& result, int suffixWidth = 0) const;
    bool Better(const FormatLayoutCandidate& candidate, const FormatLayoutCandidate& incumbent) const;
    static bool SameState(const FormatLayoutCandidate& left, const FormatLayoutCandidate& right) {
        return left.endColumn == right.endColumn &&
            left.endIndentLevel == right.endIndentLevel &&
            left.endLineHasText == right.endLineHasText &&
            left.currentLineOverflowRecorded == right.currentLineOverflowRecorded &&
            left.ownExpansionCharged == right.ownExpansionCharged &&
            left.compactNextStreamOperand == right.compactNextStreamOperand;
    }
    bool Dominates(const FormatLayoutCandidate& left, const FormatLayoutCandidate& right) const;
    void AddPruned(FormatLayoutCandidates& results, FormatLayoutCandidate candidate) const;
    static void Sort(FormatLayoutCandidates& results);

private:
    static bool StateLess(const FormatLayoutCandidate& left, const FormatLayoutCandidate& right);

    int columnLimit_;
};

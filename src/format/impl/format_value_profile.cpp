#include "format/impl/format_value_profile.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <span>

FormatValueProfile::FormatValueProfile(const FormatValueProfile& other) :
    inlineEntries_(other.inlineEntries_), inlineSize_(other.inlineSize_)
{
    if (other.heapEntries_ != nullptr) {
        heapEntries_ = std::make_unique<std::vector<Entry>>(*other.heapEntries_);
    }
}

FormatValueProfile& FormatValueProfile::operator=(const FormatValueProfile& other) {
    if (this == &other) {
        return *this;
    }
    inlineEntries_ = other.inlineEntries_;
    inlineSize_ = other.inlineSize_;
    if (other.heapEntries_ == nullptr) {
        heapEntries_.reset();
    } else if (heapEntries_ == nullptr) {
        heapEntries_ = std::make_unique<std::vector<Entry>>(*other.heapEntries_);
    } else {
        *heapEntries_ = *other.heapEntries_;
    }
    return *this;
}

void FormatValueProfile::AddValue(int value) {
    assert(value >= 0);
    if (value > 0) {
        AddOccurrences(value, 1);
    }
}

void FormatValueProfile::Add(const FormatValueProfile& other) {
    for (const Entry& entry : other.Entries()) {
        AddOccurrences(entry.value, entry.occurrences);
    }
}

bool FormatValueProfile::Empty() const { return inlineSize_ == 0 && heapEntries_ == nullptr; }

int FormatValueProfile::GreatestValue() const {
    const std::span<const Entry> entries = Entries();
    return entries.empty() ? 0 : entries.back().value;
}

void FormatValueProfile::AddOccurrences(int value, int occurrences) {
    assert(value > 0);
    assert(occurrences > 0);
    if (heapEntries_ != nullptr) {
        const auto found =
            std::lower_bound(heapEntries_->begin(), heapEntries_->end(), value, [](const Entry& entry, int candidate) {
                return entry.value < candidate;
            });
        if (found != heapEntries_->end() && found->value == value) {
            found->occurrences += occurrences;
        } else {
            heapEntries_->insert(found, {.value = value, .occurrences = occurrences});
        }
        return;
    }
    size_t index = 0;
    while (index < inlineSize_ && inlineEntries_[index].value < value) {
        ++index;
    }
    if (index < inlineSize_ && inlineEntries_[index].value == value) {
        inlineEntries_[index].occurrences += occurrences;
        return;
    }
    if (inlineSize_ < inlineEntries_.size()) {
        std::move_backward(
            inlineEntries_.begin() + static_cast<std::ptrdiff_t>(index),
            inlineEntries_.begin() + static_cast<std::ptrdiff_t>(inlineSize_),
            inlineEntries_.begin() + static_cast<std::ptrdiff_t>(inlineSize_ + 1)
        );
        inlineEntries_[index] = {.value = value, .occurrences = occurrences};
        ++inlineSize_;
        return;
    }
    heapEntries_ = std::make_unique<std::vector<Entry>>(inlineEntries_.begin(), inlineEntries_.end());
    AddOccurrences(value, occurrences);
}

std::span<const FormatValueProfile::Entry> FormatValueProfile::Entries() const {
    if (heapEntries_ != nullptr) {
        return *heapEntries_;
    }
    return {inlineEntries_.data(), inlineSize_};
}

int CompareFormatValueProfiles(const FormatValueProfile& left, const FormatValueProfile& right) {
    return CompareFormatValueProfilesWithAdditionalValues(left, 0, right, 0);
}

int CompareFormatValueProfilesWithAdditionalValues(
    const FormatValueProfile& left, int additionalLeftValue, const FormatValueProfile& right, int additionalRightValue
) {
    assert(additionalLeftValue >= 0);
    assert(additionalRightValue >= 0);
    const std::span<const FormatValueProfile::Entry> leftEntries = left.Entries();
    const std::span<const FormatValueProfile::Entry> rightEntries = right.Entries();
    size_t leftIndex = leftEntries.size();
    size_t rightIndex = rightEntries.size();
    bool hasAdditionalLeft = additionalLeftValue > 0;
    bool hasAdditionalRight = additionalRightValue > 0;
    while (leftIndex > 0 || rightIndex > 0 || hasAdditionalLeft || hasAdditionalRight) {
        int value = 0;
        if (leftIndex > 0) {
            value = std::max(value, leftEntries[leftIndex - 1].value);
        }
        if (rightIndex > 0) {
            value = std::max(value, rightEntries[rightIndex - 1].value);
        }
        if (hasAdditionalLeft) {
            value = std::max(value, additionalLeftValue);
        }
        if (hasAdditionalRight) {
            value = std::max(value, additionalRightValue);
        }

        int leftOccurrences = 0;
        if (leftIndex > 0 && leftEntries[leftIndex - 1].value == value) {
            leftOccurrences = leftEntries[leftIndex - 1].occurrences;
            --leftIndex;
        }
        if (hasAdditionalLeft && additionalLeftValue == value) {
            ++leftOccurrences;
            hasAdditionalLeft = false;
        }

        int rightOccurrences = 0;
        if (rightIndex > 0 && rightEntries[rightIndex - 1].value == value) {
            rightOccurrences = rightEntries[rightIndex - 1].occurrences;
            --rightIndex;
        }
        if (hasAdditionalRight && additionalRightValue == value) {
            ++rightOccurrences;
            hasAdditionalRight = false;
        }

        if (leftOccurrences != rightOccurrences) {
            return leftOccurrences < rightOccurrences ? -1 : 1;
        }
    }
    return 0;
}

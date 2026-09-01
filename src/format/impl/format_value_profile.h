#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

class FormatValueProfile {
public:
    FormatValueProfile() = default;
    FormatValueProfile(const FormatValueProfile& other);
    FormatValueProfile(FormatValueProfile&& other) noexcept = default;
    FormatValueProfile& operator=(const FormatValueProfile& other);
    FormatValueProfile& operator=(FormatValueProfile&& other) noexcept = default;

    void AddValue(int value);
    void Add(const FormatValueProfile& other);

    bool Empty() const;
    int GreatestValue() const;

private:
    struct Entry {
        int value = 0;
        int occurrences = 0;
    };

    void AddOccurrences(int value, int occurrences);
    std::span<const Entry> Entries() const;

    std::array<Entry, 4> inlineEntries_{};
    size_t inlineSize_ = 0;
    std::unique_ptr<std::vector<Entry>> heapEntries_;

    friend int CompareFormatValueProfiles(const FormatValueProfile& left, const FormatValueProfile& right);
    friend int CompareFormatValueProfilesWithAdditionalValues(
        const FormatValueProfile& left,
        int additionalLeftValue,
        const FormatValueProfile& right,
        int additionalRightValue
    );
};

// Returns a negative value when left is better, a positive value when right is better, and zero when equal.
int CompareFormatValueProfiles(const FormatValueProfile& left, const FormatValueProfile& right);

// Compares without materializing one additional occurrence in either profile. Non-positive values are omitted.
int CompareFormatValueProfilesWithAdditionalValues(
    const FormatValueProfile& left, int additionalLeftValue, const FormatValueProfile& right, int additionalRightValue
);

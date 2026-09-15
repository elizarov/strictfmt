#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

struct SyntaxNode;

// Append-only syntax identity lookup. Flat storage avoids one allocation per
// node; references are invalidated by growth. Null is a supported separate key.
template <class Value>
class FormatSyntaxMap {
    static_assert(std::is_trivially_copyable_v<Value>);

public:
    void Reserve(size_t count) {
        const size_t capacity = std::bit_ceil(std::max(size_t{8}, count * 2));
        if (capacity <= entries_.size()) {
            return;
        }
        auto previous = std::move(entries_);
        entries_.resize(capacity);
        for (const Entry& entry : previous) {
            if (entry.key != nullptr) {
                entries_[Slot(entry.key)] = entry;
            }
        }
    }

    const Value* Find(const SyntaxNode* key) const {
        if (key == nullptr) {
            return nullValue_ ? &*nullValue_ : nullptr;
        }
        if (entries_.empty()) {
            return nullptr;
        }
        const Entry& entry = entries_[Slot(key)];
        return entry.key == nullptr ? nullptr : &entry.value;
    }

    bool Contains(const SyntaxNode* key) const { return Find(key) != nullptr; }

    Value* Find(const SyntaxNode* key) { return const_cast<Value*>(std::as_const(*this).Find(key)); }

    std::pair<Value*, bool> Insert(const SyntaxNode* key, Value value) {
        if (key == nullptr) {
            const bool inserted = !nullValue_;
            if (inserted) {
                nullValue_ = value;
            }
            return {&*nullValue_, inserted};
        }
        if (count_ * 2 >= entries_.size()) {
            Reserve(count_ + 1);
        }
        Entry& entry = entries_[Slot(key)];
        const bool inserted = entry.key == nullptr;
        if (inserted) {
            entry = {key, value};
            ++count_;
        }
        return {&entry.value, inserted};
    }

    void InsertOrAssign(const SyntaxNode* key, Value value) { *Insert(key, value).first = value; }

private:
    struct Entry {
        const SyntaxNode* key = nullptr;
        Value value{};
    };

    std::vector<Entry> entries_;
    size_t count_ = 0;
    std::optional<Value> nullValue_;

    size_t Slot(const SyntaxNode* key) const {
        // Mix aligned, often contiguous node addresses before masking. Equality
        // always compares the original pointer, including on hash collisions.
        const auto bits = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(key));
        const auto mixed = (bits >> 3) * UINT64_C(11400714819323198485);
        const size_t mask = entries_.size() - 1;
        size_t index = static_cast<size_t>(mixed ^ (mixed >> 32)) & mask;
        while (entries_[index].key != nullptr && entries_[index].key != key) {
            index = (index + 1) & mask;
        }
        return index;
    }
};

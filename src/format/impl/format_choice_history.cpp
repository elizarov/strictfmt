#include "format/impl/format_choice_history.h"

#include <algorithm>
#include <deque>
#include <limits>

struct FormatChoiceHistory::Entry {
    const Entry* left = nullptr;
    const Entry* right = nullptr;
    size_t index = 0;
    int firstNodeId = -1;
    int lastNodeId = -1;
    int nodeId = -1;
    int indentLevel = -1;
    std::uint32_t attachedChainOperator = std::numeric_limits<std::uint32_t>::max();
    FormatBreakChoice choice = FormatBreakChoice::Compact;
    bool leaf = false;
};

struct FormatChoiceHistory::Impl {
    std::deque<Entry> choiceArena_;
    std::vector<std::uint32_t> visited_;
    std::vector<const Entry*> pending_;
    std::uint32_t query_ = 0;

    const Entry* MakeEntry(Entry entry) {
        entry.index = choiceArena_.size();
        choiceArena_.push_back(entry);
        return &choiceArena_.back();
    }

    const Entry* MakeChoice(int nodeId, FormatBreakChoice choice, int indentLevel) {
        return MakeEntry(Entry{
            .firstNodeId = nodeId,
            .lastNodeId = nodeId,
            .nodeId = nodeId,
            .indentLevel = indentLevel,
            .choice = choice,
            .leaf = true,
        });
    }

    const Entry* ConcatChoices(const Entry* left, const Entry* right) {
        if (left == nullptr) {
            return right;
        }
        if (right == nullptr) {
            return left;
        }
        return MakeEntry(Entry{
            .left = left,
            .right = right,
            .firstNodeId = std::min(left->firstNodeId, right->firstNodeId),
            .lastNodeId = std::max(left->lastNodeId, right->lastNodeId),
        });
    }

};

FormatChoiceHistory::FormatChoiceHistory() : impl_(std::make_unique<Impl>()) {}
FormatChoiceHistory::~FormatChoiceHistory() = default;
FormatChoiceHistory::Handle FormatChoiceHistory::Concat(Handle left, Handle right) {
    return impl_->ConcatChoices(left, right);
}
FormatChoiceHistory::Handle
    FormatChoiceHistory::AddChoice(Handle history, int nodeId, FormatBreakChoice choice, int indentLevel)
{
    return Concat(history, impl_->MakeChoice(nodeId, choice, indentLevel));
}
FormatChoiceHistory::Handle FormatChoiceHistory::AddAttachedOperator(Handle history, std::uint32_t sourceIndex) {
    return Concat(history, impl_->MakeEntry(Entry{.attachedChainOperator = sourceIndex, .leaf = true}));
}
std::optional<FormatBreakChoice> FormatChoiceHistory::Find(Handle tree, int nodeId) {
    if (tree == nullptr || nodeId < tree->firstNodeId || nodeId > tree->lastNodeId) {
        return std::nullopt;
    }
    if (tree->leaf) {
        return tree->choice;
    }
    auto& state = *impl_;
    if (++state.query_ == 0) {
        std::fill(state.visited_.begin(), state.visited_.end(), 0);
        ++state.query_;
    }
    if (state.visited_.size() <= tree->index) {
        state.visited_.resize(tree->index + 1, 0);
    }
    state.pending_.clear();
    state.pending_.push_back(tree);
    while (!state.pending_.empty()) {
        const auto* entry = state.pending_.back();
        state.pending_.pop_back();
        if (nodeId < entry->firstNodeId || nodeId > entry->lastNodeId || state.visited_[entry->index] == state.query_) {
            continue;
        }
        state.visited_[entry->index] = state.query_;
        if (entry->leaf) {
            return entry->choice;
        }
        // Right-first traversal finds the latest record. Repeating a shared
        // subtree cannot change a failed search, so visit it once per query.
        state.pending_.push_back(entry->left);
        state.pending_.push_back(entry->right);
    }
    return std::nullopt;
}

FormatBreakSolution FormatChoiceHistory::Materialize(Handle history, size_t choiceCount) {
    FormatBreakSolution solution;
    solution.choices.assign(choiceCount, FormatBreakChoice::Compact);
    solution.indentLevels.assign(choiceCount, -1);
    std::vector<bool> assigned(choiceCount, false);
    std::vector<bool> visited(history == nullptr ? 0 : history->index + 1, false);
    std::vector<Handle> pending;
    if (history != nullptr) {
        pending.push_back(history);
    }
    while (!pending.empty()) {
        const auto* entry = pending.back();
        pending.pop_back();
        if (visited[entry->index]) {
            continue;
        }
        visited[entry->index] = true;
        if (!entry->leaf) {
            // Left-first traversal preserves the first record for each node.
            pending.push_back(entry->right);
            pending.push_back(entry->left);
            continue;
        }
        const size_t index = static_cast<size_t>(entry->nodeId);
        if (index < choiceCount && !assigned[index]) {
            solution.choices[index] = entry->choice;
            solution.indentLevels[index] = entry->indentLevel;
            assigned[index] = true;
        }
        if (entry->attachedChainOperator != std::numeric_limits<std::uint32_t>::max()) {
            solution.attachedChainOperators.push_back(entry->attachedChainOperator);
        }
    }
    std::sort(solution.attachedChainOperators.begin(), solution.attachedChainOperators.end());
    solution.attachedChainOperators.erase(
        std::unique(solution.attachedChainOperators.begin(), solution.attachedChainOperators.end()),
        solution.attachedChainOperators.end()
    );
    return solution;
}

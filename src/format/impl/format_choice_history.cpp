#include "format/impl/format_choice_history.h"

#include <algorithm>
#include <deque>
#include <limits>

struct FormatChoiceHistory::Entry {
    const Entry* left = nullptr;
    const Entry* right = nullptr;
    int nodeId = -1;
    int indentLevel = -1;
    int declarationValueContinuationLines = -1;
    std::uint32_t attachedChainOperator = std::numeric_limits<std::uint32_t>::max();
    FormatBreakChoice choice = FormatBreakChoice::Compact;
    bool leaf = false;
};

struct FormatChoiceHistory::Impl {
    std::deque<Entry> choiceArena_;

    const Entry* MakeChoice(int nodeId, FormatBreakChoice choice, int indentLevel) {
        choiceArena_.push_back(Entry{.nodeId = nodeId, .indentLevel = indentLevel, .choice = choice, .leaf = true});
        return &choiceArena_.back();
    }

    const Entry* ConcatChoices(const Entry* left, const Entry* right) {
        if (left == nullptr) {
            return right;
        }
        if (right == nullptr) {
            return left;
        }
        choiceArena_.push_back(Entry{.left = left, .right = right});
        return &choiceArena_.back();
    }

};

namespace {

void AppendChoices(
    const FormatChoiceHistory::Entry* tree,
    std::vector<FormatBreakChoice>& choices,
    std::vector<int>& indentLevels,
    std::vector<bool>& assigned
) {
    if (tree == nullptr) {
        return;
    }
    if (tree->leaf) {
        const size_t index = static_cast<size_t>(tree->nodeId);
        if (index < choices.size() && !assigned[index]) {
            choices[index] = tree->choice;
            indentLevels[index] = tree->indentLevel;
            assigned[index] = true;
        }
        return;
    }
    AppendChoices(tree->left, choices, indentLevels, assigned);
    AppendChoices(tree->right, choices, indentLevels, assigned);
}

void
    AppendDeclarationValueContinuationLines(const FormatChoiceHistory::Entry* tree, std::vector<int>& continuationLines)
{
    if (tree == nullptr) {
        return;
    }
    if (tree->leaf) {
        const size_t index = static_cast<size_t>(tree->nodeId);
        if (index < continuationLines.size() && tree->declarationValueContinuationLines >= 0) {
            continuationLines[index] = tree->declarationValueContinuationLines;
        }
        return;
    }
    AppendDeclarationValueContinuationLines(tree->left, continuationLines);
    AppendDeclarationValueContinuationLines(tree->right, continuationLines);
}

void AppendAttachedChainOperators(const FormatChoiceHistory::Entry* tree, std::vector<std::uint32_t>& sourceIndices) {
    if (tree == nullptr) {
        return;
    }
    if (tree->leaf) {
        if (tree->attachedChainOperator != std::numeric_limits<std::uint32_t>::max()) {
            sourceIndices.push_back(tree->attachedChainOperator);
        }
        return;
    }
    AppendAttachedChainOperators(tree->left, sourceIndices);
    AppendAttachedChainOperators(tree->right, sourceIndices);
}

}  // namespace

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
FormatChoiceHistory::Handle FormatChoiceHistory::AddContinuationLines(Handle history, int nodeId, int lines) {
    impl_->choiceArena_.push_back(Entry{.nodeId = nodeId, .declarationValueContinuationLines = lines, .leaf = true});
    return Concat(history, &impl_->choiceArena_.back());
}
FormatChoiceHistory::Handle FormatChoiceHistory::AddAttachedOperator(Handle history, std::uint32_t sourceIndex) {
    impl_->choiceArena_.push_back(Entry{.attachedChainOperator = sourceIndex, .leaf = true});
    return Concat(history, &impl_->choiceArena_.back());
}
std::optional<FormatBreakChoice> FormatChoiceHistory::Find(Handle tree, int nodeId) {
    if (tree == nullptr) {
        return std::nullopt;
    }
    if (tree->leaf) {
        return tree->nodeId == nodeId ? std::optional(tree->choice) : std::nullopt;
    }
    if (std::optional<FormatBreakChoice> choice = Find(tree->right, nodeId)) {
        return choice;
    }
    return Find(tree->left, nodeId);
}

FormatBreakSolution FormatChoiceHistory::Materialize(Handle history, size_t choiceCount) {
    FormatBreakSolution solution;
    solution.choices.assign(choiceCount, FormatBreakChoice::Compact);
    solution.indentLevels.assign(choiceCount, -1);
    solution.declarationValueContinuationLines.assign(choiceCount, -1);
    std::vector<bool> assigned(choiceCount, false);
    AppendChoices(history, solution.choices, solution.indentLevels, assigned);
    AppendDeclarationValueContinuationLines(history, solution.declarationValueContinuationLines);
    AppendAttachedChainOperators(history, solution.attachedChainOperators);
    std::sort(solution.attachedChainOperators.begin(), solution.attachedChainOperators.end());
    solution.attachedChainOperators.erase(
        std::unique(solution.attachedChainOperators.begin(), solution.attachedChainOperators.end()),
        solution.attachedChainOperators.end()
    );
    return solution;
}

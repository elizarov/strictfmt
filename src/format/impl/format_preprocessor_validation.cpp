#include "format/impl/format_preprocessor_validation.h"

#include <algorithm>
#include <string_view>

#include "format/impl/format_model.h"

namespace {

bool HasClass(const SyntaxNode& node, SyntaxNodeClass syntaxNodeClass) {
    return (node.classes & static_cast<std::uint64_t>(syntaxNodeClass)) != 0;
}

bool IsTrivia(const SyntaxNode& node) {
    return HasClass(node, SyntaxNodeClass::Trivia);
}

bool IsConditionalPreprocessor(const SyntaxNode& node) {
    switch (node.kind) {
        case SyntaxNodeKind::PreprocIf:
        case SyntaxNodeKind::PreprocIfdef:
        case SyntaxNodeKind::PreprocElse:
        case SyntaxNodeKind::PreprocElif:
            return true;
        default:
            return false;
    }
}

bool IsPreprocessorPlacementCandidate(const SyntaxNode& node) {
    return IsConditionalPreprocessor(node) || node.kind == SyntaxNodeKind::PreprocInclude;
}

bool HasStructuredPreprocessorChildren(const SyntaxNode& node) {
    return std::any_of(node.children.begin(), node.children.end(), [](const SyntaxNode* child) {
        return child != nullptr && !IsTrivia(*child);
    });
}

bool IsDirectiveLine(std::string_view line) {
    while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
        line.remove_prefix(1);
    }
    return line.starts_with("#elif") || line.starts_with("#else") || line.starts_with("#endif");
}

bool IsIncompleteBranchTail(char ch) {
    switch (ch) {
        case '|':
        case '&':
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '^':
        case '=':
        case '<':
        case '!':
        case '?':
            return true;
        default:
            return false;
    }
}

bool BranchBeforeDirectiveHasIncompleteTail(std::string_view text) {
    size_t branchStart = 0;
    size_t lineStart = 0;
    while (lineStart <= text.size()) {
        size_t lineEnd = text.find_first_of("\r\n", lineStart);
        if (lineEnd == std::string_view::npos) {
            lineEnd = text.size();
        }
        std::string_view line = text.substr(lineStart, lineEnd - lineStart);
        if (IsDirectiveLine(line)) {
            std::string_view currentBranch = text.substr(branchStart, lineStart - branchStart);
            while (!currentBranch.empty() && (
                currentBranch.back() == ' ' ||
                currentBranch.back() == '\t' ||
                currentBranch.back() == '\r' ||
                currentBranch.back() == '\n'
            )) {
                currentBranch.remove_suffix(1);
            }
            if (!currentBranch.empty() && IsIncompleteBranchTail(currentBranch.back())) {
                return true;
            }
            branchStart = lineEnd;
        }

        if (lineEnd == text.size()) {
            break;
        }
        lineStart = lineEnd + 1;
        if (lineStart < text.size() && text[lineEnd] == '\r' && text[lineStart] == '\n') {
            ++lineStart;
        }
        if (branchStart == lineEnd) {
            branchStart = lineStart;
        }
    }
    return false;
}

const SyntaxNode* EffectiveParent(const SyntaxNode& node) {
    const SyntaxNode* parent = node.parent;
    while (parent != nullptr && parent->kind == SyntaxNodeKind::IncludeRun) {
        parent = parent->parent;
    }
    return parent;
}

bool HasAncestorWithClass(const SyntaxNode& node, SyntaxNodeClass syntaxNodeClass) {
    for (const SyntaxNode* parent = node.parent; parent != nullptr; parent = parent->parent) {
        if (HasClass(*parent, syntaxNodeClass)) {
            return true;
        }
    }
    return false;
}

bool IsSupportedIncludePlacement(const SyntaxNode& node) {
    const SyntaxNode* parent = EffectiveParent(node);
    return parent != nullptr && HasClass(*parent, SyntaxNodeClass::AllowedPreprocessorContainer);
}

bool IsSupportedConditionalPlacement(const SyntaxNode& node) {
    if (HasClass(node, SyntaxNodeClass::SupportedPreprocessorPlacement)) {
        return !HasAncestorWithClass(node, SyntaxNodeClass::AllowedListPreprocessorContainer) ||
            !BranchBeforeDirectiveHasIncompleteTail(node.text);
    }

    if (HasClass(node, SyntaxNodeClass::ConditionalPreprocessorTree) ||
        HasClass(node, SyntaxNodeClass::DeclarationModifierPreprocessor) ||
        HasClass(node, SyntaxNodeClass::ConditionalRhsPreprocessor)) {
        return true;
    }

    const SyntaxNode* parent = EffectiveParent(node);
    if (parent == nullptr) {
        return false;
    }

    if (HasClass(*parent, SyntaxNodeClass::AllowedListPreprocessorContainer)) {
        return !BranchBeforeDirectiveHasIncompleteTail(node.text);
    }

    if (HasClass(*parent, SyntaxNodeClass::AllowedPreprocessorContainer)) {
        return HasStructuredPreprocessorChildren(node);
    }

    return false;
}

bool IsSupportedPreprocessorPlacement(const SyntaxNode& node) {
    if (node.kind == SyntaxNodeKind::PreprocInclude) {
        return IsSupportedIncludePlacement(node);
    }
    return IsSupportedConditionalPlacement(node);
}

std::pair<int, int> SourcePoint(const FormatModel& model, const SyntaxNode& node) {
    int line = 1;
    int column = 1;
    if (model.sourceText == nullptr || node.text.empty()) {
        return {line, column};
    }

    const std::string_view source(*model.sourceText);
    const char* sourceBegin = source.data();
    const char* sourceEnd = sourceBegin + source.size();
    const char* nodeBegin = node.text.data();
    if (nodeBegin < sourceBegin || nodeBegin > sourceEnd) {
        return {line, column};
    }

    for (const char* current = sourceBegin; current < nodeBegin; ++current) {
        if (*current == '\r') {
            ++line;
            column = 1;
            if (current + 1 < nodeBegin && current[1] == '\n') {
                ++current;
            }
        } else if (*current == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
    }
    return {line, column};
}

std::string UnsupportedPlacementMessage(const FormatModel& model, const SyntaxNode& node) {
    const auto [line, column] = SourcePoint(model, node);
    std::string message = "warning at ";
    message += std::to_string(line);
    message += ":";
    message += std::to_string(column);
    message += ": unsupported placement of ";
    message += node.kind == SyntaxNodeKind::PreprocInclude ? "local include" : "conditional compilation";
    return message;
}

void ValidateNode(const FormatModel& model, const SyntaxNode& node, std::vector<std::string>& warnings) {
    if (IsPreprocessorPlacementCandidate(node) && !IsSupportedPreprocessorPlacement(node)) {
        warnings.push_back(UnsupportedPlacementMessage(model, node));
    }
    for (const SyntaxNode* child : node.children) {
        if (child != nullptr) {
            ValidateNode(model, *child, warnings);
        }
    }
}

}  // namespace

std::vector<std::string> ValidatePreprocessorPlacement(const FormatModel& model) {
    std::vector<std::string> warnings;
    if (model.root != nullptr) {
        ValidateNode(model, *model.root, warnings);
    }
    return warnings;
}

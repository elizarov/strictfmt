#include "format/impl/format_dsl.h"

#include <algorithm>

#include "format/impl/format_break_model_inline_helpers.h"

namespace {

bool HasCallArguments(const FormatBreakNode& node) {
    if (node.kind == FormatBreakNodeKind::Token) {
        return FormatBreakTokenSyntaxKind(node.token) == SyntaxNodeKind::ArgumentList;
    }
    if (node.kind == FormatBreakNodeKind::Delimited) {
        const auto* open = node.children.empty() ? nullptr : FormatBreakNodeToken(node.children.front());
        return node.delimiterKind == FormatBreakDelimiterKind::Paren &&
            open != nullptr &&
            FormatBreakTokenValue(*open).parentKind == SyntaxNodeKind::ArgumentList;
    }
    return node.kind == FormatBreakNodeKind::Sequence &&
        std::any_of(node.children.begin(), node.children.end(), [](const auto* child) {
            return HasCallArguments(*child);
        });
}

bool MatchesCall(const FormatBreakNode& operand, std::string_view name) {
    return HasCallArguments(operand) && FormatBreakLeadingNameMatches(operand, name);
}

bool MatchesCalls(const FormatBreakNode& operand, const std::vector<std::string>& names) {
    return std::any_of(names.begin(), names.end(), [&](const auto& name) { return MatchesCall(operand, name); });
}

bool ConfigureChain(FormatBreakModel& model, FormatBreakNode& chain, const BuilderChainProfile& profile, size_t entry) {
    struct Scope {
        const BuilderScope* role;
        size_t operand;
    };
    std::vector<Scope> stack;
    std::vector<FormatBuilderStep> steps(chain.operators.size());
    bool nonemptyScope = false;
    const bool entryHasRole = entry > 0 && (
        MatchesCalls(*chain.operands[entry], profile.bindToNext) ||
        std::any_of(profile.scopes.begin(), profile.scopes.end(), [&](const auto& scope) {
            return MatchesCall(*chain.operands[entry], scope.open);
        })
    );
    const size_t firstStep = entry + (entryHasRole ? 0 : 1);
    for (size_t index = 1; index < chain.operands.size(); ++index) {
        auto& step = steps[index - 1];
        if (index < firstStep) {
            step = {.indent = 0, .breakBefore = false};
            continue;
        }
        const auto& operand = *chain.operands[index];
        const auto open = std::find_if(profile.scopes.begin(), profile.scopes.end(), [&](const auto& scope) {
            return MatchesCall(operand, scope.open);
        });
        const bool close = std::any_of(profile.scopes.begin(), profile.scopes.end(), [&](const auto& scope) {
            return MatchesCall(operand, scope.close);
        });
        bool emptyScope = false;
        if (close) {
            if (stack.empty() || !MatchesCall(operand, stack.back().role->close)) {
                return false;
            }
            emptyScope = stack.back().operand + 1 == index;
            nonemptyScope |= !emptyScope;
            stack.pop_back();
        }
        step.indent = static_cast<int>(stack.size()) + 1;
        step.breakBefore = !emptyScope &&
            (close || index == firstStep || !MatchesCalls(*chain.operands[index - 1], profile.bindToNext));
        if (index - 1 < chain.commentsBeforeOperators.size() && !chain.commentsBeforeOperators[index - 1].empty()) {
            step.breakBefore = true;
        }
        if (open != profile.scopes.end()) {
            stack.push_back({&*open, index});
        }
    }
    if (!stack.empty()) {
        return false;
    }
    chain.builderSteps = model.builderSteps.Append(steps);
    chain.forceSplit |= nonemptyScope;
    model.hasBuilderChains = true;
    return true;
}

void ConfigureNode(FormatBreakModel& model, FormatBreakNode& node, const FormatterConfig& config) {
    if (node.kind == FormatBreakNodeKind::Chain && node.chainKind == FormatBreakChainKind::MemberBeforeOperator) {
        for (size_t entry = 0; entry < node.operands.size(); ++entry) {
            const auto profile =
                std::find_if(config.builderChains.begin(), config.builderChains.end(), [&](const auto& candidate) {
                    return MatchesCalls(*node.operands[entry], candidate.entryCalls);
                });
            if (profile != config.builderChains.end()) {
                ConfigureChain(model, node, *profile, entry);
                break;
            }
        }
    }
    for (auto* child : node.children) {
        ConfigureNode(model, *child, config);
    }
    for (auto* operand : node.operands) {
        ConfigureNode(model, *operand, config);
    }
    for (auto& item : node.items) {
        ConfigureNode(model, *item.node, config);
    }
}

}  // namespace

void ConfigureBuilderChains(FormatBreakModel& model, const FormatterConfig& config) {
    if (!config.builderChains.empty() && model.root != nullptr) {
        ConfigureNode(model, *model.root, config);
    }
}

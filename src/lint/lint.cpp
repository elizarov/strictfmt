#include "lint/lint.h"

#include <algorithm>
#include <map>
#include <set>

#include "lint/declarations.h"
#include "syntax/tree_sitter.h"
#include "util/strings.h"
#include "tools/tools_common.h"

namespace {

// Suppressions are collected from comment nodes, never from literal contents.
class Suppressions {
public:
    void Collect(TSNode node, std::string_view source) {
        if (NodeType(node) == "comment") {
            auto text = NodeText(node, source);
            uint32_t line = ts_node_start_point(node).row + 1;
            for (const auto& part : SplitLines(text)) {
                for (size_t pos = part.find("NOLINT"); pos != std::string::npos; pos = part.find("NOLINT", pos + 6)) {
                    const auto isWord = [](char c) {
                        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
                    };
                    if (pos != 0 && isWord(part[pos - 1])) {
                        continue;
                    }
                    std::string_view token(part.data() + pos, part.size() - pos);
                    std::string_view command = "NOLINT";
                    for (auto candidate : {"NOLINTNEXTLINE", "NOLINTBEGIN", "NOLINTEND"}) {
                        if (token.starts_with(candidate)) {
                            command = candidate;
                            break;
                        }
                    }
                    token.remove_prefix(command.size());
                    if (!token.empty() && isWord(token.front())) {
                        continue;
                    }
                    bool applies = true;
                    if (token.starts_with("(")) {
                        const auto close = token.find(')');
                        if (close == std::string_view::npos) {
                            continue;
                        }
                        const auto checks = token.substr(1, close - 1);
                        applies = false;
                        size_t start = 0;
                        while (start <= checks.size()) {
                            const auto end = checks.find(',', start);
                            const auto check = Trim(
                                checks
                                    .substr(start, end == std::string_view::npos ? checks.size() - start : end - start)
                            );
                            if (check == "*" || check == "readability-identifier-naming") {
                                applies = true;
                                break;
                            }
                            if (end == std::string_view::npos) {
                                break;
                            }
                            start = end + 1;
                        }
                    }
                    if (!applies) {
                        continue;
                    }
                    if (command == "NOLINTBEGIN") {
                        events_[line] += 1;
                    } else if (command == "NOLINTEND") {
                        events_[line] -= 1;
                    } else {
                        lines_.insert(line + (command == "NOLINTNEXTLINE" ? 1 : 0));
                    }
                }
                ++line;
            }
            return;
        }
        VisitChildren(node, [&](TSNode child, std::string_view) { Collect(child, source); });
    }
    bool Contains(uint32_t line) const {
        if (lines_.contains(line)) {
            return true;
        }
        int depth = 0;
        for (const auto& [at, delta] : events_) {
            if (at > line) {
                break;
            }
            depth = std::max(0, depth + delta);
        }
        return depth > 0;
    }

private:
    std::set<uint32_t> lines_;
    std::map<uint32_t, int> events_;
};

}

std::vector<LintDiagnostic> LintSyntaxTree(TSNode root, std::string_view source, const LintConfig& config) {
    std::vector<LintDiagnostic> diagnostics;
    if (!config.Active()) {
        return diagnostics;
    }
    Suppressions suppressions;
    suppressions.Collect(root, source);
    VisitDeclarations(root, source, [&](const Declaration& declaration) {
        const auto point = ts_node_start_point(declaration.name);
        if (suppressions.Contains(point.row + 1)) {
            return;
        }
        const auto name = NodeText(declaration.name, source);
        const NamingRule* failureRule = nullptr;
        DeclarationFacts failureDeclaration = declaration.facts;
        const unsigned accessMask = declaration.possibleAccess == 0 ?
            1u << static_cast<unsigned>(declaration.facts.access) : declaration.possibleAccess;
        for (const auto& facts : {declaration.facts, declaration.alternative.value_or(declaration.facts)}) {
            for (unsigned access = 0; access < 3; ++access) {
                if ((accessMask & (1u << access)) == 0) {
                    continue;
                }
                DeclarationFacts alternative = facts;
                alternative.access = static_cast<DeclarationAccess>(access);
                const auto rule = std::find_if(config.naming.begin(), config.naming.end(), [&](const auto& candidate) {
                    return MatchesDeclaration(candidate.declarations, alternative);
                });
                if (rule == config.naming.end() || rule->names.Accepts(name)) {
                    return;
                }
                if (failureRule == nullptr) {
                    failureRule = &*rule;
                    failureDeclaration = alternative;
                }
            }
        }
        if (failureRule != nullptr) {
            diagnostics.push_back({
                ts_node_start_byte(declaration.name),
                point.row + 1,
                point.column + 1,
                "error: " + std::string(DeclarationKindName(failureDeclaration.kind)) +
                    " '" + std::string(name) +
                    "' must use " + failureRule->names.Description() +
                    " [identifier-naming:" + failureRule->name +
                    "]"
            });
        }
    });
    std::stable_sort(diagnostics.begin(), diagnostics.end(), [](const auto& a, const auto& b) {
        return a.offset < b.offset;
    });
    diagnostics.erase(
        std::unique(diagnostics.begin(), diagnostics.end(), [](const auto& a, const auto& b) {
            return a.offset == b.offset && a.message == b.message;
        }),
        diagnostics.end()
    );
    return diagnostics;
}

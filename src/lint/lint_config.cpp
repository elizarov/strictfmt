#include "lint/lint_config.h"

#include <array>
#include <stdexcept>

#include "util/strings.h"

namespace {

constexpr std::array<std::string_view, static_cast<size_t>(DeclarationKind::Count)> kKinds = {
    "Class",
    "Struct",
    "Union",
    "Enum",
    "EnumConstant",
    "Namespace",
    "Concept",
    "Typedef",
    "TypeAlias",
    "TypeTemplateParameter",
    "ValueTemplateParameter",
    "TemplateTemplateParameter",
    "Function",
    "Method",
    "Variable",
    "Binding",
    "Field",
    "Parameter",
    "ParameterPack",
    "Macro"
};

std::regex CompilePattern(std::string_view value) {
    try {
        return std::regex(std::string(value), std::regex::extended | std::regex::optimize);
    } catch (const std::regex_error&) {
        throw std::runtime_error("invalid lint regular expression: " + std::string(value));
    }
}

}
std::string_view DeclarationKindName(DeclarationKind kind) { return kKinds[static_cast<size_t>(kind)]; }
bool ParseLintBoolean(std::string_view value) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::runtime_error("lint boolean must be true or false");
}
void SetNamingRuleOption(NamingRule& rule, std::string_view key, std::string_view value) {
    if (key == "Kinds") {
        if (value.empty() || value.back() == ',') {
            throw std::runtime_error("Kinds requires a nonempty declaration list");
        }
        rule.declarations.kinds = 0;
        while (!value.empty()) {
            const auto comma = value.find(',');
            const auto name = Trim(value.substr(0, comma));
            bool found = false;
            for (size_t i = 0; i < kKinds.size(); ++i) {
                if (name == kKinds[i]) {
                    rule.declarations.kinds |= uint64_t{1} << i;
                    found = true;
                    break;
                }
            }
            if (!found) {
                throw std::runtime_error("unknown lint declaration kind: " + name);
            }
            if (comma == std::string_view::npos) {
                break;
            }
            value.remove_prefix(comma + 1);
        }
    } else if (key == "Scope") {
        if (value.empty() || value.back() == ',') {
            throw std::runtime_error("Scope requires a nonempty scope list");
        }
        rule.declarations.scopes = 0;
        while (!value.empty()) {
            const auto comma = value.find(',');
            const auto scope = Trim(value.substr(0, comma));
            if (scope == "Global") {
                rule.declarations.scopes |= 1u << static_cast<unsigned>(DeclarationScope::Global);
            } else if (scope == "Member") {
                rule.declarations.scopes |= 1u << static_cast<unsigned>(DeclarationScope::Member);
            } else if (scope == "Local") {
                rule.declarations.scopes |= 1u << static_cast<unsigned>(DeclarationScope::Local);
            } else {
                throw std::runtime_error("unknown lint scope: " + scope);
            }
            if (comma == std::string_view::npos) {
                break;
            }
            value.remove_prefix(comma + 1);
        }
    } else if (key == "Access") {
        if (value == "Public") {
            rule.declarations.access = DeclarationAccess::Public;
        } else if (value == "Protected") {
            rule.declarations.access = DeclarationAccess::Protected;
        } else if (value == "Private") {
            rule.declarations.access = DeclarationAccess::Private;
        } else {
            throw std::runtime_error("unknown lint access: " + std::string(value));
        }
    } else if (key == "Const") {
        rule.declarations.isConst = ParseLintBoolean(value);
    } else if (key == "Constexpr") {
        rule.declarations.isConstexpr = ParseLintBoolean(value);
    } else if (key == "Static") {
        rule.declarations.isStatic = ParseLintBoolean(value);
    } else if (key == "Case") {
        std::string_view pattern;
        if (value == "lower_case") {
            pattern = "[a-z][a-z0-9_]*";
        } else if (value == "UPPER_CASE") {
            pattern = "[A-Z][A-Z0-9_]*";
        } else if (value == "CamelCase") {
            pattern = "[A-Z][a-zA-Z0-9]*";
        } else if (value == "camelBack") {
            pattern = "[a-z][a-zA-Z0-9]*";
        } else if (value != "aNy_CasE") {
            throw std::runtime_error("unknown lint case: " + std::string(value));
        }
        rule.names.caseName = value;
        rule.names.casePattern = pattern.empty() ? std::nullopt : std::optional{CompilePattern(pattern)};
    } else if (key == "Prefix") {
        rule.names.prefix = value;
    } else if (key == "Suffix") {
        rule.names.suffix = value;
    } else if (key == "IgnoredRegexp") {
        rule.names.ignoredRegexp = value;
        rule.names.ignoredPattern = value.empty() ? std::nullopt : std::optional{CompilePattern(value)};
    } else {
        throw std::runtime_error("unknown lint naming option: " + std::string(key));
    }
}
bool NamePolicy::Accepts(std::string_view name) const {
    if (ignoredPattern && std::regex_match(name.begin(), name.end(), *ignoredPattern)) {
        return true;
    }
    if (!name.starts_with(prefix) || !name.ends_with(suffix) || name.size() < prefix.size() + suffix.size()) {
        return false;
    }
    const auto middle = name.substr(prefix.size(), name.size() - prefix.size() - suffix.size());
    return !middle.empty() && (!casePattern || (
        !middle.starts_with("_") &&
        !middle.ends_with("_") &&
        std::regex_match(middle.begin(), middle.end(), *casePattern)
    ));
}

std::string NamePolicy::Description() const {
    std::string result = caseName.empty() ? "a name" : caseName;
    if (!prefix.empty()) {
        result += " with prefix '" + prefix + "'";
    }
    if (!suffix.empty()) {
        result += " with suffix '" + suffix + "'";
    }
    return result;
}

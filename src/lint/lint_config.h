#pragma once

#include <cstdint>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

// Classification and policy are independent: selectors only inspect declaration facts.
enum class DeclarationKind {
    Class,
    Struct,
    Union,
    Enum,
    EnumConstant,
    Namespace,
    Concept,
    Typedef,
    TypeAlias,
    TypeTemplateParameter,
    ValueTemplateParameter,
    TemplateTemplateParameter,
    Function,
    Method,
    Variable,
    Binding,
    Field,
    Parameter,
    ParameterPack,
    Macro,
    Count
};

enum class DeclarationScope {
    Global,
    Member,
    Local
};

enum class DeclarationAccess {
    Public,
    Protected,
    Private
};

std::string_view DeclarationKindName(DeclarationKind kind);

struct DeclarationSelector {
    uint64_t kinds = 0;
    unsigned scopes = 0;
    std::optional<DeclarationAccess> access;
    std::optional<bool> isConst;
    std::optional<bool> isConstexpr;
    std::optional<bool> isStatic;
};

struct NamePolicy {
    std::string caseName;
    std::string prefix;
    std::string suffix;
    std::optional<std::regex> casePattern;
    std::string ignoredRegexp;
    std::optional<std::regex> ignoredPattern;

    bool Accepts(std::string_view name) const;
    std::string Description() const;
};

struct NamingRule {
    std::string name;
    DeclarationSelector declarations;
    NamePolicy names;
};

struct LintConfig {
    bool enabled = true;
    std::vector<NamingRule> naming;

    bool Active() const { return enabled && !naming.empty(); }
};

void SetNamingRuleOption(NamingRule& rule, std::string_view key, std::string_view value);
bool ParseLintBoolean(std::string_view value);

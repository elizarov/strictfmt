#pragma once
#include <functional>
#include <string_view>
#include <tree_sitter/api.h>
#include "lint/lint_config.h"

struct DeclarationFacts {
    DeclarationKind kind = DeclarationKind::Variable;
    DeclarationScope scope = DeclarationScope::Global;
    DeclarationAccess access = DeclarationAccess::Public;
    bool isConst = false;
    bool isConstexpr = false;
    bool isStatic = false;
};

struct Declaration {
    TSNode name = {};
    DeclarationFacts facts;
    std::optional<DeclarationFacts> alternative;
    unsigned possibleAccess = 0;
};

// Visits spelled declarations only; macro replacement lists and generated names are opaque.
void VisitDeclarations(TSNode root, std::string_view source, const std::function<void(const Declaration&)>& visit);
bool MatchesDeclaration(const DeclarationSelector& selector, const DeclarationFacts& declaration);

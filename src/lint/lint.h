#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <tree_sitter/api.h>
#include "lint/lint_config.h"

struct LintDiagnostic {
    uint32_t offset = 0;
    uint32_t line = 0;
    uint32_t column = 0;
    std::string message;
};

std::vector<LintDiagnostic> LintSyntaxTree(TSNode root, std::string_view source, const LintConfig& config);

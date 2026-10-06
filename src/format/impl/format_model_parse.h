#pragma once

#include <string_view>

#include "format/impl/format_model.h"
#include "lint/lint.h"

struct FormatterConfig;

FormatModel ParseFormatModel(
    std::string_view text,
    const FormatterConfig& config,
    std::vector<LintDiagnostic>* lint = nullptr,
    bool lintOnly = false
);

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "format/impl/format_config.h"
#include "lint/lint.h"

struct SourceFormatResult {
    bool ok = true;
    bool changed = false;
    std::string formatted;
    std::string error;
    std::vector<std::string> warnings;
    std::vector<LintDiagnostic> lint;
};

// Formats parsed source once. Optional validation reparses the output and checks
// idempotence. Parse/validation failures set error; naming failures populate lint.
// Both set ok=false. lintOnly checks without producing formatted text.
SourceFormatResult FormatSourceText(
    std::string_view text,
    const FormatterConfig& config,
    std::string_view sourcePath,
    bool validate = false,
    bool lint = true,
    bool lintOnly = false
);

int RunFormat(int argc, char** argv);

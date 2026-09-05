#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "format/impl/format_config.h"

struct SourceFormatResult {
    bool ok = true;
    bool changed = false;
    std::string formatted;
    std::string error;
    std::vector<std::string> warnings;
};

// Formats parsed source once. Optional validation reparses the output and checks
// idempotence; any parse or validation failure is returned as an error, never as unchanged input.
SourceFormatResult FormatSourceText(
    std::string_view text, const FormatterConfig& config, std::string_view sourcePath, bool validate = false
);

int RunFormat(int argc, char** argv);

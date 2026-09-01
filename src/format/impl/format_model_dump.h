#pragma once

#include <cstdio>
#include <optional>
#include <string>
#include <string_view>

struct FormatterConfig;

int DumpFormatModelText(
    std::string_view sourceText,
    const FormatterConfig& config,
    FILE* output,
    FILE* errorOutput,
    std::string_view commandName
);
int DumpFormatModel(
    std::string_view sourcePath,
    const std::optional<std::string>& explicitStylePath,
    FILE* output,
    FILE* errorOutput,
    std::string_view commandName
);
int DumpFormatBreakTreeText(
    std::string_view sourceText,
    const FormatterConfig& config,
    std::string_view sourcePath,
    FILE* output,
    FILE* errorOutput,
    std::string_view commandName
);
int DumpFormatBreakTree(
    std::string_view sourcePath,
    const std::optional<std::string>& explicitStylePath,
    FILE* output,
    FILE* errorOutput,
    std::string_view commandName
);
int RunFormatModelDump(int argc, char** argv);

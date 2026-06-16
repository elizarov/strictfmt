#pragma once

#include <cstdio>
#include <optional>
#include <string>
#include <string_view>

int DumpFormatModel(
    std::string_view sourcePath,
    const std::optional<std::string>& explicitStylePath,
    FILE* output,
    FILE* errorOutput,
    std::string_view commandName
);
int RunFormatModelDump(int argc, char** argv);

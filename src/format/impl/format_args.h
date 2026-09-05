#pragma once

#include <cstddef>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

enum class FormatMode {
    Stdout,
    InPlace,
    DryRun,
    Diff,
};

enum class FormatDumpKind {
    None,
    SyntaxTree,
    BreakTree,
};

struct FormatOptions {
    FormatMode mode = FormatMode::Stdout;
    bool verbose = false;
    bool validate = false;
    bool help = false;
    bool version = false;
    bool readStdin = false;
    bool fileListProvided = false;
    bool recursiveInputProvided = false;
    bool concurrencyProvided = false;
    FormatDumpKind dumpKind = FormatDumpKind::None;
    size_t concurrency = 0;
    std::optional<std::string> dumpFile;
    std::optional<std::string> explicitStylePath;
    std::vector<std::string> files;
    std::vector<std::string> recursiveRoots;
};

std::optional<FormatOptions> ParseFormatArgs(int argc, char** argv, std::string& error);
void PrintFormatUsage(FILE* output);

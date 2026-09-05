#include "format/impl/format_args.h"

#include <cstdio>

#include "tools/tools_common.h"
#include "tools/tools_parallel.h"
#include "util/file_path.h"
#include "util/strings.h"

namespace {

std::optional<std::string> ParseStyleValue(std::string_view value, std::string& error) {
    if (value == "file") {
        error = "--style file is not supported; pass --style <config-file> or omit --style for upward discovery";
        return std::string{};
    }
    constexpr std::string_view filePrefix = "file:";
    if (StartsWith(value, filePrefix)) {
        error = "--style file:<path> is not supported; pass --style <config-file>";
        return std::string{};
    }
    if (value.empty()) {
        error = "--style requires a value";
        return std::string{};
    }
    return AbsolutePath(value);
}

bool AppendFilesFromList(std::string_view path, FormatOptions& options, std::string& error) {
    std::optional<std::vector<std::string>> files = ReadToolFileList(path, error);
    if (!files.has_value()) {
        return false;
    }
    options.fileListProvided = true;
    options.files.insert(options.files.end(), files->begin(), files->end());
    return true;
}

bool AppendRecursiveRoot(std::string_view path, FormatOptions& options, std::string& error) {
    if (path.empty()) {
        error = "-r requires a path";
        return false;
    }
    options.recursiveInputProvided = true;
    options.recursiveRoots.emplace_back(path);
    return true;
}

std::string_view FormatModeOption(FormatMode mode) {
    switch (mode) {
        case FormatMode::Stdout:
            return "default output";
        case FormatMode::InPlace:
            return "-i";
        case FormatMode::DryRun:
            return "--dry-run";
        case FormatMode::Diff:
            return "--diff";
    }
    return "format mode";
}

bool SelectFormatMode(FormatOptions& options, FormatMode mode, std::string_view option, std::string& error) {
    if (options.mode != FormatMode::Stdout && options.mode != mode) {
        error = std::string(option) + " is incompatible with " + std::string(FormatModeOption(options.mode));
        return false;
    }
    options.mode = mode;
    return true;
}

}  // namespace

std::optional<FormatOptions> ParseFormatArgs(int argc, char** argv, std::string& error) {
    FormatOptions options;
    for (int index = 0; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "-h" || arg == "--help") {
            options.help = true;
        } else if (arg == "--version") {
            options.version = true;
        } else if (arg == "-i") {
            if (!SelectFormatMode(options, FormatMode::InPlace, arg, error)) {
                return std::nullopt;
            }
        } else if (arg == "-n" || arg == "--dry-run") {
            if (!SelectFormatMode(options, FormatMode::DryRun, "--dry-run", error)) {
                return std::nullopt;
            }
        } else if (arg == "--diff") {
            if (!SelectFormatMode(options, FormatMode::Diff, arg, error)) {
                return std::nullopt;
            }
        } else if (arg == "-v" || arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "--stdin") {
            options.readStdin = true;
        } else if (arg == "--validate") {
            options.validate = true;
        } else if (arg == "--dump-syntax-tree" || arg == "--dump-break-tree") {
            if (options.dumpKind != FormatDumpKind::None) {
                error = "only one dump mode can be specified";
                return std::nullopt;
            }
            options.dumpKind = arg == "--dump-syntax-tree" ? FormatDumpKind::SyntaxTree : FormatDumpKind::BreakTree;
            if (index + 1 < argc && argv[index + 1][0] != '-') {
                options.dumpFile = argv[++index];
            }
        } else if (arg == "-r" || arg == "--recursive") {
            if (index + 1 >= argc) {
                error = "-r requires a path";
                return std::nullopt;
            }
            if (!AppendRecursiveRoot(argv[++index], options, error)) {
                return std::nullopt;
            }
        } else if (arg == "--concurrency") {
            if (index + 1 >= argc) {
                error = "--concurrency requires a value";
                return std::nullopt;
            }
            options.concurrencyProvided = true;
            if (!ParseToolConcurrency(argv[++index], options.concurrency, error)) {
                return std::nullopt;
            }
        } else if (arg == "--files") {
            if (index + 1 >= argc) {
                error = "--files requires a path";
                return std::nullopt;
            }
            if (!AppendFilesFromList(argv[++index], options, error)) {
                return std::nullopt;
            }
        } else if (arg == "--style") {
            if (index + 1 >= argc) {
                error = "--style requires a value";
                return std::nullopt;
            }
            std::optional<std::string> parsed = ParseStyleValue(argv[++index], error);
            if (!error.empty()) {
                return std::nullopt;
            }
            options.explicitStylePath = std::move(parsed);
        } else if (!arg.empty() && arg[0] == '-') {
            error = "unknown argument " + arg;
            return std::nullopt;
        } else {
            options.files.push_back(arg);
        }
    }
    if (options.dumpKind != FormatDumpKind::None) {
        const std::string_view optionName =
            options.dumpKind == FormatDumpKind::SyntaxTree ? "--dump-syntax-tree" : "--dump-break-tree";
        if (options.mode != FormatMode::Stdout) {
            error = std::string(optionName) + " is incompatible with -i, --dry-run, and --diff";
            return std::nullopt;
        }
        if (options.fileListProvided || options.recursiveInputProvided || !options.files.empty()) {
            error = std::string(optionName) + " cannot be combined with format inputs";
            return std::nullopt;
        }
        if (options.dumpFile.has_value() && options.readStdin) {
            error = std::string(optionName) + " <file> cannot be combined with --stdin";
            return std::nullopt;
        }
        if (!options.dumpFile.has_value() && !options.readStdin) {
            error = std::string(optionName) + " requires a file unless combined with --stdin";
            return std::nullopt;
        }
        if (options.concurrencyProvided) {
            error = std::string(optionName) + " is incompatible with --concurrency";
            return std::nullopt;
        }
        if (options.validate) {
            error = std::string(optionName) + " is incompatible with --validate";
            return std::nullopt;
        }
    }
    if (options.readStdin && (options.fileListProvided || options.recursiveInputProvided || !options.files.empty())) {
        error = "--stdin cannot be combined with file inputs";
        return std::nullopt;
    }
    if (options.readStdin && options.mode == FormatMode::InPlace) {
        error = "--stdin is incompatible with -i";
        return std::nullopt;
    }
    if (
        options.mode == FormatMode::InPlace &&
        options.files.empty() &&
        !options.fileListProvided &&
        !options.recursiveInputProvided
    ) {
        error = "-i requires at least one file";
        return std::nullopt;
    }
    return options;
}

void PrintFormatUsage(FILE* out) {
    std::fprintf(out, "Usage:\n");
    std::fprintf(out, "  strictfmt [options] [ <file>... | -r <path> | --stdin | --files <path> ]\n");
    std::fprintf(out, "\n");
    std::fprintf(out, "Inputs:\n");
    std::fprintf(out, "  <file>...               Format the listed source files and write formatted text to stdout.\n");
    std::fprintf(out, "  -r, --recursive <path>  Recursively format supported C/C++ files under a directory.\n");
    std::fprintf(out, "  --stdin                 Read one source file from stdin.\n");
    std::fprintf(out, "  --files <path>          Read input file paths from a newline-delimited file list.\n");
    std::fprintf(out, "\n");
    std::fprintf(out, "Modes:\n");
    std::fprintf(out, "  -i                      Rewrite files in place.\n");
    std::fprintf(out, "  -n, --dry-run           Check formatting and return 1 when formatting changes are needed.\n");
    std::fprintf(out, "  --diff                  Print a unified diff and use the dry-run exit code.\n");
    std::fprintf(out, "  --dump-syntax-tree      Print the parsed syntax tree for debugging to stdout.\n");
    std::fprintf(out, "  --dump-break-tree       Print break-decision trees for debugging to stdout.\n");
    std::fprintf(out, "                          For either dump mode, pass one file or combine it with --stdin.\n");
    std::fprintf(out, "                          With no mode option, write formatted text to stdout.\n");
    std::fprintf(out, "\n");
    std::fprintf(out, "Configuration:\n");
    std::fprintf(out, "  --style <config-file>   Use this .cpp-format file for every input.\n");
    std::fprintf(out, "                          By default, searches upward from each input for .cpp-format.\n");
    std::fprintf(out, "\n");
    std::fprintf(out, "Other options:\n");
    std::fprintf(out, "  --validate              Reparse formatted output and check idempotence; fail on errors.\n");
    std::fprintf(out, "  --concurrency <n>       Limit worker threads. Defaults to hardware concurrency.\n");
    std::fprintf(out, "  -v, --verbose           Verbose progress output.\n");
    std::fprintf(out, "  --version               Print the strictfmt version.\n");
    std::fprintf(out, "  -h, --help              Print this help text.\n");
}

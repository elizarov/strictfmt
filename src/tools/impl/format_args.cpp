#include "tools/impl/format_args.h"

#include <cstdio>

#include "tools/impl/tools_common.h"
#include "tools/impl/tools_parallel.h"
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

}  // namespace

std::optional<FormatOptions> ParseFormatArgs(int argc, char** argv, std::string& error) {
    FormatOptions options;
    for (int index = 0; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "-h" || arg == "--help") {
            options.help = true;
        } else if (arg == "-i") {
            if (options.mode == FormatMode::DryRun) {
                error = "-i is incompatible with --dry-run";
                return std::nullopt;
            }
            options.mode = FormatMode::InPlace;
        } else if (arg == "-n" || arg == "--dry-run") {
            if (options.mode == FormatMode::InPlace) {
                error = "--dry-run is incompatible with -i";
                return std::nullopt;
            }
            options.mode = FormatMode::DryRun;
        } else if (arg == "-v" || arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "--stdin") {
            options.readStdin = true;
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

void PrintFormatUsage(FILE* output) {
    std::fprintf(output, "Usage:\n");
    std::fprintf(output, "  strictfmt [options] [file...]\n");
    std::fprintf(output, "  strictfmt --stdin [options]\n");
    std::fprintf(output, "\n");
    std::fprintf(output, "Inputs:\n");
    std::fprintf(
        output,
        "  file...                 Format the listed source files and write formatted text to stdout.\n"
    );
    std::fprintf(
        output,
        "  --stdin                 Read one source file from stdin and write formatted text to stdout.\n"
    );
    std::fprintf(output, "  -r, --recursive <path>  Recursively format supported C/C++ files under a directory.\n");
    std::fprintf(output, "  --files <path>          Read input file paths from a newline-delimited file list.\n");
    std::fprintf(output, "\n");
    std::fprintf(output, "Modes:\n");
    std::fprintf(
        output,
        "  -i                      Rewrite files in place. Requires file, --files, or --recursive input.\n"
    );
    std::fprintf(
        output,
        "  -n, --dry-run           Check formatting and return 1 when formatting changes are needed.\n"
    );
    std::fprintf(
        output,
        "                          Without -i or --dry-run, file inputs and --stdin write formatted text to stdout.\n"
    );
    std::fprintf(output, "\n");
    std::fprintf(output, "Configuration:\n");
    std::fprintf(output, "  --style <config-file>   Use this .cpp-format file for every input.\n");
    std::fprintf(
        output,
        "                          When omitted, strictfmt searches upward from each input for .cpp-format.\n"
    );
    std::fprintf(output, "\n");
    std::fprintf(output, "Other options:\n");
    std::fprintf(
        output,
        "  --concurrency <n>       Limit worker threads for file formatting. Defaults to hardware concurrency.\n"
    );
    std::fprintf(
        output,
        "  -v, --verbose           Reserved for verbose progress output. Final summaries are always printed.\n"
    );
    std::fprintf(output, "  -h, --help              Print this help text.\n");
}

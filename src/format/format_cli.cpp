#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "format/format.h"
#include "format/impl/format_args.h"
#include "format/impl/format_config.h"
#include "format/impl/format_diff.h"
#include "format/impl/format_model_dump.h"
#include "strictfmt_version.h"
#include "tools/tools_common.h"
#include "tools/tools_parallel.h"
#include "tools/tools_progress.h"
#include "util/file_path.h"
#include "util/strings.h"

namespace {

struct PendingFileFormat {
    std::string file;
    std::string diff;
    SourceFormatResult result;
};

struct ResolvedFileFormat {
    std::string file;
    std::string diffPath;
    const FormatterConfig* config = nullptr;
};

struct CompletedFileFormat {
    PendingFileFormat pending;
    std::string sortKey;
    int lineCount = 0;
    int changedLineCount = 0;
    bool hasPending = false;
    bool readFailed = false;
};

void SetBinaryMode(FILE* file) {
#ifdef _WIN32
    _setmode(_fileno(file), _O_BINARY);
#else
    (void)file;
#endif
}

std::string ReadStdinText() {
    std::string text;
    char buffer[4096];
    while (true) {
        const size_t bytesRead = std::fread(buffer, 1, sizeof(buffer), stdin);
        if (bytesRead > 0) {
            text.append(buffer, bytesRead);
        }
        if (bytesRead < sizeof(buffer)) {
            break;
        }
    }
    return text;
}

bool IsCheckMode(const FormatOptions& options) {
    return options.mode == FormatMode::DryRun || options.mode == FormatMode::Diff;
}

FILE* SummaryStream(const FormatOptions& options) {
    return options.mode == FormatMode::Stdout || options.mode == FormatMode::Diff ? stderr : stdout;
}

void PrintSourceError(FILE* output, std::string_view file, std::string_view error) {
    const std::vector<std::string> lines = SplitLines(error);
    if (lines.empty()) {
        std::fprintf(output, "%.*s: formatter error\n", static_cast<int>(file.size()), file.data());
        return;
    }
    for (const std::string& line : lines) {
        std::fprintf(output, "%.*s: %s\n", static_cast<int>(file.size()), file.data(), line.c_str());
    }
}

void PrintSourceWarnings(FILE* output, std::string_view file, const std::vector<std::string>& warnings) {
    for (const std::string& warning : warnings) {
        std::fprintf(output, "%.*s: %s\n", static_cast<int>(file.size()), file.data(), warning.c_str());
    }
}

void PrintVerboseFileProgress(
    FILE* output,
    std::mutex& outputMutex,
    size_t fileIndex,
    size_t totalFiles,
    bool scanning,
    std::string_view action,
    std::string_view file,
    std::optional<std::chrono::steady_clock::duration> elapsed = std::nullopt
) {
    std::lock_guard<std::mutex> lock(outputMutex);
    std::fprintf(
        output,
        "[%s/%s%s] %.*s %.*s",
        FormatCount(static_cast<int>(fileIndex + 1)).c_str(),
        FormatCount(static_cast<int>(totalFiles)).c_str(),
        scanning ? "+" : "",
        static_cast<int>(action.size()),
        action.data(),
        static_cast<int>(file.size()),
        file.data()
    );
    if (elapsed.has_value()) {
        std::fprintf(output, " in %s", FormatToolElapsed(*elapsed).c_str());
    }
    std::fprintf(output, "\n");
    std::fflush(output);
}

std::string FileCountText(int count, size_t totalCount, bool showTotal) {
    std::string text = FormatCount(count);
    if (showTotal || count != static_cast<int>(totalCount)) {
        text += "/" + FormatCount(static_cast<int>(totalCount));
    }
    text += totalCount == 1 ? " file" : " files";
    return text;
}

int CountSourceLines(std::string_view text) {
    int lines = 0;
    size_t index = 0;
    while (index < text.size()) {
        if (text[index] == '\r' || text[index] == '\n') {
            ++lines;
            if (text[index] == '\r' && index + 1 < text.size() && text[index + 1] == '\n') {
                ++index;
            }
        }
        ++index;
    }
    if (!text.empty() && text.back() != '\r' && text.back() != '\n') {
        ++lines;
    }
    return lines;
}

bool IsFormatRecursiveInput(std::string_view path) {
    const std::string suffix = ToLower(Extension(path));
    static const std::vector<std::string> supportedSuffixes =
        {".c", ".cc", ".cpp", ".cxx", ".c++", ".h", ".hh", ".hpp", ".hxx", ".h++", ".ipp", ".inl", ".tpp"};
    return std::find(supportedSuffixes.begin(), supportedSuffixes.end(), suffix) != supportedSuffixes.end();
}

class FormatRecursiveFileFilter final : public ToolFileDiscoveryFilter {
public:
    explicit FormatRecursiveFileFilter(FormatStyleCache& styleCache) : styleCache_(styleCache) {}

    bool ShouldVisitDirectory(std::string_view path, std::string& error) override {
        return !styleCache_.IsIgnored(path, error);
    }

    bool ShouldIncludeFile(std::string_view path, std::string& error) override {
        return IsFormatRecursiveInput(path) && !styleCache_.IsIgnored(path, error);
    }

private:
    FormatStyleCache& styleCache_;
};

const char* SummaryAction(bool checkMode, bool changed) {
    return checkMode ? (changed ? "Formatting is required for" : "Checked") : "Formatted";
}

void PrintFormatSummary(
    FILE* output,
    const char* verb,
    std::string_view subject,
    const char* changeVerb,
    int changedLineCount,
    int lineCount,
    int ignoredCount,
    int formatErrorCount,
    std::chrono::steady_clock::time_point start
) {
    std::fprintf(
        output,
        "%s %.*s. %s/%s LOC %s.",
        verb,
        static_cast<int>(subject.size()),
        subject.data(),
        FormatCount(changedLineCount).c_str(),
        FormatCount(lineCount).c_str(),
        changeVerb
    );
    if (ignoredCount > 0) {
        std::fprintf(
            output, " Skipped %s ignored file%s.", FormatCount(ignoredCount).c_str(), ignoredCount == 1 ? "" : "s"
        );
    }
    if (formatErrorCount > 0) {
        std::fprintf(
            output,
            " %s file%s failed formatting.",
            FormatCount(formatErrorCount).c_str(),
            formatErrorCount == 1 ? "" : "s"
        );
    }
    std::fprintf(output, " Done in %s.\n", FormatToolElapsed(std::chrono::steady_clock::now() - start).c_str());
}

}  // namespace

int RunFormat(int argc, char** argv) {
    const auto start = std::chrono::steady_clock::now();
    std::string optionsError;
    std::optional<FormatOptions> parsed = ParseFormatArgs(argc, argv, optionsError);
    if (!parsed) {
        if (!optionsError.empty()) {
            std::fprintf(stderr, "%s\n", optionsError.c_str());
        }
        PrintFormatUsage(stderr);
        return 2;
    }
    const FormatOptions& options = *parsed;
    if (options.version) {
        std::fprintf(stdout, "strictfmt %s\n", kStrictfmtVersion);
        return 0;
    }
    if (options.help) {
        PrintFormatUsage(stdout);
        return 0;
    }
    if (options.dumpFile.has_value()) {
        if (options.dumpKind == FormatDumpKind::BreakTree) {
            return DumpFormatBreakTree(
                *options.dumpFile, options.explicitStylePath, stdout, stderr, "strictfmt --dump-break-tree"
            );
        }
        return DumpFormatModel(
            *options.dumpFile, options.explicitStylePath, stdout, stderr, "strictfmt --dump-syntax-tree"
        );
    }

    FormatStyleCache styleCache(options.explicitStylePath);
    const std::string currentDirectory = AbsolutePath(CurrentDirectoryPath().string());
    FILE* summary = SummaryStream(options);

    if (!options.readStdin && options.files.empty() && !options.fileListProvided && !options.recursiveInputProvided) {
        PrintFormatUsage(stdout);
        return 0;
    }

    if (options.readStdin) {
        std::string error;
        const FormatterConfig* config = styleCache.ConfigForPath(currentDirectory, error);
        if (config == nullptr) {
            std::fprintf(stderr, "%s\n", error.c_str());
            return 2;
        }
        SetBinaryMode(stdin);
        std::string stdinText = ReadStdinText();
        if (options.dumpKind == FormatDumpKind::SyntaxTree) {
            return DumpFormatModelText(stdinText, *config, stdout, stderr, "strictfmt --dump-syntax-tree");
        }
        if (options.dumpKind == FormatDumpKind::BreakTree) {
            return
                DumpFormatBreakTreeText(stdinText, *config, "<stdin>", stdout, stderr, "strictfmt --dump-break-tree");
        }
        SourceFormatResult result = FormatSourceText(stdinText, *config, "<stdin>", options.validate);
        if (!result.ok) {
            PrintSourceError(stderr, "<stdin>", result.error);
            return 1;
        }
        PrintSourceWarnings(stderr, "<stdin>", result.warnings);
        const FormatDiffResult diff = ComputeFormatDiff(
            stdinText,
            result.formatted,
            options.mode == FormatMode::Diff ? std::optional<std::string_view>{"<stdin>"} : std::nullopt
        );
        if (options.mode == FormatMode::Stdout) {
            SetBinaryMode(stdout);
            std::fwrite(result.formatted.data(), 1, result.formatted.size(), stdout);
        } else if (options.mode == FormatMode::Diff) {
            SetBinaryMode(stdout);
            std::fwrite(diff.diff.data(), 1, diff.diff.size(), stdout);
        }
        PrintFormatSummary(
            summary,
            SummaryAction(IsCheckMode(options), result.changed),
            "stdin",
            IsCheckMode(options) ? "will change" : "changed",
            static_cast<int>(diff.changedLineCount),
            CountSourceLines(stdinText),
            0,
            0,
            start
        );
        return IsCheckMode(options) && result.changed ? 1 : 0;
    }

    bool failed = false;
    int formatErrorCount = 0;
    int changedCount = 0;
    int ignoredCount = 0;
    int processedCount = 0;
    int lineCount = 0;
    int changedLineCount = 0;
    std::vector<PendingFileFormat> pendingResults;
    std::vector<std::unique_ptr<CompletedFileFormat>> completed;
    std::atomic<size_t> listedCount = 0;
    std::atomic<bool> scanning = true;
    size_t recursiveStart = 0;
    std::string discoveryError;
    bool discoveryOk = false;
    std::mutex verboseOutputMutex;
    ToolFileProgress progress(summary, "format", start, !options.verbose);
    const auto formatFile = [&](const ResolvedFileFormat& item, size_t index, CompletedFileFormat& result) {
        const auto printProgress = [&](std::string_view action, auto elapsed) {
            const bool stillScanning = scanning.load();
            PrintVerboseFileProgress(
                summary, verboseOutputMutex, index, listedCount.load(), stillScanning, action, item.file, elapsed
            );
        };
        if (options.verbose) {
            printProgress("Formatting", std::nullopt);
        }
        const auto fileStart = std::chrono::steady_clock::now();
        std::optional<std::string> text = ReadFileBinary(item.file);
        if (!text) {
            result.readFailed = true;
            result.pending.result.ok = false;
        } else {
            result.hasPending = true;
            result.lineCount = CountSourceLines(*text);
            result.pending.result = FormatSourceText(*text, *item.config, item.file, options.validate);
            if (result.pending.result.ok && result.pending.result.changed) {
                FormatDiffResult diff = ComputeFormatDiff(
                    *text,
                    result.pending.result.formatted,
                    options.mode == FormatMode::Diff ? std::optional<std::string_view>{item.diffPath} : std::nullopt
                );
                result.changedLineCount = static_cast<int>(diff.changedLineCount);
                result.pending.diff = std::move(diff.diff);
            }
            if (IsCheckMode(options)) {
                std::string{}.swap(result.pending.result.formatted);
            }
        }
        if (options.verbose) {
            printProgress("Finished", std::chrono::steady_clock::now() - fileStart);
        }
    };
    try {
        RunToolParallel(options.concurrency, &progress, [&](const ToolWorkSubmit& submit) {
            const auto queueFile = [&](std::string_view path, bool recursive) {
                const std::string file = AbsolutePath(path);
                if (!recursive && styleCache.IsIgnored(file, discoveryError)) {
                    ++ignoredCount;
                    return true;
                }
                if (!discoveryError.empty()) {
                    return false;
                }
                const FormatterConfig* config = styleCache.ConfigForPath(file, discoveryError);
                if (config == nullptr) {
                    return false;
                }
                auto result = std::make_unique<CompletedFileFormat>();
                result->pending.file = file;
                if (recursive) {
                    result->sortKey = NormalizePathKey(file);
                }
                CompletedFileFormat* destination = result.get();
                const size_t index = completed.size();
                completed.push_back(std::move(result));
                listedCount.store(completed.size());
                ResolvedFileFormat item{
                    file,
                    options.mode == FormatMode::Diff ? RelativePath(file, currentDirectory) : std::string{},
                    config
                };
                submit([&, item = std::move(item), index, destination]() { formatFile(item, index, *destination); });
                return true;
            };
            discoveryOk = [&]() {
                for (const std::string& file : options.files) {
                    if (!queueFile(file, false)) {
                        return false;
                    }
                }
                recursiveStart = completed.size();
                FormatRecursiveFileFilter filter(styleCache);
                return DiscoverRecursiveToolFiles(
                    options.recursiveRoots,
                    filter,
                    [&](std::string_view file) { return queueFile(file, true); },
                    discoveryError
                );
            }();
            scanning.store(false);
        });
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Failed to process files: %s\n", error.what());
        return 1;
    }
    if (!discoveryOk) {
        std::fprintf(stderr, "%s\n", discoveryError.c_str());
        return 2;
    }
    std::stable_sort(completed.begin() + recursiveStart, completed.end(), [](const auto& left, const auto& right) {
        return left->sortKey < right->sortKey;
    });

    for (const auto& entry : completed) {
        CompletedFileFormat& completedFormat = *entry;
        const std::string& file = completedFormat.pending.file;
        if (completedFormat.readFailed) {
            std::fprintf(stderr, "Failed to read %s\n", file.c_str());
            failed = true;
            continue;
        }
        if (!completedFormat.hasPending) {
            continue;
        }
        ++processedCount;
        lineCount += completedFormat.lineCount;
        changedLineCount += completedFormat.changedLineCount;
        SourceFormatResult& result = completedFormat.pending.result;
        if (!result.ok) {
            PrintSourceError(stderr, file, result.error);
            ++formatErrorCount;
            failed = true;
            continue;
        }
        PrintSourceWarnings(stderr, file, result.warnings);
        if (result.changed) {
            ++changedCount;
        }
        pendingResults.push_back(std::move(completedFormat.pending));
    }
    if (!failed || options.mode == FormatMode::Diff) {
        for (const PendingFileFormat& pending : pendingResults) {
            if (options.mode == FormatMode::Stdout) {
                SetBinaryMode(stdout);
                std::fwrite(pending.result.formatted.data(), 1, pending.result.formatted.size(), stdout);
            } else if (options.mode == FormatMode::InPlace && pending.result.changed) {
                if (!WriteFileBinary(pending.file, pending.result.formatted)) {
                    std::fprintf(stderr, "Failed to write %s\n", pending.file.c_str());
                    failed = true;
                }
            } else if (options.mode == FormatMode::Diff && pending.result.changed) {
                SetBinaryMode(stdout);
                std::fwrite(pending.diff.data(), 1, pending.diff.size(), stdout);
            }
        }
    }
    const bool checkMode = IsCheckMode(options);
    const bool showChangedFiles = !failed && (!checkMode || changedCount > 0);
    PrintFormatSummary(
        summary,
        failed ? "Formatting failed. Checked" : SummaryAction(checkMode, changedCount > 0),
        FileCountText(showChangedFiles ? changedCount : processedCount, completed.size(), showChangedFiles),
        failed ? "need formatting" : checkMode ? "will change" : "changed",
        changedLineCount,
        lineCount,
        ignoredCount,
        formatErrorCount,
        start
    );
    return failed || (checkMode && changedCount > 0) ? 1 : 0;
}

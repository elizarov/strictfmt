#include "tools/impl/tools_common.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <system_error>

#include "util/file_path.h"
#include "util/strings.h"

namespace {

namespace fs = std::filesystem;

bool IsSeparator(char ch) {
    return ch == '\\' || ch == '/';
}

bool IsDrivePrefix(std::string_view path) {
    return path.size() >= 2 &&
        path[1] == ':' &&
        ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z'));
}

std::string TrimTrailingSeparators(std::string value) {
    while (value.size() > 1 && IsSeparator(value.back())) {
        if (value.size() == 3 && IsDrivePrefix(value)) {
            break;
        }
        value.pop_back();
    }
    return value;
}

fs::path NativePath(std::string_view path) {
    return fs::path(std::string(path));
}

std::string PathText(const fs::path& path) {
    return path.generic_string();
}

void RecursiveFilesInto(std::string_view root, std::vector<std::string>& files) {
    std::error_code error;
    fs::directory_iterator iterator(NativePath(root), fs::directory_options::skip_permission_denied, error);
    if (error) {
        return;
    }
    for (const fs::directory_entry& entry : iterator) {
        const std::string path = PathText(entry.path());
        if (entry.is_directory(error)) {
            RecursiveFilesInto(path, files);
        } else if (!error) {
            files.push_back(AbsolutePath(path));
        }
        error.clear();
    }
}

bool DiscoverRecursiveToolFilesInto(
    std::string_view root,
    ToolFileDiscoveryFilter& filter,
    ToolFileDiscoveryResult& result,
    std::string& error
) {
    std::error_code entryError;
    fs::directory_iterator iterator(NativePath(root), fs::directory_options::skip_permission_denied, entryError);
    if (entryError) {
        return true;
    }
    for (const fs::directory_entry& entry : iterator) {
        std::error_code typeError;
        const std::string path = AbsolutePath(PathText(entry.path()));
        if (entry.is_directory(typeError)) {
            if (!filter.ShouldVisitDirectory(path, error)) {
                if (!error.empty()) {
                    return false;
                }
                continue;
            }
            if (!DiscoverRecursiveToolFilesInto(path, filter, result, error)) {
                return false;
            }
        } else if (!typeError && filter.ShouldIncludeFile(path, error)) {
            result.files.push_back(path);
        } else {
            if (!error.empty()) {
                return false;
            }
            ++result.skippedFiles;
        }
    }
    return true;
}

}  // namespace

bool ToolFileDiscoveryFilter::ShouldVisitDirectory(std::string_view path, std::string& error) {
    (void)path;
    (void)error;
    return true;
}

std::string AbsolutePath(std::string_view path) {
    std::error_code error;
    const fs::path absolute = fs::absolute(NativePath(path), error);
    if (error) {
        return NormalizeSeparators(std::string(path));
    }
    return NormalizeSeparators(PathText(absolute.lexically_normal()));
}

std::string RelativePath(std::string_view path, std::string_view root) {
    const std::string normalizedPath = NormalizeSeparators(AbsolutePath(path));
    std::string normalizedRoot = TrimTrailingSeparators(NormalizeSeparators(AbsolutePath(root)));
    const std::string lowerPath = ToLower(normalizedPath);
    const std::string lowerRoot = ToLower(normalizedRoot);
    if (lowerPath == lowerRoot) {
        return {};
    }
    if (StartsWith(lowerPath, lowerRoot + "/")) {
        return normalizedPath.substr(normalizedRoot.size() + 1);
    }
    return normalizedPath;
}

std::string NormalizeSeparators(std::string value) {
    std::replace(value.begin(), value.end(), '\\', '/');
    return value;
}

std::string NormalizePathKey(std::string_view path) {
    return ToLower(NormalizeSeparators(AbsolutePath(path)));
}

std::string Extension(std::string_view path) {
    const size_t slash = path.find_last_of("\\/");
    const size_t dot = path.find_last_of('.');
    if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) {
        return {};
    }
    return std::string(path.substr(dot));
}

std::string RemoveExtension(std::string_view path) {
    const size_t slash = path.find_last_of("\\/");
    const size_t dot = path.find_last_of('.');
    if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) {
        return std::string(path);
    }
    return std::string(path.substr(0, dot));
}

bool DirectoryExists(std::string_view path) {
    std::error_code error;
    return fs::is_directory(NativePath(path), error);
}

bool EnsureParentDirectory(std::string_view path) {
    const std::string parent = FilePath(path).ParentPath().string();
    if (parent.empty()) {
        return true;
    }
    std::error_code error;
    return fs::create_directories(NativePath(parent), error) || (!error && DirectoryExists(parent));
}

std::optional<std::uint64_t> LastWriteTime(std::string_view path) {
    std::error_code error;
    const fs::file_time_type writeTime = fs::last_write_time(NativePath(path), error);
    if (error) {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>(writeTime.time_since_epoch().count());
}

std::vector<std::string> RecursiveFiles(std::string_view root) {
    std::vector<std::string> files;
    RecursiveFilesInto(root, files);
    return files;
}

std::optional<std::vector<std::string>> ReadToolFileList(std::string_view path, std::string& error) {
    if (path.empty()) {
        error = "--files requires a path";
        return std::nullopt;
    }

    std::optional<std::string> text = ReadFileBinary(AbsolutePath(path));
    if (!text.has_value()) {
        error = "failed to read --files list " + std::string(path);
        return std::nullopt;
    }

    std::vector<std::string> files;
    for (std::string line : SplitLines(*text)) {
        line = Trim(line);
        if (!line.empty()) {
            files.push_back(std::move(line));
        }
    }
    return files;
}

std::optional<ToolFileDiscoveryResult> DiscoverRecursiveToolFiles(
    const std::vector<std::string>& roots,
    ToolFileDiscoveryFilter& filter,
    std::string& error
) {
    ToolFileDiscoveryResult result;
    for (const std::string& root : roots) {
        const std::string absoluteRoot = AbsolutePath(root);
        if (!DirectoryExists(absoluteRoot)) {
            error = "recursive root does not exist: " + root;
            return std::nullopt;
        }
        if (!filter.ShouldVisitDirectory(absoluteRoot, error)) {
            if (!error.empty()) {
                return std::nullopt;
            }
            continue;
        }
        if (!DiscoverRecursiveToolFilesInto(absoluteRoot, filter, result, error)) {
            return std::nullopt;
        }
    }
    std::sort(result.files.begin(), result.files.end(), [](const std::string& left, const std::string& right) {
        return NormalizePathKey(left) < NormalizePathKey(right);
    });
    return result;
}

bool StartsWith(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

bool EndsWith(std::string_view value, std::string_view suffix) {
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

bool Contains(std::string_view value, std::string_view needle) {
    return value.find(needle) != std::string_view::npos;
}

std::string NormalizeTrailingLineCommentSpacing(std::string_view line) {
    bool inString = false;
    bool inChar = false;
    for (size_t index = 0; index + 1 < line.size(); ++index) {
        const char ch = line[index];
        const char next = line[index + 1];
        if (ch == '\\' && (inString || inChar)) {
            ++index;
            continue;
        }
        if (ch == '"' && !inChar) {
            inString = !inString;
            continue;
        }
        if (ch == '\'' && !inString) {
            inChar = !inChar;
            continue;
        }
        if (inString || inChar || ch != '/' || next != '/') {
            continue;
        }

        std::string_view before = line.substr(0, index);
        while (!before.empty() && (before.back() == ' ' || before.back() == '\t')) {
            before.remove_suffix(1);
        }
        if (before.empty()) {
            return std::string(line);
        }
        std::string result(before);
        result.append("  ");
        result.append(line.substr(index));
        return result;
    }
    return std::string(line);
}

std::vector<std::string> SplitLines(std::string_view text) {
    std::vector<std::string> lines;
    size_t start = 0;
    size_t index = 0;
    while (index < text.size()) {
        if (text[index] == '\r' || text[index] == '\n') {
            lines.emplace_back(text.substr(start, index - start));
            if (text[index] == '\r' && index + 1 < text.size() && text[index + 1] == '\n') {
                ++index;
            }
            ++index;
            start = index;
            continue;
        }
        ++index;
    }
    if (start < text.size()) {
        lines.emplace_back(text.substr(start));
    }
    return lines;
}

std::vector<std::string> Split(std::string_view text, char delimiter) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= text.size()) {
        const size_t index = text.find(delimiter, start);
        if (index == std::string_view::npos) {
            parts.emplace_back(text.substr(start));
            break;
        }
        parts.emplace_back(text.substr(start, index - start));
        start = index + 1;
    }
    return parts;
}

std::string ReplaceAll(std::string value, std::string_view from, std::string_view to) {
    if (from.empty()) {
        return value;
    }
    size_t index = 0;
    while ((index = value.find(from, index)) != std::string::npos) {
        value.replace(index, from.size(), to);
        index += to.size();
    }
    return value;
}

std::string CollapseWhitespace(const std::vector<std::string>& parts) {
    std::string result;
    for (const std::string& part : parts) {
        if (!result.empty()) {
            result.push_back(' ');
        }
        result += part;
    }
    return result;
}

std::string NormalizeInclude(std::string value) {
    return NormalizeSeparators(std::move(value));
}

bool HasRoot(std::string_view relative, const std::vector<std::string>& roots) {
    for (const std::string& root : roots) {
        if (relative == root || StartsWith(relative, root + "/")) {
            return true;
        }
    }
    return false;
}

bool IsExcluded(std::string_view relative, const std::vector<std::string>& prefixes) {
    for (const std::string& prefix : prefixes) {
        if (StartsWith(relative, prefix)) {
            return true;
        }
    }
    return false;
}

std::string FormatCount(int value) {
    std::string digits = std::to_string(value);
    std::string formatted;
    int count = 0;
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        if (count == 3) {
            formatted.push_back(',');
            count = 0;
        }
        formatted.push_back(*it);
        ++count;
    }
    std::reverse(formatted.begin(), formatted.end());
    return formatted;
}

std::string StripCommentsAndStrings(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    size_t i = 0;
    bool inLineComment = false;
    bool inBlockComment = false;
    bool inString = false;
    bool inChar = false;
    while (i < text.size()) {
        const char ch = text[i];
        const char next = i + 1 < text.size() ? text[i + 1] : '\0';
        if (inLineComment) {
            if (ch == '\n') {
                inLineComment = false;
                result.push_back('\n');
            } else {
                result.push_back(' ');
            }
            ++i;
            continue;
        }
        if (inBlockComment) {
            if (ch == '*' && next == '/') {
                result += "  ";
                inBlockComment = false;
                i += 2;
            } else {
                result.push_back(ch == '\n' ? '\n' : ' ');
                ++i;
            }
            continue;
        }
        if (inString) {
            if (ch == '\\' && next != '\0') {
                result += "  ";
                i += 2;
                continue;
            }
            result.push_back(ch == '\n' ? '\n' : ' ');
            if (ch == '"') {
                inString = false;
            }
            ++i;
            continue;
        }
        if (inChar) {
            if (ch == '\\' && next != '\0') {
                result += "  ";
                i += 2;
                continue;
            }
            result.push_back(ch == '\n' ? '\n' : ' ');
            if (ch == '\'') {
                inChar = false;
            }
            ++i;
            continue;
        }
        if (ch == '/' && next == '/') {
            result += "  ";
            inLineComment = true;
            i += 2;
            continue;
        }
        if (ch == '/' && next == '*') {
            result += "  ";
            inBlockComment = true;
            i += 2;
            continue;
        }
        if (ch == '"') {
            result.push_back(' ');
            inString = true;
            ++i;
            continue;
        }
        if (ch == '\'') {
            result.push_back(' ');
            inChar = true;
            ++i;
            continue;
        }
        result.push_back(ch);
        ++i;
    }
    return result;
}

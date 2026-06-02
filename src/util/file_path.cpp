#include "util/file_path.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

namespace {

namespace fs = std::filesystem;

bool IsSeparator(char ch) {
    return ch == '\\' || ch == '/';
}

bool HasDrivePrefix(std::string_view path) {
    return path.size() >= 2 &&
        path[1] == ':' &&
        ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z'));
}

size_t RootLength(std::string_view path) {
    if (path.size() >= 2 && IsSeparator(path[0]) && IsSeparator(path[1])) {
        size_t serverEnd = path.find_first_of("\\/", 2);
        if (serverEnd == std::string_view::npos) {
            return path.size();
        }
        size_t shareEnd = path.find_first_of("\\/", serverEnd + 1);
        return shareEnd == std::string_view::npos ? path.size() : shareEnd + 1;
    }
    if (HasDrivePrefix(path)) {
        return path.size() >= 3 && IsSeparator(path[2]) ? 3 : 2;
    }
    return !path.empty() && IsSeparator(path[0]) ? 1 : 0;
}

std::string TrimTrailingSeparators(std::string path) {
    const size_t rootLength = RootLength(path);
    while (path.size() > rootLength && IsSeparator(path.back())) {
        path.pop_back();
    }
    return path;
}

fs::path NativePath(std::string_view path) {
    return fs::path(std::string(path));
}

std::string PathText(const fs::path& path) {
    return path.generic_string();
}

}  // namespace

FilePath::FilePath(const char* path) : path_(path != nullptr ? path : "") {}

FilePath::FilePath(std::string path) : path_(std::move(path)) {}

FilePath::FilePath(std::string_view path) : path_(path) {}

bool FilePath::Empty() const {
    return path_.empty();
}

bool FilePath::empty() const {
    return Empty();
}

bool FilePath::IsAbsolute() const {
    return NativePath(path_).is_absolute();
}

bool FilePath::is_absolute() const {
    return IsAbsolute();
}

bool FilePath::HasParentPath() const {
    return !ParentPath().Empty();
}

bool FilePath::has_parent_path() const {
    return HasParentPath();
}

FilePath FilePath::ParentPath() const {
    if (path_.empty()) {
        return {};
    }
    fs::path parent = NativePath(TrimTrailingSeparators(path_)).parent_path();
    if (parent.empty()) {
        return FilePath{};
    }
    return FilePath(PathText(parent));
}

FilePath FilePath::parent_path() const {
    return ParentPath();
}

std::string FilePath::string() const {
    return path_;
}

FilePath JoinPath(const FilePath& base, const FilePath& child) {
    if (base.Empty() || child.IsAbsolute()) {
        return child;
    }
    if (child.Empty()) {
        return base;
    }
    return FilePath(PathText(NativePath(base.string()) / NativePath(child.string())));
}

FilePath JoinPath(const FilePath& base, const char* child) {
    return JoinPath(base, FilePath(child));
}

FilePath operator/(const FilePath& base, const FilePath& child) {
    return JoinPath(base, child);
}

FilePath operator/(const FilePath& base, const char* child) {
    return JoinPath(base, child);
}

FilePath CurrentDirectoryPath() {
    std::error_code error;
    const fs::path path = fs::current_path(error);
    if (error) {
        return {};
    }
    return FilePath(PathText(path));
}

FilePath TempDirectoryPath() {
    std::error_code error;
    const fs::path path = fs::temp_directory_path(error);
    if (error) {
        return {};
    }
    return FilePath(PathText(path));
}

bool FileExists(const FilePath& path) {
    if (path.Empty()) {
        return false;
    }
    std::error_code error;
    return fs::is_regular_file(NativePath(path.string()), error);
}

bool RemoveFileIfExists(const FilePath& path) {
    if (path.Empty()) {
        return false;
    }
    std::error_code error;
    if (fs::remove(NativePath(path.string()), error)) {
        return true;
    }
    if (!error) {
        return true;
    }
    std::error_code existsError;
    return !fs::exists(NativePath(path.string()), existsError) && !existsError;
}

std::optional<std::string> ReadFileBinary(const FilePath& path) {
    std::ifstream file(NativePath(path.string()), std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    file.seekg(0, std::ios::end);
    const std::streampos length = file.tellg();
    if (length < std::streampos{}) {
        return std::nullopt;
    }
    file.seekg(0, std::ios::beg);
    std::string content(static_cast<size_t>(length), '\0');
    if (!content.empty() && !file.read(content.data(), static_cast<std::streamsize>(content.size()))) {
        return std::nullopt;
    }
    return content;
}

bool WriteFileBinary(const FilePath& path, std::string_view text) {
    std::ofstream file(NativePath(path.string()), std::ios::binary);
    if (!file) {
        return false;
    }
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    return file.good();
}

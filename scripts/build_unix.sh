#!/usr/bin/env bash
set -eu

script_dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
repo_root="$(CDPATH= cd -- "$script_dir/.." && pwd)"

case "$(uname -s)" in
    Linux)
        os_name="linux"
        ;;
    Darwin)
        os_name="macos"
        ;;
    *)
        echo "error: unsupported operating system: $(uname -s)" >&2
        exit 1
        ;;
esac

build_root="$repo_root/build_$os_name"
cmake_build_dir="$build_root/cmake"

require_tool() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "error: $1 was not found in PATH." >&2
        exit 1
    fi
}

find_clang_pair() {
    if command -v clang >/dev/null 2>&1 && command -v clang++ >/dev/null 2>&1; then
        CC="clang"
        CXX="clang++"
        return 0
    fi

    for version in 20 19 18 17 16 15 14; do
        if command -v "clang-$version" >/dev/null 2>&1 &&
            command -v "clang++-$version" >/dev/null 2>&1; then
            CC="clang-$version"
            CXX="clang++-$version"
            return 0
        fi
    done

    return 1
}

if [ -z "${CC+x}" ] && [ -z "${CXX+x}" ]; then
    if ! find_clang_pair; then
        echo "error: no clang/clang++ toolchain was found in PATH." >&2
        exit 1
    fi
else
    : "${CC:=clang}"
    : "${CXX:=clang++}"
fi
export CC CXX

require_tool cmake
require_tool make
require_tool "$CC"
require_tool "$CXX"

mkdir -p "$build_root"

cmake -S "$repo_root" -B "$cmake_build_dir" -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$build_root" \
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="$build_root/lib" \
    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY="$build_root/lib"

cmake --build "$cmake_build_dir" --target strictfmt

if [ ! -x "$build_root/strictfmt" ]; then
    echo "error: expected executable was not produced: $build_root/strictfmt" >&2
    exit 1
fi

echo "Built $build_root/strictfmt"

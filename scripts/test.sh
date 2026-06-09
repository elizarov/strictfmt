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

cmake_build_dir="$repo_root/build/$os_name/cmake"

"$script_dir/build.sh"
cmake --build "$cmake_build_dir" --target strictfmt_tests

#!/usr/bin/env bash
set -eu

script_dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
repo_root="$(CDPATH= cd -- "$script_dir/.." && pwd)"

"$script_dir/build.sh"
"$repo_root/build/strictfmt" -i -r "$repo_root/src"

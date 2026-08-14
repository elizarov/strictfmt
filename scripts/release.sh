#!/usr/bin/env bash
set -eu

script_dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
repo_root="$(CDPATH= cd -- "$script_dir/.." && pwd)"

if [ "$#" -ne 1 ]; then
    echo "usage: scripts/release.sh <version>" >&2
    exit 2
fi

version="$1"
if [[ ! "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "error: version must have the form <major>.<minor>.<patch>" >&2
    exit 2
fi

if ! command -v git >/dev/null 2>&1; then
    echo "error: git was not found in PATH." >&2
    exit 1
fi

cd "$repo_root"

if [ -n "$(git status --porcelain)" ]; then
    echo "error: the worktree must be clean before creating a release" >&2
    exit 1
fi

branch="$(git branch --show-current)"
if [ "$branch" != "main" ]; then
    echo "error: releases must be created from the main branch, not $branch" >&2
    exit 1
fi

remote="$(git config --get "branch.$branch.remote" || true)"
if [ -z "$remote" ]; then
    echo "error: branch $branch has no configured upstream remote" >&2
    exit 1
fi

tag="v$version"
if git rev-parse --verify --quiet "refs/tags/$tag" >/dev/null; then
    echo "error: tag $tag already exists locally" >&2
    exit 1
fi
if [ -n "$(git ls-remote --tags "$remote" "refs/tags/$tag")" ]; then
    echo "error: tag $tag already exists on $remote" >&2
    exit 1
fi

git push "$remote" "$branch"
git tag -a "$tag" -m "strictfmt $version"
if ! git push "$remote" "refs/tags/$tag"; then
    git tag -d "$tag" >/dev/null
    echo "error: failed to push $tag; removed the local tag" >&2
    exit 1
fi

echo "Released strictfmt $version with tag $tag."

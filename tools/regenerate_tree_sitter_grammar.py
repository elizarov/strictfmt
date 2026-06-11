#!/usr/bin/env python3
"""Regenerate the vendored tree-sitter C++ grammar outputs."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import os
import platform
import shutil
import subprocess
import sys
import urllib.request
from dataclasses import dataclass
from pathlib import Path


TREE_SITTER_CLI_VERSION = "0.24.7"
TREE_SITTER_RELEASE_URL = f"https://github.com/tree-sitter/tree-sitter/releases/download/v{TREE_SITTER_CLI_VERSION}"


@dataclass(frozen=True)
class TreeSitterCliAsset:
    archive_name: str
    sha512: str
    executable_name: str


TREE_SITTER_CLI_ASSETS = {
    ("linux", "arm64"): TreeSitterCliAsset(
        archive_name="tree-sitter-linux-arm64.gz",
        sha512=(
            "7B600860B0407B0DBFC8FE255700A5359C3F75839F3598C1F10B9CADCA83749"
            "E8EE22D37666F72A6C637C725AA10548E303E09A91E17BB7A43C99B0DCE415D76"
        ),
        executable_name="tree-sitter",
    ),
    ("linux", "x64"): TreeSitterCliAsset(
        archive_name="tree-sitter-linux-x64.gz",
        sha512=(
            "D2B96C79BF1C224416144A4FB97AF0C1181583355E5160AD00FDB597363F6559"
            "408E3F92966914C8A10DCD99A1178F46ABEA07F0D06AF417A5D1753845072C1B"
        ),
        executable_name="tree-sitter",
    ),
    ("macos", "arm64"): TreeSitterCliAsset(
        archive_name="tree-sitter-macos-arm64.gz",
        sha512=(
            "8FCE2E4A457DF84EE646C96A858EDB59F994521D702378ABF6452F02CC8C8F7A"
            "85F27F3A921EA47306135588FEBA4936E2A74B72DB9169469685B1FA5E60A1BB"
        ),
        executable_name="tree-sitter",
    ),
    ("macos", "x64"): TreeSitterCliAsset(
        archive_name="tree-sitter-macos-x64.gz",
        sha512=(
            "6E734B1D2201E960CF81B5B28E0E3F3F3874CD0205EF654084D3B8C4AB0B3C"
            "345F0A98E7A24826FBEA7C7453AAD6CF58BF6494DFCC15FE92A818E555998F98A2"
        ),
        executable_name="tree-sitter",
    ),
    ("windows", "x64"): TreeSitterCliAsset(
        archive_name="tree-sitter-windows-x64.gz",
        sha512=(
            "4CEFF1C79CF8491B1099CBC401AC4F2B85BAC45716C8C4B24C3EDA35A38C01E4996000CAF86979323E3F6352B2BF61CE2904C971"
            "627AFC4B0BCDEFD4E40C8A36"
        ),
        executable_name="tree-sitter.exe",
    ),
}


def fail(message: str) -> None:
    print(message, file=sys.stderr)
    raise SystemExit(1)


def host_platform_key() -> str:
    if sys.platform.startswith("linux"):
        return "linux"
    if sys.platform == "darwin":
        return "macos"
    if sys.platform in ("win32", "cygwin"):
        return "windows"
    fail(f"Unsupported host platform for pinned tree-sitter CLI download: {sys.platform}")


def host_arch_key() -> str:
    machine = platform.machine().lower()
    if machine in ("amd64", "x86_64"):
        return "x64"
    if machine in ("aarch64", "arm64"):
        return "arm64"
    fail(f"Unsupported host architecture for pinned tree-sitter CLI download: {platform.machine()}")


def detect_tree_sitter_cli_asset() -> tuple[str, str, TreeSitterCliAsset]:
    platform_key = host_platform_key()
    arch_key = host_arch_key()
    asset = TREE_SITTER_CLI_ASSETS.get((platform_key, arch_key))
    if asset is None:
        fail(
            "Unsupported host for pinned tree-sitter CLI download: "
            f"{platform_key}-{arch_key}. Pass --tree-sitter-cli <path>."
        )
    return platform_key, arch_key, asset


def ensure_tree_sitter_cli(repo_root: Path, tree_sitter_cli: str | None) -> Path:
    if tree_sitter_cli:
        cli_path = Path(tree_sitter_cli)
        if not cli_path.exists():
            fail(f"tree-sitter CLI was not found: {cli_path}")
        return cli_path

    platform_key, arch_key, asset = detect_tree_sitter_cli_asset()
    cli_dir = (
        repo_root / "build" / f"tree-sitter-cli-v{TREE_SITTER_CLI_VERSION}" / f"{platform_key}-{arch_key}"
    )
    cli_path = cli_dir / asset.executable_name
    if cli_path.exists():
        return cli_path

    archive_path = cli_dir / asset.archive_name
    archive_stem = asset.archive_name.removesuffix(".gz")
    legacy_archive_path = repo_root / "build" / f"{archive_stem}-v{TREE_SITTER_CLI_VERSION}.gz"
    cli_dir.mkdir(parents=True, exist_ok=True)
    if legacy_archive_path.exists():
        shutil.copyfile(legacy_archive_path, archive_path)
    elif not archive_path.exists():
        url = f"{TREE_SITTER_RELEASE_URL}/{asset.archive_name}"
        print(f"Downloading tree-sitter CLI v{TREE_SITTER_CLI_VERSION} to {archive_path}", flush=True)
        with urllib.request.urlopen(url, timeout=60) as response:
            archive_path.write_bytes(response.read())
    archive_hash = hashlib.sha512(archive_path.read_bytes()).hexdigest().upper()
    if archive_hash != asset.sha512:
        archive_path.unlink(missing_ok=True)
        fail("Downloaded tree-sitter CLI archive hash did not match the pinned SHA512.")

    with gzip.open(archive_path, "rb") as compressed:
        cli_path.write_bytes(compressed.read())
    if os.name != "nt":
        cli_path.chmod(cli_path.stat().st_mode | 0o111)
    return cli_path


def run_tree_sitter_generate(cpp_grammar_dir: Path, vendor_root: Path, tree_sitter_cli: Path) -> None:
    env = os.environ.copy()
    existing_node_path = env.get("NODE_PATH")
    env["NODE_PATH"] = str(vendor_root) if not existing_node_path else str(vendor_root) + os.pathsep + existing_node_path

    subprocess.run([str(tree_sitter_cli), "generate"], cwd=cpp_grammar_dir, env=env, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--tree-sitter-cli",
        help="Optional path to an existing tree-sitter CLI executable. Defaults to a pinned CLI under build/.",
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    vendor_root = repo_root / "vendor" / "tree-sitter"
    cpp_grammar_dir = vendor_root / "tree-sitter-cpp"
    c_grammar_dir = vendor_root / "tree-sitter-c"

    for required_path in (cpp_grammar_dir / "grammar.js", c_grammar_dir / "grammar.js"):
        if not required_path.exists():
            fail(f"Missing required grammar regeneration input: {required_path}")

    tree_sitter_cli = ensure_tree_sitter_cli(repo_root, args.tree_sitter_cli)
    run_tree_sitter_generate(cpp_grammar_dir, vendor_root, tree_sitter_cli)
    print(f"Regenerated tree-sitter C++ grammar outputs under {cpp_grammar_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Regenerate the vendored tree-sitter C++ grammar outputs."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import urllib.request
from dataclasses import dataclass
from pathlib import Path


TREE_SITTER_CLI_VERSION = "0.24.7"
TREE_SITTER_RELEASE_URL = f"https://github.com/tree-sitter/tree-sitter/releases/download/v{TREE_SITTER_CLI_VERSION}"
MAX_GENERATED_PARSER_BYTES = 50_000_000

# These are the reviewed lexical leaves in the generated grammar. Keeping the
# list closed makes a new named terminal or external token a deliberate change
# instead of an easy way to hide composite syntax from the formatter.
LEXICAL_GRAMMAR_TERMINALS = frozenset({
    "char_literal",
    "comment",
    "escape_sequence",
    "false",
    "identifier",
    "literal_suffix",
    "macro_comment_argument",
    "number_literal",
    "preproc_directive",
    "preprocessing_number",
    "primitive_type",
    "pure_virtual_zero",
    "suffixed_string_literal",
    "system_lib_string",
    "true",
})
LEXICAL_EXTERNAL_TOKENS = frozenset({
    "_line_break_whitespace",
    "_preproc_directive_end",
    "bare_macro_identifier",
    "call_syntax_macro_identifier",
    "declaration_prefix_macro_identifier",
    "preprocessor_argument_macro_identifier",
    "raw_macro_definition_identifier",
    "raw_string_content",
    "raw_string_delimiter",
    "semicolonless_call_macro_identifier",
    "statement_argument_macro_identifier",
    "type_specifier_macro_identifier",
})
OPAQUE_EXTERNAL_TOKENS = frozenset({"raw_macro_replacement"})
TERMINAL_RULE_TYPES = frozenset({"IMMEDIATE_TOKEN", "PATTERN", "TOKEN"})
PREPROCESSOR_DIRECTIVE_ARGUMENTS = frozenset({
    "define",
    "elif",
    "elifdef",
    "elifndef",
    "if",
    "ifdef",
    "ifndef",
    "include",
})
PREPROCESSOR_DIRECTIVES = PREPROCESSOR_DIRECTIVE_ARGUMENTS | frozenset({"else", "endif", "using"})
STRING_CONTENT_PATTERN = r'[^\\"\n]+'

SYMBOL_ENUM_RE = re.compile(r"enum ts_symbol_identifiers \{(?P<body>.*?)\n\};", re.DOTALL)
SYMBOL_VALUE_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*([0-9]+),$", re.MULTILINE)
SYMBOL_REFERENCE_RE = re.compile(
    r"\b(?:(?:alias|anon|aux)_sym|sym)_[A-Za-z0-9_]+\b|\bts_builtin_sym_end\b"
)
IDENTITY_TABLE_ENTRY_RE = re.compile(r"\b(?:ACTIONS|STATE)\(([0-9]+)\)")
LEADING_INDENT_RE = re.compile(r"^[ \t]+", re.MULTILINE)


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


def validate_structural_grammar(grammar_json_path: Path) -> None:
    grammar = json.loads(grammar_json_path.read_text(encoding="utf-8"))
    grammar_terminals = {
        name
        for name, rule in grammar["rules"].items()
        if rule["type"] in TERMINAL_RULE_TYPES
    }
    unexpected_terminals = grammar_terminals - LEXICAL_GRAMMAR_TERMINALS
    if unexpected_terminals:
        fail(
            "Composite syntax must use structured grammar productions; "
            "unreviewed named terminals: " + ", ".join(sorted(unexpected_terminals))
        )

    def is_reviewed_inline_pattern(pattern: str) -> bool:
        if pattern == STRING_CONTENT_PATTERN:
            return True
        for command in PREPROCESSOR_DIRECTIVES:
            separator = r"[ \t]+" if command in PREPROCESSOR_DIRECTIVE_ARGUMENTS else ""
            if pattern == rf"#[ \t]*{command}{separator}":
                return True
            # Inherited tree-sitter-c rules use a RegExp constructed from a
            # JavaScript string, so the generated JSON contains a literal tab.
            if pattern == f"#[ \t]*{command}":
                return True
        return False

    def unwrap_token_content(node: dict[str, object]) -> dict[str, object]:
        while node.get("type") == "PREC":
            content = node.get("content")
            if not isinstance(content, dict):
                fail(f"Malformed token precedence in {grammar_json_path}: {node!r}")
            node = content
        return node

    def validate_rule_node(rule_name: str, node: dict[str, object]) -> None:
        node_type = node.get("type")
        if node_type == "PATTERN":
            pattern = node.get("value")
            if not isinstance(pattern, str) or not is_reviewed_inline_pattern(pattern):
                fail(
                    "Composite syntax must use structured grammar productions; "
                    f"unreviewed pattern in {rule_name}: {pattern!r}"
                )
        if node_type in {"IMMEDIATE_TOKEN", "TOKEN"}:
            content = node.get("content")
            if not isinstance(content, dict):
                fail(f"Malformed token in {rule_name}: {node!r}")
            lexical_leaf = unwrap_token_content(content)
            if lexical_leaf.get("type") not in {"PATTERN", "STRING"}:
                fail(
                    "Composite syntax must use structured grammar productions; "
                    f"composite token wrapper in {rule_name}: {lexical_leaf.get('type')!r}"
                )
        for child in node.values():
            if isinstance(child, dict):
                validate_rule_node(rule_name, child)
            elif isinstance(child, list):
                for item in child:
                    if isinstance(item, dict):
                        validate_rule_node(rule_name, item)

    for name, rule in grammar["rules"].items():
        if name not in LEXICAL_GRAMMAR_TERMINALS:
            validate_rule_node(name, rule)

    external_tokens = set()
    for external in grammar["externals"]:
        if external.get("type") != "SYMBOL" or not isinstance(external.get("name"), str):
            fail(f"Unreviewed external-token declaration in {grammar_json_path}: {external!r}")
        external_tokens.add(external["name"])
    unexpected_externals = external_tokens - LEXICAL_EXTERNAL_TOKENS - OPAQUE_EXTERNAL_TOKENS
    if unexpected_externals:
        fail(
            "Composite syntax must use structured grammar productions; "
            "unreviewed external tokens: " + ", ".join(sorted(unexpected_externals))
        )
    if external_tokens & OPAQUE_EXTERNAL_TOKENS != OPAQUE_EXTERNAL_TOKENS:
        fail("raw_macro_replacement must remain the sole opaque external token")


def compact_generated_parser(cpp_grammar_dir: Path) -> None:
    parser_path = cpp_grammar_dir / "src" / "parser.c"
    parser_header_path = cpp_grammar_dir / "src" / "tree_sitter" / "parser.h"
    parser_header = parser_header_path.read_text(encoding="utf-8")
    for macro in ("ACTIONS", "STATE"):
        if f"#define {macro}(id) id" not in parser_header:
            fail(f"Cannot compact parser table: {macro} is not an identity macro in {parser_header_path}")

    generated = parser_path.read_text(encoding="utf-8")
    symbol_enum = SYMBOL_ENUM_RE.search(generated)
    if symbol_enum is None:
        fail(f"Cannot compact parser table: symbol enum was not found in {parser_path}")

    symbol_values = {"ts_builtin_sym_end": "0"}
    symbol_values.update(dict(SYMBOL_VALUE_RE.findall(symbol_enum.group("body"))))

    table_start = generated.find("static const uint16_t ts_parse_table")
    table_end = generated.find("static const TSParseActionEntry ts_parse_actions[]")
    if table_start < 0 or table_end < 0 or table_start >= table_end:
        fail(f"Cannot compact parser table: generated table boundaries were not found in {parser_path}")

    table = generated[table_start:table_end]

    def replace_symbol(match: re.Match[str]) -> str:
        symbol = match.group(0)
        value = symbol_values.get(symbol)
        if value is None:
            fail(f"Cannot compact parser table: generated symbol has no numeric value: {symbol}")
        return value

    table = SYMBOL_REFERENCE_RE.sub(replace_symbol, table)
    table = IDENTITY_TABLE_ENTRY_RE.sub(r"\1", table)
    table = LEADING_INDENT_RE.sub("", table)
    compacted = generated[:table_start] + table + generated[table_end:]
    parser_path.write_bytes(compacted.encode("utf-8"))

    parser_size = parser_path.stat().st_size
    if parser_size > MAX_GENERATED_PARSER_BYTES:
        fail(
            f"Compacted generated parser is {parser_size:,} bytes; "
            f"the limit is {MAX_GENERATED_PARSER_BYTES:,} bytes."
        )
    print(f"Compacted generated parser from {len(generated):,} to {parser_size:,} bytes", flush=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--tree-sitter-cli",
        help="Optional path to an existing tree-sitter CLI executable. Defaults to a pinned CLI under build/.",
    )
    parser.add_argument(
        "--validate-structure-only",
        action="store_true",
        help="Validate the checked-in grammar JSON without regenerating parser outputs.",
    )
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    vendor_root = repo_root / "vendor" / "tree-sitter"
    cpp_grammar_dir = vendor_root / "tree-sitter-cpp"
    c_grammar_dir = vendor_root / "tree-sitter-c"

    for required_path in (cpp_grammar_dir / "grammar.js", c_grammar_dir / "grammar.js"):
        if not required_path.exists():
            fail(f"Missing required grammar regeneration input: {required_path}")

    grammar_json_path = cpp_grammar_dir / "src" / "grammar.json"
    if args.validate_structure_only:
        validate_structural_grammar(grammar_json_path)
        print(f"Validated structured grammar terminals in {grammar_json_path}")
        return 0

    tree_sitter_cli = ensure_tree_sitter_cli(repo_root, args.tree_sitter_cli)
    run_tree_sitter_generate(cpp_grammar_dir, vendor_root, tree_sitter_cli)
    validate_structural_grammar(grammar_json_path)
    compact_generated_parser(cpp_grammar_dir)
    print(f"Regenerated tree-sitter C++ grammar outputs under {cpp_grammar_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

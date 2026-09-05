from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from contextlib import contextmanager
from dataclasses import dataclass
from pathlib import Path


TEST_ROOT = Path(__file__).resolve().parent
STRICTFMT_ROOT = Path(os.environ.get("STRICTFMT_PROJECT_ROOT", TEST_ROOT.parents[1])).resolve()
TEST_TEMP_ROOT = Path(os.environ.get("STRICTFMT_TEST_TEMP_ROOT", STRICTFMT_ROOT / "build")).resolve()
FORMAT_EXE = Path(os.environ.get("STRICTFMT_EXE", STRICTFMT_ROOT / "build" / "strictfmt.exe")).resolve()
FORMAT_EXE_ARGS = os.environ.get("STRICTFMT_EXE_ARGS", "").split()
EXPECTED_VERSION = os.environ.get("STRICTFMT_EXPECTED_VERSION")
PLATFORM_LINE_ENDING = os.linesep.encode("ascii")
PRETTY_PRINTER_SOURCE = STRICTFMT_ROOT / "src" / "format" / "impl" / "format_pretty_printer.cpp"
GRAMMAR_REGENERATOR = STRICTFMT_ROOT / "tools" / "regenerate_tree_sitter_grammar.py"
EXTERNAL_ROOT = STRICTFMT_ROOT / "external"
DOCS_ROOT = STRICTFMT_ROOT / "docs"
DOCUMENTATION_CONFIG_START = "<!-- .cpp-format"
DOCUMENTATION_CONFIG_END = "-->"
DOCUMENTATION_CPP_LANGUAGES = frozenset(
    ("c", "c++", "cc", "cpp", "cxx", "h", "h++", "hh", "hpp", "hxx")
)
MARKDOWN_FENCE_PATTERN = re.compile(r"^(?P<fence>`{3,}|~{3,})(?P<info>.*)$")
SOURCE_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".c++",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".h++",
    ".ipp",
    ".inl",
    ".tpp",
}
INPUT_FIXTURE = Path("src") / "format_test_input.cpp"
OUTPUT_FIXTURE = Path("src") / "format_test_output.cpp"
MAIN_INCLUDE_INPUT_FIXTURE = Path("src") / "format_main_include_input.cpp"
MAIN_INCLUDE_OUTPUT_FIXTURE = Path("src") / "format_main_include_output.cpp"
OPTIMIZATION_INPUT_FIXTURE = Path("src") / "format_optimization_input.cpp"
OPTIMIZATION_OUTPUT_FIXTURE = Path("src") / "format_optimization_output.cpp"
NON_ASCII_INPUT_FIXTURE = Path("src") / "format_non_ascii_input.cpp"
NON_ASCII_OUTPUT_FIXTURE = Path("src") / "format_non_ascii_output.cpp"
USERVER_INPUT_FIXTURE = Path("src") / "format_userver_input.cpp"
USERVER_OUTPUT_FIXTURE = Path("src") / "format_userver_output.cpp"
IFDEF_INPUT_FIXTURE = Path("src") / "format_ifdef_input.cpp"
IFDEF_OUTPUT_FIXTURE = Path("src") / "format_ifdef_output.cpp"
UNSUPPORTED_INPUT_FIXTURE = Path("src") / "format_unsupported_input.cpp"
UNSUPPORTED_OUTPUT_FIXTURE = Path("src") / "format_unsupported_output.cpp"
UNSUPPORTED_WARNINGS_FIXTURE = Path("src") / "format_unsupported_output.txt"
ERROR_INPUT_FIXTURE = Path("src") / "format_error_input.cpp"
ERROR_OUTPUT_FIXTURE = Path("src") / "format_error_output.txt"
USERVER_FORMAT_CONFIG = TEST_ROOT / ".cpp-format-userver"
DEFAULT_FORMAT_CONFIG = TEST_ROOT / ".cpp-format"
OPTIMIZATION_FORMAT_CONFIG = TEST_ROOT / ".cpp-format-optimization"
NON_ASCII_FORMAT_CONFIG = TEST_ROOT / ".cpp-format-non-ascii"
FORMATTED_GOLDEN_OUTPUTS = (
    ("default", OUTPUT_FIXTURE, None),
    ("optimization", OPTIMIZATION_OUTPUT_FIXTURE, OPTIMIZATION_FORMAT_CONFIG),
    ("non-ascii", NON_ASCII_OUTPUT_FIXTURE, NON_ASCII_FORMAT_CONFIG),
    ("userver", USERVER_OUTPUT_FIXTURE, USERVER_FORMAT_CONFIG),
    ("ifdef", IFDEF_OUTPUT_FIXTURE, USERVER_FORMAT_CONFIG),
    ("unsupported", UNSUPPORTED_OUTPUT_FIXTURE, USERVER_FORMAT_CONFIG),
)


@dataclass(frozen=True)
class DocumentationCodeExample:
    document: Path
    fence_line: int
    source: str
    config: str | None


def markdown_fence_end(lines: list[str], start: int, fence: str, document: Path) -> int:
    fence_character = re.escape(fence[0])
    closing_pattern = re.compile(rf"^{fence_character}{{{len(fence)},}}[ \t]*$")
    for index in range(start + 1, len(lines)):
        if closing_pattern.fullmatch(lines[index].rstrip("\r\n")):
            return index
    raise AssertionError(f"{document.relative_to(STRICTFMT_ROOT)}:{start + 1}: unclosed Markdown fence")


def documentation_code_examples() -> list[DocumentationCodeExample]:
    examples = []
    documents = (STRICTFMT_ROOT / "README.md", *sorted(DOCS_ROOT.glob("*.md")))
    for document in documents:
        with document.open(encoding="utf-8", newline="") as source:
            lines = source.read().splitlines(keepends=True)
        index = 0
        while index < len(lines):
            config = None
            if lines[index].rstrip("\r\n") == DOCUMENTATION_CONFIG_START:
                config_start = index
                index += 1
                config_lines = []
                while index < len(lines) and lines[index].rstrip("\r\n") != DOCUMENTATION_CONFIG_END:
                    config_lines.append(lines[index])
                    index += 1
                if index == len(lines):
                    relative = document.relative_to(STRICTFMT_ROOT)
                    raise AssertionError(f"{relative}:{config_start + 1}: unclosed .cpp-format comment")
                config = "".join(config_lines)
                index += 1
                if index == len(lines):
                    relative = document.relative_to(STRICTFMT_ROOT)
                    raise AssertionError(f"{relative}:{config_start + 1}: .cpp-format comment has no code fence")

            opening = MARKDOWN_FENCE_PATTERN.fullmatch(lines[index].rstrip("\r\n"))
            if opening is None:
                if config is not None:
                    relative = document.relative_to(STRICTFMT_ROOT)
                    raise AssertionError(
                        f"{relative}:{config_start + 1}: .cpp-format comment must immediately precede a code fence"
                    )
                index += 1
                continue

            fence = opening.group("fence")
            fence_end = markdown_fence_end(lines, index, fence, document)
            info = opening.group("info").strip().lower()
            language = info.split(maxsplit=1)[0] if info else ""
            if config is not None and language not in DOCUMENTATION_CPP_LANGUAGES:
                relative = document.relative_to(STRICTFMT_ROOT)
                raise AssertionError(
                    f"{relative}:{config_start + 1}: .cpp-format comment precedes non-C/C++ fence {language!r}"
                )
            if language in DOCUMENTATION_CPP_LANGUAGES:
                examples.append(
                    DocumentationCodeExample(
                        document=document,
                        fence_line=index + 1,
                        source="".join(lines[index + 1:fence_end]),
                        config=config,
                    )
                )
            index = fence_end + 1
    return examples


def format_command(args: tuple[str, ...], validate: bool) -> list[str]:
    validation = ["--validate"] if validate and not any(
        arg in {"--dump-syntax-tree", "--dump-break-tree"} for arg in args
    ) else []
    return [str(FORMAT_EXE), *FORMAT_EXE_ARGS, *validation, *args]


def native_format(
    *args: str, cwd: Path = STRICTFMT_ROOT, input_text: str | None = None, timeout: float | None = None,
    validate: bool = True
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        format_command(args, validate),
        cwd=cwd,
        input=input_text,
        check=False,
        capture_output=True,
        text=True,
        timeout=timeout,
    )


def native_format_bytes(
    *args: str, cwd: Path = STRICTFMT_ROOT, input_bytes: bytes | None = None, validate: bool = True
) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(
        format_command(args, validate),
        cwd=cwd,
        input=input_bytes,
        check=False,
        capture_output=True,
    )


def read_fixture(path: Path) -> str:
    return (TEST_ROOT / path).read_text(encoding="utf-8")


def fixture_loc(path: Path) -> str:
    return f"{len(read_fixture(path).splitlines()):,}"


def write_empty_ignore(root: Path) -> None:
    (root / ".cpp-format-ignore").write_text("", encoding="utf-8")


def join_lines(lines: list[bytes], line_ending: bytes) -> bytes:
    return line_ending.join(lines) + line_ending


def copy_default_config(root: Path) -> None:
    shutil.copyfile(DEFAULT_FORMAT_CONFIG, root / ".cpp-format")


def load_ignore_entries(root: Path) -> list[str]:
    ignore_file = root / ".cpp-format-ignore"
    if not ignore_file.exists():
        return []

    entries = []
    for line in ignore_file.read_text(encoding="utf-8").splitlines():
        entry = line.split("#", maxsplit=1)[0].replace("\\", "/").strip()
        while entry.startswith("./"):
            entry = entry[2:]
        entry = entry.rstrip("/")
        if entry:
            entries.append(entry.lower())
    return entries


def is_ignored_path(relative_path: Path, ignore_entries: list[str]) -> bool:
    normalized = relative_path.as_posix().lower()
    parts = normalized.split("/")
    for entry in ignore_entries:
        if "/" in entry:
            if normalized == entry or normalized.startswith(f"{entry}/"):
                return True
        elif entry in parts:
            return True
    return False


def discover_source_files(root: Path) -> list[Path]:
    ignore_entries = load_ignore_entries(root)
    sources = []
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in SOURCE_SUFFIXES:
            continue
        relative = path.relative_to(root)
        if not is_ignored_path(relative, ignore_entries):
            sources.append(relative)
    return sorted(sources)


def read_files(root: Path, paths: list[Path]) -> dict[Path, bytes]:
    return {path: (root / path).read_bytes() for path in paths}


class MethodNameTestResult(unittest.TextTestResult):
    def getDescription(self, test: unittest.case.TestCase) -> str:
        return test.id().rsplit(".", maxsplit=1)[-1]


@contextmanager
def copied_fixtures(*paths: Path):
    build_dir = TEST_TEMP_ROOT
    build_dir.mkdir(exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="format_fixtures_", dir=build_dir) as temp_dir:
        root = Path(temp_dir)
        copy_default_config(root)
        write_empty_ignore(root)
        copies = {}
        for path in paths:
            copied_path = root / path.name
            shutil.copyfile(TEST_ROOT / path, copied_path)
            copies[path] = copied_path
        yield copies


class FormatCommandTests(unittest.TestCase):
    maxDiff = None

    def assert_no_unsupported_placement_warnings(self, result: subprocess.CompletedProcess[str]) -> None:
        self.assertNotIn(": warning at ", result.stderr)

    def test_version_prints_embedded_version(self) -> None:
        result = native_format("--version")

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual("", result.stderr)
        if EXPECTED_VERSION is None:
            self.assertRegex(result.stdout, r"^strictfmt [0-9A-Za-z][0-9A-Za-z.+-]*\n$")
        else:
            self.assertEqual(f"strictfmt {EXPECTED_VERSION}\n", result.stdout)

    def test_strictfmt_sources_are_formatted(self) -> None:
        result = native_format("--dry-run", "-r", "src")

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual("", result.stderr)

    def test_documentation_code_examples_are_canonically_formatted(self) -> None:
        examples = documentation_code_examples()
        self.assertGreater(len(examples), 0)

        TEST_TEMP_ROOT.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="format_documentation_", dir=TEST_TEMP_ROOT) as temp_dir:
            temp_root = Path(temp_dir)
            for number, example in enumerate(examples):
                relative = example.document.relative_to(STRICTFMT_ROOT)
                location = f"{relative}:{example.fence_line}"
                with self.subTest(location=location):
                    args = ["--stdin"]
                    if example.config is not None:
                        config = temp_root / f"example-{number}.cpp-format"
                        config.write_bytes(example.config.encode("utf-8"))
                        args.extend(("--style", str(config)))

                    expected = example.source.encode("utf-8")
                    result = native_format_bytes(*args, input_bytes=expected)

                    self.assertEqual(
                        0,
                        result.returncode,
                        msg=(
                            f"{location}\nstdout:\n{result.stdout.decode('utf-8', errors='replace')}\n\n"
                            f"stderr:\n{result.stderr.decode('utf-8', errors='replace')}"
                        ),
                    )
                    self.assertNotIn(b": warning at ", result.stderr)
                    self.assertEqual(expected, result.stdout, msg=location)

    def assert_external_project_sources_parse_without_warnings_and_format_idempotently(
        self,
        name: str,
    ) -> None:
        project_root = EXTERNAL_ROOT / name
        if not (project_root / ".cpp-format").exists():
            self.skipTest(f"external/{name} submodule is not initialized")

        source_files = discover_source_files(project_root)
        self.assertGreater(len(source_files), 0)

        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix=f"{name}_format_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            shutil.copyfile(project_root / ".cpp-format", root / ".cpp-format")
            ignore_file = project_root / ".cpp-format-ignore"
            if ignore_file.exists():
                shutil.copyfile(ignore_file, root / ".cpp-format-ignore")
            else:
                write_empty_ignore(root)
            for relative in source_files:
                copied = root / relative
                copied.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(project_root / relative, copied)

            first_result = native_format("-i", "-r", ".", cwd=root)

            self.assertEqual(
                0,
                first_result.returncode,
                msg=f"stdout:\n{first_result.stdout}\n\nstderr:\n{first_result.stderr}",
            )
            self.assertNotIn("parse failed", first_result.stderr)
            self.assert_no_unsupported_placement_warnings(first_result)

            after_first_pass = read_files(root, source_files)
            second_result = native_format("-i", "-r", ".", cwd=root)

            self.assertEqual(
                0,
                second_result.returncode,
                msg=f"stdout:\n{second_result.stdout}\n\nstderr:\n{second_result.stderr}",
            )
            self.assertNotIn("parse failed", second_result.stderr)
            self.assert_no_unsupported_placement_warnings(second_result)
            after_second_pass = read_files(root, source_files)
            for relative in source_files:
                if after_first_pass[relative] != after_second_pass[relative]:
                    self.fail(f"{relative} changed on the second formatter pass")

    def test_stdin_formats_to_expected_output(self) -> None:
        result = native_format("--stdin", cwd=TEST_ROOT, input_text=read_fixture(INPUT_FIXTURE))

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(read_fixture(OUTPUT_FIXTURE), result.stdout)
        self.assert_no_unsupported_placement_warnings(result)
        self.assertRegex(result.stderr, r"Formatted stdin in (?:\d+ms|\d+\.\d{3}s)\.\s*$")

    def test_golden_input_parses_without_errors(self) -> None:
        with copied_fixtures(INPUT_FIXTURE) as fixtures:
            result = native_format(str(fixtures[INPUT_FIXTURE]))

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertNotIn("parse failed", result.stderr)
        self.assert_no_unsupported_placement_warnings(result)

    def test_optimization_stdin_formats_to_expected_output(self) -> None:
        result = native_format(
            "--stdin",
            "--style",
            str(OPTIMIZATION_FORMAT_CONFIG),
            cwd=TEST_ROOT,
            input_text=read_fixture(OPTIMIZATION_INPUT_FIXTURE),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(read_fixture(OPTIMIZATION_OUTPUT_FIXTURE), result.stdout)
        self.assert_no_unsupported_placement_warnings(result)

    def test_optimization_golden_input_parses_without_errors(self) -> None:
        with copied_fixtures(OPTIMIZATION_INPUT_FIXTURE) as fixtures:
            result = native_format(
                "--style", str(OPTIMIZATION_FORMAT_CONFIG), str(fixtures[OPTIMIZATION_INPUT_FIXTURE])
            )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertNotIn("parse failed", result.stderr)
        self.assert_no_unsupported_placement_warnings(result)

    def test_non_ascii_stdin_formats_to_expected_output(self) -> None:
        source = read_fixture(NON_ASCII_INPUT_FIXTURE).encode("utf-8")
        expected = read_fixture(NON_ASCII_OUTPUT_FIXTURE).encode("utf-8")
        for ending in (b"\n", b"\r\n"):
            with self.subTest(line_ending=ending):
                result = native_format_bytes(
                    "--stdin",
                    "--style",
                    str(NON_ASCII_FORMAT_CONFIG),
                    cwd=TEST_ROOT,
                    input_bytes=source.replace(b"\n", ending),
                )

                self.assertEqual(0, result.returncode, msg=result.stderr)
                self.assertEqual(expected.replace(b"\n", ending), result.stdout)
                self.assertNotIn(b": warning at ", result.stderr)

    def test_golden_outputs_reparse_and_format_idempotently(self) -> None:
        for name, fixture, style in FORMATTED_GOLDEN_OUTPUTS:
            with self.subTest(name=name):
                args = ["--stdin"]
                if style is not None:
                    args.extend(("--style", str(style)))

                expected = read_fixture(fixture)
                result = native_format(*args, cwd=TEST_ROOT, input_text=expected)

                self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
                self.assertEqual(expected, result.stdout)
                if name != "unsupported":
                    self.assert_no_unsupported_placement_warnings(result)

    def test_userver_stdin_formats_to_expected_output(self) -> None:
        result = native_format(
            "--stdin",
            "--style",
            str(USERVER_FORMAT_CONFIG),
            cwd=TEST_ROOT,
            input_text=read_fixture(USERVER_INPUT_FIXTURE),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(read_fixture(USERVER_OUTPUT_FIXTURE), result.stdout)
        self.assert_no_unsupported_placement_warnings(result)
        self.assertRegex(result.stderr, r"Formatted stdin in (?:\d+ms|\d+\.\d{3}s)\.\s*$")

    def test_userver_golden_input_parses_without_errors(self) -> None:
        with copied_fixtures(USERVER_INPUT_FIXTURE) as fixtures:
            result = native_format("--style", str(USERVER_FORMAT_CONFIG), str(fixtures[USERVER_INPUT_FIXTURE]))

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertNotIn("parse failed", result.stderr)
        self.assert_no_unsupported_placement_warnings(result)

    def test_ifdef_stdin_formats_to_expected_output(self) -> None:
        result = native_format(
            "--stdin",
            "--style",
            str(USERVER_FORMAT_CONFIG),
            cwd=TEST_ROOT,
            input_text=read_fixture(IFDEF_INPUT_FIXTURE),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(read_fixture(IFDEF_OUTPUT_FIXTURE), result.stdout)
        self.assert_no_unsupported_placement_warnings(result)
        self.assertRegex(result.stderr, r"Formatted stdin in (?:\d+ms|\d+\.\d{3}s)\.\s*$")

    def test_ifdef_golden_input_parses_without_errors(self) -> None:
        with copied_fixtures(IFDEF_INPUT_FIXTURE) as fixtures:
            result = native_format("--style", str(USERVER_FORMAT_CONFIG), str(fixtures[IFDEF_INPUT_FIXTURE]))

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertNotIn("parse failed", result.stderr)
        self.assert_no_unsupported_placement_warnings(result)

    def test_unsupported_stdin_formats_to_current_output(self) -> None:
        result = native_format(
            "--stdin",
            "--style",
            str(USERVER_FORMAT_CONFIG),
            cwd=TEST_ROOT,
            input_text=read_fixture(UNSUPPORTED_INPUT_FIXTURE),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(read_fixture(UNSUPPORTED_OUTPUT_FIXTURE), result.stdout)
        self.assertRegex(
            result.stderr,
            re.escape(read_fixture(UNSUPPORTED_WARNINGS_FIXTURE)) +
            r"Formatted stdin in (?:\d+ms|\d+\.\d{3}s)\.\s*$",
        )

    def test_error_stdin_reports_expected_parse_errors(self) -> None:
        result = native_format(
            "--stdin",
            "--style",
            str(USERVER_FORMAT_CONFIG),
            cwd=TEST_ROOT,
            input_text=read_fixture(ERROR_INPUT_FIXTURE),
        )

        self.assertEqual(1, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual("", result.stdout)
        self.assertEqual(read_fixture(ERROR_OUTPUT_FIXTURE), result.stderr)

    def test_missing_include_categories_preserves_opening_include_blocks(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_preserve_includes_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            config = root / ".cpp-format"
            config.write_text("---\nColumnLimit: 120\nIndentWidth: 4\nTabWidth: 4\n", encoding="utf-8")
            cases = [
                (
                    "#pragma once\n\n"
                    "#include <zeta>\n"
                    "#include <alpha>\n\n"
                    "#include \"b.h\"\n"
                    "#include \"a.h\"\n\n"
                    "int value;\n"
                ),
                (
                    "#ifndef PRESERVE_FIXTURE_HPP\n"
                    "#define PRESERVE_FIXTURE_HPP\n\n"
                    "#include <zeta>\n"
                    "#include <alpha>\n\n"
                    "#include \"b.h\"\n"
                    "#include \"a.h\"\n\n"
                    "int value;\n\n"
                    "#endif\n"
                ),
            ]
            for text in cases:
                with self.subTest(text=text.splitlines()[0]):
                    result = native_format("--stdin", "--style", str(config), input_text=text)

                    self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
                    self.assertEqual(text, result.stdout)

    def test_include_categories_sort_opening_include_blocks(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_sort_includes_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            config = root / ".cpp-format"
            config.write_text(
                "---\n"
                "ColumnLimit: 120\n"
                "IndentWidth: 4\n"
                "TabWidth: 4\n"
                "IncludeCategories:\n"
                "  - Regex: '^<.*>$'\n"
                "    Priority: 1\n"
                "  - Regex: '^\".*\"$'\n"
                "    Priority: 2\n",
                encoding="utf-8",
            )
            cases = [
                (
                    "#pragma once",
                    "#pragma once\n\n"
                    "#include \"b.h\"\n"
                    "#include <zeta>\n\n"
                    "#include \"a.h\"\n"
                    "#include \"A.h\"\n"
                    "#include <Zeta>\n"
                    "#include <alpha>\n\n"
                    "int value;\n",
                    "#pragma once\n\n"
                    "#include <Zeta>\n"
                    "#include <alpha>\n"
                    "#include <zeta>\n\n"
                    "#include \"A.h\"\n"
                    "#include \"a.h\"\n"
                    "#include \"b.h\"\n\n"
                    "int value;\n",
                ),
                (
                    "#ifndef",
                    "#ifndef SORT_FIXTURE_HPP\n"
                    "#define SORT_FIXTURE_HPP\n\n"
                    "#include \"b.h\"\n"
                    "#include <zeta>\n\n"
                    "#include \"a.h\"\n"
                    "#include \"A.h\"\n"
                    "#include <Zeta>\n"
                    "#include <alpha>\n\n"
                    "int value;\n\n"
                    "#endif\n",
                    "#ifndef SORT_FIXTURE_HPP\n"
                    "#define SORT_FIXTURE_HPP\n\n"
                    "#include <Zeta>\n"
                    "#include <alpha>\n"
                    "#include <zeta>\n\n"
                    "#include \"A.h\"\n"
                    "#include \"a.h\"\n"
                    "#include \"b.h\"\n\n"
                    "int value;\n\n"
                    "#endif\n",
                ),
            ]
            for name, input_text, expected in cases:
                with self.subTest(name=name):
                    result = native_format("--stdin", "--style", str(config), input_text=input_text)

                    self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
                    self.assertEqual(expected, result.stdout)

    def test_main_include_detection_matches_source_filename(self) -> None:
        TEST_TEMP_ROOT.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="format_main_include_", dir=TEST_TEMP_ROOT) as temp_dir:
            root = Path(temp_dir)
            write_empty_ignore(root)
            config = root / ".cpp-format"
            config_text = (
                "IncludeCategories:\n"
                "  - Regex: '^\"first.hpp\"$'\n"
                "    Priority: -1\n"
                "  - Regex: '^<.*>$'\n"
                "    Priority: 1\n"
                "  - Regex: '^\".*\"$'\n"
                "    Priority: 2\n"
            )

            def check(filename: str, text: str, expected: str, extra_config: str = "") -> None:
                config.write_text(config_text + extra_config, encoding="utf-8")
                source = root / filename
                source.write_text(text, encoding="utf-8")
                result = native_format(str(source), cwd=root)
                self.assertEqual(0, result.returncode, msg=result.stderr)
                self.assertEqual(expected, result.stdout)
                source.write_text(result.stdout, encoding="utf-8")
                second = native_format("-n", str(source), cwd=root)
                self.assertEqual(0, second.returncode, msg=f"{second.stdout}\n{second.stderr}")

            service_config = "IncludeIsMainRegex: '([-_](test|unittest))?$'\n"
            golden_input = read_fixture(MAIN_INCLUDE_INPUT_FIXTURE)
            golden_output = read_fixture(MAIN_INCLUDE_OUTPUT_FIXTURE)
            no_main_output = (
                "#include <algorithm>\n"
                "#include <vector>\n\n"
                '#include "../widget.hpp"\n'
                '#include "helper.hpp"\n'
                '#include "widget_extra.hpp"\n'
            )
            for filename in (
                "widget.c", "widget.cc", "widget.cpp", "widget.c++", "widget.cxx", "widget.m", "widget.mm",
                "widget_test.cpp", "widget_unittest.cc", "widget-test.cpp", "widget-unittest.cxx",
                "Widget_TEST.cpp", "widget.cu.cc", "widget_test.cu.cc",
            ):
                with self.subTest(filename=filename):
                    check(filename, golden_input, golden_output, service_config)
            for filename in (
                "widget.h", "widget.hh", "widget.hpp", "widget.hxx", "widget.h++",
                "widget.ipp", "widget.inl", "widget.tpp", "widget.CPP", "other.cpp", "widget_test_extra.cpp",
            ):
                with self.subTest(filename=filename):
                    check(filename, golden_input, no_main_output, service_config)

            for filename, target, suffix, is_main in (
                ("widgetTest.cpp", '"widget.hpp"', None, True),
                ("widget.cpp", '"widgetTest.hpp"', None, False),
                ("widget_test.cpp", '"widget.hpp"', "$", False),
                ("widget_more.cpp", '"widget.hpp"', "", True),
                ("widget_spec_more.cpp", '"widget.hpp"', "_spec", True),
                ("widget_spec.cpp", '"widget.hpp"', "(_spec)?$", True),
                ("widget_spec_more.cpp", '"widget.hpp"', "(_spec)?$", False),
                ("widget.cpp", '"Widget.hpp"', "$", True),
                ("widget.v2.cpp", '"widget.v2.hpp"', "$", True),
                ("widget.cpp", '"widget.v2.hpp"', "$", False),
                ("widgetXv2.cpp", '"widget.v2.hpp"', "", False),
                ("widget.cpp", '"widget.hpp"', "[", False),
                ("widget.cpp", "<widget.hpp>", "$", False),
            ):
                with self.subTest(filename=filename, target=target, suffix=suffix):
                    directive = "#include " + target + "\n"
                    text = "#include <vector>\n" + directive
                    expected = (
                        directive + "\n#include <vector>\n" if is_main else
                        "#include <vector>\n" + ("\n" if target.startswith('"') else "") + directive
                    )
                    extra = "" if suffix is None else f"IncludeIsMainRegex: '{suffix}'\n"
                    check(filename, text, expected, extra)

            with self.subTest(name="angle main include"):
                check(
                    "widget.cpp", "#include <vector>\n#include <widget.hpp>\n",
                    "#include <widget.hpp>\n\n#include <vector>\n", "MainIncludeChar: AngleBracket\n",
                )
            with self.subTest(name="first match and negative category"):
                check(
                    "widget.cpp",
                    '#include "other/widget.hpp"\n#include <vector>\n#include "widget.hpp"\n#include "first.hpp"\n',
                    '#include "first.hpp"\n\n#include "other/widget.hpp"\n\n#include <vector>\n\n#include "widget.hpp"\n',
                )
            with self.subTest(name="only first include run"):
                check(
                    "widget.cpp",
                    '#include <vector>\n\n// Later includes.\n#include "widget.hpp"\n#include <algorithm>\n',
                    '#include <vector>\n\n// Later includes.\n#include <algorithm>\n\n#include "widget.hpp"\n',
                )

    def test_file_argument_formats_to_stdout(self) -> None:
        with copied_fixtures(OUTPUT_FIXTURE) as fixtures:
            result = native_format(str(fixtures[OUTPUT_FIXTURE]))

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(read_fixture(OUTPUT_FIXTURE), result.stdout)
        self.assertRegex(
            result.stderr,
            rf"Formatted 1 file, {fixture_loc(OUTPUT_FIXTURE)} LOC in (?:\d+ms|\d+\.\d{{3}}s)\.\s*$",
        )

    def test_dry_run_accepts_idempotent_file_and_rejects_unformatted_file(self) -> None:
        with copied_fixtures(INPUT_FIXTURE, OUTPUT_FIXTURE) as fixtures:
            ok_result = native_format("--dry-run", str(fixtures[OUTPUT_FIXTURE]))

            self.assertEqual(0, ok_result.returncode, msg=f"stdout:\n{ok_result.stdout}\n\nstderr:\n{ok_result.stderr}")
            self.assertRegex(
                ok_result.stdout,
                rf"Checked 1 file, {fixture_loc(OUTPUT_FIXTURE)} LOC in (?:\d+ms|\d+\.\d{{3}}s)\.\s*$",
            )

            bad_result = native_format("--dry-run", str(fixtures[INPUT_FIXTURE]))

            self.assertEqual(1, bad_result.returncode, msg=f"stdout:\n{bad_result.stdout}\n\nstderr:\n{bad_result.stderr}")
            self.assertIn("Formatting is required for 1 file", bad_result.stdout)
            self.assertRegex(
                bad_result.stdout,
                rf"Checked 1 file, {fixture_loc(INPUT_FIXTURE)} LOC in (?:\d+ms|\d+\.\d{{3}}s)\.\s*$",
            )

    def test_diff_outputs_unified_diff_and_uses_dry_run_exit_codes(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_diff_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            copy_default_config(root)
            write_empty_ignore(root)
            source = root / "value.cpp"
            source.write_text("int value=1;\n", encoding="utf-8")

            changed = native_format("--diff", str(source), cwd=root)

            self.assertEqual(1, changed.returncode, msg=f"stdout:\n{changed.stdout}\n\nstderr:\n{changed.stderr}")
            self.assertEqual(
                "--- value.cpp\n"
                "+++ value.cpp\n"
                "@@ -1 +1 @@\n"
                "-int value=1;\n"
                "+int value = 1;\n",
                changed.stdout,
            )
            self.assertIn("Formatting is required for 1 file", changed.stderr)
            self.assertEqual("int value=1;\n", source.read_text(encoding="utf-8"))

            source.write_text("int value = 1;\n", encoding="utf-8")
            clean = native_format("--diff", str(source), cwd=root)

            self.assertEqual(0, clean.returncode, msg=f"stdout:\n{clean.stdout}\n\nstderr:\n{clean.stderr}")
            self.assertEqual("", clean.stdout)
            self.assertRegex(clean.stderr, r"Checked 1 file, 1 LOC in (?:\d+ms|\d+\.\d{3}s)\.\s*$")

    def test_diff_supports_stdin_and_marks_a_missing_final_newline(self) -> None:
        result = native_format("--diff", "--stdin", input_text="int value=1;")

        self.assertEqual(1, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "--- <stdin>\n"
            "+++ <stdin>\n"
            "@@ -1 +1 @@\n"
            "-int value=1;\n"
            "\\ No newline at end of file\n"
            "+int value = 1;\n",
            result.stdout,
        )
        self.assertIn("Formatting is required for stdin", result.stderr)

    def test_diff_separates_distant_changes_into_clear_hunks(self) -> None:
        result = native_format(
            "--diff",
            "--stdin",
            input_text=(
                "void Test() {\n"
                "    int first=1;\n"
                "    Keep1();\n"
                "    Keep2();\n"
                "    Keep3();\n"
                "    Keep4();\n"
                "    Keep5();\n"
                "    Keep6();\n"
                "    Keep7();\n"
                "    int second=2;\n"
                "}\n"
            ),
        )

        self.assertEqual(1, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(2, result.stdout.count("@@ -"))
        self.assertIn("-    int first=1;\n+    int first = 1;\n", result.stdout)
        self.assertIn("-    int second=2;\n+    int second = 2;\n", result.stdout)

    def test_files_option_reads_newline_file_list(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_files_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            copy_default_config(root)
            write_empty_ignore(root)
            source = root / OUTPUT_FIXTURE.name
            shutil.copyfile(TEST_ROOT / OUTPUT_FIXTURE, source)
            file_list = root / "files.txt"
            file_list.write_text(f"{source}\n\n", encoding="utf-8")

            result = native_format("--dry-run", "--concurrency", "1", "--files", str(file_list))

            self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
            self.assertRegex(
                result.stdout,
                rf"Checked 1 file, {fixture_loc(OUTPUT_FIXTURE)} LOC in (?:\d+ms|\d+\.\d{{3}}s)\.\s*$",
            )

    def test_recursive_option_discovers_cpp_and_headers(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_recursive_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            nested = root / "src" / "nested"
            nested.mkdir(parents=True)
            shutil.copyfile(STRICTFMT_ROOT / ".cpp-format", root / ".cpp-format")
            (root / ".cpp-format-ignore").write_text("ignored\n", encoding="utf-8")
            suffixes = [
                ".c",
                ".cc",
                ".cpp",
                ".cxx",
                ".c++",
                ".h",
                ".hh",
                ".hpp",
                ".hxx",
                ".h++",
                ".ipp",
                ".inl",
                ".tpp",
            ]
            for index, suffix in enumerate(suffixes):
                (nested / f"sample_{index}{suffix}").write_text("#pragma once\n", encoding="utf-8")
            (nested / "sample.txt").write_text("int ignored(){return 1;}\n", encoding="utf-8")
            ignored = root / "ignored"
            ignored.mkdir()
            (ignored / "ignored.cpp").write_text("int ignored(){return 1;}\n", encoding="utf-8")

            result = native_format("--dry-run", "-r", ".", cwd=root)

            self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
            self.assertRegex(result.stdout, r"Checked 13 files, 13 LOC in (?:\d+ms|\d+\.\d{3}s)\.\s*$")

    def test_userver_submodule(self) -> None:
        self.assert_external_project_sources_parse_without_warnings_and_format_idempotently("userver")

    def test_casedash_submodule(self) -> None:
        self.assert_external_project_sources_parse_without_warnings_and_format_idempotently("casedash")

    def test_googletest_submodule(self) -> None:
        self.assert_external_project_sources_parse_without_warnings_and_format_idempotently("googletest")

    def test_pfr_submodule(self) -> None:
        self.assert_external_project_sources_parse_without_warnings_and_format_idempotently("pfr")

    def test_concurrency_one_preserves_file_list_output_order(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_order_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            shutil.copyfile(STRICTFMT_ROOT / ".cpp-format", root / ".cpp-format")
            write_empty_ignore(root)
            first = root / "first.cpp"
            second = root / "second.cpp"
            first.write_text("int first(){return 1;}\n", encoding="utf-8")
            second.write_text("int second(){return 2;}\n", encoding="utf-8")
            file_list = root / "files.txt"
            file_list.write_text(f"{second}\n{first}\n", encoding="utf-8")

            result = native_format("--concurrency", "1", "--files", str(file_list), cwd=root)

            self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
            self.assertEqual(
                "int second() { return 2; }\n"
                "int first() { return 1; }\n",
                result.stdout,
            )
            self.assertRegex(result.stderr, r"Formatted 2 files, 2 LOC in (?:\d+ms|\d+\.\d{3}s)\.\s*$")

    def test_verbose_reports_each_file_start_and_completion(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_verbose_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            shutil.copyfile(STRICTFMT_ROOT / ".cpp-format", root / ".cpp-format")
            write_empty_ignore(root)
            first = root / "first.cpp"
            second = root / "second.cpp"
            first.write_text("int first(){return 1;}\n", encoding="utf-8")
            second.write_text("int second(){return 2;}\n", encoding="utf-8")

            result = native_format("--verbose", "--concurrency", "1", str(first), str(second), cwd=root)

            self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
            escaped_first = re.escape(str(first.resolve()))
            escaped_second = re.escape(str(second.resolve()))
            self.assertRegex(
                result.stderr,
                rf"^\[1/2\] Formatting {escaped_first}\n"
                rf"\[1/2\] Finished {escaped_first} in (?:\d+ms|\d+\.\d{{3}}s)\n"
                rf"\[2/2\] Formatting {escaped_second}\n"
                rf"\[2/2\] Finished {escaped_second} in (?:\d+ms|\d+\.\d{{3}}s)\n"
                rf"Formatted 2 files, 2 LOC in (?:\d+ms|\d+\.\d{{3}}s)\.\s*$",
            )

    def test_in_place_formats_file(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_in_place_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            shutil.copyfile(STRICTFMT_ROOT / ".cpp-format", root / ".cpp-format")
            write_empty_ignore(root)
            source = root / "sample.cpp"
            source.write_text("int main(){return 1;}\n", encoding="utf-8")

            result = native_format("-i", str(source), cwd=root)

            self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
            self.assertEqual("int main() { return 1; }\n", source.read_text(encoding="utf-8").replace("\r\n", "\n"))
            self.assertIn("Formatted 1 file, 1 LOC", result.stdout)

    def test_in_place_preserves_unambiguous_line_endings(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_line_endings_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            shutil.copyfile(STRICTFMT_ROOT / ".cpp-format", root / ".cpp-format")
            write_empty_ignore(root)
            input_lines = [b"int main(){", b"return 1;", b"}"]
            expected_lines = [b"int main() { return 1; }"]
            cases = [
                ("lf", b"\n"),
                ("crlf", b"\r\n"),
                ("cr", b"\r"),
            ]
            for name, line_ending in cases:
                with self.subTest(name=name):
                    source = root / f"sample_{name}.cpp"
                    source.write_bytes(join_lines(input_lines, line_ending))

                    result = native_format_bytes("-i", str(source), cwd=root)

                    self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout!r}\n\nstderr:\n{result.stderr!r}")
                    self.assertEqual(join_lines(expected_lines, line_ending), source.read_bytes())

    def test_in_place_normalizes_mixed_line_endings_to_platform_default(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_mixed_line_endings_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            shutil.copyfile(STRICTFMT_ROOT / ".cpp-format", root / ".cpp-format")
            write_empty_ignore(root)
            source = root / "sample.cpp"
            source.write_bytes(b"int main() {\r\n    return 1;\n}\r\n")

            result = native_format_bytes("-i", str(source), cwd=root)

            self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout!r}\n\nstderr:\n{result.stderr!r}")
            self.assertEqual(
                join_lines([b"int main() { return 1; }"], PLATFORM_LINE_ENDING),
                source.read_bytes(),
            )

    def test_pretty_printer_does_not_hard_code_crlf_line_break_searches(self) -> None:
        pretty_printer = PRETTY_PRINTER_SOURCE.read_text(encoding="utf-8")

        self.assertNotIn('"\\r\\n"', pretty_printer)

    def test_grammar_has_only_reviewed_lexical_terminals(self) -> None:
        result = subprocess.run(
            [sys.executable, str(GRAMMAR_REGENERATOR), "--validate-structure-only"],
            cwd=STRICTFMT_ROOT,
            check=False,
            capture_output=True,
            text=True,
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")

    def test_dump_prints_format_model_for_small_source(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_dump_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            copy_default_config(root)
            source = root / "sample.cpp"
            source.write_text("int main(){return 1;}\n", encoding="utf-8")

            result = native_format("--dump-syntax-tree", str(source), cwd=root)

            self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
            self.assertEqual("", result.stderr)
            self.assertIn("kind: TranslationUnit\n", result.stdout)
            self.assertIn("- kind: FunctionDefinition\n", result.stdout)
            self.assertIn("text: \"main\"", result.stdout)
            self.assertIn("- kind: KeywordReturn\n", result.stdout)

    def test_friend_operator_bodies_remain_structured(self) -> None:
        source = (
            "struct FriendOperators {\n"
            "[[maybe_unused]] friend bool operator==(const char* lhs, FriendOperators) "
            "{ return *lhs == '\\0'; }\n"
            "[[maybe_unused]] friend bool operator!=(const char* lhs, FriendOperators) "
            "{ return *lhs != '\\0'; }\n"
            "};\n"
        )
        dump = native_format("--stdin", "--dump-syntax-tree", input_text=source)

        self.assertEqual(0, dump.returncode, msg=f"stdout:\n{dump.stdout}\n\nstderr:\n{dump.stderr}")
        self.assertEqual(2, dump.stdout.count("- kind: FunctionDefinition\n"))
        self.assertEqual(2, dump.stdout.count("- kind: CompoundStatement\n"))
        self.assertEqual(2, dump.stdout.count("- kind: ReturnStatement\n"))
        self.assertNotIn("RawMacroReplacement", dump.stdout)

        formatted = native_format("--stdin", input_text=source)
        self.assertEqual(0, formatted.returncode, msg=f"stdout:\n{formatted.stdout}\n\nstderr:\n{formatted.stderr}")
        self.assertEqual(
            "struct FriendOperators {\n"
            "    [[maybe_unused]] friend bool operator==(const char* lhs, FriendOperators) { return *lhs == '\\0'; }\n"
            "    [[maybe_unused]] friend bool operator!=(const char* lhs, FriendOperators) { return *lhs != '\\0'; }\n"
            "};\n",
            formatted.stdout,
        )

    def test_dump_reads_stdin_source(self) -> None:
        result = native_format("--stdin", "--dump-syntax-tree", input_text="int value(){return 2;}\n")

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual("", result.stderr)
        self.assertIn("kind: TranslationUnit\n", result.stdout)
        self.assertIn("text: \"value\"", result.stdout)
        self.assertIn("text: \"2\"", result.stdout)

        reversed_result = native_format("--dump-syntax-tree", "--stdin", input_text="int other(){return 3;}\n")

        self.assertEqual(
            0,
            reversed_result.returncode,
            msg=f"stdout:\n{reversed_result.stdout}\n\nstderr:\n{reversed_result.stderr}",
        )
        self.assertIn("text: \"other\"", reversed_result.stdout)

    def test_callable_template_ambiguity_is_resolved_by_grammar(self) -> None:
        cases = (
            ("(a+b)<Outer<Inner<c>>>(d)", 3, 1),
            ("(a+b)<c>(d)<e>(f)", 2, 1),
            ("((a+b)<c)>(d)", 0, 3),
        )
        for expression, template_lists, binary_expressions in cases:
            with self.subTest(expression=expression):
                result = native_format(
                    "--stdin",
                    "--dump-syntax-tree",
                    input_text=f"auto f(){{return {expression};}}\n",
                )

                self.assertEqual(
                    0,
                    result.returncode,
                    msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}",
                )
                self.assertEqual(template_lists, result.stdout.count("- kind: TemplateArgumentList\n"))
                self.assertEqual(binary_expressions, result.stdout.count("- kind: BinaryExpression\n"))

    def test_declaration_template_ambiguity_is_resolved_by_grammar(self) -> None:
        source = "void f(){box<a,b> c;(box<a),(b>c);}\n"
        result = native_format("--stdin", "--dump-syntax-tree", input_text=source)

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(1, result.stdout.count("- kind: Declaration\n"))
        self.assertEqual(1, result.stdout.count("- kind: TemplateArgumentList\n"))
        self.assertEqual(1, result.stdout.count("- kind: CommaExpression\n"))
        self.assertEqual(2, result.stdout.count("- kind: BinaryExpression\n"))

    def test_break_tree_dump_shows_costs_and_final_lambda_discount(self) -> None:
        result = native_format(
            "--stdin",
            "--dump-break-tree",
            input_text=(
                "Vector<int> func(int a);\n"
                "void f(){make<int>(value);"
                "visit(items,[](const auto& item){return Serialize(item);});}\n"
            ),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual("", result.stderr)
        self.assertIn('tokens: "Vector<int> func(int a);"\n', result.stdout)
        self.assertIn("kind: FunctionSignature\n", result.stdout)
        self.assertIn("raw-depth: 3\n", result.stdout)
        self.assertIn("surcharge: 2\n", result.stdout)
        self.assertIn("discount: 0\n", result.stdout)
        self.assertIn("effective-cost: 5\n", result.stdout)
        self.assertIn('tokens: "make<int>(value);"\n', result.stdout)
        self.assertIn("body-header-is-lambda: true\n", result.stdout)
        self.assertRegex(result.stdout, r"raw-depth: 7\n\s+surcharge: 0\n\s+discount: 7\n\s+effective-cost: 0\n")
        self.assertRegex(result.stdout, r"raw-depth: 12\n\s+surcharge: 0\n\s+discount: 7\n\s+effective-cost: 5\n")

    def test_dump_prints_model_for_parse_error_tree(self) -> None:
        result = native_format(
            "--stdin",
            "--dump-syntax-tree",
            "--style",
            str(USERVER_FORMAT_CONFIG),
            input_text=read_fixture(ERROR_INPUT_FIXTURE),
        )

        self.assertEqual(1, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertIn("parse failed", result.stderr)
        self.assertIn("kind: TranslationUnit\n", result.stdout)
        self.assertIn("- kind: Error\n", result.stdout)
        self.assertIn("- kind: Missing\n", result.stdout)

    def test_declarator_reference_tokens_include_managed_cpp(self) -> None:
        result = native_format(
            "--stdin",
            input_text="void f(Object ^ handle,Object % tracking,int && moved,int * pointer){}\n",
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "void f(Object^ handle, Object% tracking, int&& moved, int* pointer) {}\n",
            result.stdout,
        )

    def test_preprocessor_directive_whitespace_is_canonicalized(self) -> None:
        result = native_format(
            "--stdin",
            input_text=(
                "#   if FOO\n"
                "int value;\n"
                "#   else\n"
                "int other;\n"
                "#   endif\n"
            ),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "#if FOO\n"
            "int value;\n"
            "#else\n"
            "int other;\n"
            "#endif\n",
            result.stdout,
        )

    def test_dump_uses_preprocessor_directive_tokens(self) -> None:
        result = native_format(
            "--stdin",
            "--dump-syntax-tree",
            input_text=(
                "#   if FOO\n"
                "int value;\n"
                "#   else\n"
                "int other;\n"
                "#   endif\n"
            ),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual("", result.stderr)
        self.assertIn("- kind: PreprocessorDirectiveIf\n", result.stdout)
        self.assertIn("- kind: PreprocessorDirectiveElse\n", result.stdout)
        self.assertIn("- kind: PreprocessorDirectiveEndif\n", result.stdout)

    def test_trailing_comma_normalization_follows_brace_list_layout(self) -> None:
        result = native_format(
            "--stdin",
            input_text=(
                "enum E { A, B };\n"
                "enum F { C, D, };\n"
                "int values[] = {1, 2,};\n"
                "void f(){ Use({1, 2,}); }\n"
                "auto long_values = Values{firstValueWithAnExtremelyLongNameForTrailingCommaNormalization, "
                "secondValueWithAnExtremelyLongNameForTrailingCommaNormalization};\n"
            ),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "enum E {\n"
            "    A,\n"
            "    B,\n"
            "};\n"
            "\n"
            "enum F {\n"
            "    C,\n"
            "    D,\n"
            "};\n"
            "\n"
            "int values[] = {1, 2};\n"
            "\n"
            "void f() { Use({1, 2}); }\n"
            "\n"
            "auto long_values = Values{\n"
            "    firstValueWithAnExtremelyLongNameForTrailingCommaNormalization,\n"
            "    secondValueWithAnExtremelyLongNameForTrailingCommaNormalization,\n"
            "};\n",
            result.stdout,
        )

    def test_enum_macro_call_final_item_keeps_trailing_comma(self) -> None:
        result = native_format(
            "--stdin",
            input_text=(
                "#define DECLARE_ENUM(ItemsMacro) \\\n"
                "enum G { ItemsMacro(EMIT) };\n"
            ),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "#define DECLARE_ENUM(ItemsMacro) \\\n"
            "    enum G { \\\n"
            "        ItemsMacro(EMIT), \\\n"
            "    };\n",
            result.stdout,
        )

    def test_atomic_preprocessor_directives_do_not_create_groups(self) -> None:
        result = native_format(
            "--stdin",
            input_text=(
                "#define VALUE 1\n"
                "int value;\n"
                "#undef VALUE\n"
                "#line 200\n"
                "int remapped;\n"
            ),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "#define VALUE 1\n"
            "int value;\n"
            "#undef VALUE\n"
            "#line 200\n"
            "int remapped;\n",
            result.stdout,
        )

    def test_atomic_preprocessor_directives_preserve_source_grouping(self) -> None:
        result = native_format(
            "--stdin",
            input_text=(
                "int before; // attached to previous item\n"
                "#pragma first\n"
                "// attached to the next pragma\n"
                "#pragma second\n"
                "\n"
                "// attached to the definition\n"
                "#define VALUE 1\n"
                "#undef VALUE\n"
                "#undef OTHER\n"
                "#define OTHER 2\n"
                "\n"
                "int after;\n"
            ),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "int before;  // attached to previous item\n"
            "#pragma first\n"
            "// attached to the next pragma\n"
            "#pragma second\n"
            "\n"
            "// attached to the definition\n"
            "#define VALUE 1\n"
            "#undef VALUE\n"
            "#undef OTHER\n"
            "#define OTHER 2\n"
            "\n"
            "int after;\n",
            result.stdout,
        )

    def test_atomic_preprocessor_directives_stay_attached_to_context(self) -> None:
        result = native_format(
            "--stdin",
            input_text=(
                "void f(){\n"
                "#pragma omp parallel for\n"
                "for(int index=0;index<4;++index){use(index);}\n"
                "#define VALUE 1\n"
                "int value;\n"
                "#undef VALUE\n"
                "}\n"
                "#line 200\n"
                "int remapped;\n"
                "#if FLAG\n"
                "#pragma second\n"
                "int other;\n"
                "#undef OTHER\n"
                "#endif\n"
                "#undef AFTER_CONDITIONAL\n"
            ),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "void f() {\n"
            "#pragma omp parallel for\n"
            "    for (int index = 0; index < 4; ++index) {\n"
            "        use(index);\n"
            "    }\n"
            "#define VALUE 1\n"
            "    int value;\n"
            "#undef VALUE\n"
            "}\n"
            "\n"
            "#line 200\n"
            "int remapped;\n"
            "#if FLAG\n"
            "#pragma second\n"
            "int other;\n"
            "#undef OTHER\n"
            "#endif\n"
            "#undef AFTER_CONDITIONAL\n",
            result.stdout,
        )

    def test_win32_boolean_macros_preserve_spelling(self) -> None:
        result = native_format(
            "--stdin",
            input_text=(
                "int false_value=FALSE;\n"
                "int true_value=TRUE;\n"
                "bool standard_false=false;\n"
                "bool standard_true=true;\n"
            ),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "int false_value = FALSE;\n"
            "int true_value = TRUE;\n"
            "bool standard_false = false;\n"
            "bool standard_true = true;\n",
            result.stdout,
        )

    def test_macro_decltype_argument_formats_structurally(self) -> None:
        result = native_format(
            "--stdin",
            input_text=(
                "#define STRICTFMT_LOAD_OPTIONAL(function, name) \\\n"
                "function=reinterpret_cast<decltype(function)>(GetProcAddress(module_,name))\n"
            ),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "#define STRICTFMT_LOAD_OPTIONAL(function, name) \\\n"
            "    function = reinterpret_cast<decltype(function)>(GetProcAddress(module_, name))\n",
            result.stdout,
        )

    def test_structured_macro_definitions_format_replacements(self) -> None:
        source = (
            "#define EMPTY_OBJECT\n"
            "#define STRUCTURED_OBJECT Foo(1,2)\n"
            "#define STRUCTURED_FUNCTION(first,second) (first+second)\n"
        )
        result = native_format("--stdin", input_text=source)

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "#define EMPTY_OBJECT\n"
            "#define STRUCTURED_OBJECT Foo(1, 2)\n"
            "#define STRUCTURED_FUNCTION(first, second) (first + second)\n",
            result.stdout,
        )

        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="format_structured_macros_", dir=build_dir) as temp_dir:
            source_path = Path(temp_dir) / "sample.cpp"
            source_path.write_text(source, encoding="utf-8")

            dump = native_format("--dump-syntax-tree", str(source_path))

            self.assertEqual(0, dump.returncode, msg=f"stdout:\n{dump.stdout}\n\nstderr:\n{dump.stderr}")
            self.assertIn("- kind: MacroDefinition\n", dump.stdout)
            self.assertIn("- kind: MacroReplacementList\n", dump.stdout)
            self.assertNotIn("PreprocDef", dump.stdout)
            self.assertNotIn("PreprocFunctionDef", dump.stdout)
            self.assertNotIn("RawMacroReplacement", dump.stdout)

    def test_structured_macro_replacement_call_sequence_splits_by_unit(self) -> None:
        source = (
            '#define ONE(X) X(Alpha,"alpha")\n'
            '#define MANY(X) X(Alpha,"alpha") X(Beta,"beta") X(Gamma,"gamma")\n'
            "#define DIFFERENT(Y) Produce(Alpha) Consume(Beta)\n"
        )
        expected = (
            '#define ONE(X) X(Alpha, "alpha")\n'
            "#define MANY(X) \\\n"
            '    X(Alpha, "alpha") \\\n'
            '    X(Beta, "beta") \\\n'
            '    X(Gamma, "gamma")\n'
            "#define DIFFERENT(Y) \\\n"
            "    Produce(Alpha) \\\n"
            "    Consume(Beta)\n"
        )

        formatted = native_format("--stdin", input_text=source)

        self.assertEqual(0, formatted.returncode, msg=f"stdout:\n{formatted.stdout}\n\nstderr:\n{formatted.stderr}")
        self.assertEqual(expected, formatted.stdout)
        idempotent = native_format("--dry-run", "--stdin", input_text=formatted.stdout)
        self.assertEqual(
            0,
            idempotent.returncode,
            msg=f"stdout:\n{idempotent.stdout}\n\nstderr:\n{idempotent.stderr}",
        )

    def test_structured_macro_definition_with_templated_struct_body_reparses(self) -> None:
        expected = (
            "#define DECLARE_TRAITS(Type) \\\n"
            "    template <> \\\n"
            "    struct Traits<Type> { \\\n"
            "        static constexpr auto value = Type{}; \\\n"
            "    }\n"
        )
        sources = (
            (
                "#define DECLARE_TRAITS(Type) \\\n"
                "    template <> struct Traits<Type>{static constexpr auto value = Type{}; }\n"
            ),
            (
                "#define DECLARE_TRAITS(Type) \\\n"
                "    template <> \\\n"
                "    struct Traits<Type>{ \\\n"
                "        static constexpr auto value = Type{}; }\n"
            ),
        )

        for source in sources:
            with self.subTest(source=source):
                result = native_format("--stdin", input_text=source)

                self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
                self.assertEqual(expected, result.stdout)

    def test_structured_macro_non_fitting_definition_splits_after_header(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="format_macro_value_solver_", dir=build_dir) as temp_dir:
            config = Path(temp_dir) / ".cpp-format"
            config.write_text(
                "---\n"
                "ColumnLimit: 22\n"
                "IndentWidth: 4\n"
                "TabWidth: 4\n",
                encoding="utf-8",
            )
            source = "#define VALUE Build(first,second,third)\n#define D(v) void f(v)\n"
            expected = (
                "#define VALUE \\\n"
                "    Build( \\\n"
                "        first, \\\n"
                "        second, \\\n"
                "        third \\\n"
                "    )\n"
                "#define D(v) void f(v)\n"
            )

            formatted = native_format("--stdin", "--style", str(config), input_text=source)

            self.assertEqual(0, formatted.returncode, msg=f"stdout:\n{formatted.stdout}\n\nstderr:\n{formatted.stderr}")
            self.assertEqual(expected, formatted.stdout)
            idempotent = native_format(
                "--dry-run", "--stdin", "--style", str(config), input_text=formatted.stdout
            )
            self.assertEqual(
                0,
                idempotent.returncode,
                msg=f"stdout:\n{idempotent.stdout}\n\nstderr:\n{idempotent.stderr}",
            )

            config.write_text(
                "---\n"
                "ColumnLimit: 20\n"
                "IndentWidth: 4\n"
                "TabWidth: 4\n",
                encoding="utf-8",
            )
            suffix_constrained = native_format(
                "--stdin",
                "--style",
                str(config),
                input_text=(
                    "#define VALUE Build(first,second,third)\n"
                    "#define EMPTY(first,second,third)\n"
                ),
            )
            self.assertEqual(
                0,
                suffix_constrained.returncode,
                msg=f"stdout:\n{suffix_constrained.stdout}\n\nstderr:\n{suffix_constrained.stderr}",
            )
            self.assertEqual(
                "#define VALUE \\\n"
                "    Build( \\\n"
                "        first, \\\n"
                "        second, \\\n"
                "        third \\\n"
                "    )\n"
                "#define EMPTY( \\\n"
                "    first, \\\n"
                "    second, \\\n"
                "    third \\\n"
                ")\n",
                suffix_constrained.stdout,
            )

    def test_raw_macro_definitions_format_raw_replacements(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="format_raw_macros_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            config = root / ".cpp-format"
            config.write_text(
                "---\n"
                "ColumnLimit: 120\n"
                "IndentWidth: 4\n"
                "TabWidth: 4\n"
                "MacroCategories:\n"
                "  RawMacroDefinitions:\n"
                "    - RAW_OBJECT\n"
                "    - RAW_FUNCTION\n"
                "    - RAW_BLOCK\n"
                "    - RAW_ALREADY_INDENTED\n",
                encoding="utf-8",
            )
            source = root / "sample.cpp"
            source.write_text(
                "#define RAW_OBJECT value ## suffix\n"
                "#define RAW_FUNCTION(first,second) first ## second\n"
                "#define RAW_BLOCK(first,second) \\\n"
                "first ## second; \\\n"
                "    second ## first\n"
                "#define RAW_ALREADY_INDENTED(first,second) \\\n"
                "        first ## second\n",
                encoding="utf-8",
            )

            result = native_format("--stdin", "--style", str(config), input_text=source.read_text(encoding="utf-8"))

            self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
            self.assertEqual(
                "#define RAW_OBJECT value ## suffix\n"
                "#define RAW_FUNCTION(first, second) first ## second\n"
                "#define RAW_BLOCK(first, second) \\\n"
                "    first ## second; \\\n"
                "        second ## first\n"
                "#define RAW_ALREADY_INDENTED(first, second) \\\n"
                "    first ## second\n",
                result.stdout,
            )

            dump = native_format("--dump-syntax-tree", str(source), "--style", str(config))

            self.assertEqual(0, dump.returncode, msg=f"stdout:\n{dump.stdout}\n\nstderr:\n{dump.stderr}")
            self.assertIn("- kind: MacroDefinition\n", dump.stdout)
            self.assertIn("- kind: RawMacroReplacement\n", dump.stdout)
            self.assertNotIn("MacroReplacementList", dump.stdout)
            self.assertNotIn("PreprocDef", dump.stdout)
            self.assertNotIn("PreprocFunctionDef", dump.stdout)

    def test_token_paste_macro_is_structured_unless_configured_raw(self) -> None:
        source = "#define HASH_JOIN(first,second) first ## second\n"
        structured = native_format("--stdin", input_text=source)

        self.assertEqual(
            0,
            structured.returncode,
            msg=f"stdout:\n{structured.stdout}\n\nstderr:\n{structured.stderr}",
        )
        self.assertEqual("#define HASH_JOIN(first, second) first##second\n", structured.stdout)

        structured_dump = native_format("--stdin", "--dump-syntax-tree", input_text=source)

        self.assertEqual(
            0,
            structured_dump.returncode,
            msg=f"stdout:\n{structured_dump.stdout}\n\nstderr:\n{structured_dump.stderr}",
        )
        self.assertIn("- kind: MacroReplacementList\n", structured_dump.stdout)
        self.assertNotIn("RawMacroReplacement", structured_dump.stdout)

        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="format_hash_join_macro_", dir=build_dir) as temp_dir:
            config = Path(temp_dir) / ".cpp-format"
            config.write_text(
                "---\n"
                "ColumnLimit: 120\n"
                "IndentWidth: 4\n"
                "TabWidth: 4\n"
                "MacroCategories:\n"
                "  RawMacroDefinitions:\n"
                "    - HASH_JOIN\n",
                encoding="utf-8",
            )

            formatted = native_format("--stdin", "--style", str(config), input_text=source)

            self.assertEqual(0, formatted.returncode, msg=f"stdout:\n{formatted.stdout}\n\nstderr:\n{formatted.stderr}")
            self.assertEqual("#define HASH_JOIN(first, second) first ## second\n", formatted.stdout)

    def test_raw_macro_definition_category_is_definition_side_only(self) -> None:
        source = (
            "#define RAW_ONLY(name) name ## _impl\n"
            "RAW_ONLY(Generated, Case) {\n"
            "Run();\n"
            "}\n"
        )
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="format_raw_macro_use_side_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            raw_only_config = root / "raw_only.cpp-format"
            raw_only_config.write_text(
                "---\n"
                "ColumnLimit: 120\n"
                "IndentWidth: 4\n"
                "TabWidth: 4\n"
                "MacroCategories:\n"
                "  RawMacroDefinitions:\n"
                "    - RAW_ONLY\n",
                encoding="utf-8",
            )
            failed = native_format("--stdin", "--style", str(raw_only_config), input_text=source)

            self.assertEqual(1, failed.returncode, msg=f"stdout:\n{failed.stdout}\n\nstderr:\n{failed.stderr}")
            self.assertEqual("", failed.stdout)
            self.assertIn("parse failed", failed.stderr)

            explicit_use_config = root / "raw_and_call.cpp-format"
            explicit_use_config.write_text(
                "---\n"
                "ColumnLimit: 120\n"
                "IndentWidth: 4\n"
                "TabWidth: 4\n"
                "MacroCategories:\n"
                "  RawMacroDefinitions:\n"
                "    - RAW_ONLY\n"
                "  CallSyntaxMacros:\n"
                "    - RAW_ONLY\n",
                encoding="utf-8",
            )
            formatted = native_format("--stdin", "--style", str(explicit_use_config), input_text=source)

            self.assertEqual(0, formatted.returncode, msg=f"stdout:\n{formatted.stdout}\n\nstderr:\n{formatted.stderr}")

    def test_type_specifier_macro_classifies_after_horizontal_whitespace(self) -> None:
        source = (
            "template<typename T,typename U>struct TypeMacroFixture{\n"
            "typedef REMOVE_CV_REF(T) RawT;\n"
            "typedef typename REMOVE_CV_REF(T,U) BoundT;\n"
            "using Address=const REMOVE_CV_REF(T)*;\n"
            "};\n"
        )
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="format_type_macro_", dir=build_dir) as temp_dir:
            config = Path(temp_dir) / ".cpp-format"
            config.write_text(
                "---\n"
                "ColumnLimit: 120\n"
                "IndentWidth: 4\n"
                "TabWidth: 4\n"
                "MacroCategories:\n"
                "  TypeSpecifierMacros:\n"
                "    - REMOVE_CV_REF\n",
                encoding="utf-8",
            )

            formatted = native_format("--stdin", "--style", str(config), input_text=source)

            self.assertEqual(0, formatted.returncode, msg=f"stdout:\n{formatted.stdout}\n\nstderr:\n{formatted.stderr}")
            self.assertEqual(
                "template <typename T, typename U>\n"
                "struct TypeMacroFixture {\n"
                "    typedef REMOVE_CV_REF(T) RawT;\n"
                "    typedef typename REMOVE_CV_REF(T, U) BoundT;\n"
                "    using Address = const REMOVE_CV_REF(T)*;\n"
                "};\n",
                formatted.stdout,
            )

    def test_preprocessor_argument_macro_accepts_balanced_token_sequences(self) -> None:
        source = (
            "void PreprocessorArguments(){\n"
            "PP_EXPANSION(\"+=\", PP_CAT(+, =));\n"
            "PP_EXPANSION(\"comma\", PP_HAS_COMMA(, ));\n"
            "PP_EXPANSION(\"tokens\", PP_PARENS(sss() sss));\n"
            "PP_EXPANSION(\"raw\", PP_RAW(R\"tag((,))tag\"));\n"
            "PP_EXPANSION(PP_ITEM, ~, (int, float));\n"
            "using GeneratedTypes=Test<PP_EXPANSION(PP_ITEM, ~, (int, float))>;\n"
            "PP_EXPANSION(item, );\n"
            "PP_EXPANSION(, );\n"
            "}\n"
        )
        unconfigured = native_format("--stdin", input_text=source)

        self.assertEqual(1, unconfigured.returncode, msg=f"stdout:\n{unconfigured.stdout}\n\nstderr:\n{unconfigured.stderr}")
        self.assertIn("parse failed", unconfigured.stderr)

        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="format_preprocessor_arguments_", dir=build_dir) as temp_dir:
            config = Path(temp_dir) / ".cpp-format"
            config.write_text(
                "---\n"
                "ColumnLimit: 120\n"
                "IndentWidth: 4\n"
                "TabWidth: 4\n"
                "MacroCategories:\n"
                "  PreprocessorArgumentMacros:\n"
                "    - PP_EXPANSION\n",
                encoding="utf-8",
            )

            formatted = native_format("--stdin", "--style", str(config), input_text=source)

            self.assertEqual(0, formatted.returncode, msg=f"stdout:\n{formatted.stdout}\n\nstderr:\n{formatted.stderr}")
            self.assertEqual(
                "void PreprocessorArguments() {\n"
                "    PP_EXPANSION(\"+=\", PP_CAT(+, =));\n"
                "    PP_EXPANSION(\"comma\", PP_HAS_COMMA(, ));\n"
                "    PP_EXPANSION(\"tokens\", PP_PARENS(sss() sss));\n"
                "    PP_EXPANSION(\"raw\", PP_RAW(R\"tag((,))tag\"));\n"
                "    PP_EXPANSION(PP_ITEM, ~, (int, float));\n"
                "    using GeneratedTypes = Test<PP_EXPANSION(PP_ITEM, ~, (int, float))>;\n"
                "    PP_EXPANSION(item, );\n"
                "    PP_EXPANSION(, );\n"
                "}\n",
                formatted.stdout,
            )

            idempotent = native_format("--dry-run", "--stdin", "--style", str(config), input_text=formatted.stdout)
            self.assertEqual(0, idempotent.returncode, msg=f"stdout:\n{idempotent.stdout}\n\nstderr:\n{idempotent.stderr}")

    def test_macro_arrow_chain_formats_and_reparses(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_reparse_guard_", dir=build_dir) as temp_dir:
            config = Path(temp_dir) / ".cpp-format"
            config.write_text(
                "---\n"
                "ColumnLimit: 120\n"
                "IndentWidth: 4\n"
                "TabWidth: 4\n"
                "MacroCategories:\n"
                "  CallSyntaxMacros:\n"
                "    - BENCHMARK\n",
                encoding="utf-8",
            )
            source = "BENCHMARK(Foo)\n    ->Args({1, 2});\n"

            result = native_format("--stdin", "--style", str(config), input_text=source)

            self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
            self.assertEqual("BENCHMARK(Foo)->Args({1, 2});\n", result.stdout)

            second_result = native_format("--dry-run", "--stdin", "--style", str(config), input_text=result.stdout)

            self.assertEqual(
                0,
                second_result.returncode,
                msg=f"stdout:\n{second_result.stdout}\n\nstderr:\n{second_result.stderr}",
            )

    def test_compact_empty_brace_ternary_colon_keeps_space(self) -> None:
        result = native_format(
            "--stdin",
            input_text=(
                "auto snapshot=preferred?TreeViewportSnapshot{}:CaptureTreeViewportSnapshot();\n"
                "auto text=empty?std::string{}:value;\n"
            ),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "auto snapshot = preferred ? TreeViewportSnapshot{} : CaptureTreeViewportSnapshot();\n"
            "auto text = empty ? std::string{} : value;\n",
            result.stdout,
        )

    def test_nested_list_search_reuses_alternatives(self) -> None:
        depth = 12
        source = "auto value = " + "Value{.first = 1, .second = " * depth + "Leaf{2, 3}" + "}" * depth + ";\n"
        formatted = native_format("--stdin", input_text=source, timeout=10)

        self.assertEqual(0, formatted.returncode, msg=formatted.stderr)
        self.assert_no_unsupported_placement_warnings(formatted)
        compact_source = re.sub(r"\s+", "", source)
        compact_formatted = re.sub(r",(?=})", "", re.sub(r"\s+", "", formatted.stdout))
        self.assertEqual(compact_source, compact_formatted)
        self.assertIn("\n    .first = 1,\n", formatted.stdout)
        self.assertIn("Value{.first = 1, .second = Leaf{2, 3}}", formatted.stdout)
        self.assertLessEqual(max(map(len, formatted.stdout.splitlines())), 120)

        idempotent = native_format("--stdin", "--dry-run", input_text=formatted.stdout, timeout=10)
        self.assertEqual(0, idempotent.returncode, msg=idempotent.stderr)

    def test_compact_initializer_braces_stay_tight_in_split_context(self) -> None:
        result = native_format(
            "--stdin",
            input_text=(
                "const auto matchesDrag = [&](const LayoutEditOverlayOwner& owner) {\n"
                "return owner.childIndex==drag.currentIndex&&\n"
                "MatchesLayoutContainerEditKey(LayoutContainerEditKey{owner.key.editCardId,owner.key.nodePath},\n"
                "LayoutContainerEditKey{drag.key.editCardId,drag.key.nodePath});\n"
                "};\n"
                "bool hits(){return MatchesRegionHit(regions,region,RenderPoint{x,y})&&\n"
                "MatchesRegionHit(regions,region,RenderPoint{x+3,y});}\n"
            ),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "const auto matchesDrag = [&](const LayoutEditOverlayOwner& owner) {\n"
            "    return owner.childIndex == drag.currentIndex && MatchesLayoutContainerEditKey(\n"
            "        LayoutContainerEditKey{owner.key.editCardId, owner.key.nodePath},\n"
            "        LayoutContainerEditKey{drag.key.editCardId, drag.key.nodePath}\n"
            "    );\n"
            "};\n"
            "\n"
            "bool hits() {\n"
            "    return MatchesRegionHit(regions, region, RenderPoint{x, y}) &&\n"
            "        MatchesRegionHit(regions, region, RenderPoint{x + 3, y});\n"
            "}\n",
            result.stdout,
        )

    def test_control_body_brace_normalization(self) -> None:
        result = native_format(
            "--stdin",
            input_text=(
                "void f(int* values,int count){\n"
                "if(count) values[0]+=1;\n"
                "else values[0]=0;\n"
                "if(count==0) values[0]=0;\n"
                "else if(count==1) values[0]=1;\n"
                "else values[0]=2;\n"
                "while(count) --count;\n"
                "for(int i=0;i<count;++i) values[i]+=i;\n"
                "do ++count; while(count<10);\n"
                "if(count) { return; } else { if(count) return; }\n"
                "}\n"
            ),
        )

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "void f(int* values, int count) {\n"
            "    if (count) {\n"
            "        values[0] += 1;\n"
            "    } else {\n"
            "        values[0] = 0;\n"
            "    }\n"
            "    if (count == 0) {\n"
            "        values[0] = 0;\n"
            "    } else if (count == 1) {\n"
            "        values[0] = 1;\n"
            "    } else {\n"
            "        values[0] = 2;\n"
            "    }\n"
            "    while (count) {\n"
            "        --count;\n"
            "    }\n"
            "    for (int i = 0; i < count; ++i) {\n"
            "        values[i] += i;\n"
            "    }\n"
            "    do {\n"
            "        ++count;\n"
            "    } while (count < 10);\n"
            "    if (count) {\n"
            "        return;\n"
            "    } else if (count) {\n"
            "        return;\n"
            "    }\n"
            "}\n",
            result.stdout,
        )

    def test_semicolonless_sentinel_is_statement_in_unbraced_switch_case(self) -> None:
        source = (
            "void WarningSentinelsAfterUnbracedSwitchCases(){\n"
            "GTEST_DISABLE_MSC_WARNINGS_PUSH_(4065)\n"
            "switch(0)\n"
            "default:\n"
            "UseDefault();\n"
            "switch(0)\n"
            "case 0:\n"
            "UseCase();\n"
            "GTEST_DISABLE_MSC_WARNINGS_POP_()\n"
            "}\n"
        )
        expected = (
            "void WarningSentinelsAfterUnbracedSwitchCases() {\n"
            "    GTEST_DISABLE_MSC_WARNINGS_PUSH_(4065)\n"
            "    switch (0) {\n"
            "        default:\n"
            "            UseDefault();\n"
            "            switch (0) {\n"
            "                case 0:\n"
            "                    UseCase();\n"
            "                    GTEST_DISABLE_MSC_WARNINGS_POP_()\n"
            "            }\n"
            "    }\n"
            "}\n"
        )

        result = native_format("--stdin", "--style", str(USERVER_FORMAT_CONFIG), input_text=source)

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(expected, result.stdout)
        self.assert_no_unsupported_placement_warnings(result)

        second_result = native_format(
            "--stdin", "--style", str(USERVER_FORMAT_CONFIG), input_text=result.stdout
        )

        self.assertEqual(
            0,
            second_result.returncode,
            msg=f"stdout:\n{second_result.stdout}\n\nstderr:\n{second_result.stderr}",
        )
        self.assertEqual(expected, second_result.stdout)

    def test_lambda_argument_and_split_function_parameters_are_allowed(self) -> None:
        input_text = (
            "struct IncludeGroup { int priority; };\n"
            "void SortIncludeGroups(std::vector<IncludeGroup>& groups) {\n"
            "    std::sort(groups.begin(), groups.end(), [](const IncludeGroup& left, const IncludeGroup& right) {\n"
            "        return left.priority < right.priority;\n"
            "    });\n"
            "}\n"
            "\n"
            "std::set<std::string> RequireSuffixGroup(\n"
            "    const std::map<std::string, std::set<std::string>>& suffixGroups,\n"
            "    std::string_view configPath,\n"
            "    std::string_view groupName\n"
            ") {\n"
            "   return {};\n"
            "}\n"
        )
        result = native_format("--stdin", input_text=input_text)

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual(
            "struct IncludeGroup {\n"
            "    int priority;\n"
            "};\n"
            "\n"
            "void SortIncludeGroups(std::vector<IncludeGroup>& groups) {\n"
            "    std::sort(groups.begin(), groups.end(), [](const IncludeGroup& left, const IncludeGroup& right) {\n"
            "        return left.priority < right.priority;\n"
            "    });\n"
            "}\n"
            "\n"
            "std::set<std::string> RequireSuffixGroup(\n"
            "    const std::map<std::string, std::set<std::string>>& suffixGroups,\n"
            "    std::string_view configPath,\n"
            "    std::string_view groupName\n"
            ") {\n"
            "    return {};\n"
            "}\n",
            result.stdout,
        )

    def test_validation_regressions(self) -> None:
        cases = (
            (
                "conditional calls",
                "#if HAS(feature)\nint a;\n#elif CHECK(major, minor)\nint b;\n#else\nint c;\n#endif\n",
                "#if HAS(feature)\nint a;\n#elif CHECK(major, minor)\nint b;\n#else\nint c;\n#endif\n",
            ),
            (
                "header and body comments with identical text",
                "#ifdef /* guard */ FEATURE\n/* guard */ int a;\n#endif\n",
                "#ifdef /* guard */ FEATURE\n/* guard */ int a;\n#endif\n",
            ),
            (
                "leading macro replacement comment",
                "#define FIELD(data, elem) \\\n    /* annotation */ \\\n    decltype(data::elem) elem;\n",
                "#define FIELD(data, elem) \\\n    /* annotation */ \\\n    decltype(data::elem) elem;\n",
            ),
            (
                "terminal macro comment before endif",
                "#if HAS(feature)\n#define FLAG 1 // note\n#endif\nint n;\n",
                "#if HAS(feature)\n#define FLAG \\\n    1  // note\n#endif\nint n;\n",
            ),
            (
                "packed parameter block comment",
                "void SomeQuiteLongFunctionName(LongType a, LongType /*b*/);\n",
                "void SomeQuiteLongFunctionName(\n    LongType a, LongType /*b*/\n);\n",
            ),
            (
                "template header comment",
                "template <typename T> // element type\nclass Queue;\n",
                "template <typename T>  // element type\nclass Queue;\n",
            ),
            (
                "number before pack expansion",
                "template<int... I> void f() { use({ I ? I : 0 ... }); }\n",
                "template <int... I>\nvoid f() { use({I ? I : 0 ...}); }\n",
            ),
            (
                "macro terminators and standalone comment",
                "void f(){\n    ITEM(one);\n    ITEM(two); // tail\n    ITEM(three)\n"
                "    // standalone\n    int a;\n}\n",
                "void f() {\n    ITEM(one);\n    ITEM(two);  // tail\n    ITEM(three)\n"
                "    // standalone\n    int a;\n}\n",
            ),
            (
                "partially guarded namespace",
                "#if FEATURE\nnamespace {\nint x;\n#endif\n}\n",
                "#if FEATURE\nnamespace {\nint x;\n#endif\n}\n",
            ),
            (
                "conditional ends before consequence",
                "void f() {\n#if FEATURE\nif (a) { g(); } else if (b)\n#endif\nh();\n}\n",
                "void f() {\n#if FEATURE\nif (a) { g(); } else if (b)\n#endif\nh();\n}\n",
            ),
            (
                "conditional ends before else",
                "void f() {\n#if FEATURE\nif (a) { g(); }\n#endif\nelse { h(); }\n}\n",
                "void f() {\n#if FEATURE\nif (a) { g(); }\n#endif\nelse { h(); }\n}\n",
            ),
        )
        TEST_TEMP_ROOT.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="format_validation_", dir=TEST_TEMP_ROOT) as temp_dir:
            config = Path(temp_dir) / ".cpp-format"
            config.write_text(
                "ColumnLimit: 50\nIndentWidth: 4\nMacroCategories:\n  SemicolonlessCallMacros:\n    - ITEM\n",
                encoding="utf-8",
            )
            for name, source, expected in cases:
                with self.subTest(name=name):
                    checked = native_format("--stdin", "--style", str(config), input_text=source)
                    self.assertEqual(0, checked.returncode, msg=checked.stderr)
                    self.assertEqual(expected, checked.stdout)
                    # Normal mode emits the same transformation without spending another parse/format pass.
                    normal = native_format("--stdin", "--style", str(config), input_text=source, validate=False)
                    self.assertEqual(0, normal.returncode, msg=normal.stderr)
                    self.assertEqual(expected, normal.stdout)


    def test_parse_error_rejects_stdout_formatting(self) -> None:
        for validate in (False, True):
            with self.subTest(validate=validate):
                result = native_format("--stdin", input_text="int main( { return 1; }\n", validate=validate)
                self.assertEqual(1, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
                self.assertEqual("", result.stdout)
                self.assertIn("parse failed", result.stderr)
                self.assertNotIn("tree-sitter", result.stderr)

    def test_parse_error_does_not_write_in_place_batch(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_parse_error_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            shutil.copyfile(STRICTFMT_ROOT / ".cpp-format", root / ".cpp-format")
            write_empty_ignore(root)
            valid = root / "valid.cpp"
            invalid = root / "invalid.cpp"
            valid.write_text("int main(){return 1;}\n", encoding="utf-8")
            invalid.write_text("int main( { return 1; }\n", encoding="utf-8")

            result = native_format("-i", str(valid), str(invalid), cwd=root)

            self.assertEqual(1, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
            self.assertEqual("int main(){return 1;}\n", valid.read_text(encoding="utf-8").replace("\r\n", "\n"))
            self.assertIn("parse failed", result.stderr)
            self.assertNotIn("tree-sitter", result.stderr)
            self.assertIn("failed formatting", result.stdout)

    def test_explicit_style_file_and_upward_discovery(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_style_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            nested = root / "a" / "b"
            nested.mkdir(parents=True)
            shutil.copyfile(STRICTFMT_ROOT / ".cpp-format", root / ".cpp-format")
            write_empty_ignore(root)
            source = nested / "sample.cpp"
            source.write_text("int main(){return 1;}\n", encoding="utf-8")

            discovered = native_format("--dry-run", str(source), cwd=nested)
            explicit = native_format("--style", str(root / ".cpp-format"), "--dry-run", str(source), cwd=nested)

            self.assertEqual(1, discovered.returncode, msg=f"stdout:\n{discovered.stdout}\n\nstderr:\n{discovered.stderr}")
            self.assertIn("Formatting is required", discovered.stdout)
            self.assertEqual(1, explicit.returncode, msg=f"stdout:\n{explicit.stdout}\n\nstderr:\n{explicit.stderr}")
            self.assertIn("Formatting is required", explicit.stdout)

    def test_style_file_inherits_parent_config_and_overrides_include_categories(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_style_inherit_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            nested = root / "a" / "b"
            nested.mkdir(parents=True)
            write_empty_ignore(root)
            (root / ".cpp-format").write_text(
                "---\n"
                "IndentWidth: 2\n"
                "IncludeCategories:\n"
                "  - Regex: '^<.*>$'\n"
                "    Priority: 1\n"
                "  - Regex: '^\".*\"$'\n"
                "    Priority: 2\n",
                encoding="utf-8",
            )
            source = nested / "sample.cpp"
            source.write_text('#include "b.h"\n#include <a>\n\nint main(){return 1;}\n', encoding="utf-8")

            (nested / ".cpp-format").write_text("---\nInherit: Parent\nColumnLimit: 80\n", encoding="utf-8")
            inherited = native_format(str(source), cwd=nested)

            self.assertEqual(0, inherited.returncode, msg=f"stdout:\n{inherited.stdout}\n\nstderr:\n{inherited.stderr}")
            self.assertEqual(
                '#include <a>\n'
                "\n"
                '#include "b.h"\n'
                "\n"
                "int main() { return 1; }\n",
                inherited.stdout,
            )

            (nested / ".cpp-format").write_text(
                "---\n"
                "Inherit: Parent\n"
                "IndentWidth: 4\n"
                "IncludeCategories:\n"
                "  - Regex: '^\".*\"$'\n"
                "    Priority: 1\n"
                "  - Regex: '^<.*>$'\n"
                "    Priority: 2\n",
                encoding="utf-8",
            )
            overridden = native_format("--style", str(nested / ".cpp-format"), str(source), cwd=root)

            self.assertEqual(0, overridden.returncode, msg=f"stdout:\n{overridden.stdout}\n\nstderr:\n{overridden.stderr}")
            self.assertEqual(
                '#include "b.h"\n'
                "\n"
                "#include <a>\n"
                "\n"
                "int main() { return 1; }\n",
                overridden.stdout,
            )

    def test_style_file_merges_stream_configuration_methods_with_parent(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_stream_config_inherit_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            nested = root / "nested"
            nested.mkdir()
            write_empty_ignore(root)
            (root / ".cpp-format").write_text(
                "---\n"
                "ColumnLimit: 42\n"
                "StreamShift:\n"
                "  ConfigurationMethods:\n"
                "    - PARENT_MANIPULATOR\n"
                "    - SHARED_MANIPULATOR\n",
                encoding="utf-8",
            )
            (nested / ".cpp-format").write_text(
                "---\n"
                "Inherit: Parent\n"
                "StreamShift:\n"
                "  ConfigurationMethods:\n"
                "    - CHILD_MANIPULATOR\n"
                "    - SHARED_MANIPULATOR\n",
                encoding="utf-8",
            )
            source = nested / "sample.cpp"
            source.write_text(
                "void Print(){\n"
                'output<<"parent="<<PARENT_MANIPULATOR<<parentValue'
                '<<", child="<<CHILD_MANIPULATOR<<childValue;\n'
                "}\n",
                encoding="utf-8",
            )

            result = native_format(str(source), cwd=nested)

            self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
            self.assertEqual(
                "void Print() {\n"
                "    output\n"
                '        << "parent="\n'
                "        << PARENT_MANIPULATOR << parentValue\n"
                '        << ", child="\n'
                "        << CHILD_MANIPULATOR << childValue;\n"
                "}\n",
                result.stdout,
            )

    def test_style_file_merges_every_macro_category_with_parent(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_macro_category_inherit_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            nested = root / "nested"
            nested.mkdir()
            write_empty_ignore(root)
            (root / ".cpp-format").write_text(
                "---\n"
                "MacroCategories:\n"
                "  RawMacroDefinitions:\n"
                "    - PARENT_RAW\n"
                "  BareIdentifierMacros:\n"
                "    - PARENT_BARE\n"
                "  DeclarationPrefixMacros:\n"
                "    - PARENT_PREFIX\n"
                "  CallSyntaxMacros:\n"
                "    - PARENT_CALL\n"
                "    - SHARED_CALL\n"
                "  SemicolonlessCallMacros:\n"
                "    - PARENT_SEMILESS\n"
                "  StatementArgumentMacros:\n"
                "    - PARENT_STATEMENT\n"
                "  TypeSpecifierMacros:\n"
                "    - PARENT_TYPE\n"
                "  PreprocessorArgumentMacros:\n"
                "    - PARENT_PP\n",
                encoding="utf-8",
            )
            (nested / ".cpp-format").write_text(
                "---\n"
                "Inherit: Parent\n"
                "MacroCategories:\n"
                "  RawMacroDefinitions:\n"
                "    - CHILD_RAW\n"
                "  BareIdentifierMacros:\n"
                "    - CHILD_BARE\n"
                "  DeclarationPrefixMacros:\n"
                "    - CHILD_PREFIX\n"
                "  CallSyntaxMacros:\n"
                "    - CHILD_CALL\n"
                "    - SHARED_CALL\n"
                "  SemicolonlessCallMacros:\n"
                "    - CHILD_SEMILESS\n"
                "  StatementArgumentMacros:\n"
                "    - CHILD_STATEMENT\n"
                "  TypeSpecifierMacros:\n"
                "    - CHILD_TYPE\n"
                "  PreprocessorArgumentMacros:\n"
                "    - CHILD_PP\n",
                encoding="utf-8",
            )
            source = nested / "sample.cpp"
            source.write_text(
                "#define PARENT_RAW(first,second) first ## second\n"
                "#define CHILD_RAW(first,second) first ## second\n"
                "PARENT_BARE\n"
                "CHILD_BARE\n"
                "PARENT_PREFIX int ParentDeclaration();\n"
                "CHILD_PREFIX int ChildDeclaration();\n"
                "PARENT_CALL(ParentSuite,ParentCase){Run();}\n"
                "CHILD_CALL(ChildSuite,ChildCase){Run();}\n"
                "SHARED_CALL(SharedSuite,SharedCase){Run();}\n"
                "typedef PARENT_TYPE(Value) ParentType;\n"
                "typedef CHILD_TYPE(Value) ChildType;\n"
                "PARENT_SEMILESS()\n"
                "CHILD_SEMILESS()\n"
                "void Exercise(){\n"
                "PARENT_STATEMENT(auto parent=Make(),Error);\n"
                "CHILD_STATEMENT(auto child=Make(),Error);\n"
                'PARENT_PP("parent",PP_CAT(+, =));\n'
                'CHILD_PP("child",PP_CAT(+, =));\n'
                "}\n",
                encoding="utf-8",
            )

            result = native_format(str(source), cwd=nested)

            self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
            self.assertIn("#define PARENT_RAW(first, second) first ## second\n", result.stdout)
            self.assertIn("#define CHILD_RAW(first, second) first ## second\n", result.stdout)

    def test_ignore_file_skips_simple_directory_entries(self) -> None:
        build_dir = TEST_TEMP_ROOT
        build_dir.mkdir(exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="format_ignore_", dir=build_dir) as temp_dir:
            root = Path(temp_dir)
            vendor = root / "src" / "vendor"
            vendor.mkdir(parents=True)
            shutil.copyfile(STRICTFMT_ROOT / ".cpp-format", root / ".cpp-format")
            (root / ".cpp-format-ignore").write_text("src/vendor\n", encoding="utf-8")
            source = vendor / "ignored.cpp"
            source.write_text("int main(){return 1;}\n", encoding="utf-8")

            result = native_format("--dry-run", str(source), cwd=root)

            self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
            self.assertIn("Checked 0 files, 0 LOC", result.stdout)
            self.assertIn("Skipped 1 ignored file", result.stdout)

    def test_no_input_prints_help_instead_of_reading_stdin(self) -> None:
        result = native_format()

        self.assertEqual(0, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
        self.assertEqual("", result.stderr)
        self.assertIn("Usage:", result.stdout)
        self.assertIn("strictfmt [options] [ <file>... | -r <path> | --stdin | --files <path> ]", result.stdout)
        self.assertIn("--dump-syntax-tree", result.stdout)
        self.assertIn("--dump-break-tree", result.stdout)
        self.assertIn("--style <config-file>", result.stdout)

    def test_invalid_native_usage_is_rejected(self) -> None:
        invalid_cases = [
            ("-i",),
            ("-i", "--dry-run", str(TEST_ROOT / OUTPUT_FIXTURE)),
            ("-i", "--diff", str(TEST_ROOT / OUTPUT_FIXTURE)),
            ("--dry-run", "--diff", str(TEST_ROOT / OUTPUT_FIXTURE)),
            ("--style",),
            ("--dump",),
            ("--dump-syntax-tree",),
            ("--dump-syntax-tree", str(TEST_ROOT / OUTPUT_FIXTURE), "--stdin"),
            ("--dump-syntax-tree", str(TEST_ROOT / OUTPUT_FIXTURE), "--dry-run"),
            ("--dump-syntax-tree", str(TEST_ROOT / OUTPUT_FIXTURE), "--diff"),
            ("--dump-break-tree", str(TEST_ROOT / OUTPUT_FIXTURE), "--concurrency", "1"),
            ("--stdin", "--dump-syntax-tree", "--validate"),
            ("--stdin", "--dump-break-tree", "--validate"),
            ("--stdin", "--dump-syntax-tree", "--dump-break-tree"),
            ("--files",),
            ("-r",),
            ("--concurrency",),
            ("--concurrency", "0"),
            ("--concurrency", "nope"),
            ("--stdin", "-i"),
            ("--stdin", str(TEST_ROOT / OUTPUT_FIXTURE)),
            ("--unknown",),
        ]

        for args in invalid_cases:
            with self.subTest(args=args):
                result = native_format(*args)

                self.assertEqual(2, result.returncode, msg=f"stdout:\n{result.stdout}\n\nstderr:\n{result.stderr}")
                self.assertIn("Usage:", result.stderr)


if __name__ == "__main__":
    runner = unittest.TextTestRunner(verbosity=2, resultclass=MethodNameTestResult)
    unittest.main(testRunner=runner)
